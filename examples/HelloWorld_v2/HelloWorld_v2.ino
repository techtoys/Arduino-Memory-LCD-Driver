/**
 * @brief Hello world sketch to start development with Sharp Memory LCD.<br>
 *        This program demonstrates various API functions to draw pixel, line, rectangle, print text ASCII code and Unicode, etc.<br>
 *        User interaction is available from either of the two tact switches SW2 and SW3 onboard, or 3 seconds expired.<br>
 *        Select the right LCD model from MemoryLCD.h by uncomment the model to test with<br>
 *        e.g. we are testing 3.2" Memory model.
 *        //#define   LS027B7DH01
 *          #define  LS032B7DD02
 *        //#define   LS044Q7DH01
 *        //#define   LS006B7DH03
 *        //#define   LS011B7DH03
 *        
 * @note  Programmer : John Leung <br>
 *        Date:        14th June 2018
 */

 /**
  * Add support for two new Memory LCD models, LS006B7DH03(64*64) & LS011B7DH03(160*68)
  * Add more icons incl. step_64x64, swim_64x64, run_64x64, etc.
  * Date: 7th Dec 2018
  * Programmer : John Leung
  */ 

  // Unify tone generation for ESP32 to tone(pin,freq,duration) and for other platforms to buzz(pin,freq,duration)
  // Date: 16th March 2025
  // Programmer: John Leung

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

extern const BFC_FONT fontConsolas24h;
extern const BFC_FONT fontArial_Rounded_MT_Bold55h;
extern const BFC_FONT fontSimHei_35h;
extern const BFC_FONT fontBerlinSans_FB30h;

extern const tImage cat_400x246;
extern const tImage qr_code_248x248;
extern const tImage qrcode_33x33;
extern const tImage beating_64x64;
extern const tImage pulse_64x48;
extern const tImage run_64x64;
extern const tImage step_64x64;
extern const tImage swim_64x64;

///@note Font: SimHei 35 こんにちは in unicode 16
const uint16_t hello_japanese[]={0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'};
///@note Font: SimHei 35 你好 in unicode 16
const uint16_t hello_chinese[] ={0x4F60, 0x597D, '\0'};

/**
 * @brief Generate a tone on the buzzer for a duration
 * @param pin: the pin number to generate the tone
 * @param freq: the frequency of the tone
 * @param duration: the duration of the tone
 */
void buzz(int pin, unsigned int freq, unsigned long duration)
{
  tone(pin,freq,duration);
}

void setup() {
  // put your setup code here, to run once:
  hal_bsp_init();
  GFXDisplayPowerOn();
  pinMode(SW3, INPUT_PULLUP); //set SW3 pin as input
  pinMode(SW2, INPUT_PULLUP); //set SW2 pin as input
  pinMode(BUZZ, OUTPUT);      //set BUZZ pin as output
}

/**
 * @brief A very simple function to hold the program for any key press SW2 or SW3.
 *        This function halt the program with an infinite loop. 
 */
void waitKeyPress(void)
{
  uint16_t timeout = 3000;	//count in msec
  
  while(digitalRead(SW3)==HIGH && digitalRead(SW2)==HIGH && timeout>0)
  {
	delay(1);
	timeout--;
  }
  
    delay(30);  //debounce for 30ms
    if(digitalRead(SW3)==LOW || digitalRead(SW2)==LOW)
    {
      buzz(BUZZ,1000,100);
      //wait until key release
      while(digitalRead(SW3)==LOW)
            ;
      while(digitalRead(SW2)==LOW)
          ;   
    }
}

/**
 * Main loop to run each demo page in round-robin. Press either SW2 or SW3 to cycle through...
 */
void loop() {  
#if defined (LS011B7DH03) || defined (LS006B7DH03)
    uint16_t stringWidth;
    GFXDisplayPutImage(5,5,&run_64x64,0);
    #if defined (LS011B7DH03)    
    stringWidth = GFXDisplayGetStringWidth(&fontConsolas24h, "RUN");
    GFXDisplayPutString(150-stringWidth,32,&fontConsolas24h, "RUN", BLACK, WHITE);  //1.08" demo
    #endif
    waitKeyPress();GFXDisplayAllClear();
    
    GFXDisplayPutImage(5,5,&step_64x64,0);
    #if defined (LS011B7DH03) 
    stringWidth = GFXDisplayGetStringWidth(&fontConsolas24h, "STEP");
    GFXDisplayPutString(150-stringWidth,32,&fontConsolas24h, "STEP", BLACK, WHITE);  //1.08" demo
    #endif
    waitKeyPress();GFXDisplayAllClear();

    GFXDisplayPutImage(5,5,&swim_64x64,0);
    #if defined (LS011B7DH03)
    stringWidth = GFXDisplayGetStringWidth(&fontConsolas24h, "SWIM");
    GFXDisplayPutString(150-stringWidth,32,&fontConsolas24h, "SWIM", BLACK, WHITE);  //1.08" demo
    #endif
    waitKeyPress();GFXDisplayAllClear();

    GFXDisplayPutImage(5,5,&beating_64x64,0);
    #if defined (LS011B7DH03)
    stringWidth = GFXDisplayGetStringWidth(&fontConsolas24h, "86 BPM");
    GFXDisplayPutString(150-stringWidth,32,&fontConsolas24h, "86 BPM", BLACK, WHITE);  //1.08" demo
    #endif
    waitKeyPress();GFXDisplayAllClear();
    
    GFXDisplayPutImage(7,5,&qrcode_33x33,0);
    stringWidth = GFXDisplayGetStringWidth(&fontConsolas24h, "SCAN");
    GFXDisplayPutString(0,40,&fontConsolas24h, "SCAN", BLACK, WHITE);  //0.56" demo
    waitKeyPress();GFXDisplayAllClear();   

    uint8_t rtc_min = 13;
    uint8_t rtc_sec = 0;
    String secString,prev_secString;

    secString=prev_secString=String("00");

    GFXDisplayPutString(7,2,&fontBerlinSans_FB30h, (String(rtc_min) + String(":")).c_str(), BLACK, WHITE);
    #if defined (LS011B7DH03)
    GFXDisplayPutImage(80,5,&run_64x64,0);
    #endif
    do{
      if(rtc_sec==0)
      {
        secString = String("00");
      }
      else if(rtc_sec<10)
      {
        secString = String("0")+String(rtc_sec);
      } 
      else
      {
        secString = String(rtc_sec);
      }
      uint16_t tbWidth = GFXDisplayGetStringWidth(&fontBerlinSans_FB30h, prev_secString.c_str()); //width of the text box to clear before new digits got printed
      GFXDisplayDrawRect(7,34,7+tbWidth,32+30, WHITE);  //clear the area for previous digits
      if(rtc_sec==10){
        GFXDisplayPutString(2,34,&fontBerlinSans_FB30h, "Press to cont.", BLACK, WHITE);
      }else {
        GFXDisplayPutString(7,34,&fontBerlinSans_FB30h, secString.c_str(), BLACK, WHITE);
      }
      prev_secString = secString;
      buzz(BUZZ,300,50);
      delay(1000-50);
    }while(rtc_sec++ < 10);
    
    waitKeyPress(); GFXDisplayAllClear();
    
    GFXDisplayLineDrawH(0, GFXDisplayGetLCDWidth()-1, 5, BLACK, 2);
    GFXDisplayLineDrawV(5, 0, GFXDisplayGetLCDHeight()-1, BLACK, 2);
    waitKeyPress();
    uint16_t y = GFXDisplayGetLCDHeight()/2;
    uint16_t amplitudeMax = GFXDisplayGetLCDHeight()/4;
    uint16_t d;
    //plot 3 sine curves in different phases
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++) //plot a sine curve
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD *d), BLACK);
    }
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++) 
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD * (d + 90)), BLACK);
    }
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++)
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD * (d+180)), BLACK);
    }
    waitKeyPress(); GFXDisplayAllClear();

