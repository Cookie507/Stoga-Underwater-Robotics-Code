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
#define SERVOMIN  200 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  410 // this is the 'maximum' pulse length count (out of 4096)
//Servo arm;
//Servo wrist;
//Servo hand;
//Servo cam;
//Servo cam2;
//Servo rise;
//Servo rise2;
//Servo forL;
//Servo forR;



int risePin = 0; //triggers
int rise2Pin = 1;
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

void setup() {
  Serial.begin(38400);
  
  pwm.begin();
  
  pwm.setPWMFreq(50);  // ESC's run at ~50 Hz updates

  delay(10);

  pwm.setPWM(forLPin, 0, SERVOMIN); 
  pwm.setPWM(forRPin, 0, SERVOMIN); 
  pwm.setPWM(armPin, 0, angleToPulseLength(armAngle));
  pwm.setPWM(wristPin, 0, angleToPulseLength(wristAngle));
  pwm.setPWM(handPin, 0, angleToPulseLength(handAngle));
  pwm.setPWM(camPin, 0, angleToPulseLength(camAngle));
  pwm.setPWM(cam2Pin, 0, angleToPulseLength(cam2Angle));
  pwm.setPWM(risePin, 0, angleToPulseLength(90));
  pwm.setPWM(rise2Pin, 0, angleToPulseLength(94));
  
}

void loop() {
  if(Serial.available() >= 16) {
     while(Serial.read()!=250){} //only start taking data if it begins with this flag byte
     for(int i = 0; i< 16; i++) {
        bytes[i] = Serial.read();
        if(bytes[i]==250) { //throw the array away if there's a 250 in it (as 250 is invalid)
          i=-1;
          for(int j = 0; j< 16; j++)bytes[j]=0;
        }
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
     }
     
     for(int i=0; i<16; i++) { //send data back to java so we know what we're doing
       Serial.print(bytes[i]);
       Serial.print("\t");
     }
     Serial.println();
     
     if(bytes[0] <=60 && bytes[0] >=40)bytes[0]=50; //make sure we have a little tolerance
     if(bytes[1] <=60 && bytes[1] >=40)bytes[1]=50;
     if(bytes[2] <=60 && bytes[2] >=40)bytes[2]=50;
     if(bytes[3] <=60 && bytes[3] >=40)bytes[3]=50;
     if(bytes[4] <=53 && bytes[4] >=47)bytes[4]=50; //the triggers need less of a dead spot
     
     differentialDrive(bytes[1], bytes[0]);
     iDontKnowWhyThisWorksButItDo(bytes[4]);
     
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
     
     if(bytes[12]==1)pinMode(13,HIGH);
     else pinMode(13,LOW); //debugging light, uses start button
  }
  // delay(5); //just to make sure we don't update TOO fast
}

/*this will change an angle. It's just here so I
 *don't have to write the same three lines over and over
 */
int updateAngle(int input, int angle, int increment) {
  if(input > 60 && angle <= 150) angle+=increment;
  else if(input < 40 && angle >= 30) angle -=increment;
  return angle;
}


/*this will change angles on the servos of the claw's
 *arm and hand. It's just here so I
 *don't have clutter the body of loop()
 */
int servoIncrement(int b1, int b2, int angle) {
  if((b1 == 1 && b2 == 0) && (angle <= 160)) angle+=5;
  else if((b1 == 0 && b2 == 1) && (angle >= 20)) angle-=5;
  return angle;
}

/*this will take a combo of x and y vals from a joystick
 *and turn it into motion. Pin params aren't necessary, as
 *this will only ever control forL and forR
 */
 //turn=x value (0,100)
 //thrust=y value (0,100)
void differentialDrive(int turn, int thrust) {
  turn -= 50;
  turn=-turn;
  int rMotor = thrust+constrain(thrust + turn,0,100);
  int lMotor = thrust+constrain(thrust - turn,0,100);
  pwm.setPWM(forRPin, 0, angleToPulseLength(rMotor));
  pwm.setPWM(forLPin, 0, angleToPulseLength(lMotor));
//  DEBUGGING
//  Serial.print("R: ");
//  Serial.print(map(rMotor,0,100,75,105)); 
//  Serial.print(" L: ");
//  Serial.println(map(lMotor,0,100,75,105));
}

void iDontKnowWhyThisWorksButItDo(byte angle) {
  angle = map(angle, 0, 100, 60, 120);
  // Serial.print("Rise Angle: ");
  // Serial.println(angle);
  pwm.setPWM(risePin, 0, angleToPulseLength(angle));
  if(angle >=90 && angle <=94) pwm.setPWM(rise2Pin, 0, angleToPulseLength(94));
  else pwm.setPWM(rise2Pin, 0, angleToPulseLength(angle));
}

int angleToPulseLength(int angle)
{
int pulselength = map(angle, 0, 180, SERVOMIN, SERVOMAX);
return pulselength;
}

