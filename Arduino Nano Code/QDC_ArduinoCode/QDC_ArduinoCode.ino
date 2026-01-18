// Qirab Digitization Column
// http://qirab.org
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

// The QDC uses the following Arduino libraries which you mut download in teh Arduino IDE.
#include <AccelStepper.h>
#include <Keypad.h>

// Variables for button system
long motorsteps = 3200 * 0.125;  // stepper driver microstop setting adjusted for leadscrew calibration factor
long receivedDistance = 0;       //rotations mm from the computer
long receivedSpeed = 0;          //delay between two steps, received from the computer

bool runallowed = false;   // booleans for new data from serial, and runallowed flag
bool downdirection = false;  // set direction as UP default

// ============================================
// KY-040 Rotary Encoder Configuration
// ============================================
// Define rotary encoder pins
#define encoderCLK A2   // CLK pin of KY-040
#define encoderDT A1    // DT pin of KY-040
#define encoderSW A0    // SW (button) pin of KY-040

// Variables for rotary encoder
bool encoderActive = false;         // encoder is inactive by default
int lastCLKState;                   // stores the last state of CLK pin
int lastDTState;                    // stores the last state of DT pin for state validation
unsigned long lastRotationTime = 0; // time of last rotation for debouncing
const int rotationDebounce = 5;     // minimum ms between rotation signals (time-based debounce)

// Button debounce variables
int buttonReading;                  // current raw reading from button pin
int buttonState = HIGH;             // debounced button state
int lastButtonReading = HIGH;       // previous raw reading for edge detection
unsigned long lastButtonChangeTime = 0;  // time when button reading last changed
const int buttonDebounce = 50;      // button must be stable for 50ms to register

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
  Serial.println("Qirab Digitization Column v1.3");
  Serial.println("QDC100 - Serial: 000000");
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

  // ============================================
  // KY-040 Rotary Encoder Setup
  // ============================================
  // Set encoder pins as inputs with internal pull-up resistors
  pinMode(encoderCLK, INPUT_PULLUP);
  pinMode(encoderDT, INPUT_PULLUP);
  pinMode(encoderSW, INPUT_PULLUP);

  // Read the initial state of the CLK and DT pins
  lastCLKState = digitalRead(encoderCLK);
  lastDTState = digitalRead(encoderDT);

  Serial.println("KY-040 Rotary Encoder Initialized (Inactive)");
}

