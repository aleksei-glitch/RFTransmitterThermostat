/*
check out for ready eeprom and lcd shield work
http://www.homebrewtalk.com/showthread.php?t=505492
Simple Temperature RF transmitter
Transmits Temperatuce over RF frequency. Has transmission id [TX_ID] to help distinguishing source and format on receiving side.
Has n values for data - will be received/mapped in same order by RF receiver
Temperature sensors used:
* Dallas Temperature sensor DS18B20 for temperatures up to 120 C
* Thermocouple with MAX6675 module for temperatures up to 800 C
For debug mode set variable bDebugMode to true. This will invoke Serial with baud rate 9600 and will print dessages.
Set bDebugMode=false; for standalone mode.
Arduino pinout
-------------------------------
+5v -> Vcc of MAX6675
PIN 5 -> CS pin on MAX6675
PIN 6 -> SO pin of MAX6675
PIN 7 -> SCK pin of MAX6675
GND -> GND pin of MAX6675
+5v -> Vcc of DS18B20
+5v -> R1(4.7k ohm) -> data of DS18B20 (middle leg)
PIN 8 -> data of DS18B20 (middle leg)
GND -> GND of DS18B20
PIN 13 -> sending indicator led
-------------------------------
Created By: Aleksei Ivanov
Creation Date: 2015/05/23
Last Updated Date:2015/05/23
*/
#include <LiquidCrystal.h>   // include LCD library
#include <EEPROMex.h>          // include expanded eeprom library

#include <VirtualWire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
int TX_ID = 2; // Transmitter ID address
long DELAY_TIME = 3000; // Delay time
boolean bDebugMode=false;// to invoke Serial to print messages
boolean bTCinitOK=false; // to determine on startup if thermocouple is working properly


// Pins in use
#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
#define LCD_BACKLIGHT_PIN         10  // D10 controls LCD backlight
#define HEAT_PIN                 0   //Heat Relay Pin
#define COOL_PIN                 1   //Cooling Relay Pin
// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            145  // up
#define DOWN_10BIT_ADC          329  // down
#define LEFT_10BIT_ADC          505  // left
#define SELECT_10BIT_ADC        741  // right
#define BUTTONHYSTERESIS         20  // hysteresis for valid button sensing window
//return values for ReadButtons()
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 
#define MENU_SETTEMP              0  
#define MENU_TEMP_SWING           1
#define MENU_COMPRESSOR_DELAY     2
#define MENU_EXIT                 3
byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events
float tempReading;                        //analog reading from sensor

/*--------------------------------------------------------------------------------------
  Init the LCD library with the LCD pins to be used
--------------------------------------------------------------------------------------*/
LiquidCrystal lcd( 8, 9, 4, 5, 6, 7 );   //Pins for the freetronics 16x2 LCD shield. LCD: ( RS, E, LCD-D4, LCD-D5, LCD-D6, LCD-D7 )

// Dallas Temperature sensor
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// RF data to transfer
typedef struct roverRemoteData// Data Structure
{
  int TX_ID; // Initialize a storage place for the outgoing TX ID
  float Sensor1Data;// Initialize a storage place for the first integar that you wish to Send
  float Sensor2Data;// Initialize a storage place for the Second integar that you wish to Send
  float Sensor3Data;// Initialize a storage place for the Third integar that you wish to Send
  float Sensor4Data;// Initialize a storage place for the Forth integar that you wish to Send
};

