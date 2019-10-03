import pygame
import serial
import sys
import time
from pygame.locals import *
from serial.tools.list_ports import comports
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *



class GUI(QWidget):
    def __init__(self, *args, **kwargs):
        super(GUI, self).__init__(*args, **kwargs)

        self.comm = Communications()
        self.my_joystick = None
        self.joystick_list = []
        self.joystick_names = []
        self.g_keys = None
        self.axes1 = None
        self.axes2 = None

        # pygame startup
        pygame.init()
        # Set up the joystick
        pygame.joystick.init()
        self.update_numbered_list_of_controllers()
        # By default, load the first available joystick.
        if len(self.joystick_list) > 0:
            self.connect_to_controller(0)
        else:
            print("No controllers found")

        # # Enumerate joysticks
        # for i in range(0, pygame.joystick.get_count()):
        #     self.joystick_names.append(pygame.joystick.Joystick(i).get_name())
        # print(self.joystick_names)

        # # By default, load the first available joystick.
        # if len(self.joystick_names) > 0:
        #     self.my_joystick = pygame.joystick.Joystick(0)
        #     self.my_joystick.init()

        # Start timer for controller polling
        self.__timer_init = QTimer()
        self.__timer_init.timeout.connect(self.pollController)
        self.__timer_init.setSingleShot(False)
        self.__timer_init.start(50)

        self.button_array = []
        self.axes_array = []
        self.hat_array = []
        self.plot_array = []
        self.axes1 = None
        self.initUI()

    def update_numbered_list_of_controllers(self):
        # Enumerate joysticks
        self.joystick_list.clear()
        for i in range(0, pygame.joystick.get_count()):
            self.joystick_list.append((i, pygame.joystick.Joystick(i).get_name()))
        if len(self.joystick_list):
            for num, name in self.joystick_list:
                print(num, ":", name)

    def initUI(self):

        # Controller selection combo box. Only lists currently connected controllers.
        hbox_cont_select = QHBoxLayout()
        cb_controllers = QComboBox()
        for item in self.joystick_list:
            cb_controllers.addItem(item[1])
        hbox_cont_select.addWidget(cb_controllers)
        cb_controllers.currentIndexChanged.connect(self.selection_change)

        # Number box that alters the value of the variable restrictor (limits how much current the motor can draw by limiting magnitude of thrust) in arduino
        number_box_layout = QHBoxLayout()
        description = QLabel(self)
        description.setText('Restrictor Value')
        number_box_layout.addWidget(description)

        self.numberBox = QSpinBox()
        self.numberBox.setMinimum(0)
        self.numberBox.setMaximum(100)
        self.numberBox.setSingleStep(1)
        number_box_layout.addWidget(self.numberBox)

        numberboxButton = QPushButton("Send")
        numberboxButton.clicked.connect(self.sendnumberboxbuttonState)
        number_box_layout.addWidget(numberboxButton)


        # Layout for axes display
        vbox_axes = QVBoxLayout()
        vbox_axes.addStretch(1)
        label_axes = QLabel(self)
        label_axes.setText('Axes')
        vbox_axes.addWidget(label_axes)
        self.axes1 = AxesWidget()
        self.axes2 = AxesWidget()
        # Assume 4 axes are on controller
        # for i in range(0, self.my_joystick.get_numaxes()):
        for name in ['Movement', 'Camera']:
            slider = QSlider(Qt.Horizontal, self)
            slider.setRange(-100.0, 100.0)
            self.axes_array.append(slider)
            label = QLabel(self)
            label.setText(name + '-X')
            vbox_axes.addWidget(label)
            vbox_axes.addWidget(slider)

            slider = QSlider(Qt.Horizontal, self)
            slider.setRange(-100.0, 100.0)
            self.axes_array.append(slider)
            label = QLabel(self)
            label.setText(name + '-Y')
            vbox_axes.addWidget(label)
            vbox_axes.addWidget(slider)

        hbox_axesplot = QHBoxLayout()

        vbox = QVBoxLayout()
        axesTitle = QLabel(self)
        axesTitle.setText("Movement Axes")
        vbox.addWidget(axesTitle)
        self.axes_array[0].valueChanged[int].connect(self.axes1.set_x_value)
        self.axes_array[1].valueChanged[int].connect(self.axes1.set_y_value)
        vbox.addWidget(self.axes1)
        hbox_axesplot.addLayout(vbox)

        vbox = QVBoxLayout()
        axesTitle = QLabel(self)
        axesTitle.setText("Camera Axes")
        vbox.addWidget(axesTitle)
        self.axes_array[2].valueChanged[int].connect(self.axes2.set_x_value)
        self.axes_array[3].valueChanged[int].connect(self.axes2.set_y_value)
        vbox.addWidget(self.axes2)
        hbox_axesplot.addLayout(vbox)

        vbox_axes.addLayout(hbox_axesplot)

        # Layout for button checkboxes
        hbox_buttons = QHBoxLayout()
        hbox_buttons.addStretch(1)
        label_buttons = QLabel(self)
        label_buttons.setText('Buttons')
        for i in range(0, self.my_joystick.get_numbuttons()):
            cb = QCheckBox(str(i), self)
            self.button_array.append(cb)
            cb.setEnabled(False)
            cb.setChecked(False)
            hbox_buttons.addWidget(cb)

        # Layout for D-pad
        hbox_dpad = QHBoxLayout()
        label_dpad = QLabel(self)
        label_dpad.setText('D-Pad')
        for i in range(0, self.my_joystick.get_numhats()):
            lbl_2 = QLabel(self)
            self.hat_array.append(lbl_2)
            lbl_2.setText(str(self.my_joystick.get_hat(i)))
            hbox_dpad.addWidget(lbl_2)

        vbox = QVBoxLayout()
        vbox.addStretch(1)
        vbox.addLayout(hbox_cont_select)
        vbox.addLayout(number_box_layout)
        vbox.addLayout(vbox_axes)
        vbox.addWidget(label_buttons)
        vbox.addLayout(hbox_buttons)
        vbox.addWidget(label_dpad)
        vbox.addLayout(hbox_dpad)


        button = QPushButton("Reset Motors")
        button.clicked.connect(self.send_reset)
        vbox.addWidget(button)

        # self.setGeometry(300, 300, 300, 150)
        self.setLayout(vbox)

        # Qt show window
        self.show()

    def connect_to_controller(self, index: int):
        """

        :param index: 0-based combo box selection index
        :return:
        """
        # Disconnect from controller
        if self.my_joystick:
            self.my_joystick.quit()
        try:
            if len(self.joystick_list) > index:
                self.my_joystick = pygame.joystick.Joystick(index)
                self.my_joystick.init()
                num_axes = self.my_joystick.get_numaxes()
                num_buttons = self.my_joystick.get_numbuttons()
                num_hats = self.my_joystick.get_numhats()
                print('Found', num_axes, 'axes')
                print('Found', num_buttons, 'buttons')
                print('Found', num_hats, 'pov controllers')

                # if num_axes % 2 != 0:
                #     # raise ValueError('Expected pairs of axes, got ' + str(num_axes))
                #     num_axes = num_axes
            else:
                print("Invalid index specified:", index)
        except Exception:
            print("Exception while connecting to controller", index)

    def selection_change(self, index):
        """

        :param index: selection index, 0 based
        :return:
        """
        print("Items selection changed to:", self.joystick_list[index])
        # Connect to selected controller - disconnect from previous
        self.connect_to_controller(index)

    def pollController(self):
        # Values ranges to collect and send to Arduino
        # 0: Left Stick Y, 0-100
        # 1: Left Stick X, 0-100
        # 2: Right Stick Y, 0-100
        # 3: Right Stick X, 0-100
        # 4: Triggers, 0-100
        # 5-8: A, B, X, Y
        # 9-10: Bumpers
        # 11-12: Back, Start
        # 13: Left Stick Button
        # 14: Right Stick Button
        # 15: Hat Switch, 0-8
        # For Hat, 0 is resting, 1:NW, 2:N, continues clockwise
        g_keys = pygame.event.get()
        for event in g_keys:
            if event.type == JOYAXISMOTION or JOYBALLMOTION or JOYBUTTONDOWN or JOYBUTTONUP or JOYHATMOTION:
                # for i in range(0, self.my_joystick.get_numaxes()):
                #     ax_value = self.my_joystick.get_axis(i)
                #     self.axes_array[i].setValue(ax_value * 100)

                # For display only
                if self.my_joystick.get_numaxes() == 5:
                    self.axes_array[0].setValue(self.my_joystick.get_axis(0) * 100)  # left stick
                    self.axes_array[1].setValue(self.my_joystick.get_axis(1) * 100)  # left stick
                    self.axes_array[2].setValue(self.my_joystick.get_axis(4) * 100)  # right stick
                    self.axes_array[3].setValue(self.my_joystick.get_axis(3) * 100)  # right stick

                self.axes1.repaint()
                self.axes2.repaint()

                for i in range(0, self.my_joystick.get_numbuttons()):
                    state = self.my_joystick.get_button(i) == 1
                    self.button_array[i].setChecked(state)

                for i in range(0, self.my_joystick.get_numhats()):
                    state = self.my_joystick.get_hat(i)
                    self.hat_array[i].setText(str(state))

                # this is to collect the byte values to send to Arduino
                to_send = bytearray(16)

                # read value and flip sign so that -1 is stick-down and +1 is stick-up
                yStick = -self.my_joystick.get_axis(1)
                # remaps (-1,1) to (0,200)
                yStick=(yStick+1)*100

                # read value and flip sign so that -1 is stick-down and +1 is stick-up
                xStick = -self.my_joystick.get_axis(0)
                # remaps (-1,1) to (0,200)
                xStick = (xStick + 1) * 100

                # Collect bytes to send from joystick

                #ROV Movement Bytes (Left Joystick)
                to_send[0] = int(yStick)  # Left stick Y
                to_send[1] = int(xStick)  # Left Stick X
                #Camera Control Bytes (Right Joystick)
                to_send[2] = int(getAxisValueInPercentage(-self.my_joystick.get_axis(4)))  # Right Stick Y
                to_send[3] = int(getAxisValueInPercentage(self.my_joystick.get_axis(3)))  # Right Stick X
                #ROV Vertical Movement Bytes (Triggers)
                to_send[4] = int(getAxisValueInPercentage(self.my_joystick.get_axis(2)))  # Trigger buttons
                #Miscellaneous Buttons
                to_send[5] = (self.my_joystick.get_button(0))  # A or Cross
                to_send[6] = (self.my_joystick.get_button(1))  # B or Circle
                to_send[7] = (self.my_joystick.get_button(2))  # X or Square
                to_send[8] = (self.my_joystick.get_button(3))  # Y or Triangle
                to_send[9] = (self.my_joystick.get_button(4))  # Left Bumper
                to_send[10] = (self.my_joystick.get_button(5))  # Right Bumper
                to_send[11] = (self.my_joystick.get_button(6))  # Back or Select
                to_send[12] = (self.my_joystick.get_button(7))  # Start
                to_send[13] = (self.my_joystick.get_button(8))  # Left Stick Button
                to_send[14] = (self.my_joystick.get_button(9))  # Right Stick Button
                to_send[15] = 0  # D-Pad

                # This is the flag in the array
                to_send.insert(0, 250)
                self.comm.sendData(to_send)

                # For debug from Python, uncomment
                dumb = []
                for b in to_send:
                    dumb.append(b)
                print(dumb)

                # For debug from Arduino, uncomment
                self.comm.getBytesAvailableToRead()

    def sendnumberboxbuttonState(self):
        sendArray = bytearray(2)
        sendArray[0] = int(252)  # Flag value
        sendArray[1] = int(self.numberBox.value())  # restrictor value
        self.comm.sendData(sendArray)
        # For debug from Arduino, uncomment
        # self.comm.getBytesAvailableToRead()

    def quit(self):
        if self.my_joystick and self.my_joystick.get_init():
            self.my_joystick.quit()
        pygame.quit()

    # Overrides
    def closeEvent(self, event):
        # do stuff
        self.quit()
        event.accept()  # let the window close

    def send_reset(self):
        to_send = bytearray(0)
        to_send.insert(0, 251)
        self.comm.sendData(to_send)
        print ('Motors Are Reset!')

