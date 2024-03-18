import os
import numpy as np
import picamera
from picamera.array import PiMotionAnalysis
from time import sleep
from datetime import datetime

class GestureDetector(PiMotionAnalysis):
    QUEUE_SIZE = 10 # the number of consecutive frames to analyze
    THRESHOLD = 3.0 # the minimum average motion required in either axis

    def __init__(self, camera):
        super(GestureDetector, self).__init__(camera)
        self.x_queue = np.zeros(self.QUEUE_SIZE, dtype=np.float)
        self.y_queue = np.zeros(self.QUEUE_SIZE, dtype=np.float)
        self.last_move = ''
        self.output = "../data/data_server.txt"
        self.line_nb = self.get_last_line_number() # the number of line already in data_file

    def get_last_line_number(self):
        try:
            with open(self.output,'r') as f:
                lines = f.readlines()
                if lines:
                    nb = int(lines[-1].split(';')[0])+1
                    return nb
                else:
                    return 0
        except FileNotFoundError:
            return 0


    def analyze(self, a):
        # Roll the queues and overwrite the first element with a new
        # mean (equivalent to pop and append, but faster)
        self.x_queue[1:] = self.x_queue[:-1]
        self.y_queue[1:] = self.y_queue[:-1]
        self.x_queue[0] = a['x'].mean()
        self.y_queue[0] = a['y'].mean()
        # Calculate the mean of both queues
        x_mean = self.x_queue.mean()
        y_mean = self.y_queue.mean()
        x_move = (
            '' if abs(x_mean) < self.THRESHOLD else
            'left' if x_mean < 0.0 else
            'right')
        y_move = (
            '' if abs(y_mean) < self.THRESHOLD else
            'down' if y_mean < 0.0 else
            'up')
        # Update the display
        movement = ('%s %s' % (x_move, y_move)).strip()
        if movement != self.last_move:
            self.last_move = movement
            if movement: 
                print(movement)
                print("x :", self.x_queue[0], "y :", self.y_queue[0])
                current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                with open(self.output, 'a') as f:
                    f.write(f"{self.line_nb};{current_time}\n")
                    self.line_nb+=1


with picamera.PiCamera(resolution='VGA', framerate=24) as camera:
    with GestureDetector(camera) as detector:
        camera.start_recording(
            os.devnull, format='h264', motion_output=detector)
        try:
            while True:
                camera.wait_recording(5)
        finally:
            camera.stop_recording()