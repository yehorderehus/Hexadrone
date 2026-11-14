import rclpy
from rclpy.impl.implementation_singleton import rclpy_implementation as _rclpy
from sensor_msgs.msg import Joy

KEY_W = ord('W')
KEY_S = ord('S')
KEY_A = ord('A')
KEY_D = ord('D')
KEY_I = ord('I')
KEY_K = ord('K')
KEY_J = ord('J')
KEY_L = ord('L')
KEY_SPACE = ord(' ')
KEY_Z = ord('Z')
KEY_X = ord('X')
KEY_C = ord('C')
KEY_Q = ord('Q')
KEY_E = ord('E')
KEY_B = ord('B')
KEY_1 = ord('1')
KEY_2 = ord('2')
KEY_3 = ord('3')
KEY_4 = ord('4')
KEY_5 = ord('5')
KEY_6 = ord('6')

KEYBOARD_THROTTLE = 0.5


class KeyboardPlugin:
    def init(self, webots_node, properties):
        self.__robot = webots_node.robot

        if not rclpy.ok():
            rclpy.init(args=None)
        self.__node = rclpy.create_node('keyboard_plugin')

        self.__keyboard = self.__robot.getKeyboard()
        self.__keyboard.enable(int(self.__robot.getBasicTimeStep()))

        self.__pub = self.__node.create_publisher(Joy, '/joy', 10)

        self.__armed = False
        self.__posture = 0.0
        self.__prev_keys = set()

        self.__node.get_logger().info(
            'KeyboardPlugin ready: Space arm/disarm, W/S move, A/D yaw, '
            'I/K pitch, J/L roll, Z/X/C posture, Q/E select leg.'
        )

    def step(self):
        if not rclpy.ok():
            return

        try:
            rclpy.spin_once(self.__node, timeout_sec=0)
        except _rclpy.RCLError:
            if rclpy.ok():
                raise
            return

        if not rclpy.ok():
            return

        keys = set()
        key = self.__keyboard.getKey()
        while key != -1:
            keys.add(key)
            key = self.__keyboard.getKey()

        new_keys = keys - self.__prev_keys
        self.__prev_keys = keys

        if KEY_SPACE in new_keys:
            self.__armed = not self.__armed
        if KEY_Z in new_keys:
            self.__posture = -1.0
        if KEY_X in new_keys:
            self.__posture = 0.0
        if KEY_C in new_keys:
            self.__posture = 1.0

        forward = KEY_W in keys
        backward = KEY_S in keys
        if forward != backward:
            throttle = KEYBOARD_THROTTLE
            gear_forward = forward
            gear_backward = backward
        else:
            throttle = 0.0
            gear_forward = False
            gear_backward = False

        roll = (1.0 if KEY_L in keys else 0.0) - (1.0 if KEY_J in keys else 0.0)
        pitch = (1.0 if KEY_I in keys else 0.0) - (1.0 if KEY_K in keys else 0.0)
        yaw = (1.0 if KEY_D in keys else 0.0) - (1.0 if KEY_A in keys else 0.0)

        axes = [
            -roll,               # BridgeOps inverts roll.
            -pitch,              # BridgeOps inverts pitch.
            -throttle,           # BridgeOps inverts throttle.
            -yaw,                # BridgeOps inverts yaw.
            -self.__posture,     # BridgeOps inverts posture.
        ]
        buttons = [0] * 13
        buttons[0] = int(self.__armed)
        buttons[1] = int(KEY_B in keys)
        buttons[3] = int(KEY_1 in keys)
        buttons[4] = int(KEY_2 in keys)
        buttons[5] = int(KEY_3 in keys)
        buttons[6] = int(KEY_4 in keys)
        buttons[7] = int(KEY_5 in keys)
        buttons[8] = int(KEY_6 in keys)
        buttons[9] = int(KEY_Q in keys)
        buttons[10] = int(KEY_E in keys)
        buttons[11] = int(gear_forward)
        buttons[12] = int(gear_backward)

        msg = Joy()
        msg.header.stamp = self.__node.get_clock().now().to_msg()
        msg.axes = axes
        msg.buttons = buttons
        try:
            self.__pub.publish(msg)
        except _rclpy.RCLError:
            if rclpy.ok():
                raise
