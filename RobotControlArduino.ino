#include <Servo.h>

#include <Wire.h>

byte bytes[16];

//Servo wires
//Brown - Ground
//Red - Power
//Orange - Signal

Servo arm;
Servo wrist;
Servo hand;
Servo cam;
Servo cam2;
Servo rise;
Servo rise2;
Servo forL;
Servo forR;

int risePin = 6; //triggers
int rise2Pin = 7;
int forLPin = 4; //left stick combo
int forRPin = 5; //left stick combo
int armPin = 10; //a and b
int wristPin = 11;
int handPin = 12; //bumpers
int camPin = 8; //right stick x
int cam2Pin = 9; //right stick y

int armAngle = 90;
int wristAngle = 90;
int handAngle = 90;
int camAngle = 90;
int cam2Angle = 90;

void setup() {
  Serial.begin(115200);

  arm.attach(armPin);
  arm.write(armAngle);
    
  wrist.attach(wristPin);
  wrist.write(wristAngle);
  
  hand.attach(handPin);
  hand.write(handAngle);

  cam.attach(camPin);
  cam.write(camAngle);

  rise.attach(risePin);
  rise.write(90);
  rise2.attach(rise2Pin);
  rise2.write(94);//this one is weird, it needs to be a little over 90
  forL.attach(forLPin);
  forR.attach(forRPin);

  cam2.attach(cam2Pin);
  cam2.write(cam2Angle);
  
  delay(2000);
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
     //for(int i=0; i<16; i++) { //send data back to java so we know what we're doing
       //Serial.print(bytes[i]);
       //Serial.print("\t");
     //}
     //Serial.println();
     if(bytes[0] <=60 && bytes[0] >=40)bytes[0]=50; //make sure we have a little tolerance
     if(bytes[1] <=60 && bytes[1] >=40)bytes[1]=50;
     if(bytes[2] <=60 && bytes[2] >=40)bytes[2]=50;
     if(bytes[3] <=60 && bytes[3] >=40)bytes[3]=50;
     if(bytes[4] <=53 && bytes[4] >=47)bytes[4]=50; //the triggers need less of a dead spot
     
     differentialDrive(bytes[1], bytes[0]);
     iDontKnowWhyThisWorksButItDo(bytes[4]);
     
     cam2Angle = updateAngle(bytes[2], cam2Angle, 2);
     cam2Angle = constrain(cam2Angle, 60, 90);
     cam2.write(cam2Angle);
     //Serial.print("Camera Angles: ");
     //Serial.print(cam2Angle);
     //Serial.print(" ");
     
     camAngle = updateAngle(bytes[3], camAngle, 2);
     camAngle = constrain(camAngle, 60, 150);
     cam.write(camAngle);
     //Serial.println(camAngle);
     
     handAngle = servoIncrement(bytes[9], bytes[10], handAngle);
     hand.write(handAngle);
     
     armAngle = servoIncrement(bytes[5], bytes[6], armAngle);
     arm.write(armAngle);

     wristAngle = servoIncrement(bytes[7], bytes[8], wristAngle); //this is for testing the servo ONLY
     wrist.write(wristAngle);
     
     if(bytes[12]==1)pinMode(13,HIGH);
     else pinMode(13,LOW); //debugging light, uses start button
  }
  delay(5); //just to make sure we don't update TOO fast
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
void differentialDrive(int turn, int thrust) {
  turn -= 50;
  int rMotor = 100-constrain(thrust + turn,0,100);
  int lMotor = 100-constrain(thrust - turn,0,100);
  forR.write(map(rMotor,0,100,75,105)+3);
  forL.write(map(lMotor,0,100,75,105)+3);//TRUST THE MATH, IT'S TOO COMPLEX FOR MY BRAIN
//  DEBUGGING
//  Serial.print("R: ");
//  Serial.print(map(rMotor,0,100,75,105)); 
//  Serial.print(" L: ");
//  Serial.println(map(lMotor,0,100,75,105));
}

void iDontKnowWhyThisWorksButItDo(byte angle) {
  angle = map(angle, 0, 100, 60, 120);
  Serial.print("Rise Angle: ");
  Serial.println(angle);
  rise.write(angle);
  if(angle >=90 && angle <=94) rise2.write(94);
  else rise2.write(angle);
}