void loop() {
  checkKey();       //check keypad matrix for new commands
  checkEncoder();   //check rotary encoder for rotation and button press
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
      // Print position in steps and calculated mm
      long currentPos = stepper.currentPosition();
      float positionMM = (float)currentPos / motorsteps;  // convert steps to mm
      Serial.print("POS: ");
      Serial.print(currentPos);
      Serial.print(" steps (");
      Serial.print(positionMM, 2);  // 2 decimal places
      Serial.println(" mm)");
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

    // Variables for the pressed button's parameters
    long buttonDistance = 0;
    long buttonSpeed = 0;
    bool buttonDown = false;

    switch (customKey) {
      case '0':
        runallowed = false;
        Serial.println("STOP");
        break;
      case '1':
        Serial.print("Down 1600mm ");
        buttonDistance = 1600 * motorsteps;
        buttonSpeed = 8000;
        buttonDown = true;
        break;
      case '2':
        Serial.print("Down 10mm ");
        buttonDistance = 10 * motorsteps;
        buttonSpeed = 5000;
        buttonDown = true;
        break;
      case '3':
        Serial.print("Down 1mm ");
        buttonDistance = 1 * motorsteps;
        buttonSpeed = 5000;
        buttonDown = true;
        break;
      case '4':
        Serial.print("Down 0.5mm ");
        buttonDistance = 0.5 * motorsteps;
        buttonSpeed = 3000;
        buttonDown = true;
        break;
      case '5':
        Serial.print("UP 1mm ");
        buttonDistance = 1 * motorsteps;
        buttonSpeed = 5000;
        buttonDown = false;
        break;
      case '6':
        Serial.print("UP 10mm ");
        buttonDistance = 10 * motorsteps;
        buttonSpeed = 5000;
        buttonDown = false;
        break;
      case '7':
        Serial.print("UP 1600mm ");
        buttonDistance = 1600 * motorsteps;
        buttonSpeed = 8000;
        buttonDown = false;
        break;
      default:
        return;  // unknown key, exit
    }

    // Handle STOP button separately
    if (customKey == '0') {
      return;
    }

    // Cumulative behavior: same direction accumulates, direction change resets
    if (runallowed == true && downdirection == buttonDown) {
      // Same direction while moving - accumulate distance
      receivedDistance = receivedDistance + buttonDistance;
      if (buttonSpeed > receivedSpeed) {
        receivedSpeed = buttonSpeed;
      }
      Serial.print("(accumulated: ");
      Serial.print((float)receivedDistance / motorsteps, 1);
      Serial.println("mm)");
    } else if (runallowed == true && downdirection != buttonDown) {
      // Direction change while moving - stop and start fresh
      Serial.println("(direction change)");
      stopall();
      receivedDistance = buttonDistance;
      receivedSpeed = buttonSpeed;
      downdirection = buttonDown;
      runallowed = true;
    } else {
      // Not currently moving - start fresh
      Serial.println("");
      receivedDistance = buttonDistance;
      receivedSpeed = buttonSpeed;
      downdirection = buttonDown;
      runallowed = true;
    }

    // Set motor parameters and start movement
    if (runallowed == true && downdirection == true) {
      stepper.setMaxSpeed(receivedSpeed);
      stepper.move(receivedDistance);
    } else if (runallowed == true && downdirection == false) {
      stepper.setMaxSpeed(receivedSpeed);
      stepper.move(-1 * receivedDistance);
    }
  }
}


