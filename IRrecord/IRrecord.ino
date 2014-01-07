/*
 * IRrecord: record and play back IR signals as a minimal 
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * An IR LED must be connected to the output PWM pin 3.
 * A button must be connected to the input BUTTON_PIN; this is the
 * send button.
 * A visible LED can be connected to STATUS_PIN to provide status.
 *
 * The logic is:
 * If the button is pressed, send the IR code.
 * If an IR code is received, record it.
 *
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#define cs   10
#define dc   9
#define rst  8  // you can also connect this to the Arduino reset

#include <IRremote.h>
#include <EEPROMex.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

int RECV_PIN = 2;
int BUTTON_PIN = 4;
int STATUS_PIN = 13;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

//16 bytes
struct signal {
  char name[8];
  long code;
  int len;
};

struct signal commands[3];

boolean pendingRead = false;
boolean pendingWrite = false;
boolean EEPROMwrite = false;
String content = String("");
char* writename = "";
char* readname = "";
char character;

void setup()
{
  Serial.begin(9600); 
  Serial.println("Program starting.");
  irrecv.enableIRIn(); // Start the receiver
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(STATUS_PIN, OUTPUT);
  memset(&commands, 0, 3*sizeof(struct signal));
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

struct signal EEPSearch(char* query)
{
  Serial.write("\nBeginning EEPROM search for ");
  Serial.println(query);
  int startingByte = 0;
  
  struct signal resultCommand;
  struct signal tempRead;
  memset(&resultCommand, 0, 16);
  //char temp[8];
  
  for(int i = 0; i < 64;i++)
  {
    startingByte = i*16;
    EEPROM.readBlock(startingByte,tempRead);
    
    memcpy(&resultCommand, &tempRead, 16);
    
    Serial.print("Found something at memory address ");
    Serial.print(startingByte);
    Serial.print(" called ");
    Serial.print(resultCommand.name);
    Serial.print(" with code ");
    Serial.println(resultCommand.code,HEX);
    
    if (strcmp(resultCommand.name,query) == 0)
    {
      Serial.println("Match found.");
      break;
    }
  }
  
  //Son, it's a glorious time in an Arduino's life when he's searched through EEPROM to find
  //a data structure and found the one he's looking for. Let's return that shit and dip.
  return resultCommand;
}

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) 
{
  codeType = results->decode_type;
  int count = results->rawlen;
  if (codeType == UNKNOWN) 
  {
    Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) 
    {
      if (i % 2) 
      {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } 
      else 
      {
        // Space
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else
  {
    if (codeType == NEC)
    {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT)
      {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
      } 
      else if (results->value != REPEAT)
      {
        Serial.println("Not a repeat!");
        Serial.println(sizeof(commands)/sizeof(*commands));
        Serial.println(writename);
        Serial.println(sizeof(struct signal));

        //look through commands array to see if it's already stored
        //But first, make sure we're even trying to store in program memory
        if (pendingWrite == true && EEPROMwrite == false)
        {
          for(int i=0;i<sizeof(commands)/sizeof(*commands);i++)
          { 
            
            Serial.println("Iterating...");
            Serial.write("Name: ");
            Serial.println(commands[i].code);
  
            //If commands[i] is empty, then write to it
            if (commands[i].code == 0)
            {
              memcpy(&commands[i].name,&writename,sizeof(commands[i].name));
              memcpy(&commands[i].code,&results->value,sizeof(commands[i].code));
              memcpy(&commands[i].len,&results->bits,sizeof(commands[i].len));
              
              Serial.write("\nWrote: ");
              Serial.println(writename);
              Serial.write("\nVerify: ");
              Serial.println(commands[i].name);
              Serial.println(commands[i].code);
              Serial.println(commands[i].len);
              
              break;
            }
          } // end FOR
        }
        pendingWrite = false;
        
        if(EEPROMwrite)
        {
          struct signal tempCommand;
          strcpy(tempCommand.name,writename);
          tempCommand.code = results->value;
          tempCommand.len = results->bits;
          //Iterate through EEPROM in 16 byte chunks
          for(int i = 0;i<64;i++)
          {
            int startingByte = 16*i;
            
            //Check for a 0 starting byte
            if (EEPROM.read(startingByte) == 0)
            {
              
              EEPROM.updateBlock(startingByte,tempCommand);
              Serial.print("Entered ");
              Serial.print(tempCommand.name);
              Serial.print(" at memory address ");
              Serial.print(startingByte);
              Serial.print(" in EEPROM with code ");
              Serial.println(tempCommand.code,HEX);
              break;
            }
          }
        EEPROMwrite = false;
        pendingWrite = false;
        }
      }
    } 
    else if (codeType == SONY) 
    {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == RC5) 
    {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) 
    {
      Serial.print("Received RC6: ");
    } 
    else 
    {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    codeValue = results->value;
    codeLen = results->bits;
  }
}

void sendCode(int repeat) 
{
  if (codeType == NEC) 
  {
    if (repeat) 
    {
      irsend.sendNEC(REPEAT, codeLen);
      Serial.println("Sent NEC repeat");
    } 
    else 
    {
      irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == SONY) 
  {
    irsend.sendSony(codeValue, codeLen);
    Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
  } 
  else if (codeType == RC5 || codeType == RC6) 
  {
    if (!repeat) 
    {
      // Flip the toggle bit for a new button press
      toggle = 1 - toggle;
    }
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) 
    {
      Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    } 
    else 
    {
      irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == UNKNOWN /* i.e. raw */) 
  {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    Serial.println("Sent raw");
  }
}

