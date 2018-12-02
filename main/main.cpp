/*
 * Stoga Underwater Robotics Team
 * 
 *  Main.cpp
 *
 *  Authors: Arnav Kaushik, Eugene Mak
 *
 *
 *  © 2018 Arnav Kaushik. All Rights Reserved.
 *
 */

/*
 * Version 0.0.1
 * 
 * • Imports required libraries 
 * • Initializes variables
 * • Sets serial to 9600 baud rate
 * • Creates servo functions 
 */


//Libraries
#include <Servo.h>
#include <SoftwareSerial.h>

//Servos
Servo rightESC;  /*!< The servo corresponding to the right ESC */
Servo leftESC;  /*!< The servo corresponding to the left ESC */

//Pin Definitions
const int RIGHT_ESC_PIN = 8;  /*!< The pin corresponding to the right ESC */
const int LEFT_ESC_PIN = 9;  /*!< The pin corresponding to the left ESC */


void setup() {

  //Begin Serial
  Serial.begin(9600);

  //Attach Pins
  rightESC.attach(RIGHT_ESC_PIN);
  leftESC.attach(LEFT_ESC_PIN);
}

void loop() {
  //DELETE THIS CODE
  delay(1000);
  stopMotor(leftESC);
  startMotor(rightESC);
  delay(1000);
  stopMotor(rightESC);
  startMotor(leftESC);
}

//! This function sets the servo to 90
/*!
   \param servo A Servo object.
   \sa stopMotor()
*/
void startMotor(Servo servo) {
  servo.write(90);
}

//! This function sets the servo to 0
/*!
   \param servo A Servo object.
   \sa startMotor()
*/
void stopMotor(Servo servo) {
  servo.write(0);
}
