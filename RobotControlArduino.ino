
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

byte bytes[16];

//Servo wires
//Brown - Ground
//Red - Power
//Orange - Signal

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
// For 50Hz PWM update rate: 4096 (counts) / 20 ms = 205 counts/ms
// min position at 1ms = 205 counts
// max position at 2ms = 410 counts
#define SERVOMIN  190 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  440 // this is the 'maximum' pulse length count (out of 4096)

int risePin = 0; //triggers
int sinkPin = 1;
int forLPin = 2; //left stick combo
int forRPin = 3; //left stick combo
int armPin = 4; //a and b
int wristPin = 5;
int handPin = 6; //bumpers
int camPin = 7; //right stick x
int cam2Pin = 8; //right stick y

int armAngle = 90;
int wristAngle = 90;
int handAngle = 90;
int camAngle = 90;
int cam2Angle = 90;

int dataCount = 0;
bool collectData = false;
bool failsafestate = true;

void setup() {
  Serial.begin(38400);

  pwm.begin();

  pwm.setPWMFreq(50);  // ESC's run at ~50 Hz updates

  delay(10);

  Serial.println("Setting up da servos!");

  pwm.setPWM(forLPin, 0, SERVOMIN);
  pwm.setPWM(forRPin, 0, SERVOMIN);
  pwm.setPWM(risePin, 0, SERVOMIN);
  pwm.setPWM(sinkPin, 0, SERVOMIN);
  pwm.setPWM(armPin, 0, angleToPulseLength(armAngle));
  pwm.setPWM(wristPin, 0, angleToPulseLength(wristAngle));
  pwm.setPWM(handPin, 0, angleToPulseLength(handAngle));
  pwm.setPWM(camPin, 0, angleToPulseLength(camAngle));
  pwm.setPWM(cam2Pin, 0, angleToPulseLength(cam2Angle));

  delay(8000);
  Serial.println("Ready!");
}

void loop() {
  if (Serial.available())
  {
    if (Serial.peek() == 250)
    {
      Serial.read();
      collectData = true;
      dataCount = 0;
    }
    else if (Serial.peek() == 251)
    {
      Serial.read();
      pwm.setPWM(forLPin, 0, SERVOMIN);
      pwm.setPWM(forRPin, 0, SERVOMIN);
      pwm.setPWM(risePin, 0, SERVOMIN);
      pwm.setPWM(sinkPin, 0, SERVOMIN);
      delay(3000);
    }
    else if (collectData && dataCount < 16)
    {
      bytes[dataCount] = Serial.read();
      dataCount += 1;
    }
    else
    {
      collectData = false;
      dataCount = 0;
    }

    // remap  pls
    //0: Left Stick Y, 0-100
    //1: Left Stick X, 0-100
    //2: Right Stick Y, 0-100
    //3: Right Stick X, 0-100
    //4: Triggers, 0-100
    //5-8: A, B, X, Y
    //9-10: Bumpers
    //11-12: Back, Start
    //13: Left Stick Button
    //14: Right Stick Button
    //15: Hat Switch, 0-8
    //For Hat, 0 is resting, 1:NW, 2:N, continues clockwise

    if (dataCount == 16)
    {
      // process the byte buffer
      //      for(int i=0; i<16; i++) { //send data back to java so we know what we're doing
      //        Serial.print(bytes[i]);
      //        Serial.print(", ");
      //      }
      //      Serial.println("");

      // Set motors and camera positions
      if (bytes[2] <= 60 && bytes[2] >= 40)bytes[2] = 50;
      if (bytes[3] <= 60 && bytes[3] >= 40)bytes[3] = 50;
      if (bytes[4] <= 55 && bytes[4] >= 45)bytes[4] = 50; //the triggers need less of a dead spot

      if (bytes[12] == 1)
      {
        failsafestate = !failsafestate;
      }

      if (failsafestate == true)
      {
        Serial.println("FAILSAFESTATE TRUE");
        pwm.setPWM(forLPin, 0, SERVOMIN);
        pwm.setPWM(forRPin, 0, SERVOMIN);
        pwm.setPWM(risePin, 0, SERVOMIN);
        pwm.setPWM(sinkPin, 0, SERVOMIN);
        pwm.setPWM(armPin, 0, angleToPulseLength(armAngle));
        pwm.setPWM(wristPin, 0, angleToPulseLength(wristAngle));
        pwm.setPWM(handPin, 0, angleToPulseLength(handAngle));
        pwm.setPWM(camPin, 0, angleToPulseLength(camAngle));
        pwm.setPWM(cam2Pin, 0, angleToPulseLength(cam2Angle));
      }
      else
      {
        Serial.println("FAILSAFESTATE FALSE");
        differentialDrive(bytes[1], bytes[0]);
        moveUpAndDown(bytes[4]);

        cam2Angle = updateAngle(bytes[2], cam2Angle, 2);
        cam2Angle = constrain(cam2Angle, 60, 90);
        pwm.setPWM(cam2Pin, 0, angleToPulseLength(cam2Angle));
        //Serial.print("Camera Angles: ");
        //Serial.print(cam2Angle);
        //Serial.print(" ");

        camAngle = updateAngle(bytes[3], camAngle, 2);
        camAngle = constrain(camAngle, 60, 150);
        pwm.setPWM(camPin, 0, angleToPulseLength(camAngle));
        //Serial.println(camAngle);

        handAngle = servoIncrement(bytes[9], bytes[10], handAngle);
        pwm.setPWM(handPin, 0, angleToPulseLength(handAngle));

        armAngle = servoIncrement(bytes[5], bytes[6], armAngle);
        pwm.setPWM(armPin, 0, angleToPulseLength(armAngle));

        wristAngle = servoIncrement(bytes[7], bytes[8], wristAngle); //this is for testing the servo ONLY
        pwm.setPWM(wristPin, 0, angleToPulseLength(wristAngle));

        if (bytes[12] == 1)pinMode(13, HIGH);
        else pinMode(13, LOW); //debugging light, uses start button

      }


    }

  }
  return;
}

