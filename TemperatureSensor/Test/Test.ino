#include <OneWire.h>
#include <SoftwareSerial.h>

OneWire ds(12);

#define DS18S20_ID asdfasdf
#define DS18B20_ID 0x28
float temp;

const int softwareTx = 8;
const int softwareRx = 7;

SoftwareSerial s7s(softwareRx, softwareTx);
char tempString[10];

void setup(){
  Serial.begin(9600);
  Serial.println("Program starting...");
  s7s.begin(9600);
  clearDisplay();
  s7s.print("INIT");
  setBrightness(127);
}

void loop(){
  
  getTemperature();
  double ftemp = ((temp*(9.0/5.0))+32.0);
  sprintf(tempString, "%4d", ((int)(ftemp*100)));
  s7s.print(tempString);
  Serial.print(temp);
  Serial.print(" degrees Celsius, ");
  Serial.print(ftemp);
  Serial.println(" degrees Fahrenheit.");
  setDecimals(0b00000010);
}

void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}

void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

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
  return true;
}
