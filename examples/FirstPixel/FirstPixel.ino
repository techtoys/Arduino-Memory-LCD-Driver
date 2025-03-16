/**
 * @brief Simple sketch to draw pixels on Sharp Memory LCD.<br>
 *        On system startup the LCD is blank (all white). Pressing SW3 will draw some pixels. Pressing SW2 will clear screen. <br>
 *        Select the right LCD model from MemoryLCD.h by uncomment the model to test with<br>
 *        e.g. we are testing 3.2" Memory model.
 *        //#define   LS027B7DH01
 *          #define  LS032B7DD02
 *        //#define   LS044Q7DH01
 *        
 * @note  Programmer : John Leung <br>
 *        Date:        14th June 2018
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

void buzz(int pin, unsigned int freq, unsigned long duration)
{
  tone(pin,freq,duration);
}

void drawLine(int x1, int y1, int x2, int y2, COLOR color);

void setup() {
  hal_bsp_init();
  GFXDisplayPowerOn();
  pinMode(SW3, INPUT_PULLUP); //set SW3 pin as input
  pinMode(SW2, INPUT_PULLUP); //set SW2 pin as input
  pinMode(BUZZ, OUTPUT); //set BUZZ pin as output
}

void loop() {    
    if(digitalRead(SW3)==LOW)
    {
      delay(10);
      if(digitalRead(SW3)==LOW)
      {
        buzz(BUZZ,1000,100);
        drawLine(0,0,DISP_HOR_RESOLUTION-1,DISP_VER_RESOLUTION-1,BLACK);  //draw a diagonal from top left to bottom right
        drawLine(DISP_HOR_RESOLUTION-1,0,0,DISP_VER_RESOLUTION-1,BLACK);  //draw a diagonal from top right to bottom left
        
        drawLine(1,1,DISP_HOR_RESOLUTION-2,1,BLACK);  //horizontal top line at y=1
        drawLine(1,DISP_VER_RESOLUTION-2,DISP_HOR_RESOLUTION-2,DISP_VER_RESOLUTION-2,BLACK);  //horizontal bottom line at y=DISP_VER_RESOLUTION-2

        drawLine(1,1,1,DISP_VER_RESOLUTION-2,BLACK);  //vertical line at x=1
        drawLine(DISP_HOR_RESOLUTION-2,1,DISP_HOR_RESOLUTION-2,DISP_VER_RESOLUTION-2,BLACK);  //vertical line at x=DISP_HOR_RESOLUTION-2
        while(digitalRead(SW3)==LOW)
        ;
      }
    }

    if(digitalRead(SW2)==LOW)
    {
      delay(10);
      if(digitalRead(SW2)==LOW){
        GFXDisplayAllClear();
        buzz(BUZZ,400,200);
        while(digitalRead(SW2)==LOW)
          ;
      }
    }
  }

/*
**********************************************************************************************************
* @brief  Line draw function from Programming The Nintendo Game Boy Advance: The Unofficial Guide
*         Copyright (c)2003 by Jonathan S. Harbour -- http://www.jharbour.com
**********************************************************************************************************
*/
void drawLine(int x1, int y1, int x2, int y2, COLOR color)
{
  int i, deltax, deltay, numpixels;
  int d, dinc1, dinc2;
  int x, xinc1, xinc2;
  int y, yinc1, yinc2;
  //calculate deltaX and deltaY
  deltax = fabs(x2 - x1);
  deltay = fabs(y2 - y1);

  //initialize
  if(deltax >= deltay)
  {
  //If x is independent variable
  numpixels = deltax + 1;
  d = (2 * deltay) - deltax;
  dinc1 = deltay << 1;
  dinc2 = (deltay - deltax) << 1;
  xinc1 = 1;
  xinc2 = 1;
  yinc1 = 0;
  yinc2 = 1;
  }
  else
  {
  //if y is independent variable
  numpixels = deltay + 1;
  d = (2 * deltax) - deltay;
  dinc1 = deltax << 1;
  dinc2 = (deltax - deltay) << 1;
  xinc1 = 0;
  xinc2 = 1;
  yinc1 = 1;
  yinc2 = 1;
  }

  //move the right direction
  if(x1 > x2)
  {
  xinc1 = -xinc1;
  xinc2 = -xinc2;
  }
  if(y1 > y2)
  {
  yinc1 = -yinc1;
  yinc2 = -yinc2;
  }

  x = x1;
  y = y1;
  //draw the pixels
  for(i = 1; i < numpixels; i++)
  {
  GFXDisplayPutPixel(x, y, color);
  if(d < 0)
  {
    d = d + dinc1;
    x = x + xinc1;
    y = y + yinc1;
  }
  else
  {
    d = d + dinc2;
    x = x + xinc2;
    y = y + yinc2;
  }
  }  
}