/*this will change an angle. It's just here so I
  don't have to write the same three lines over and over
*/
int updateAngle(int input, int angle, int increment) {
  if (input > 60 && angle <= 150) angle += increment;
  else if (input < 40 && angle >= 30) angle -= increment;
  return angle;
}


/*this will change angles on the servos of the claw's
  arm and hand. It's just here so I
  don't have clutter the body of loop()
*/
int servoIncrement(int b1, int b2, int angle) {
  if ((b1 == 1 && b2 == 0) && (angle <= 160)) angle += 5;
  else if ((b1 == 0 && b2 == 1) && (angle >= 20)) angle -= 5;
  return angle;
}

/*this will take a combo of x and y vals from a joystick
  and turn it into motion. Pin params aren't necessary, as
  this will only ever control forL and forR
*/
//turn=x value (0,100)
//thrust=y value (0,100)
void differentialDrive(int turn, int thrust) {
  thrust -= 100; // now range is -100 to 100
  turn -= 100; // now range is -100 to 100
  //  int rMotor = (-turn + 100)/2;
  //  int lMotor = (turn + 100)/2;
  double rMotor = 0.0;
  double lMotor = 0.0;
  double A = 0.0;

  // Only move in quadrant 1 or 2
  if (thrust > 0)  //positive y values
  {
    A = sqrt(sq(turn) + sq(thrust));
    if (A >= 10)
    {
      if (turn < 0)
      {
        rMotor = A;
        lMotor = ((turn / 100.0) + 1.0) * A; //smaller value then A
      }
      else
      {
        lMotor = A;
        rMotor = (1.0 - (turn / 100.0)) * A;
      }
    }
    else
    {
      lMotor = 0;
      rMotor = 0;
    }
  }

  if (thrust < 0) //negative y values (going backwards wheeeee)
  {
    A = sqrt(sq(turn) + sq(thrust));
    if (A >= 10)
    {
      if (turn < 0)
      {
        rMotor = -A;
        lMotor = -((turn / 100.0) + 1.0) * A; //smaller value then A
      }
      else
      {
        lMotor = -A;
        rMotor = -(1.0 - (turn / 100.0)) * A;
      }
    }
    else
    {
      lMotor = 0;
      rMotor = 0;
    }
  }

  //  DEBUGGING
  //  Serial.print("lMotor: ");
  //  Serial.print(lMotor);
  //  Serial.print(", rMotor: ");
  //  Serial.print(rMotor);
  //  Serial.print(", turn: ");
  //  Serial.print(turn);
  //  Serial.print(", thrust: ");
  //  Serial.print(thrust);
  //  Serial.print(", A: ");
  //  Serial.println(A);

  rMotor = constrain(rMotor, -100, 100);
  lMotor = constrain(lMotor, -100, 100);
  rMotor = map(rMotor, -100, 100, SERVOMIN, SERVOMAX);
  lMotor = map(lMotor, -100, 100, SERVOMIN, SERVOMAX);

  pwm.setPWM(forRPin, 0, int(rMotor));
  pwm.setPWM(forLPin, 0, int(lMotor));
}

// value has range (0, 100). 0=Full down, 100=full up, 50=stay put
void moveUpAndDown(int value) {
  // angle = map(angle, 0, 100, 60, 120);
  // Serial.print("Rise Angle: ");
  // Serial.println(angle);

  // Go down
  if (value <= 45)
  {
    value = map(value, 45, 0, SERVOMIN, SERVOMAX);
    pwm.setPWM(risePin, 0, SERVOMIN);
    pwm.setPWM(sinkPin, 0, value);
  }
  // Go up
  else if (value >= 55)
  {
    value = map(value, 55, 100, SERVOMIN, SERVOMAX);
    pwm.setPWM(risePin, 0, value);
    pwm.setPWM(sinkPin, 0, SERVOMIN);
  }
  else
  {
    pwm.setPWM(risePin, 0, SERVOMIN);
    pwm.setPWM(sinkPin, 0, SERVOMIN);
  }
}

int angleToPulseLength(int angle)
{
  int pulselength = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  return pulselength;
}













