/**
 * @brief This sketch mimics the GUI of a blood pressure measuring device with pseudo IoT feature.<br>
 *        A third-party commercial program called BitFontCreator has been used to convert three system fonts installed in a PC for this embedded project.<br>
 *        Another program called LCD Image Converter was used to convert bitmap pictures to C files just like the font data created by BitFontCreator.<br>
 *        LCD Image Converter is an open source program that can be downloaded for free whereas BitFontCreator is a commercial program costing at an affordable level.<br>
 *        The purpose of using different programs to convert graphical assets (fonts and bitmaps) is to show flexiblity. <br>
 *        There is no restriction on which tool to use as long as we are drawing pixels on LCD correctly from arrays, no matter they have been converted <br> 
 *        by a commercial program or shareware.<br>
 *        The conversion procedures for both BitFontCreator and LCD Image Converter are described in the user guide.<br>
 *        
 *        Pseudo IoT concept is illustrated with a message box displayed below blood pressure data for 3.2" Memory LCD.<br>
 *        Future firmware version will support adhoc message display in form of picture-in-picture for other LCD sizes as well.<br>
 *        Don't forget ESP32 is a WiFi and BLE SoC. On top of measuring blood pressure with the right sensor, data collected can be stored and <br>
 *        uploaded to the cloud these days. After data reading and analyzing the medical professionals can feedback with text message via Internet to <br>
 *        this IoT-enabled Blood Pressure Device for first impression or appointment.<br>
 *        Surely some infrastructure is required in the story like a server cloud storage, security measures against patient privacy,<br> 
 *        and allocation of medical staff to reply messages, just to name few of them.<br>
 *        Nevertheless, this is a viable concept that can be done at least in the hardware level.<br>
 *        
 *        At time of writing, only 3.2" is supported with pseudo IoT message. <br>
 *       
 *        Select the right LCD model from MemoryLCD.h by uncomment the model to test with<br>
 *        e.g. we are testing 3.2" Memory model.
 *        //#define   LS027B7DH01
 *          #define  LS032B7DD02
 *        //#define   LS044Q7DH01
 *        
 * @note  Programmer : John Leung <br>
 *        Date:        14th June 2018
 */

#include "MemoryLCD.h"

#if defined (ESP32)
const int SW3 = 35;
const int SW2 = 34;
const int BUZZ = 27;
#elif defined (_VARIANT_ARDUINO_ZERO_)
const int SW3 = 6;
const int SW2 = 5;
const int BUZZ = 7;
#endif

/**
 * @note  A trick to play when creating fontArial_Rounded_MT_Bold55h. The font width of <space> (hex 0x0020) has been increased to 
 *        the same width as digits 0-9. In BitFontCreator, select 0x0020 with a left mouse click. At the left upper panel locate 
 *        the button with a red + sign. Click on it until the width is adjusted to 29 indicating by a message like this:
 *        Character Editor (w:29*h:55) - 0020<>.
 *        This is to make life easier in vitalSignUpdate() function. Instead of clearing the background color of a field, we
 *        simply need to append empty spaces in front of a vital sign value to get the background cleared for number of different decimal places. 
 *        Example, if a single digit is to be printed in a field of 3 digits, two <space> is appended in front e.g.
 *        String("  ") + String("9") to get the correct indentation.
 */
extern const BFC_FONT fontArial_Rounded_MT_Bold55h;
extern const BFC_FONT fontConsolas24h;
extern const BFC_FONT fontSimHei_35h;

