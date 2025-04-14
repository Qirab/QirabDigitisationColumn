// Qirab Digitization Column
// http://qirab.org
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

// The QDC uses the following Arduino libraries which you mut download in teh Arduino IDE.
#include <AccelStepper.h>
#include <Keypad.h>

long motorsteps = 3200 * 0.125;  // stepper driver microstop setting adjusted for leadscrew calibration factor
long receivedDistance = 0;       //rotations mm from the computer
long receivedSpeed = 0;          //delay between two steps, received from the computer

bool runallowed = false;   // booleans for new data from serial, and runallowed flag
bool downdirection = false;  // set direction as UP default

// direction Digital 2 (CCW), pulses Digital 3 (CLK)
AccelStepper stepper(1, 3, 2);

// Define endstop pins 
#define botstopPin 5
#define topstopPin 4

const byte ROWS = 4;  //four rows
const byte COLS = 2;  //twobr columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  { '3', '7' },  //
  { '2', '6' },
  { '1', '5' },
  { '0', '4' }
};
byte rowPins[ROWS] = { 9, 8, 7, 6 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 11, 10 };      //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);  //define baud rate
  Serial.println("Qirab Digitization Column v1.2");
  Serial.println("QDC100 - Serial: 000010");
  Serial.println("This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.");
  Serial.println("http://qirab.org");
  Serial.println("Camera Column Motor Intialized");
  Serial.print("\n");

  //setting up some default values for maximum speed and maximum acceleration
  stepper.setMaxSpeed(8000);     //SPEED = Steps / second
  stepper.setAcceleration(500);  //ACCELERATION = Steps /(second)

  stepper.disableOutputs();  //disable outputs, so the motor is not getting warm (no current)

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // Declare Endstop pins as output:
  pinMode(botstopPin, INPUT_PULLUP);
  pinMode(topstopPin, INPUT_PULLUP);

  pinMode(12, OUTPUT);  // sets the digital pin 12 as output for stepper drive enable/disable
}

void loop() {
  checkKey();       //check keypad matrix for new commands
  continuousRun();  //method to handle the motor
}


void continuousRun()  //method for the motor
{
  volatile int topstopValue = digitalRead(topstopPin);
  volatile int botstopValue = digitalRead(botstopPin);

  if (topstopValue == 0 and downdirection  == true) {
    // bottom endstop switch logic
    runallowed = false;
    downdirection = false;
    Serial.print("STOP Bottom Endstop - topstopValue:");
    Serial.print(topstopValue);
    Serial.print(" botstopValue:");
    Serial.print(botstopValue);
    Serial.print("\n");
    return;
  }

  if (botstopValue == 0 and downdirection == false) {
        // top endstop switch logic
    runallowed = false;
    downdirection = true;
    Serial.print("STOP Top Endstop - topstopValue:");
    Serial.print(topstopValue);
    Serial.print(" botstopValue:");
    Serial.print(botstopValue);
    Serial.print("\n");
    return;
  }



  if (runallowed == true) {
    if (abs(stepper.currentPosition()) < receivedDistance) {
      // actiovate motor
      stepper.enableOutputs();  //enable pins
      digitalWrite(12, HIGH);   // enable motor driver
      stepper.run();            //step the motor (this will step the motor by 1 step at each loop)
    } else                      //program enters this part if the required distance is completed
    {
      Serial.print("POS: ");
      Serial.println(stepper.currentPosition());  // print pos -> this will show you the latest relative number of steps
      stopall();
    }
  } else  //program enters this part if the runallowed is FALSE, we do not do anything
  {
    stopall();
    return;
  }
}

void checkKey()  //method for receiving the commands
{
  char customKey = customKeypad.getKey();

  if (customKey) {
    digitalWrite(LED_BUILTIN, LOW);
    switch (customKey) {
      case '0':
        runallowed = false;
        Serial.println("STOP");
        break;
      case '1':
        Serial.println("Down 1600mm ");        //print action
        receivedSpeed = 8000;                  //set speed
        receivedDistance = 1600 * motorsteps;  //set max distance; for QDC150
      downdirection  = true;
        runallowed = true;
        break;
      case '2':
        Serial.println("Down 10mm ");        //print action
        receivedSpeed = 5000;                //set speed
        receivedDistance = 10 * motorsteps;  //set distance
      downdirection  = true;
        runallowed = true;
        break;
      case '3':
        Serial.println("Down 1mm ");        //print action
        receivedSpeed = 5000;               //set speed
        receivedDistance = 1 * motorsteps;  //set distance
      downdirection  = true;
        runallowed = true;
        break;
      case '4':
        Serial.println("Down 0.5mm");         //print action
        receivedSpeed = 3000;                 //set speed
        receivedDistance = 0.5 * motorsteps;  //set distance
      downdirection  = true;
        runallowed = true;
        break;
      case '5':
        Serial.println("UP 1mm");           //print action
        receivedSpeed = 5000;               //set speed
        receivedDistance = 1 * motorsteps;  //set distance
      downdirection  = false;
        runallowed = true;  //allow running
        break;
      case '6':
        Serial.println("UP 10mm");           //print action
        receivedSpeed = 5000;                //set speed
        receivedDistance = 10 * motorsteps;  //set distance
      downdirection  = false;
        runallowed = true;
        break;
      case '7':
        Serial.println("UP 1600mm");           //print action
        receivedSpeed = 8000;                  //set speed
        receivedDistance = 1600 * motorsteps;  //set max distance; for QDC150
      downdirection  = false;
        runallowed = true;
        break;
    }
    if (runallowed == true and downdirection  == true) {
      stepper.setMaxSpeed(receivedSpeed);  // speed
      stepper.move(receivedDistance);      // move down
    } else if (runallowed == true and downdirection  == false) {
      stepper.setMaxSpeed(receivedSpeed);   //speed
      stepper.move(-1 * receivedDistance);  // move up
    }
  }
}


void stopall() {
  stepper.stop();                 //stop motor
  stepper.setCurrentPosition(0);  // reset position
  stepper.disableOutputs();       // disable power
  digitalWrite(12, LOW);          // diasble motor but setting PIN12 to LOW
  runallowed = false;             // disable run in software
  digitalWrite(LED_BUILTIN, HIGH);
}

void blink() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100);                       // wait for a second
}