struct signal setColor(char* searchparam)
{
  uint16_t textColor = ST7735_WHITE;
  
  tft.fillRect(0,72,128,87,ST7735_BLACK);
  
  Serial.print(F("\nSearching for "));
  Serial.println(searchparam);
  
  struct signal colorcommand = EEPSearch(searchparam);
  
  Serial.println(F("Attempting to send."));
  irsend.sendNEC(colorcommand.code, colorcommand.len);
  
  Serial.print(F("First char: "));
  Serial.println(colorcommand.name[0]);
  
  switch(colorcommand.name[0]){
    case 'g':
      textColor = ST7735_GREEN;
      break;
    case 'r':
      textColor = ST7735_RED;
      break;
    case 'b':
      textColor = ST7735_BLUE;
      break;
    case 'y':
      textColor = ST7735_YELLOW;
      break;
    default:
      textColor = ST7735_BLUE;
      break;
  }
  
  Serial.print(F("First char: "));
  Serial.println(colorcommand.name[0]);
  
  drawText("Settinglights to ", ST7735_WHITE, 3);
  drawText(0,72,colorcommand.name,textColor, 3);
  
  Serial.print(F("Sent: "));
  Serial.println(colorcommand.name);
  Serial.println(colorcommand.code, HEX);
  Serial.println(F("Colorset done"));
  return colorcommand;
}

void drawText(const char* text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void drawText(const char* text, uint16_t color, const int textsize) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.setTextSize(textsize);
  tft.print(text);
}

