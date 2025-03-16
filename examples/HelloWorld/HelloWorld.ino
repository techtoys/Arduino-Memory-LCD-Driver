/**
 * @brief Hello world sketch to start development with Sharp Memory LCD.<br>
 *        This program demonstrates various API functions to draw pixel, line, rectangle, print text ASCII code and Unicode, etc.<br>
 *        User interaction is available from either of the two tact switches SW2 and SW3 onboard, or 3 seconds expired.<br>
 *        Select the right LCD model from MemoryLCD.h by uncomment the model to test with<br>
 *        e.g. we are testing 3.2" Memory model.
 *        //#define   LS027B7DH01
 *          #define  LS032B7DD02
 *        //#define   LS044Q7DH01
 *        
 * @note  Programmer : John Leung <br>
 *        Date:        14th June 2018
 */

  // Unify tone generation for ESP32 to tone(pin,freq,duration)
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

extern const BFC_FONT fontArial_Rounded_MT_Bold55h;
extern const BFC_FONT fontSimHei_35h;
extern const tImage cat_400x246;
extern const tImage qr_code_248x248;

///@note Font: SimHei 35 こんにちは in unicode 16
const uint16_t hello_japanese[]={0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'};
///@note Font: SimHei 35 你好 in unicode 16
const uint16_t hello_chinese[] ={0x4F60, 0x597D, '\0'};

/**
 * @brief Generate a tone on the buzzer for a duration
 * @param pin: the pin number of the buzzer
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
 * @brief A very simple function to hold the program for any key press SW2 or SW3 or 3 seconds expired
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
  
    delay(30);  //debounce for 30ms if SW2/3 pressed
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
    waitKeyPress();
    GFXDisplayAllClear();
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
    waitKeyPress();
    GFXDisplayAllClear();
    GFXDisplayPutImage((GFXDisplayGetLCDWidth()-248)/2,0,&qr_code_248x248,0);
    waitKeyPress();
    GFXDisplayOff();  //take DISP in '0' to switch display off. Memory content no change. It is a good time to start checking on power consumption.
                      //measure TP7 vs TP8 with a better multimeter (set to mV measurement) to cross check the current
    waitKeyPress();
    GFXDisplayOn();   //take DISP in '1' to switch display on to restore graphical content
                      //measure TP7 vs TP8 with a better multimeter (set to mV measurement) to cross check the current
    waitKeyPress();
    GFXDisplayAllClear();
  }



