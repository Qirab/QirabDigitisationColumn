#include <AccelStepper.h>
#include <Keypad.h>

long motorsteps = 800 * 0.94; // stepper driver microstop setting adjusted for leadscrew calibration factor
long receivedDistance = 0; //rotations mm from the computer
long receivedSpeed = 0; //delay between two steps, received from the computer

bool runallowed = false; // booleans for new data from serial, and runallowed flag
bool updirection = true; // set direction as up default

// direction Digital 2 (CCW), pulses Digital 3 (CLK)
AccelStepper stepper(1, 3, 2);

// Define endstop pins
#define topstopPin 4
#define botstopPin 5

const byte ROWS = 4; //four rows
const byte COLS = 2; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
    {'6', 'A'},  //
    {'9', 'D'},
    {'#', 'C'},
    {'3', 'B'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {11, 10}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup()
{
    Serial.begin(9600); //define baud rate
    Serial.println("Camera Column Motor Intialized"); //print a message

    //setting up some default values for maximum speed and maximum acceleration
    stepper.setMaxSpeed(7000); //SPEED = Steps / second
    stepper.setAcceleration(600); //ACCELERATION = Steps /(second)

    stepper.disableOutputs(); //disable outputs, so the motor is not getting warm (no current)

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Declare Endstop pins as output:
    pinMode(topstopPin, INPUT_PULLUP);
    pinMode(botstopPin, INPUT_PULLUP);
}

void loop()
{
    checkKey(); //check keypad matrix for new commands
    continuousRun(); //method to handle the motor
}


void continuousRun() //method for the motor
{
    volatile int topstopValue = digitalRead(topstopPin);
    if (topstopValue == 0 and updirection == true)
    {
        stopall();
        Serial.print("STOP Top Endstop\n");
        return;
    }

    volatile int botstopValue = digitalRead(botstopPin);
    if (botstopValue == 0 and updirection == false)
    {
        stopall();
        Serial.print("STOP Bottom Endstop\n");
        return;
    }

    if (runallowed == true)
    {
        if (abs(stepper.currentPosition()) < receivedDistance)
        {
            stepper.enableOutputs(); //enable pins
            stepper.run(); //step the motor (this will step the motor by 1 step at each loop)
        }
        else //program enters this part if the required distance is completed
        {
            Serial.print("POS: ");
            Serial.println(stepper.currentPosition()); // print pos -> this will show you the latest relative number of steps
            stopall();
        }
        }
        else //program enters this part if the runallowed is FALSE, we do not do anything
        {
            return;
        }
}

void checkKey() //method for receiving the commands
{
    char customKey = customKeypad.getKey();

    if (customKey){
        digitalWrite(LED_BUILTIN, LOW);
        switch(customKey) {
            case '#':
                Serial.println("Down 100mm "); //print action
                receivedSpeed = 7000; //set speed
                receivedDistance = 100 * motorsteps; //set distance
                updirection = false;
                runallowed = true;
                break;
            case '9':
                Serial.println("Down 10mm "); //print action
                receivedSpeed = 5000; //set speed
                receivedDistance = 10 * motorsteps; //set distance
                updirection = false;
                runallowed = true;
                break;
            case '6':
                Serial.println("Down 1mm "); //print action
                receivedSpeed = 5000; //set speed
                receivedDistance = 1 * motorsteps; //set distance
                updirection = false;
                runallowed = true;
                break;
            case '3':
                stopall();
                Serial.println("STOP");
                break;
            case 'D':
                Serial.println("UP 100mm"); //print action
                receivedSpeed = 7000; //set speed
                receivedDistance = 100 * motorsteps; //set distance
                updirection = true;
                runallowed = true;
                break;
            case 'C':
                Serial.println("UP 10mm"); //print action
                receivedSpeed = 5000; //set speed
                receivedDistance = 10 * motorsteps; //set distance
                updirection = true;
                runallowed = true;
                break;
            case 'B':
                Serial.println("UP 1mm"); //print action
                receivedSpeed = 5000; //set speed
                receivedDistance = 1 * motorsteps; //set distance
                updirection = true;
                runallowed = true; //allow running
                break;
            case 'A':
                 Serial.println("UP 1000mm"); //print action
                receivedSpeed = 7000; //set speed
                receivedDistance = 1000 * motorsteps; //set distance
                updirection = true;
                runallowed = true;
                break;
        }
        if (runallowed == true and updirection == true) {
            stepper.setMaxSpeed(receivedSpeed); // speed
            stepper.move(receivedDistance); // move
        }
        else if (runallowed == true and updirection == false)
        {
            stepper.setMaxSpeed(receivedSpeed); //speed
            stepper.move(-1 * receivedDistance); // move reverse
        }
    }
}


void stopall ()
{
    stepper.stop(); //stop motor
    stepper.setCurrentPosition(0); // reset position
    stepper.disableOutputs(); // disable power
    runallowed = false; // stop run if either endstop is active
    digitalWrite(LED_BUILTIN, HIGH);
}

void blink()
{
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(100);                       // wait for a second
}
