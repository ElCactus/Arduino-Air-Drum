#include <Wire.h>


#define DEVICE_A (0x53)    //first ADXL345 device address
#define DEVICE_B (0x1D)    //second ADXL345 device address

int regAddress = 0x36;      //third axis-acceleration-data register on the ADXL345
float z1, z2, z3;

int devices[2] = {DEVICE_A, DEVICE_B};

float offset[2] = {0, 0};
float gain[2] = {1, 1};
int photoRPin = 0;

int lastReading;
unsigned long lastDebounceTime = 0;;
unsigned long debounceDelay = 10;
int reading;
int buttonState;
int lastButtonState = LOW;

float percent;

int minSnareValue;

int pin = 2;

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.setTimeout(-1);

  //Turning on the both ADXL345s
  writeTo(DEVICE_A, 0x2D, 24);
  writeTo(DEVICE_B, 0x2D, 24);


  //calibration
  offset[0] = 0.5 * (330 - 180);
  offset[1] = 0.5 * (270 - 230);

  gain[0] = 0.5 * (330 + 180);
  gain[1] = 0.5 * (270 + 230);

  int i = 0;

  for (; i < 10; i++)
  {
    minSnareValue += analogRead(photoRPin);
  }

  minSnareValue /= i;

}


void writeTo(int device, byte address, byte val) {
  Wire.beginTransmission(device); //start transmission to device
  Wire.write(address);        // send register address
  Wire.write(val);        // send value to write
  Wire.endTransmission(); //end transmission
}

float readFrom(int device) {
  Wire.beginTransmission(device); //start transmission to device
  Wire.write(regAddress);        //sends address to read from
  int retVal = Wire.endTransmission(); //end transmission

  if (retVal != 0)
    Serial.println("not succes");

  Wire.beginTransmission(device); //start transmission to device
  Wire.requestFrom(device, 2);    // request 2 bytes from device

  int val;
  while (Wire.available())   //device may send less than requested (abnormal)
  {
    int low = Wire.read(); // receive a byte
    int high = Wire.read();

    val = (high << 8) | low;
  }
  Wire.endTransmission(); //end transmission

  int index;

  if (device == DEVICE_A)
  {
    index = 0;
  } else if (device == DEVICE_B)
  {
    index = 1;
  }

  return applyOffset(val, index);
}


float applyOffset(int v, int index)
{
  return (v - offset[index]) / gain[index];
}
float minZ = 2;

int sensVal;           // for raw sensor values
float filterVal;       // this determines smoothness  - .0001 is max  1 is off (no smoothing)
float smoothedVal;     // this holds the last loop value just use a unique variable for every different sensor that needs smoothing
float smoothedVal2;     // this holds the last loop value just use a unique variable for every different sensor that needs smoothing

unsigned long m;
float minZ2;
unsigned long m2;

boolean pressed = true;

bool notePlayed(float z)
{

  if (abs(z) < abs(minZ) + 0.2)
  {
    if (millis() - m > 100)
    {
      minZ = 2;
      m = millis();

      return true;
    }
  } else
  {
    minZ = z;
  }

  return false;
}


bool notePlayed2(float z)
{
  if (abs(z) < abs(minZ2) + 0.2)
  {
    if (millis() - m2 > 100)
    {
      minZ2 = 2;
      m2 = millis();

      return true;
    }
  } else
  {
    minZ2 = z;
  }

  return false;
}


bool isSnare()
{
  if (analogRead(photoRPin) < 7)
  {
    //pressed = true;
    return true;
  }

  return false;
}

bool playNote = false;
unsigned long n;

void loop()
{
  z1 = readFrom(DEVICE_A);
  z2 = readFrom(DEVICE_B);


  Serial.print("ACCELEROMETER1 : "); //Serial may not work properly if this line is removed. It's odd.

  smoothedVal =  smooth(z1, 0.5, smoothedVal);
  smoothedVal2 =  smooth(z2, 0.5, smoothedVal2);


  if (smoothedVal2 < -1.2) {

    if (notePlayed2(smoothedVal2))
    {

      Serial.write(1);
      Serial.flush();
    }
  }

  if (smoothedVal < -1.2) {

    if (notePlayed(smoothedVal))
    {

      Serial.write(2);
      Serial.flush();

    }
  }

  pedalCheck();
  
}

void pedalCheck() {
  int reading = digitalRead(pin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        Serial.write(3);
      }
    }
  }

  lastButtonState = reading;
}


float smooth(int data, float filterVal, float smoothedVal) {

  if (filterVal > 1) {     // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0) {
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return smoothedVal;
}