void setup() {
  if(bDebugMode) Serial.begin(9600);

  sensors.begin();
  vw_set_ptt_inverted(true); //
  vw_set_tx_pin(12);
  vw_setup(4000);
  sensors.requestTemperatures();
  delay(500);
  
  
}
void loop() {
  lcd.setCursor( 0, 0 ); 
  lcd.print( "Current Tmp" );
  lcd.setCursor(12,0);
  lcd.print(sensors.getTempCByIndex(0));
  
pinMode(13,OUTPUT);
if(bDebugMode) Serial.println("Statrt loop");
struct roverRemoteData payload;// In this section is where you would load the data that needs to be sent.
if(bDebugMode) Serial.println("RF reset");
sensors.requestTemperatures();
//Serial.println("DS Reset");
if(bDebugMode){
if(bDebugMode) Serial.print("DS Temperature: ");
if(bDebugMode) Serial.print( sensors.getTempCByIndex(0));
if(bDebugMode) Serial.print(";");
if(bDebugMode) Serial.print( sensors.getTempCByIndex(1));
if(bDebugMode) Serial.print(";");
}
payload.TX_ID = TX_ID; // Set the Radio Address
payload.Sensor1Data =0;// Relay 1
payload.Sensor2Data =0;// Relay 2
payload.Sensor3Data =sensors.getTempCByIndex(0);// DS18B20 sensor 1
payload.Sensor4Data =sensors.getTempCByIndex(1);// DS18B20 sensor 2
digitalWrite(13,HIGH); // indicate start of sending data over RF
vw_send((uint8_t *)&payload, sizeof(payload)); // Send the data
vw_wait_tx();
digitalWrite(13,LOW); // sending date over RF ended
delay(DELAY_TIME); //delay between reading
}

void checkMenu(){
  byte button = ReadButtons();  //This calls ReadButtons Function Found Below
switch( button )
   {
     //**********************************************************************************TOP LEVEL MENU*******************************************************************************************************************************************************
      case BUTTON_LEFT:            //Simply turn backlight back on by pressing Left
      {
        digitalWrite( LCD_BACKLIGHT_PIN, HIGH );
        temp=millis()/1000;
        break;
      }
      case BUTTON_RIGHT:            //Simply turn backlight back on by pressing Right
      {
        digitalWrite( LCD_BACKLIGHT_PIN, HIGH );
        temp=millis()/1000;
        break;
      }
      case BUTTON_UP:            //Simply turn backlight back on by pressing Up
      {
        digitalWrite( LCD_BACKLIGHT_PIN, HIGH );
        temp=millis()/1000;
        break;
      }
      case BUTTON_DOWN:            //Simply turn backlight back on by pressing Left, Right, Up, or Down
      {
        digitalWrite( LCD_BACKLIGHT_PIN, HIGH );
        temp=millis()/1000;
        break;
      }
      
      case BUTTON_SELECT:
      {
        menu = true;                               //Make sure menu while loop flag is set true
        menu_number=0;                             //Default first menu item to 0 case
        lcd.clear();
        delay(200);
        while(menu ==true)                         //Stay in menu as long as menu flag is true
        {
          digitalWrite( LCD_BACKLIGHT_PIN, HIGH);
          button = ReadButtons();                       //Determine which part of menu to enter, or exit menu.  Only uses Left Right, and Select Buttons
          switch( button)
          {
            case BUTTON_LEFT:
             {
               menu_number = menu_number - 1;
               if(menu_number <0 ) menu_number = 3;   
               delay(200);
               break;
              }
             case BUTTON_RIGHT:
             {
               menu_number = menu_number + 1;
               if(menu_number > 3) menu_number = 0;   
               delay(200);
               break;
              }
             case BUTTON_SELECT:
             {
               delay(200);
               switch(menu_number)
               {
     //************************************************************************************************************SET TEMP******************************************************************************************************************************************         
                 case MENU_SETTEMP:
                 {
                   set_temp = true;                              
                   lcd.clear();
                   while(set_temp == true)
                   {
                     lcd.setCursor(0,0);
                     lcd.print("Seterature");
                     lcd.setCursor(0,1);
                     lcd.print(set_temperature);
                     button = ReadButtons();
                     switch(button)
                     {
                       case BUTTON_UP:
                       {
                         set_temperature = set_temperature + .5;
                         lcd.setCursor(0,1);
                         lcd.print(set_temperature);
                         delay(200);
                         break;
                       }
                       case BUTTON_DOWN:
                       {
                         set_temperature = set_temperature - .5;
                         lcd.setCursor(0,1);
                         lcd.print(set_temperature);
                         delay(200);
                         break;
                       }
                       case BUTTON_SELECT:
                       {
                         lcd.clear();
                         set_temp = false;
                         delay(200);
                         break;
                       }
                       break;
                     }
                   }
                 break;
                 }
       //**************************************************************************************************************SET TEMP SWING*********************************************************************************************************************************        
                 case MENU_TEMP_SWING:
                 {
                   temp_swing = true;                          
                   lcd.clear();
                   while(temp_swing == true)
                   {
                     lcd.setCursor(0,0);
                     lcd.print("Setshold");
                     lcd.setCursor(0,1);
                     lcd.print(set_temp_swing);
                     button = ReadButtons();
                     switch(button)
                     {
                       case BUTTON_UP:
                       {
                         set_temp_swing = set_temp_swing + .5;
                         lcd.setCursor(0,1);
                         lcd.print(set_temp_swing);
                         delay(200);
                         break;
                       }
                       case BUTTON_DOWN:
                       {
                         set_temp_swing = set_temp_swing - .5;
                         lcd.setCursor(0,1);
                         lcd.print(set_temp_swing);
                         delay(200);
                         break;
                       }
                       case BUTTON_SELECT:
                       {
                         lcd.clear();
                         temp_swing = false;
                         delay(200);
                         break;
                       }
                       break;
                     }
                   }
                 break;
                 }
       //******************************************************************************************************************SET COMPRESSOR DELAY*************************************************************************************************************************************         
                 case MENU_COMPRESSOR_DELAY:
                 {
                   compressor_delay = true;                          
                   lcd.clear();
                   while(compressor_delay == true)
                   {
                     lcd.setCursor(0,0);
                     lcd.print("Set Delay");
                     lcd.setCursor(0,1);
                     lcd.print(set_compressor_delay);
                     button = ReadButtons();
                     switch(button)
                     {
                       case BUTTON_UP:
                       {
                         set_compressor_delay = set_compressor_delay + 1;
                         lcd.setCursor(0,1);
                         lcd.print(set_compressor_delay);
                         delay(200);
                         break;
                       }
                       case BUTTON_DOWN:
                       {
                         set_compressor_delay = set_compressor_delay - 1;
                         lcd.setCursor(0,1);
                         lcd.print(set_compressor_delay);
                         delay(200);
                         break;
                       }
                       case BUTTON_SELECT:
                       {
                         lcd.clear();
                         compressor_delay = false;
                         delay(200);
                         break;
                       }
                       break;
                     }
                   }
                 break;
                 }
        //******************************************************************************************************************EXIT MENU*************************************************************************************************************************************               
                 case MENU_EXIT:
                 {

                   menu=false;
                   EEPROM.writeFloat(0,set_temperature);                 //Upon menu exit, write these variables to eeprom so we dont loose them at power off or reset. 
                   EEPROM.writeFloat(4,set_temp_swing);
                   EEPROM.writeInt(8,set_compressor_delay);
                   lcd.clear();
                   delay(200);
                   break;
                 }
                 break;
               }
             
             }
           break;
           }
            
          lcd.setCursor(0,0);
          lcd.print("      MENU    ");
          lcd.setCursor(0,1);
          lcd.print(MENU[menu_number]);
         }
     
      }
      
      lcd.clear();
      temp = millis()/1000;
   
   }  
   
     
}