// ============================================
// KY-040 Rotary Encoder Check Function
// ============================================
// This function handles both the button press (to activate/deactivate encoder)
// and the rotation detection (to move column 0.5mm per detent)
void checkEncoder() {
  // ----------------------------------------
  // Check encoder button press (SW pin)
  // Uses proper debounce: button must be stable for buttonDebounce ms
  // ----------------------------------------
  buttonReading = digitalRead(encoderSW);

  // If reading changed, reset the debounce timer
  if (buttonReading != lastButtonReading) {
    lastButtonChangeTime = millis();
  }

  // If reading has been stable for longer than debounce period
  if ((millis() - lastButtonChangeTime) > buttonDebounce) {
    // If the stable reading is different from current debounced state
    if (buttonReading != buttonState) {
      buttonState = buttonReading;  // update debounced state

      // Only act on button press (HIGH to LOW transition)
      if (buttonState == LOW) {
        // Toggle encoder active state
        encoderActive = !encoderActive;

        // Print status to serial
        if (encoderActive) {
          Serial.println("ENCODER: Activated");
        } else {
          Serial.println("ENCODER: Deactivated");
        }
      }
    }
  }

  lastButtonReading = buttonReading;  // save reading for next iteration

  // ----------------------------------------
  // Check encoder rotation (only if active)
  // ----------------------------------------
  if (encoderActive) {
    // Read current state of both CLK and DT pins
    int currentCLKState = digitalRead(encoderCLK);
    int currentDTState = digitalRead(encoderDT);

    // ----------------------------------------
    // DEBOUNCE METHOD 1: Time-based debouncing
    // ----------------------------------------
    // Ignore signals that occur within rotationDebounce ms of each other
    unsigned long currentTime = millis();
    bool timeDebounceOK = (currentTime - lastRotationTime) > rotationDebounce;

    // ----------------------------------------
    // DEBOUNCE METHOD 2: State validation (quadrature decoding)
    // ----------------------------------------
    // Valid rotation requires a proper state transition sequence.
    // We detect rotation on CLK falling edge and validate with DT state.
    // For a valid transition, both pins should not change simultaneously
    // (simultaneous change indicates noise/bounce).
    bool clkChanged = (currentCLKState != lastCLKState);
    bool dtChanged = (currentDTState != lastDTState);
    bool stateValidationOK = clkChanged && !dtChanged;  // only CLK should change, not both

    // Detect valid rotation: CLK falling edge with both debounce checks passing
    if (currentCLKState == LOW && clkChanged && timeDebounceOK && stateValidationOK) {
      // Update rotation time for time-based debouncing
      lastRotationTime = currentTime;

      // Calculate step increment for 0.32mm movement
      // (encoder has 25 signals per rotation, column calibration = 8mm per rotation)
      // 8mm / 25 signals = 0.32mm per signal
      long stepIncrement = 0.32 * motorsteps;

      // Determine rotation direction from DT state
      // If DT is HIGH when CLK falls, rotation is clockwise (UP)
      // If DT is LOW when CLK falls, rotation is counter-clockwise (DOWN)
      if (currentDTState == HIGH) {
        // Clockwise rotation - move UP 0.5mm

        if (runallowed && downdirection == false) {
          // Already moving UP - accumulate distance
          receivedDistance += stepIncrement;
          stepper.move(-1 * receivedDistance);  // update target (negative = up)
          Serial.print("ENCODER: UP 0.32mm (accumulated: ");
          Serial.print((float)receivedDistance / motorsteps, 2);
          Serial.println("mm)");
        } else if (runallowed && downdirection == true) {
          // Currently moving DOWN - stop and reverse direction
          Serial.println("ENCODER: STOP (reversing direction)");
          stopall();
          Serial.println("ENCODER: Direction change to UP");
          receivedSpeed = 3000;
          receivedDistance = stepIncrement;
          downdirection = false;
          runallowed = true;
          stepper.setMaxSpeed(receivedSpeed);
          stepper.move(-1 * receivedDistance);
        } else {
          // Not currently moving - start new UP movement
          Serial.println("ENCODER: UP 0.32mm");
          receivedSpeed = 3000;
          receivedDistance = stepIncrement;
          downdirection = false;
          runallowed = true;
          stepper.setMaxSpeed(receivedSpeed);
          stepper.move(-1 * receivedDistance);
        }
      } else {
        // Counter-clockwise rotation - move DOWN 0.5mm

        if (runallowed && downdirection == true) {
          // Already moving DOWN - accumulate distance
          receivedDistance += stepIncrement;
          stepper.move(receivedDistance);  // update target (positive = down)
          Serial.print("ENCODER: Down 0.32mm (accumulated: ");
          Serial.print((float)receivedDistance / motorsteps, 2);
          Serial.println("mm)");
        } else if (runallowed && downdirection == false) {
          // Currently moving UP - stop and reverse direction
          Serial.println("ENCODER: STOP (reversing direction)");
          stopall();
          Serial.println("ENCODER: Direction change to Down");
          receivedSpeed = 3000;
          receivedDistance = stepIncrement;
          downdirection = true;
          runallowed = true;
          stepper.setMaxSpeed(receivedSpeed);
          stepper.move(receivedDistance);
        } else {
          // Not currently moving - start new DOWN movement
          Serial.println("ENCODER: Down 0.32mm");
          receivedSpeed = 3000;
          receivedDistance = stepIncrement;
          downdirection = true;
          runallowed = true;
          stepper.setMaxSpeed(receivedSpeed);
          stepper.move(receivedDistance);
        }
      }

      digitalWrite(LED_BUILTIN, LOW);  // turn on LED to indicate activity
    }

    // Save current states for next iteration
    lastCLKState = currentCLKState;
    lastDTState = currentDTState;
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
