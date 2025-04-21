import time
import serial


class PTServo:
    def __init__(self, serial_port, baud_rate):
        self.x = None
        self.y = None
        self.px = None
        self.py = None
        self.ready = False
        self.last_response = None
        
        self.ser = serial.Serial(serial_port, baud_rate)
        self.ser.timeout = 0
        print(f"Opened serial port {serial_port}")

        
class PTServoMB(PTServo):


    def move_a(self, x, y):
        message = f"a {x} {y}\n"\
            
        self.ser.write(message.encode())
        
    def move_r(self, x, y):
        message = f"r {int(x)} {int(y)}\n"\
            
        self.ser.write(message.encode())
        


class PTServoArduino(PTServo):

    angle_scale = 100 # Scaling factor for angles, b/c we can't parse floats wiith sscanf
    
    def __init__(self, serial_port, baud_rate):
        super().__init__(serial_port, baud_rate)
        
        for i in range(10):

            r = self.get_status().strip()
            print(r)
            if self.ready:
                break
            
            time.sleep(1)
        else:
            raise Exception("Could not connect to Arduino")

    def check_status(self):
        
        self.send_angles('?', 0, 0)
        time.sleep(0.1)
        return self.get_status()

    def move_a(self, x, y):
        
        self.send_angles('a', x, y)
        
    def move_r(self, x, y):
        
        self.send_angles('r', x, y)


    def make_command(self, code: int, angle1: float, angle2: float):
        
       return f"c: {code} {angle1} {angle2}\n"
   

    def send_angles(self, code: int, angle1 : float, angle2: float ):
        """
        Sends angles to a serial port.
        
        Args:
            ser: The serial port object.
            angle1 (float): The first angle to send.
            angle2 (float): The second angle to send.
        
        Returns:
            str: The response received from the serial port.
        """
        # Convert angles to 32-bit integers
        
        data = self.make_command(code, int(angle1*self.angle_scale), 
                                 int(angle2*self.angle_scale)).encode()
        
        self.ser.write(data)

        
    def get_status(self):
        o = ""
        while self.ser.in_waiting > 0:
            o += self.ser.read(self.ser.in_waiting).decode()
         
        lines = []
        for line in o.split('\n'):
            if line.startswith('pos:'):
                try:
                    _, x, y, self.px, self.py = line.split()
                    self.x = round(float(x), 1)
                    self.y = round(float(y), 1)
                    continue
                except Exception: 
                    print("Error parsing line:", line)
                    continue
              
            elif line.startswith('ready') or line.startswith('start'):
                self.ready = True
                continue
                
        
            
            lines.append(line)
        
        self.last_response = '\n'.join(lines)
        
        return self.last_response
    
            
        
        
    