void drawText(int cursorx, int cursory, char *text, uint16_t color) {
  tft.setCursor(cursorx, cursory);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void drawText(int cursorx, int cursory, char *text, uint16_t color, const int textsize) {
  tft.setCursor(cursorx, cursory);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.setTextSize(textsize);
  tft.print(text);
}

int lastButtonState;
String tempcontent;

void loop() {
  
  tempcontent = "";
  Serial.println("Loop begun.");
  
  memset(&content,'\0',sizeof(content));
  
  Serial.println("Strings cleared.");
  
  while(Serial.available()) 
  {
    delay(10);
    character = Serial.read();
    tempcontent.concat(character);
  }
  memcpy(&content,&tempcontent,strlen(tempcontent.c_str()));
  Serial.write(content.c_str());


Serial.println("String checks");

  //Check for read command (Read from memory, display code)
  if(strcmp(content.substring(0,4).c_str(),"read") == 0)
  {
    readname = strdup(content.substring(5).c_str());
    pendingRead = true;
    Serial.write("\nReading: ");
    Serial.println(readname);
  }
  
  //Check for write command (Write detected code to memory)
  else if(strcmp(content.substring(0,5).c_str(),"write") == 0)
  {
    writename = strdup(content.substring(6).c_str());
    pendingWrite = true;
    Serial.write("\nWriting: ");
    Serial.write(writename);
  }
  
  //Write detected code to EEPROM
  else if(strcmp(content.substring(0,8).c_str(),"eepwrite") == 0)
  {
    writename = strdup(content.substring(9).c_str());
    pendingWrite = true;
    EEPROMwrite = true;
    Serial.write("\nWriting to EEPROM: ");
    Serial.write(writename);
  }
  
  else if(strcmp(content.substring(0,7).c_str(),"eepread") == 0)
  {
    String searchstring = content.substring(8);
    char* searchparam = strdup(searchstring.c_str());
    
    Serial.println("\nEEPREAD Received...");
    Serial.println(searchparam);
    setColor(searchparam);
  }
  
  else if(strcmp(content.substring(0,7).c_str(),"eeprevw") == 0)
  {
    String searchstring = content.substring(8);
    char* searchparam = strdup(searchstring.c_str());
    
    Serial.println(F("\nEEPREVW Received..."));
    struct signal command;
    
    char temparray[16];
    temparray[0] = EEPROM.read(0);
    temparray[1] = EEPROM.read(1);
    temparray[2] = EEPROM.read(2);
    temparray[3] = EEPROM.read(3);
    temparray[4] = EEPROM.read(4);
    temparray[5] = EEPROM.read(5);
    temparray[6] = EEPROM.read(6);
    temparray[7] = EEPROM.read(7);
    temparray[8] = EEPROM.read(8);
    temparray[9] = EEPROM.read(9);
    temparray[10] = EEPROM.read(10);
    temparray[11] = EEPROM.read(11);
    temparray[12] = EEPROM.read(12);
    temparray[13] = EEPROM.read(13);
    temparray[14] = EEPROM.read(14);
    temparray[15] = EEPROM.read(15);
    
    memcpy(&command,&temparray,16);
    
    Serial.print(F("Attempting to send: "));
    Serial.println(command.name);
    Serial.println(command.code, HEX);
    command = setColor(command.name);
  }//As soon as it leaves this loop, it crashes the program ???
  
  if(strcmp(content.substring(0,4).c_str(),"send") == 0)
  {
    char* searchparam = strdup(content.substring(5).c_str());
    setColor(searchparam);
  }


Serial.println("Read check");

  if (pendingRead)
  {
    Serial.println("Read pending...");
    for(int i=0;i<sizeof(commands)/sizeof(*commands);i++)
    {
      Serial.println(commands[i].name);
      Serial.println(readname);
      if (strcmp(commands[i].name,readname) == 0)
      {
        Serial.println("Sending code...");
        irsend.sendNEC(commands[i].code, commands[i].len);
        Serial.print("Sent NEC ");
        Serial.println(commands[i].code, HEX);
        break;
      }
    }
    Serial.println("Read done.");
    pendingRead = false;
  }

  // If button pressed, send the code.
  int buttonState = digitalRead(BUTTON_PIN);
//  if (lastButtonState == LOW && buttonState == HIGH) 
//  {
//    Serial.println("Released");
//    //irrecv.enableIRIn(); // Re-enable receiver
//  }

Serial.println("Button state check");

  if (!buttonState) 
  {
    Serial.println("Pressed, sending. State: ");
    digitalWrite(STATUS_PIN, HIGH);

    for(int i = 0; i < 100; i++)
    {
      setColor("blue");
      delay(300);
      setColor("green");
      delay(300);
      setColor("red");
      delay(300);
    }
    digitalWrite(STATUS_PIN, LOW);
    delay(50); // Wait a bit between retransmissions
  }
  
  
Serial.print("Decode check: ");
Serial.println(pendingWrite);

//  if(pendingWrite == true){
//    if (irrecv.decode(&results)) 
//    {
//      Serial.println("Decoding...");
//      digitalWrite(STATUS_PIN, HIGH);
//      storeCode(&results);
//      irrecv.resume(); // resume receiver
//      digitalWrite(STATUS_PIN, LOW);
//    }
//  }
  lastButtonState = buttonState;
  
//Serial.println("Loop finished.");
}

