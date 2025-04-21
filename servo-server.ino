/*
Reads 3 32-bit integers from Serial and sets the first two to the angles of the first two servos, using an 
Adafruit 16-channel PWM/Servo driver.  ( Actually any PCA9685 will do ). 

The angles are in servo degrees, 0 to 180, and are scaled by 10000.  The data packet
incluide several checks for integrity.
*/

#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);  // default address 0x40

#define SERVOMIN  125  // minimum pulse length count (out of 4096)
#define SERVOMAX  625  // maximum pulse length count (out of 4096)
#define ANGLE_SCALE 100  // Scaling factor for angles, b/c we can't parse floats wiith sscanf

int angleToPulse(float ang) {
  int pulse = map((int)ang, 0, 180, SERVOMIN, SERVOMAX);
  return pulse;
}

// Buffer for incoming serial lines
#define LINE_BUF_SIZE 64
char lineBuf[LINE_BUF_SIZE];


bool validateAngle(char cmd, int angle) {

  if (cmd == 'r' & (angle < -180*ANGLE_SCALE or angle > 180*ANGLE_SCALE)) {
    return false;
  
  } else if (cmd == 'a' & (angle < 0 or angle > 180*ANGLE_SCALE)) {
    return false;
  }

  return true  ;
}


// Command structure to hold parsed command data
struct Command {
  char code;
  float angles[2];

  bool valid;
  char errorMessage[64];

  // Parse a line and populate the command fields
  void parse(const char* line) {
    char prefix;
    int scaled_angles[2];
    int n = sscanf(line, "%c: %c %d %d", &prefix, &code, &scaled_angles[0], &scaled_angles[1]);
    valid = false;
    
    if (n != 4 || prefix != 'c') {
      sprintf(errorMessage, "Invalid command format (n=%d, prefix=%c)", n, prefix);
    } else if (code != '?' && code != 'a' && code != 'r') {
      strcpy(errorMessage, "Invalid command code");
    } else if ( !validateAngle(code, scaled_angles[0]) || !validateAngle(code, scaled_angles[1])) {
      strcpy(errorMessage, "Invalid angle");
    } else {
      valid = true;
      angles[0] = (float)scaled_angles[0] / (float)ANGLE_SCALE;
      angles[1] = (float)scaled_angles[1] / (float)ANGLE_SCALE;
      errorMessage[0] = '\0';  // Clear error message
    }

  }
};

// Singleton instance
Command command;

void setup() {
  Serial.begin(115200);
  board1.begin();
  board1.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

  while (!Serial) {
    ; // Wait for Serial to be ready
  }
  Serial.println("start.");
  lineBuf[0] = '\0';  // Initialize line buffer
  command.valid = false;  // Initialize command validity
}

// Array to keep track of the last angles for the servos
float lastAngles[2] = {0.0, 0.0};
int lastPulses[2] = {0, 0};

// Helper to read a line from Serial into lineBuf
bool readSerialLine(char* buf, size_t bufsize) {
  static size_t idx = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue; // ignore CR
    if (c == '\n') {
      buf[idx] = '\0';
      idx = 0;
      return true;
    }
    if (idx < bufsize - 1) {
      buf[idx++] = c;
    }
  }
  return false;
}

void printPos(){
  Serial.print  ("pos: ");
  Serial.print  (lastAngles[0]);
  Serial.print  (" ");
  Serial.print  (lastAngles[1]);
  Serial.print  (" ");
  Serial.print  (lastPulses[0]);
  Serial.print  (" ");
  Serial.println(lastPulses[1]);

}


void loop() {

  while(!readSerialLine(lineBuf, LINE_BUF_SIZE));

  command.parse(lineBuf);

  if (!command.valid) {
    Serial.print("e: ");
    Serial.print(command.errorMessage);
    Serial.print(" \"");
    Serial.print(lineBuf);
    Serial.println("\"");
    return;
  } 
  
  if (command.code == '?') {
    Serial.println("ready.");
    return;
  }

  // Set angles to servos
  for (int i = 0; i < 2; i++) {
  
    if (command.code == 'r') { // angle is relative to last angle
      command.angles[i] += lastAngles[i];
    }
    int pulse = angleToPulse(command.angles[i]);
    board1.setPWM(i, 0, pulse);
    lastAngles[i] = command.angles[i];
    lastPulses[i] = pulse;
  }

  printPos();
} 
