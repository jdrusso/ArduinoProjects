//TODO: Break up code by subsystem
//TODO: Dim display late at night. Add photoresistor to automatically dim.

#include <Keypad.h>
#include <OneWire.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <EEPROM.h>
#include <Average.h>

/*****
KEYPAD
*****/
const byte numRows= 4; //number of rows on the keypad
const byte numCols= 4; //number of columns on the keypad

//keymap defines the key pressed according to the row and columns just as appears on the keypad
char keymap[numRows][numCols]= 
{
  {'1', '2', '3', 'A'}, 
  {'4', '5', '6', 'B'}, 
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

boolean isReading;
String keypadCurrString;


/*****
PIN CONNECTIONS
*****/
const int relayPin = 2;
const int servoPin = 4;
const int thermometerPin = 5;
const int displayTxPin = 7;
const int displayRxPin = 8;
//Matrix Keypad
byte rowPins[numRows] = {23,22,21,20}; //Rows 0 to 3
byte colPins[numCols]= {19,18,17,16}; //Columns 0 to 3


/*****
THERMOMETER
*****/
OneWire ds(thermometerPin);
#define DS18S20_ID 0x10
#define DS18B20_ID 0x28
//Celsius
float temp;
//Fahrenheit
float currTemp;
//Fahrenheit
float desiredTemp = 65;
//Create averaging array
float currTempArray[10];
//Average temperature
float avgTemp = 0;
//Address in EEPROM
int desiredTempAddress = 0;


/*****
DISPLAY
*****/
SoftwareSerial s7s(displayRxPin, displayTxPin);
char tempDisplayString[10];


/*****
SERVO
*****/
Servo heatServo;
int servoPos;


//initializes an instance of the Keypad class
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);


boolean controlsActive = false;
boolean controlsEnabled = true;

void setup(){
  
  Serial.begin(9600);
  Serial.println("Program starting...");
  
  //Set relayPin to output
  pinMode(relayPin, OUTPUT);
  
  //Attach servo to servoPin
  heatServo.attach(servoPin);
  
  //Initialize display
  clearDisplay();
  setBrightness(127);
  
  s7s.print("INIT");
  desiredTemp = EEPROM.read(desiredTempAddress);
}



//Every loop:
//x Get keypad input, check to see if setpoint has changed
//x Check current temperature
//x See if it's within accceptable range
//x Display current and desired temperatures
//x IF temperature is nominal, then turn fan OFF
//x IF temperature is too HOT, then turn fan ON, and turn temperature to WARM, and turn on display dot
//x IF temperature is too COLD, then turn fan ON, and turn temperature to COOL, and turn on display dot
void loop(){
  
  getKeyPress();
  
  displayTemp();
  
  if (controlsEnabled){
    
    //If current temperature is more than 4 degrees below setpoint, start heating.
    if (currTemp < (desiredTemp - 4)){
      
      heatRoom();
    }
    //If current temperature is more than 4 degrees above setpoint, start heating.
    else if (currTemp > (desiredTemp + 4)){
      
      coolRoom();
    }
    //If current temperature is within 2 of desired temperature, disable temperature controls.
    else if (abs(desiredTemp-currTemp) < 1){
      
      stopTemp();
    }
  }
}



/*****
Temperature Control Functions
*****/

void heatRoom(){
  
  controlsActive = true;
  setServoWarm();
  fanOn();
}

void coolRoom(){
  
  controlsActive = true;
  setServoCool();
  fanOn();
}

void stopTemp(){
  
  controlsActive = false;
  fanOff();
}

/*****
Display Functions
*****/
//  Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay(){
  
  s7s.write(0x76);  // Clear display command
}

//  Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals){
  
  s7s.write(0x77);
  s7s.write(decimals);
}

//  Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value){
  
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

//Function name slightly misleading. Enables 2nd 
//digit decimal to show heating controls active
void displayControlsActive(){
  
  if (controlsActive){
    
    setDecimals(0b00010010);
  }
  if (!controlsActive){
    
    setDecimals(0b00010000);
  }
}

//Display current and desired temperatures.
void displayTemp(){
  
  char combTempString[4];
  
  //Combine both temps into one long int for printing.
  //i.e. current 60, desired 68 = 6068
  int tempCombined = ((int)(currTemp*100)) + ((int)(desiredTemp));
  
  sprintf(combTempString, "%4d", tempCombined);
  
  s7s.print(combTempString);
  
  //Enable colon and second digit dot as necessary
  displayControlsActive();
}

/*****
Keypad Functions
*****/
//TODO: Break this into two functions, one to get the keypress, one to handle combining them
//TODO: Write temperature setpoint to EEPROM