#else
    
    GFXDisplayPutString(0,10,&fontArial_Rounded_MT_Bold55h, "@123:{Hello}", WHITE, BLACK);
    waitKeyPress();
    GFXDisplayPutWString(10,100,&fontSimHei_35h, hello_japanese, BLACK, WHITE);
    waitKeyPress();
    GFXDisplayPutWString(10,150,&fontSimHei_35h, hello_chinese, BLACK, WHITE);
    waitKeyPress();
    GFXDisplayAllClear();
    GFXDisplayDrawRect(10, 10, 280, 280, BLACK);
    waitKeyPress();
    GFXDisplayDrawRect(50, 50, 200, 200, WHITE);
    waitKeyPress(); GFXDisplayAllClear();

    GFXDisplayLineDrawH(0, GFXDisplayGetLCDWidth()-1, 20, BLACK, 3);
    GFXDisplayLineDrawV(10, 0, GFXDisplayGetLCDHeight()-1, BLACK, 3);
    waitKeyPress();
    uint16_t y = GFXDisplayGetLCDHeight()/2;
    uint16_t amplitudeMax = GFXDisplayGetLCDHeight()/4;
    uint16_t d;
    //plot 3 sine curves in different phases
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++) //plot a sine curve
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD *d), BLACK);
    }
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++) 
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD * (d + 90)), BLACK);
    }
    for(d=0; d<GFXDisplayGetLCDWidth()*2; d++)
    {      
      GFXDisplayPutPixel(10+d/2, y+amplitudeMax*sin(DEG_TO_RAD * (d+180)), BLACK);
    }
    waitKeyPress();
    GFXDisplayAllClear();
    
    GFXDisplayPutImage(0,0,&cat_400x246,0);
    waitKeyPress(); GFXDisplayAllClear();
    GFXDisplayPutImage((GFXDisplayGetLCDWidth()-248)/2,0,&qr_code_248x248,0);
    waitKeyPress();
    GFXDisplayOff();  //take DISP in '0' to switch display off. Memory content no change. It is a good time to start checking on power consumption.
                      //measure TP7 vs TP8 with a better multimeter (set to mV measurement) to cross check the current
    waitKeyPress();
    GFXDisplayOn();   //take DISP in '1' to switch display on to restore graphical content
                      //measure TP7 vs TP8 with a better multimeter (set to mV measurement) to cross check the current
    waitKeyPress();
    GFXDisplayAllClear();
    
#endif
  }