///@note Font: SimHei 35 こんにちは in unicode 16
const uint16_t hello_japanese[]={0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'};
///@note Font: SimHei 35 你好 in unicode 16
const uint16_t hello_chinese[] ={0x4F60, 0x597D, '\0'};

extern const tImage arrowUp_89x48;
extern const tImage arrowDown_89x48;
extern const tImage battery_46x26;
extern const tImage pulseRate_icon;
extern const tImage IoT_message;

void buzz(int pin, unsigned int freq, unsigned long duration);
void buzz(int pin, unsigned int freq, unsigned long duration)
{
#if defined (ESP32)
  ledcWriteTone(0, (double)freq);
  ledcAttachPin(BUZZ, 0);
  delay(duration);
  ledcDetachPin(BUZZ);
#else
  tone(pin,freq,duration);
#endif
}

#if defined LS027B7DH01
  #define VITAL_SIGN_LEFT_MARGIN 100
  #define VITAL_SIGN_LABEL_LEFT_MARGIN  200
#elif defined LS044Q7DH01
  #define VITAL_SIGN_LEFT_MARGIN 20
  #define VITAL_SIGN_LABEL_LEFT_MARGIN  120
#elif defined LS032B7DD02
  #define VITAL_SIGN_LEFT_MARGIN 36
  #define VITAL_SIGN_LABEL_LEFT_MARGIN  136
#endif


#define VITAL_SIGN_TOP_MARGIN   50
#define VITAL_SIGN_LABEL_TOP_MARGIN (VITAL_SIGN_TOP_MARGIN+5)

typedef enum {
  SYS_PRESSURE=0, //systolic blood pressure
  DIA_PRESSURE,   //diastolic blood pressure
  PUL_RATE}       //pulse rate of the heart
  VITAL_SIGN; 

uint8_t sysPressure, diaPressure, pulRate;
uint16_t cursorX, cursorY;

void vitalSignUpdate(VITAL_SIGN sign, uint8_t data);

bool IoT_message_received = false;

void setup() {
  USE_SERIAL.begin(115200);
  // put your setup code here, to run once:
  hal_bsp_init();
  
  GFXDisplayPowerOn();
  pinMode(SW3, INPUT_PULLUP); //set GPIO an input with pullup for SW3 & SW2 on Memory LCD Shield
  pinMode(SW2, INPUT_PULLUP);

#if defined (ESP32)
///@note  Ref: https://github.com/espressif/arduino-esp32/blob/a4305284d085caeddd1190d141710fb6f1c6cbe1/cores/esp32/esp32-hal-ledc.h#L30
ledcSetup(0, 1000, 8);  //channel 0, freq=1000, resolution_bits = 8
#endif
  //display Hello in Chinese and Japanese
  uint16_t w = GFXDisplayGetWStringWidth(&fontSimHei_35h, hello_chinese);
  uint16_t h = GFXDisplayGetFontHeight(&fontSimHei_35h);
  
  GFXDisplayPutWString( (GFXDisplayGetLCDWidth()-w)/2,
                        (GFXDisplayGetLCDHeight()-h)/3,
                        &fontSimHei_35h, hello_chinese, BLACK, WHITE);

  w = GFXDisplayGetWStringWidth(&fontSimHei_35h, hello_japanese);
  GFXDisplayPutWString((GFXDisplayGetLCDWidth()-w)/2,
                        (GFXDisplayGetLCDHeight()-h)/3 + h,
                        &fontSimHei_35h, hello_japanese, BLACK, WHITE);
                        
  w =  GFXDisplayGetStringWidth( &fontConsolas24h, "Press any key to continue");              
  GFXDisplayPutString((GFXDisplayGetLCDWidth()-w)/2,
                      (GFXDisplayGetLCDHeight()-h)/3 + 2*h,
                      &fontConsolas24h, "Press any key to continue", WHITE, BLACK);

  uint16_t timeout = 3000;//count in 1 msec
  while(digitalRead(SW2)==1 && digitalRead(SW3)==1 && timeout>0) //program halt until the user press a button or 3 seconds expired
  {
	delay(1);//delay 1 msec
	timeout--;
  }
  
  buzz(BUZZ,1000,500);
  
  GFXDisplayAllClear();
  
  //display the battery icon
  GFXDisplayPutImage(VITAL_SIGN_LABEL_LEFT_MARGIN+100,5, &battery_46x26, false);
  //display an up arrow 
  GFXDisplayPutImage(VITAL_SIGN_LABEL_LEFT_MARGIN+80,50, &arrowUp_89x48, false);
  //display the down arrow
  GFXDisplayPutImage(VITAL_SIGN_LABEL_LEFT_MARGIN+80,105, &arrowDown_89x48, false);
  //display the heart
  GFXDisplayPutImage(VITAL_SIGN_LABEL_LEFT_MARGIN+100,160, &pulseRate_icon, false);
  
  //Print SYS pressure label & unit
  cursorY = VITAL_SIGN_LABEL_TOP_MARGIN; 
  GFXDisplayPutString(VITAL_SIGN_LABEL_LEFT_MARGIN, cursorY, &fontConsolas24h, String("SYS.").c_str(), BLACK, WHITE);
  cursorY +=GFXDisplayGetFontHeight(&fontConsolas24h);
  GFXDisplayPutString(VITAL_SIGN_LABEL_LEFT_MARGIN, cursorY, &fontConsolas24h, String("mmHg").c_str(), BLACK, WHITE);
  
  //Print DIA. pressure label & unit
  cursorY = VITAL_SIGN_LABEL_TOP_MARGIN + GFXDisplayGetFontHeight(&fontArial_Rounded_MT_Bold55h);
  GFXDisplayPutString(VITAL_SIGN_LABEL_LEFT_MARGIN, cursorY, &fontConsolas24h, String("DIA.").c_str(), BLACK, WHITE);
  cursorY +=GFXDisplayGetFontHeight(&fontConsolas24h);
  GFXDisplayPutString(VITAL_SIGN_LABEL_LEFT_MARGIN, cursorY, &fontConsolas24h, String("mmHg").c_str(), BLACK, WHITE);
 
  //Print pulse rate label
  cursorY = VITAL_SIGN_LABEL_TOP_MARGIN + 2*GFXDisplayGetFontHeight(&fontArial_Rounded_MT_Bold55h) + GFXDisplayGetFontHeight(&fontConsolas24h) +5;
  GFXDisplayPutString(VITAL_SIGN_LABEL_LEFT_MARGIN, cursorY, &fontConsolas24h, String("PUL.").c_str(), BLACK, WHITE);       
}

void loop() {  
    for(sysPressure=70; sysPressure<120; sysPressure++)
    {
      vitalSignUpdate(SYS_PRESSURE, sysPressure);
      delay(50);
    }
    for(diaPressure=60; diaPressure<80; diaPressure++)
    {
      vitalSignUpdate(DIA_PRESSURE, diaPressure);
      delay(50);
    }
    for(pulRate=60; pulRate<80; pulRate++)
    {
      vitalSignUpdate(PUL_RATE, pulRate);
      delay(50);
    }

    //3.2" Memory in 336*536
    #if defined LS032B7DD02
    if(!IoT_message_received)
    {
      GFXDisplayPutImage((GFXDisplayGetLCDWidth()-320)/2,260, &IoT_message, 0);
      IoT_message_received = true;
    }
    #endif

    int count = 0;
    sysPressure=90; diaPressure=52; pulRate=66; //initialize parameters for 'some' value
    while(count++ <30)
    {
      vitalSignUpdate(SYS_PRESSURE, sysPressure+count);
      vitalSignUpdate(DIA_PRESSURE, diaPressure+count);
      vitalSignUpdate(PUL_RATE, pulRate+count);
      delay(50);
    }
  }

/**
 * @brief Function to update vital sign data on Memory LCD
 * @param sign is an enum type defined in this file
 * @param data is the vital sign value to print
 * @note  A trick to play when creating fontArial_Rounded_MT_Bold55h. The font width of <space> (hex 0x0020) has been increased to 
 *        the same width as digits 0-9. In BitFontCreator, select 0x0020 with a left mouse click. At the left upper panel locate 
 *        the button with a red + sign. Click on it until the width is adjusted to 29 indicating by a message like this:
 *        Character Editor (w:29*h:55) - 0020<>.
 *        This is to make life easier in vitalSignUpdate() function. Instead of clearing the background color of a field, we
 *        simply need to append empty spaces in front of a vital sign value to get the background cleared for number of different decimal places. 
 *        Example, if a single digit is to be printed in a field of 3 digits, two <space> is appended in front e.g.
 *        String("  ") + String("9") to get the correct indentation.
 */
void vitalSignUpdate(VITAL_SIGN sign, uint8_t data)
{
  String signString;

  cursorX = VITAL_SIGN_LEFT_MARGIN;
  
  if(sign==SYS_PRESSURE)
    cursorY = VITAL_SIGN_TOP_MARGIN;
  else if (sign==DIA_PRESSURE)
    cursorY = VITAL_SIGN_TOP_MARGIN + GFXDisplayGetFontHeight(&fontArial_Rounded_MT_Bold55h); 
  else
    cursorY = VITAL_SIGN_TOP_MARGIN + 2*GFXDisplayGetFontHeight(&fontArial_Rounded_MT_Bold55h); 

    if(data<10) //single digit
    {
     signString = String("  ")+String(data); //1x <space> in front of a single digit, with <space> adjusted to the same width as digits 0-9 in BitFontCreator
    }
    else if (data>9 && data<100) //2 digits
    {
      signString = String(" ")+String(data);//2x <space> in front of a double digit
    }
    else
    {
      signString = String(data);
    }

    GFXDisplayPutString(cursorX, cursorY, &fontArial_Rounded_MT_Bold55h, signString.c_str(), BLACK, WHITE);
}