void getKeyPress(){
  
  //If a key is pressed, this key is stored in 'keypressed' variable
  //If key is not equal to 'NO_KEY' (if it is valid), then this key is printed out
  //if count=17, then count is reset back to 0 (this means no key is pressed during the whole keypad scan process [???]
  //TODO: WTF does that line mean^?
  
  char keypressed = myKeypad.getKey();
  if (keypressed != NO_KEY){
    
    Serial.println(keypressed);
    
    //Do not add to the string if it is not a number
    if (isReading && keypressed != 'A' && keypressed != 'B' && keypressed != 'C' && keypressed != 'D' && keypressed != '*' && keypressed != '#'){
      
      keypadCurrString.concat(keypressed);
    }
    else if (keypressed == 'A'){
      controlsEnabled = false;
      stopTemp();
    }
    else if (keypressed == 'B'){
      controlsEnabled = false;
      heatRoom();
    }
    else if (keypressed == 'C'){
      controlsEnabled = false;
      coolRoom();
    }
    else if (keypressed == 'D'){
      controlsEnabled = true;
    }
    
    //If * pressed, start reading a string. If * pressed again, print the string. If # pressed, clear and output 'Cleared.'
    else if (keypressed == '*'){
      
      //Initialize input
      if(!isReading){
        
        memset(&keypadCurrString,'\0',sizeof(keypadCurrString));
        isReading = true;
      }
      //Terminate input listening
      else if (isReading){
        
        isReading = false;
        
        //keypadCurrString = keypadCurrString.substring(0, tempString.length() - 1);
        Serial.println(keypadCurrString);
        //The pun was unintentional, but you're welcome to laugh
        int keypadCurrInt = keypadCurrString.toInt();
        
        //If input number is within 45-75, go for it. If you want a temperature outside of that, go outside
        if(abs(60 - keypadCurrInt) < 15){
          s7s.write('SET');
          delay(250);
          updateSetpoint(keypadCurrInt);
        }
        
        else{
          s7s.write('NOPE');
          delay(250);
        }
      }
    }
    else if (keypressed == '#'){
      
      if(isReading){
        
        memset(&keypadCurrString,'\0',sizeof(keypadCurrString));
        Serial.println("String cleared.");
        isReading = false;
      }
    }
  }
}

void updateSetpoint(int newTemp){
 
  desiredTemp = newTemp;
  EEPROM.write(0, newTemp);
}

/*****
Thermometer Functions
*****/

//TODO: Create averaging filter for temperatures.

//Poll thermometer for current temperature and update global variable temp
boolean getTemperature(){
  
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  
  //find a device
  if (!ds.search(addr)) {
    
    ds.reset_search();
    return false;
  }
  
  if (OneWire::crc8( addr, 7) != addr[7]) {
    
    return false;
  }
  
  if (addr[0] != DS18S20_ID && addr[0] != DS18B20_ID) {
    
    return false;
  }
  
  ds.reset();
  ds.select(addr);
  
  // Start conversion
  ds.write(0x44, 1);
  
  // Wait some time...
  delay(850);
  present = ds.reset();
  ds.select(addr);
  
  // Issue Read scratchpad command
  ds.write(0xBE);
  
  // Receive 9 bytes
  for ( i = 0; i < 9; i++) {
    
  data[i] = ds.read();
  }
  
  // Calculate temperature value
  temp = ( (data[1] << 8) + data[0] )*0.0625;
  
  currTemp = celsiusToFahrenheit(temp);
  averaging();
  return true;
}

//Convert Celsius temperature to Fahrenheit
double celsiusToFahrenheit(float celsius){
  
  double ftemp = ((temp*(9.0/5.0))+32.0);
  return ftemp;
}

void averaging(){
  
  int arrayLen = sizeof(currTempArray)/sizeof(float);
  
  for(int i = 0; i < arrayLen; i++){
    
    if(currTempArray[i] == 0){
      
      currTempArray[i] = currTemp;
      avgTemp = currTemp;
      return;
    }
  }
  
  avgTemp = rollingAverage(currTempArray,10,currTemp);
}

/*****
Relay Functions
*****/

//The following two functions assume the fan is connected to 
//relayPin with OFF in NC, HIGH in NO, and COM to ground.
//
//Note: Relay is REVERSE LOGIC. LOW = energized, HIGH = off

void fanOn(){
  
  digitalWrite(relayPin, LOW);
}

void fanOff(){
  
  digitalWrite(relayPin, HIGH);
}

/*****
Servo Functions
*****/

//The following functions assume the servo is configured with
//0 degrees at WARMER and 180 degrees at COOLER on the thermostat

void setServoWarm(){
  
  heatServo.write(0);
}

void setServoCool(){
  
  heatServo.write(180);
}
