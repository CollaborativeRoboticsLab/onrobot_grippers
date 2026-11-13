#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from onrobot_vg_msgs.msg import OnRobotVGOutput
from onrobot_vg_msgs.srv import SetCommand

class OnRobotVGNode(Node):
    """Class to handle setting commands."""
    def __init__(self):
        super().__init__('OnRobotVGSimpleControllerServer')
        self.pub = self.create_publisher(OnRobotVGOutput, 'OnRobotVGOutput', 1)
        self.command = OnRobotVGOutput()
        self.set_command_srv = self.create_service(
            SetCommand,
            "/onrobot_vg/set_command",
            self.handle_set_command)

    def handle_set_command(self, request, response):
        """To handle sending commands via socket connection."""
        self.get_logger().info(str(request.command))
        self.command = self.genCommand(str(request.command), self.command)
        self.pub.publish(self.command)
        # self.create_timer(1.0, lambda: None).cancel()  # Equivalent to rospy.sleep(1)
        # response.success = None  # TODO: implement
        # response.message = None  # TODO: implement
        return response

    def genCommand(self, char, command):
        """Updates the command according to the character entered by the user."""
        if char == 'g':
            command.r_mca = 0x0100
            command.r_vca = 255
            command.r_mcb = 0x0100
            command.r_vcb = 255
        if char == 'r':
            command.r_mca = 0x0000
            command.r_vca = 0
            command.r_mcb = 0x0000
            command.r_vcb = 0
        if char == 'ga':
            command.r_mca = 0x0100
            command.r_vca = 255
        if char == 'ra':
            command.r_mca = 0x0000
            command.r_vca = 0
        if char == 'gb':
            command.r_mcb = 0x0100
            command.r_vcb = 255
        if char == 'rb':
            command.r_mcb = 0x0000
            command.r_vcb = 0

        # If the command entered is a int, assign this value to r
        try:
            if int(char) == 0:
                command.r_mca = 0x0000
                command.r_vca = 0
                command.r_mcb = 0x0000
                command.r_vcb = 0
            else:
                command.r_mca = 0x0100
                command.r_vca = min(255, int(char))
                command.r_mcb = 0x0100
                command.r_vcb = min(255, int(char))
        except ValueError:
            pass

        return command

def main(args=None):
    rclpy.init(args=args)
    node = OnRobotVGNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()