import os
import subprocess
import sys
import time
import unittest

import rclpy
from control_msgs.action import GripperCommand
from onrobot_msgs.action import VGCommand
from onrobot_msgs.msg import OnRobotRGInput
from rcl_interfaces.msg import Parameter, ParameterType, ParameterValue
from rcl_interfaces.srv import GetParameters, SetParameters
from rclpy.action import ActionClient
from rclpy.action.client import GoalStatus
from sensor_msgs.msg import JointState
from std_msgs.msg import Float64MultiArray


SINGLE_EXECUTABLE, DUAL_EXECUTABLE, VG_EXECUTABLE, STATUS_EXECUTABLE, DUAL_STATUS_EXECUTABLE, SIM_EXECUTABLE = sys.argv[1:7]
sys.argv = sys.argv[:1]


class ModelParameterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ['ROS_DOMAIN_ID'] = str(20 + os.getpid() % 80)
        os.environ['ROS_AUTOMATIC_DISCOVERY_RANGE'] = 'LOCALHOST'
        rclpy.init()
        cls.addClassCleanup(rclpy.shutdown)
        cls.node = rclpy.create_node('rg_parameter_test', namespace='parameter_test')
        cls.addClassCleanup(cls.node.destroy_node)
        for executable, parameters, namespace in (
            (SINGLE_EXECUTABLE, ['gripper_model:=rg2'], '/parameter_test'),
            (DUAL_EXECUTABLE, ['gripper_primary:=rg6', 'gripper_secondary:=rg2'], '/parameter_test'),
            (VG_EXECUTABLE, [], '/parameter_test_vg'),
            (STATUS_EXECUTABLE, [], '/parameter_test'),
            (DUAL_STATUS_EXECUTABLE, [], '/parameter_test'),
            (SIM_EXECUTABLE, ['model:=rg2', 'goal_timeout_s:=0.25'], '/parameter_test/sim_rg'),
            (SIM_EXECUTABLE, ['model:=rg2', 'goal_timeout_s:=5.0'], '/parameter_test/sim_recovery'),
        ):
            args = [executable, '--ros-args', '-r', '__ns:=' + namespace]
            for parameter in parameters:
                args.extend(['-p', parameter])
            process = subprocess.Popen(args)
            cls.addClassCleanup(cls.stop_process, process)

    @staticmethod
    def stop_process(process):
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)

    def call(self, service_type, name, request):
        client = self.node.create_client(service_type, '/parameter_test/' + name)
        self.addCleanup(self.node.destroy_client, client)
        self.assertTrue(client.wait_for_service(timeout_sec=15), f'{name} unavailable')
        future = client.call_async(request)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10)
        self.assertTrue(future.done(), f'{name} timed out')
        self.assertIsNone(future.exception())
        return future.result()

    def send_goal(self, action_type, action_name, goal, namespace='/parameter_test'):
        client = ActionClient(self.node, action_type, namespace + '/' + action_name)
        self.addCleanup(self.node.destroy_client, client)
        self.assertTrue(client.wait_for_server(timeout_sec=15), f'{action_name} unavailable')
        future = client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10)
        self.assertTrue(future.done(), f'{action_name} goal response timed out')
        self.assertIsNone(future.exception())
        return future.result()

    def test_models_are_read_only_and_limits_match_startup_models(self):
        cases = (
            ('onrobot_rg_command_server', 'gripper_model', 'rg2', 'rg6', 'gripper_command'),
            ('onrobot_rg_dual_command_server', 'gripper_primary', 'rg6', 'rg2', 'onrobot_rg/gripper_command_A'),
            ('onrobot_rg_dual_command_server', 'gripper_secondary', 'rg2', 'rg6', 'onrobot_rg/gripper_command_B'),
        )
        for node, parameter, initial, replacement, action_name in cases:
            with self.subTest(parameter=parameter):
                request = SetParameters.Request()
                request.parameters = [Parameter(
                    name=parameter,
                    value=ParameterValue(type=ParameterType.PARAMETER_STRING,
                                         string_value=replacement),
                )]
                result = self.call(SetParameters, node + '/set_parameters', request)
                self.assertEqual(len(result.results), 1)
                self.assertFalse(result.results[0].successful)
                self.assertIn('read-only', result.results[0].reason.lower())

                get_request = GetParameters.Request()
                get_request.names = [parameter]
                values = self.call(GetParameters, node + '/get_parameters', get_request)
                self.assertEqual(values.values[0].string_value, initial)

                goal = GripperCommand.Goal()
                goal.command.position = 0.01
                goal.command.max_effort = 1.0
                response = self.send_goal(GripperCommand, action_name, goal)
                self.assertFalse(response.accepted, 'goals stay unavailable without a verified hardware backend')

    def test_vg_action_is_available_but_rejects_goals_without_backend(self):
        goal = VGCommand.Goal()
        goal.mode_a = 1
        goal.target_vacuum_a_percent = 40
        goal.mode_b = 0
        goal.target_vacuum_b_percent = 0
        response = self.send_goal(VGCommand, 'gripper_command', goal, '/parameter_test_vg')
        self.assertFalse(response.accepted, 'VG goal must not be sent without a verified hardware backend')

    def test_rg_aperture_is_published_as_joint_state_in_meters(self):
        received = {}
        subscription = self.node.create_subscription(
            JointState, '/parameter_test/gripper_joint_states', lambda message: received.update(
                zip(message.name, message.position)), 10)
        self.addCleanup(self.node.destroy_subscription, subscription)

        publishers = (
            (self.node.create_publisher(OnRobotRGInput, '/parameter_test/OnRobotRGInput', 10), 125),
            (self.node.create_publisher(OnRobotRGInput, '/parameter_test/OnRobotRGInput_A', 10), 250),
            (self.node.create_publisher(OnRobotRGInput, '/parameter_test/OnRobotRGInput_B', 10), 750),
        )
        for publisher, _ in publishers:
            self.addCleanup(self.node.destroy_publisher, publisher)

        discovery_deadline = time.monotonic() + 2.0
        while time.monotonic() < discovery_deadline:
            rclpy.spin_once(self.node, timeout_sec=0.05)
            if all(publisher.get_subscription_count() > 0 for publisher, _ in publishers):
                break

        self.assertTrue(all(publisher.get_subscription_count() > 0 for publisher, _ in publishers),
                        'status listeners should discover all fake status publishers')
        for publisher, width_tenths_mm in publishers:
            status = OnRobotRGInput()
            status.g_wdf = width_tenths_mm
            publisher.publish(status)

        state_deadline = time.monotonic() + 2.0
        expected_names = {'rg_aperture', 'primary_rg_aperture', 'secondary_rg_aperture'}
        while time.monotonic() < state_deadline and not expected_names <= received.keys():
            rclpy.spin_once(self.node, timeout_sec=0.05)

        self.assertAlmostEqual(received.get('rg_aperture', -1.0), 0.0125)
        self.assertAlmostEqual(received.get('primary_rg_aperture', -1.0), 0.025)
        self.assertAlmostEqual(received.get('secondary_rg_aperture', -1.0), 0.075)

    def test_simulation_action_drives_controller_and_finishes_from_joint_feedback(self):
        action_name = '/parameter_test/sim_rg/gripper_command'
        joint_state_topic = '/parameter_test/sim_rg/joint_states'
        command_topic = '/parameter_test/sim_rg/joint_position_controller/commands'
        observed_commands = []
        joint_state_publisher = self.node.create_publisher(JointState, joint_state_topic, 10)
        command_subscription = self.node.create_subscription(
            Float64MultiArray, command_topic, observed_commands.append, 10)
        self.addCleanup(self.node.destroy_publisher, joint_state_publisher)
        self.addCleanup(self.node.destroy_subscription, command_subscription)

        client = ActionClient(self.node, GripperCommand, action_name)
        self.addCleanup(self.node.destroy_client, client)
        self.assertTrue(client.wait_for_server(timeout_sec=15), 'simulated RG action unavailable')

        current_state = JointState()
        current_state.name = ['finger_joint']
        current_state.position = [0.785398]
        discovery_deadline = time.monotonic() + 2.0
        while time.monotonic() < discovery_deadline and joint_state_publisher.get_subscription_count() == 0:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreater(joint_state_publisher.get_subscription_count(), 0)
        joint_state_publisher.publish(current_state)
        rclpy.spin_once(self.node, timeout_sec=0.05)

        goal = GripperCommand.Goal()
        goal.command.position = 0.11
        goal.command.max_effort = 0.0
        goal_response = client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self.node, goal_response, timeout_sec=5)
        self.assertTrue(goal_response.done())
        goal_handle = goal_response.result()
        self.assertTrue(goal_handle.accepted, 'position-only simulation goal should be accepted when endpoints are ready')

        command_deadline = time.monotonic() + 2.0
        while time.monotonic() < command_deadline and not observed_commands:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertTrue(observed_commands, 'action should command the Gazebo position controller')
        commanded_angle = observed_commands[-1].data[0]
        self.assertAlmostEqual(commanded_angle, -0.558505, places=5)

        current_state.position = [commanded_angle]
        joint_state_publisher.publish(current_state)
        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self.node, result_future, timeout_sec=5)
        self.assertTrue(result_future.done(), 'action should complete after the simulated joint reaches the target')
        result = result_future.result()
        self.assertEqual(result.status, GoalStatus.STATUS_SUCCEEDED)
        self.assertTrue(result.result.reached_goal)
        self.assertAlmostEqual(result.result.position, 0.11, places=4)
        self.assertEqual(result.result.effort, 0.0)

        observed_commands.clear()
        timeout_goal = GripperCommand.Goal()
        timeout_goal.command.position = 0.0
        timeout_goal.command.max_effort = 0.0
        timeout_response = client.send_goal_async(timeout_goal)
        rclpy.spin_until_future_complete(self.node, timeout_response, timeout_sec=5)
        self.assertTrue(timeout_response.result().accepted)
        timeout_result = timeout_response.result().get_result_async()
        rclpy.spin_until_future_complete(self.node, timeout_result, timeout_sec=5)
        self.assertTrue(timeout_result.done(), 'simulated goal should terminate after its configured timeout')
        self.assertEqual(timeout_result.result().status, GoalStatus.STATUS_ABORTED)
        hold_deadline = time.monotonic() + 2.0
        while time.monotonic() < hold_deadline and len(observed_commands) < 2:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreaterEqual(len(observed_commands), 2, 'aborting should publish a hold command')
        self.assertAlmostEqual(observed_commands[-1].data[0], commanded_angle, places=5)

        unsupported_goal = GripperCommand.Goal()
        unsupported_goal.command.position = 0.05
        unsupported_goal.command.max_effort = 1.0
        unsupported_response = client.send_goal_async(unsupported_goal)
        rclpy.spin_until_future_complete(self.node, unsupported_response, timeout_sec=5)
        self.assertFalse(unsupported_response.result().accepted, 'simulator must reject unmodeled force limits')

    def test_simulation_action_cancel_holds_last_joint_position(self):
        action_name = '/parameter_test/sim_recovery/gripper_command'
        joint_state_topic = '/parameter_test/sim_recovery/joint_states'
        command_topic = '/parameter_test/sim_recovery/joint_position_controller/commands'
        observed_commands = []
        joint_state_publisher = self.node.create_publisher(JointState, joint_state_topic, 10)
        command_subscription = self.node.create_subscription(
            Float64MultiArray, command_topic, observed_commands.append, 10)
        self.addCleanup(self.node.destroy_publisher, joint_state_publisher)
        self.addCleanup(self.node.destroy_subscription, command_subscription)

        client = ActionClient(self.node, GripperCommand, action_name)
        self.addCleanup(self.node.destroy_client, client)
        self.assertTrue(client.wait_for_server(timeout_sec=15), 'simulated RG action unavailable')

        current_position = 0.785398
        current_state = JointState()
        current_state.name = ['finger_joint']
        current_state.position = [current_position]
        discovery_deadline = time.monotonic() + 2.0
        while time.monotonic() < discovery_deadline and joint_state_publisher.get_subscription_count() == 0:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreater(joint_state_publisher.get_subscription_count(), 0)
        joint_state_publisher.publish(current_state)

        goal = GripperCommand.Goal()
        goal.command.position = 0.11
        goal.command.max_effort = 0.0
        goal_response = client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self.node, goal_response, timeout_sec=5)
        self.assertTrue(goal_response.result().accepted)
        goal_handle = goal_response.result()

        command_deadline = time.monotonic() + 2.0
        while time.monotonic() < command_deadline and not observed_commands:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertTrue(observed_commands, 'action should command the Gazebo position controller')

        concurrent_goal = GripperCommand.Goal()
        concurrent_goal.command.position = 0.05
        concurrent_goal.command.max_effort = 0.0
        concurrent_response = client.send_goal_async(concurrent_goal)
        rclpy.spin_until_future_complete(self.node, concurrent_response, timeout_sec=5)
        self.assertFalse(concurrent_response.result().accepted, 'only one simulated RG goal may be active')

        cancel_future = goal_handle.cancel_goal_async()
        rclpy.spin_until_future_complete(self.node, cancel_future, timeout_sec=5)
        self.assertTrue(cancel_future.result().goals_canceling, 'active simulated goal should accept cancellation')
        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self.node, result_future, timeout_sec=5)
        self.assertTrue(result_future.done(), 'canceled simulated goal should return a result')
        self.assertEqual(result_future.result().status, GoalStatus.STATUS_CANCELED)

        hold_deadline = time.monotonic() + 2.0
        while time.monotonic() < hold_deadline and len(observed_commands) < 2:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreaterEqual(len(observed_commands), 2, 'canceling should publish a hold command')
        self.assertAlmostEqual(observed_commands[-1].data[0], current_position, places=5)

    def test_simulation_action_stale_feedback_aborts_and_holds(self):
        action_name = '/parameter_test/sim_recovery/gripper_command'
        joint_state_topic = '/parameter_test/sim_recovery/joint_states'
        command_topic = '/parameter_test/sim_recovery/joint_position_controller/commands'
        observed_commands = []
        joint_state_publisher = self.node.create_publisher(JointState, joint_state_topic, 10)
        command_subscription = self.node.create_subscription(
            Float64MultiArray, command_topic, observed_commands.append, 10)
        self.addCleanup(self.node.destroy_publisher, joint_state_publisher)
        self.addCleanup(self.node.destroy_subscription, command_subscription)

        client = ActionClient(self.node, GripperCommand, action_name)
        self.addCleanup(self.node.destroy_client, client)
        self.assertTrue(client.wait_for_server(timeout_sec=15), 'simulated RG action unavailable')

        current_position = 0.785398
        current_state = JointState()
        current_state.name = ['finger_joint']
        current_state.position = [current_position]
        discovery_deadline = time.monotonic() + 2.0
        while time.monotonic() < discovery_deadline and joint_state_publisher.get_subscription_count() == 0:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreater(joint_state_publisher.get_subscription_count(), 0)
        joint_state_publisher.publish(current_state)

        goal = GripperCommand.Goal()
        goal.command.position = 0.11
        goal.command.max_effort = 0.0
        goal_response = client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self.node, goal_response, timeout_sec=5)
        self.assertTrue(goal_response.result().accepted)
        goal_handle = goal_response.result()

        command_deadline = time.monotonic() + 2.0
        while time.monotonic() < command_deadline and not observed_commands:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertTrue(observed_commands, 'action should command the Gazebo position controller')

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self.node, result_future, timeout_sec=4)
        self.assertTrue(result_future.done(), 'stale joint feedback should terminate the active goal')
        self.assertEqual(result_future.result().status, GoalStatus.STATUS_ABORTED)

        hold_deadline = time.monotonic() + 2.0
        while time.monotonic() < hold_deadline and len(observed_commands) < 2:
            rclpy.spin_once(self.node, timeout_sec=0.05)
        self.assertGreaterEqual(len(observed_commands), 2, 'stale feedback abort should publish a hold command')
        self.assertAlmostEqual(observed_commands[-1].data[0], current_position, places=5)


if __name__ == '__main__':
    unittest.main()
