import os
import subprocess
import sys
import unittest

import rclpy
from onrobot_msgs.srv import SetCommand
from rcl_interfaces.msg import Parameter, ParameterType, ParameterValue
from rcl_interfaces.srv import GetParameters, SetParameters


SINGLE_EXECUTABLE, DUAL_EXECUTABLE = sys.argv[1:3]
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
        for executable, parameters in (
            (SINGLE_EXECUTABLE, ['gripper_model:=rg2']),
            (DUAL_EXECUTABLE, ['gripper_primary:=rg6', 'gripper_secondary:=rg2']),
        ):
            args = [executable, '--ros-args', '-r', '__ns:=/parameter_test']
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

    def test_models_are_read_only_and_limits_match_startup_models(self):
        cases = (
            ('onrobot_rg_command_server', 'gripper_model', 'rg2', 'rg6',
             'onrobot_rg/set_command', 1100),
            ('onrobot_rg_dual_command_server', 'gripper_primary', 'rg6', 'rg2',
             'onrobot_rg/set_command_A', 1600),
            ('onrobot_rg_dual_command_server', 'gripper_secondary', 'rg2', 'rg6',
             'onrobot_rg/set_command_B', 1100),
        )
        for node, parameter, initial, replacement, service, maximum in cases:
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

                for width, expected in ((maximum, True), (maximum + 1, False)):
                    command = SetCommand.Request()
                    command.command = str(width)
                    response = self.call(SetCommand, service, command)
                    self.assertEqual(response.success, expected, response.message)


if __name__ == '__main__':
    unittest.main()
