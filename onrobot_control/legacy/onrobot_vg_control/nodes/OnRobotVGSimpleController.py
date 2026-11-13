#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from onrobot_vg_msgs.msg import OnRobotVGOutput


def genCommand(char, command):
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


def askForCommand(command):
    """Asks the user for a command to send to the gripper."""

    currentCommand = 'Simple OnRobot VG Controller\n-----\nCurrent command:'
    currentCommand += f' r_mca = {command.r_mca}'
    currentCommand += f', r_vca = {command.r_vca}'
    currentCommand += f', r_mcb = {command.r_mcb}'
    currentCommand += f', r_vcb = {command.r_vcb}'

    print(currentCommand)

    strAskForCommand = '-----\nAvailable commands\n\n'
    strAskForCommand += 'g: Turn on all channels\n'
    strAskForCommand += 'r: Turn off all channels\n'
    strAskForCommand += 'ga: Turn on channel A\n'
    strAskForCommand += 'ra: Turn off channel A\n'
    strAskForCommand += 'gb: Turn on channel B\n'
    strAskForCommand += 'rb: Turn off channel B\n'
    strAskForCommand += '(0 - 255): Set vacuum power for all channels\n'

    strAskForCommand += '-->'

    return input(strAskForCommand)


class OnRobotVGSimpleController(Node):
    def __init__(self):
        super().__init__('OnRobotVGSimpleController')
        self.publisher_ = self.create_publisher(OnRobotVGOutput, 'OnRobotVGOutput', 10)
        self.timer = self.create_timer(0.1, self.timer_callback)
        self.command = OnRobotVGOutput()

    def timer_callback(self):
        self.command = genCommand(askForCommand(self.command), self.command)
        self.publisher_.publish(self.command)


def main(args=None):
    rclpy.init(args=args)

    controller = OnRobotVGSimpleController()

    try:
        rclpy.spin(controller)
    except KeyboardInterrupt:
        pass

    controller.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()