/*--------------------------------------------------------------------------------------
  ReadButtons()
  Detect the button pressed and return the value
  Uses global values buttonWas, buttonJustPressed, buttonJustReleased.
--------------------------------------------------------------------------------------*/
byte ReadButtons()
{
   unsigned int buttonVoltage;
   byte button = BUTTON_NONE;   // return no button pressed if the below checks don't write to btn
 
   //read the button ADC pin voltage
   buttonVoltage = analogRead( BUTTON_ADC_PIN );

   //sense if the voltage falls within valid voltage windows
   if( buttonVoltage < ( RIGHT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_RIGHT;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UP;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_DOWN;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_LEFT;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_SELECT;
   }
   //handle button flags for just pressed and just released events
   if( ( buttonWas == BUTTON_NONE ) && ( button != BUTTON_NONE ) )
   {
      //the button was just pressed, set buttonJustPressed, this can optionally be used to trigger a once-off action for a button press event
      //it's the duty of the receiver to clear these flags if it wants to detect a new button change event
      buttonJustPressed  = true;
      buttonJustReleased = false;
   }
   if( ( buttonWas != BUTTON_NONE ) && ( button == BUTTON_NONE ) )
   {
      buttonJustPressed  = false;
      buttonJustReleased = true;
   }
 
   //save the latest button value, for change event detection next time round
   buttonWas = button;
 
   return( button );
}