def getAxisValueInPercentage(axisValue:float)->int:
    # this converts direct joystick values from (-1,1) to (0,100)
    return int(((2.0 - (1.0 - axisValue)) * 100.0) / 2.0)

# Assumes range on axes is -100 to 100
class AxesWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()
        self.value = (0, 0)

    def initUI(self):
        self.setMinimumSize(120, 120)
        self.setMaximumSize(120, 120)

    def set_x_value(self, x):
        self.value = (x, self.value[1])

    def set_y_value(self, y):
        self.value = (self.value[0], y)

    def paintEvent(self, e):
        qp = QPainter()
        qp.begin(self)
        self.drawWidget(qp)
        qp.end()

    def drawWidget(self, qp):
        size = self.size()
        h = size.height()
        # step = int(round(h / 200))
        qp.setPen(QColor(255, 0, 0))
        qp.setBrush(QColor(0, 0, 0))
        x = self.value[0] / 2 + 60
        y = self.value[1] / 2 + 60
        qp.drawEllipse(x, y, 7, 7)
        qp.setPen(QColor(0, 0, 0))
        qp.setBrush(QColor(0, 0, 0, 0))
        qp.drawRect(9, 9, h - 11, h - 11)


class Communications:
    def __init__(self):
        self.SerialPort = serial.Serial()
        self.SerialPort.baudrate = 38400
        self.SerialPort.write_timeout = 0.1
        self.getListofPorts()
        self.openPort()

    def __del__(self):
        if self.SerialPort.is_open:
            self.SerialPort.close()

    def getListofPorts(self):
        ports = comports()
        for port in ports:
            print(port)
        return ports

    def openPort(self, port_name="COM3"):
        self.SerialPort.port = port_name
        self.SerialPort.open()

    def sendData(self, vals: bytearray):
        self.SerialPort.write(vals)

    def getBytesAvailableToRead(self):
        while self.SerialPort.in_waiting:
            print(self.SerialPort.readline())


if __name__ == "__main__":
    gui = QApplication(sys.argv)
    gui.setApplicationName("ControllerGUI")
    window = GUI()
    gui.exec_()
