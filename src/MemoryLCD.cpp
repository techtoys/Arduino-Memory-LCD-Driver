/**
* @brief  	Arduino driver for Sharp Memory LCD. 5 LCD models are supported with ESP32 or Arduino M0 PRO.
*			Wiring details is described in MemoryLCD.h under @note pin number definition, hardware dependent
*/

#include "MemoryLCD.h"

#if defined (ARDUINO)
#include "Arduino.h"
#include <SPI.h>
  #if defined (_VARIANT_ARDUINO_ZERO_)
	#include "wiring_private.h"
  #else if defined (ESP32)
	hw_timer_t* timer = NULL;
  #endif
static SPISettings spiSettings(2000000, LSBFIRST, SPI_MODE0); //send data with 2MHz SPI clock with data sent from LSB first
static SPIClass *_SPI;
#endif  //#if defined (ARDUINO)

uint8_t frameBuffer[GFX_FB_CANVAS_H][GFX_FB_CANVAS_W];

/**
 * @brief	Local function to write a pixel to the frame buffer. No display on LCD yet.
 * @param	x is the x-coordinate in range 0 ~ (DISP_HOR_RESOLUTION-1)
 * @param	y is the y-coordinate in ranage 0 ~ (DISP_VER_RESOLUTION-1)
 * @param	color is an enum type defined in MemoryLCD.h
			typedef enum
			{
				BLACK = 0,
				WHITE,
				TRANSPARENT	//means leaving original color
			} COLOR;
 */
static void GFXDisplayPutPixel_FB(uint16_t x, uint16_t y, COLOR color)
{
	if(y>(GFX_FB_CANVAS_H-1)||((x>>3)>(GFX_FB_CANVAS_W-1)))//avoid running outside array index
        return;
		
	uint8_t maskBit;
	
	//maskBit = 0x80 >> (x & 0x07);	//SPI data sent with MSB first
	maskBit = 0x01 << (x & 0x07);	//SPI data sent with LSB first
	
	if(color == WHITE)
        frameBuffer[y][(x >> 3)] |= maskBit;    //frameBuffer[y][(x>>1)] &= (maskBit^0xFF); frameBuffer[y][(x>>1)] |= color;
    else
        frameBuffer[y][(x >> 3)] &= (maskBit ^ 0xFF);
}

static void GFXDisplayUpdateLine(uint16_t line, uint8_t *buf);
static void GFXDisplayUpdateBlock(uint16_t start_line, uint16_t end_line, uint8_t *buf);
static uint16_t bfc_DrawChar_RowRowUnpacked(uint16_t x0, uint16_t y0, const BFC_FONT *pFont, uint16_t ch, COLOR color, COLOR bg);

/**
 * @brief Clear memory internal data and writes white for all pixels
 */
void GFXDisplayAllClear(void)
{
  hal_spi_start_transaction();
  hal_delayUs(3); //SCS setup time of tsSCS (refer to datasheet for timing details)
  hal_spi_write_byte(0x04); //M0="L", M2="H" with LSB sent first
  hal_spi_write_byte(0x00);
  hal_delayUs(1); //SCS hold time of thSCS (refer to datasheet for timing details)
  hal_spi_end_transaction();  

  memset((void *)&frameBuffer, 0xFF, sizeof(frameBuffer));  //clear SRAM of the MCU
}

/**
 * @brief Power-on sequence with 5V0 enabled, clear pixel memory (x2), DISP pin set high, and EXTCOMIN pulse started<br>
 *        Normal operation after this function executed. EXTCOMIN_FREQ indicates the pulse frequency defined in MemoryLCD.h
 */
void GFXDisplayPowerOn(void)
{
  //hal_gpio_write(GFX_5V0_EN, HIGH); //turn ON TPS60140 for 5V0, only useful when a gpio wired to EN pin of TPS60140
  //hal_delayMs(10); 		//stablize 5V0, allow more time for it otherwise there is startup problem.
  GFXDisplayAllClear();	//need to do it twice for clear pixel memory (somehow); otherwise, pixel memory not always cleared!!!
  GFXDisplayAllClear();

  GFXDisplayOn(); //DISP = '1'
  hal_delayUs(30);
  hal_extcom_start(EXTCOMIN_FREQ); //turn on EXTCOMIN pulse
  hal_delayUs(30);
  //normal operation after this...
}

/**
 * @brief	Display OFF with DISP pin set '1'
 */
void GFXDisplayOn(void)
{
	hal_gpio_write(GFX_DISPLAY_DISP, HIGH); //DISP = '1'
}

/**
 * @brief Power-off sequence with all pixels set white, DISP pin set low, EXTCOMIN pulse stopped, and 5V0 disabled.<br>
 *        Need to run GFXDisplayPowerOn() after this function if we need to write to Memory LCD again.
 */
void GFXDisplayPowerOff(void)
{
   GFXDisplayAllClear();
   GFXDisplayOff(); 	//DISP = '0'
   hal_extcom_stop();  	//stop EXTCOMIN pulse
   hal_delayUs(30);
   //hal_gpio_write(GFX_5V0_EN, LOW); //turn OFF TPS60140 for 5V0, only useful if a GPIO is wired to EN pin of TPS60140
}

/**
 * @brief	Display OFF with DISP pin set '0'
 */
void GFXDisplayOff(void)
{
	hal_gpio_write(GFX_DISPLAY_DISP, LOW); //DISP = '0'
}

/**
 * @brief	Print a single pixel
 * @param	(x,y) indicate the position
 * @param	color is BLACK/WHITE
 */
void GFXDisplayPutPixel(uint16_t x, uint16_t y, COLOR color)
{
	GFXDisplayPutPixel_FB(x, y, color);
	GFXDisplayUpdateLine(y+1, (uint8_t *)&frameBuffer[y]);	//Update on screen. Line counts from 1 thats why y+1
}

/**
 * @brief	Draw a horizontal line 
 * @param	x1 is the starting coordinate(0~DISP_HOR_RESOLUTION-1)
 * @param	x2 is the ending coordinate(0~DISP_HOR_RESOLUTION-1)
 * @param	y is the line position (0~DISP_VER_RESOLUTION-1)
 * @param	color is BLACK/WHITE
 * @param	thick is the thickness in pixels ranges 1~255
 */
void GFXDisplayLineDrawH(uint16_t x1, uint16_t x2, uint16_t y, COLOR color, uint8_t thick)
{
	if(thick==0) return;
	
	uint16_t _y, x, x_left, x_right;
	
	if(x1 > x2)
	{
		x_right = x1; x_left = x2;
	}
	else
	{
		x_right = x2; x_left = x1;
	}
		 
	for(_y = y; _y < (y+thick); _y++)
	{
		for(x = x_left; x <= x_right; x++)
		{
			GFXDisplayPutPixel_FB(x, _y, color);
		}		
	}

	GFXDisplayUpdateBlock(y+1, y+thick, (uint8_t *)frameBuffer[y]);
}

/**
 * @brief	Draw a vertical line
 * @param	x is the horizontal position (0~DISP_HOR_RESOLUTION-1)
 * @param	y1 is the starting y position (0~DISP_VER_RESOLUTION-1)
 * @param	y2 is the ending y position (0~DISP_VER_RESOLUTION-1)
 * @param	color is BLACK/WHITE
 * @param	thick is the thickness in pixels ranges 1~255
 */
void GFXDisplayLineDrawV(uint16_t x, uint16_t y1, uint16_t y2, COLOR color, uint8_t thick)
{
	if(thick==0) return;
	
	uint16_t _x, y, y_top, y_bottom;
	
	if(y1 > y2)
	{
		y_bottom = y1; y_top = y2;
	}
	else
	{
		y_bottom = y2; y_top = y1;
	}
	
	for(y = y_top; y<= y_bottom; y++)
	{
		for(_x = x; _x < (x+thick); _x++)
		{
			GFXDisplayPutPixel_FB(_x, y, color);
		}
	}
	
	GFXDisplayUpdateBlock(y_top+1, y_bottom+1, (uint8_t *)frameBuffer[y_top]);
}

/**
 * @brief	Draw a rectangle
 * @param	left is the starting position from left (0~DISP_HOR_RESOLUTION-1)
 * @param	top is the starting position from top (0~DISP_VER_RESOLUTION-1)
 * @param	right is the ending position to the right (0~DISP_HOR_RESOLUTION-1)
 * @param	bottom is the ending position to the bottom (0~DISP_HOR_RESOLUTION-1)
 * @param	color is BLACK/WHITE
 */
void GFXDisplayDrawRect(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, COLOR color)
{
	uint16_t _left=left, _top=top, _right=right, _bottom=bottom;
	
	if(left > right)
	{
		_left = right; _right = left;
	}	
	
	if(top > bottom)
	{
		_top = bottom; _bottom = top;
	}
	
	for(uint16_t y = _top; y <= _bottom; y++)
	{
		for(uint16_t x = _left; x <= _right; x++)
		{
			GFXDisplayPutPixel_FB(x,y,color);	//update the framebuffer first
		}
	}
	
	GFXDisplayUpdateBlock(_top+1, _bottom+1, (uint8_t *)frameBuffer[_top]);
}

/**
 * @brief 	Print a picture with byte array created by a shareware LCD Assistant (http://en.radzio.dxp.pl/bitmap_converter/)
 * @note	Option in LCD Assistant: Byte orientation = Horizontal, Other = Include size, endianness=Little<, Pixels/byte=8<br>
 *			This fcn for reference only.
 * @param	left is the top left corner position
 * @param	top is the top line position
 * @param	*data is a pointer to data array in Flash space
 * @param	invert is a boolean flag for negative effect (true for negative, false for normal display)
 */
 /*
void GFXDisplayPutPicture(uint16_t left, uint16_t top, const uint8_t* data, bool invert)
{
	uint16_t imgHeight=0, imgWidth=0;
	const uint8_t *pdata;
	uint16_t x, y, _x, _y;
	uint8_t pixel, bit;
	COLOR _color;
	
	pdata = data;
	imgWidth =  (((uint16_t)*(pdata+1))<<8) + (uint16_t)*pdata;
	imgHeight = (((uint16_t)*(pdata+3))<<8) + (uint16_t)*(pdata+2);
	
	//USE_SERIAL.print("Image height = "); USE_SERIAL.println(imgHeight);
	//USE_SERIAL.print("Image width = "); USE_SERIAL.println(imgWidth);
	
	pdata += 4;	//points to the 5th byte for head of data
	
	uint16_t bytesPerLine = (imgWidth+7)/8;
	//USE_SERIAL.print("Bytes/Line = "); USE_SERIAL.println(bytesPerLine);
	for(y = 0; y < imgHeight; y++)
    {
        for(x = 0; x < imgWidth; x++)
        {
            uint16_t col = x/8;
			pixel = pdata[y*bytesPerLine + col];
			if(!invert)
				pixel^=0xff;
			//USE_SERIAL.print("pixel = "); USE_SERIAL.println(pixel, HEX);
			bit = x%8;
			pixel = pixel<<bit;
			pixel = pixel>>7;
			
			_x = left + x;
			_y = top  + y;
            (pixel==1) ? _color = WHITE :  _color = BLACK;
			//USE_SERIAL.print("(_x,_y) = "); USE_SERIAL.print(_x);USE_SERIAL.print(','); USE_SERIAL.println(_y);
            GFXDisplayPutPixel_FB(_x, _y, _color);	//save to frame buffer first
        }
    }	
	GFXDisplayUpdateBlock(top+1, top+imgHeight, (uint8_t *)&frameBuffer[top]);
}
*/

/**
 * @brief 	Print an image with byte array created by a shareware LCD Image Converter
 * @note	Project's home page: http://www.riuson.com/lcd-image-converter<br>
 *			Conversion options<br>
 *			(1)Prepare->Scanning->Main Scan Direction : Top to Bottom, Line Scan Direction : Forward<br>
 *			(2)Image->Common->split to rows, Block Size->8 bit, Byte order->Little-Endian<br>
 * @param	left is the top left corner position
 * @param	top is the top line position
 * @param	*image is a pointer to data type tImage from Flash
 * @param	invert is a boolean flag for negative effect (true for negative, false for normal display)
 */
void GFXDisplayPutImage(uint16_t left, uint16_t top, const tImage* image, bool invert)
{
	uint16_t imgHeight=image->height, imgWidth=image->width;
	const uint8_t *pdata;
	uint16_t x, y, _x, _y;
	uint8_t pixel, bit;
	COLOR _color;
	
	pdata = image->data;
	
	//USE_SERIAL.print("Image height = "); USE_SERIAL.println(imgHeight);
	//USE_SERIAL.print("Image width = "); USE_SERIAL.println(imgWidth);
	
	uint16_t bytesPerLine = (imgWidth+7)/8;
	//USE_SERIAL.print("Bytes/Line = "); USE_SERIAL.println(bytesPerLine);
	for(y = 0; y < imgHeight; y++)
    {
        for(x = 0; x < imgWidth; x++)
        {
            uint16_t col = x/8;	//col is the byte count in horizontal direction
			pixel = pdata[y*bytesPerLine + col];
			if(invert)
				pixel^=0xff;
			//USE_SERIAL.print("pixel = "); USE_SERIAL.println(pixel, HEX);
			bit = x%8;	//bit position 
			pixel = pixel<<bit;			//shifting pixel byte according to which bit position to use, bit value now at MSB position 7
			pixel = pixel>>7;			//For B/W LCD we simply shift the bit value to the LSB position at 0 for WHITE/BLACK conditional test below

			_x = left + x;
			_y = top  + y;
            (pixel==1) ? _color = WHITE :  _color = BLACK;
			//USE_SERIAL.print("(_x,_y) = "); USE_SERIAL.print(_x);USE_SERIAL.print(','); USE_SERIAL.println(_y);
            GFXDisplayPutPixel_FB(_x, _y, _color);	//save to frame buffer first
        }
    }	
	//Finally LCD refreshed with multiple lines update from frame buffer.
	GFXDisplayUpdateBlock(top+1, top+imgHeight, (uint8_t *)&frameBuffer[top]);	
}

/**
 * @brief	Display a test pattern of vertical strip with horizontal byte defined
 * @param	pattern in 8-bit to define the byte pattern
 * @param	*pfcn a pointer to function when SCS in high state, mainly for energy measurement.
 * @return	time in millisec to update the display with a vertical strip pattern
 * @note	No frame buffer is involved. This function displays a vertical strip pattern defined by the argument 'pattern' in LSB format.<br>
 *			Example to use<br>
 *				GFXDisplayTestPattern(0xF0, NULL);
 */
uint32_t GFXDisplayTestPattern(uint8_t pattern, void (*pfcn)(void))
{ 	
	uint32_t timing = 0;
#if defined (ARDUINO)
	uint32_t sMillis = millis();
#else
	#error Need to define the function to return millisec for other platforms
#endif
  hal_spi_start_transaction();
  hal_delayUs(3); //SCS setup time of tsSCS (refer to datasheet for timing details)
  
  for(uint16_t line=1; line<=DISP_VER_RESOLUTION; line++)
  {
    #ifdef LS032B7DD02
    hal_spi_write_byte(uint8_t((line<<6)|0x01));  //update one specified line with M0=H,M2=L & AG0:AG1 concatenate to Bit[1:0] sending with LSB first
    hal_spi_write_byte((uint8_t)(line>>2));       //AG2~AG9 in LSB first
    #else
    hal_spi_write_byte(0x01);                     //update one specified line with M0=H,M2=L sending with LSB first
    hal_spi_write_byte((uint8_t)line);            //AG0~AG7 in LSB first for gate line address
    #endif    
    uint32_t writePeriod = DISP_HOR_RESOLUTION>>3; //divide by 8 for 1-bit bpp
	
    while(writePeriod--){
      hal_spi_write_byte(pattern);
    }
	
	if(line==DISP_VER_RESOLUTION/2)
	{
		if(pfcn!=NULL) {
			pfcn();//run pfcn() only once sample in the middle, pls make sure sampling time is long enough
		}	
	}
  }
  
  hal_spi_write_byte(0x00); //dummy byte
  hal_spi_write_byte(0x00); //dummy byte
  
  hal_spi_end_transaction();

#if defined (ARDUINO)
	timing = millis()-sMillis;
#else
	#error Need to define the function to return millisec for other platforms
#endif  
  
  return timing;
}
/**
 * @brief Function to update one line
 * @note  The minimum payload to write to a Memory LCD is a horizontal line
 * @param line is the line number start from 1 to DISP_VER_RESOLUTION
 * @param *buf is a pointer to data
 */
static void GFXDisplayUpdateLine(uint16_t line, uint8_t *buf)
{
  if(line > DISP_VER_RESOLUTION)
    return;
  
  uint32_t writePeriod = DISP_HOR_RESOLUTION>>3; //divide by 8 for 1-bit bpp
  
  hal_spi_start_transaction();
  hal_delayUs(3); //SCS setup time of tsSCS (refer to datasheet for timing details)
  #ifdef LS032B7DD02
  hal_spi_write_byte(uint8_t((line<<6)|0x01));  //update one specified line with M0=H,M2=L & AG0:AG1 concatenate to Bit[1:0] sending with LSB first
  hal_spi_write_byte((uint8_t)(line>>2));       //AG2~AG9 in LSB first
  #else
  hal_spi_write_byte(0x01);                     //update one specified line with M0=H,M2=L sending with LSB first
  hal_spi_write_byte((uint8_t)line);            //AG0~AG7 in LSB first for gate line address
  #endif
  
  while(writePeriod--){
   hal_spi_write_byte(*buf++);   
  }
  hal_spi_write_byte(0x00); //dummy byte
  hal_spi_write_byte(0x00); //dummy byte
  hal_delayUs(1); //SCS hold time of thSCS (refer to datasheet for timing details)
  hal_spi_end_transaction();
}

/**
 * @brief Function to update multiple lines
 * @param start_line indicates the starting line number ranges 1~DISP_VER_RESOLUTION
 * @param end_line indicates the ending line number ranges 1~DISP_VER_RESOLUTION
 * @param *buf is a pointer to data
 */
static void GFXDisplayUpdateBlock(uint16_t start_line, uint16_t end_line, uint8_t *buf)
{
  if((start_line > end_line) || (start_line > DISP_VER_RESOLUTION))
    return;

  int16_t _end_line = MIN(end_line,DISP_VER_RESOLUTION);	//clip the ending gate line address
  
  hal_spi_start_transaction();
  hal_delayUs(3); //SCS setup time of tsSCS (refer to datasheet for timing details)
  for(uint16_t line=start_line; line<=_end_line; line++)
  {
    #ifdef LS032B7DD02
    hal_spi_write_byte(uint8_t((line<<6)|0x01));  //update one specified line with M0=H,M2=L & AG0:AG1 concatenate to Bit[1:0] sending with LSB first
    hal_spi_write_byte((uint8_t)(line>>2));       //AG2~AG9 in LSB first
    #else
    hal_spi_write_byte(0x01);                     //update one specified line with M0=H,M2=L sending with LSB first
    hal_spi_write_byte((uint8_t)line);            //AG0~AG7 in LSB first for gate line address
    #endif    
    uint32_t writePeriod = DISP_HOR_RESOLUTION>>3; //divide by 8 for 1-bit bpp
    while(writePeriod--){
      hal_spi_write_byte(*buf++);
    }
  }
  hal_spi_write_byte(0x00); //dummy byte
  hal_spi_write_byte(0x00); //dummy byte
  hal_delayUs(1); //SCS hold time of thSCS (refer to datasheet for timing details)
  hal_spi_end_transaction();
}

/**
 * @brief	Print a character from MCU's Flash with data created by BitFontCreator
 * @param	(x,y) is the top left corner coordinates
 * @param	BFC_FONT* pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	ch in 2-byte width represents the character
 * @param	color is the font color BLACK/WHITE
 * @param	bg is the background color BLACK/WHITE/TRANSPARENT. TRANSPARENT means the background is not changed.
 * @return	width of character printed
 * @note	Example <br>
 *			extern const BFC_FONT fontArial_Rounded_MT_Bold37h;	//font map Arial_Rounded_MT_Bold37.c created by BitFontCreator
 *			//...
 *			GFXDisplayPutChar(50,50,&fontArial_Rounded_MT_Bold37h, 0x0042, BLACK, WHITE);	//print the ASCII character 'B' with black stroke on white background
 */
uint16_t GFXDisplayPutChar(uint16_t x, uint16_t y, const BFC_FONT* pFont, const uint16_t ch, COLOR color, COLOR bg)
{
	return (uint16_t)bfc_DrawChar_RowRowUnpacked(x,y,pFont,ch,color, bg);
}

/**
 * @brief	Print string of ASCII code of 1 byte width
 * @param	(x,y) is the top left corner coordinates
 * @param	BFC_FONT* pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	*str is a pointer to character array from Flash space
 * @param	color is the font color BLACK/WHITE
 * @param	bg is the background color BLACK/WHITE/TRANSPARENT. TRANSPARENT means the background is not changed.
 * @return	width of string printed
 */
uint16_t GFXDisplayPutString(uint16_t x, uint16_t y, const BFC_FONT* pFont, const char *str, COLOR color, COLOR bg)
{
	uint16_t _x = x;
	uint16_t _y = y;
	uint16_t width = 0;
	char ch = 0;
	
	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = GFXDisplayPutChar(_x, _y, pFont, ch, color, bg);
		str++;
		_x += width;
	}  	
	
	return (uint16_t)(_x-x);
}



/**
 * @brief	Print string of ASCII code of 2 bytes width that means Unicode can be printed (Chinese, Japanese, etc)
 * @param	(x,y) is the top left corner coordinates
 * @param	BFC_FONT* pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	*str is a pointer to character array from Flash space
 * @param	color is the font color BLACK/WHITE
 * @param	bg is the background color BLACK/WHITE/TRANSPARENT. TRANSPARENT means the background is not changed.
 * @return	width of string printed
 * @note	Example<br>
 *			const uint16_t hello_japanese[]={0x3053, 0x3093, 0x306B, 0x3061, 0x306F, '\0'};
 *			//....
 *			GFXDisplayPutWString(100,150,&fontSimHei_35h, hello_japanese, BLACK, WHITE);	//SimHei_35h.c created by BitFontCreator
 */
uint16_t GFXDisplayPutWString(uint16_t x, uint16_t y, const BFC_FONT* pFont, const uint16_t *str, COLOR color, COLOR bg)
{
	uint16_t _x = x;
	uint16_t _y = y;
	uint16_t width = 0;
	uint16_t ch = 0;
	
	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		ch = *str;
		width = GFXDisplayPutChar(_x, _y, pFont, ch, color, bg);
		str++;
		_x += width;
	}  	
	
	return (uint16_t)(_x-x);	
}

/**
 * @brief	Decode BFC font
 */
static uint16_t bfc_DrawChar_RowRowUnpacked(uint16_t x0, uint16_t y0, const BFC_FONT *pFont, uint16_t ch, COLOR color, COLOR bg)
{
  // 1. find the character information first
  const BFC_CHARINFO *pCharInfo = GetCharInfo(pFont, (unsigned short)ch);
  
  if( pCharInfo != 0 )
  {
    int height = pFont->FontHeight;
    //USE_SERIAL.print("Font height = "); USE_SERIAL.println(height);
    int width = pCharInfo->Width;
    //USE_SERIAL.print("Font width = "); USE_SERIAL.println(width);
    const unsigned char *pData = pCharInfo->p.pData8;   // pointer to data array

    int bpp = GetFontBpp(pFont->FontType);              // how many bits per pixel
    int bytesPerLine = (width * bpp + 7) / 8;           // # bytes in a row
    int bLittleEndian = (GetFontEndian(pFont->FontType)==1);

    uint16_t x, y, _x, _y, col;
    unsigned char data, pixel, bit;
    
    // 2. draw all the pixels in this character
    for(y=0; y<height; y++)
    {
      for(x=0; x<width; x++)
      {
        col = (x * bpp) / 8;       // byte index in the line
        data = pData[y * bytesPerLine + col];

        // every BYTE (8 bits) data includes 8/bpp pixels,
        // we need to get each pixel color index (0,1,2,3... based on bpp) from the BYTE data
        pixel = data;
        
        // bit index in the BYTE
        // For 1-bpp: bit =  x % 8 (Big Endian),   7 -  x % 8 (Little Endian)
        // For 2-bpp: bit = 2x % 8 (Big Endian),   6 - 2x % 8 (Little Endian)
        // For 4-bpp: bit = 4x % 8 (Big Endian),   4 - 4x % 8 (Little Endian)
        bit = bLittleEndian ? (8-bpp)-(x*bpp)%8 : (x*bpp)%8;

        pixel = pixel<<bit;               // clear left pixels
        pixel = pixel>>(8/bpp-1)*bpp;     // clear right pixels
        
        _x = x0+x, 
        _y = y0+y;
          
        if(pixel) 
        {
		  GFXDisplayPutPixel_FB(_x, _y, color);	//update frame buffer, no update on screen yet.
        }
		else
		{
			if(bg!=TRANSPARENT)
				GFXDisplayPutPixel_FB(_x, _y, bg);
		}
      }
    } 
	//update framebuffer for the block area
	GFXDisplayUpdateBlock(y0+1, y0+height, (uint8_t *)&frameBuffer[y0]);
	
    return (uint16_t)width;
  }
  return 0;
} 

/**
 * @brief	Get physical width of the Memory LCD
 * @return	Width of Memory LCD
 */
uint16_t GFXDisplayGetLCDWidth(void)
{
	return DISP_HOR_RESOLUTION;
}

/**
 * @brief	Get physical height of the Memory LCD
 * @return	Height of Memory LCD
 */
uint16_t GFXDisplayGetLCDHeight(void)
{
	return DISP_VER_RESOLUTION;
}

/**
 * @brief	Return character width from a BFC font
 * @param	BFC_FONT* pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	ch in 2-byte width represents the character
 * @return	character width
 */
uint16_t GFXDisplayGetCharWidth(const BFC_FONT *pFont, const uint16_t ch)
{
  const BFC_CHARINFO *pCharInfo = GetCharInfo(pFont, (unsigned short)ch);
  uint16_t _width = 0;
  if( pCharInfo != 0 )
  {
    _width = (uint16_t)pCharInfo->Width;
  }
  
  return _width;
}

/**
 * @brief	Return BFC font height
 * @param	BFC_FONT* pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @return	Font height
 */
uint16_t GFXDisplayGetFontHeight(const BFC_FONT *pFont)
{
	uint16_t _height = 0;
	
    _height = (uint16_t)(pFont->FontHeight);

	return _height;
}

/**
 * @brief	Return a string width from a BFC font
 * @param	*pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	*str is a pointer to character array from Flash space of 1-byte character length
 * @return	string width
 */
uint16_t GFXDisplayGetStringWidth(const BFC_FONT *pFont, const char *str)
{
	uint16_t _x = 0;
	
	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		_x += GFXDisplayGetCharWidth(pFont, (const uint16_t )*str++);
	}  	
	
	return _x;	
}

/**
 * @brief	Return a wide string width from a BFC font
 * @param	*pFont is a pointer to font data from MCU's Flash. Font data created by BitFontCreator (http://www.iseasoft.com)
 * @param	*str is a pointer to character array from Flash space of 2-byte character length
 * @return	string width
 */
uint16_t GFXDisplayGetWStringWidth(const BFC_FONT *pFont, const uint16_t *str)
{
	uint16_t _x = 0;
	
	if( pFont == 0 || str == 0 )
		return 0;

	while(*str != '\0')
	{
		_x += GFXDisplayGetCharWidth(pFont, *str++);
	}  	
	
	return _x;		
}


/**
 * @brief Hardware Abstraction Layer (HAL) write to an IO pin
 * @param pin is the pin number to write
 * @param level indicates the voltage level (1=high; 0=low).
 */
void    hal_gpio_write(uint8_t pin, bool level)
{
  digitalWrite(pin, level);
}

/**
 * @brief Hardware Abstraction Layer (HAL) for a software delay in millisec
 * @param ms is the delay in millisec
 */
void    hal_delayMs(uint32_t ms)
{
  delay(ms);
}

/**
 * @brief Hardware Abstraction Layer (HAL) for a software delay in microseconds
 * @param us is the delay in microseconds
 */
void    hal_delayUs(uint32_t us)
{
  delayMicroseconds(us);
}

/**
 * @brief Hardware Abstraction Layer (HAL) to start SPI transaction
 */
inline void hal_spi_start_transaction(void)
{
  _SPI->beginTransaction(spiSettings);
  digitalWrite(GFX_DISPLAY_SCS, HIGH);  
}

/**
 * @brief Hardware Abstraction Layer (HAL) to stop SPI transaction
 */
inline void hal_spi_end_transaction(void)
{
  digitalWrite(GFX_DISPLAY_SCS, LOW);
  _SPI->endTransaction();  
}

/**
 * @brief Hardware Abstraction Layer (HAL) to send 8-bit value via SPI in LSB first
 */
void hal_spi_write_byte(uint8_t val)
{
	_SPI->transfer(val);
}


/**
 * @brief Hardware Abstraction Layer (HAL) to initialize hardware components including GPIO, SPI setup etc.
 */
void    hal_bsp_init(void)
{
  //pinMode(GFX_5V0_EN, OUTPUT); digitalWrite(GFX_5V0_EN, LOW);           //Set gpio pin for EN pin input TPS60140PWP low for default 5V0=OFF if a gpio wired to EN
  pinMode(GFX_DISPLAY_SCS, OUTPUT); digitalWrite(GFX_DISPLAY_SCS, LOW);   //Set SCS pin an output default low
  pinMode(GFX_DISPLAY_DISP, OUTPUT); digitalWrite(GFX_DISPLAY_DISP, LOW); //Set DISP pin an output default low
  pinMode(GFX_DISPLAY_EXTCOMIN, OUTPUT); digitalWrite(GFX_DISPLAY_EXTCOMIN, LOW);//Set GFX_DISPLAY_EXTCOMIN pin an output default low
  
  #if defined (_VARIANT_ARDUINO_ZERO_)
    static SPIClass raSPI(&sercom1, 12,13,11,SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
    _SPI = &raSPI;
    _SPI->begin();
    pinPeripheral(11,PIO_SERCOM);
    pinPeripheral(12,PIO_SERCOM);
    pinPeripheral(13,PIO_SERCOM);
  #else
    _SPI = &SPI;
    _SPI->begin();
  #endif
}

/**
 * @brief HAL function to start EXTCOMIN pulse
 * @param hz is the pulse frequency measured in hertz. 
 * @note  EXTCOMIN frequency should be made lower than frame frequency.
 *        Reference: http://forum.arduino.cc/index.php?topic=425385.0
 */
void    hal_extcom_start(uint8_t hz)
{
#if defined (_VARIANT_ARDUINO_ZERO_)
    // Set up the generic clock (GCLK4) used to clock timers Arduino M0 specific
    REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |          // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                      GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization
  
    REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                       GCLK_GENCTRL_GENEN |         // Enable GCLK4
                       GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                       GCLK_GENCTRL_ID(4);          // Select GCLK4
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization
  
    // Feed GCLK4 to TC4 and TC5
    REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
                       GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                       GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK4 to TC4 and TC5
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization
   
    REG_TC4_COUNT16_CC0 = ((48000000L>>11)/hz) - 1;  //0xB71A;                   // Set the TC4 CC0 register as the TOP value in match frequency mode
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization
  
    //NVIC_DisableIRQ(TC4_IRQn);
    //NVIC_ClearPendingIRQ(TC4_IRQn);
    NVIC_SetPriority(TC4_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
    NVIC_EnableIRQ(TC4_IRQn);         // Connect TC4 to Nested Vector Interrupt Controller (NVIC)
  
    REG_TC4_INTFLAG |= TC_INTFLAG_OVF;              // Clear the interrupt flags
    REG_TC4_INTENSET = TC_INTENSET_OVF;             // Enable TC4 interrupts
    // REG_TC4_INTENCLR = TC_INTENCLR_OVF;          // Disable TC4 interrupts
   
    REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1024 |   // Set prescaler to 1024, 48MHz/1024 = 46.875kHz
                     TC_CTRLA_WAVEGEN_MFRQ |        // Put the timer TC4 into match frequency (MFRQ) mode 
                     TC_CTRLA_ENABLE;               // Enable TC4
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization  

#elif defined (ESP32)
	//Use 1st timer of 4 (counted from zero).
	//Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more info)
	timer = timerBegin(0,80,true);
	//attach isr function to timer expire function
	timerAttachInterrupt(timer, &hal_extcom_toggle, true);
	timerAlarmWrite(timer, 500000L/hz, true);
	//start timer
	timerAlarmEnable(timer);
#endif
}

/**
 * @brief HAL function to stop EXTCOMIN pulse
 */
void    hal_extcom_stop(void)
{
#if defined (_VARIANT_ARDUINO_ZERO_)
    REG_TC4_INTFLAG |= TC_INTFLAG_OVF;              // Clear the interrupt flags
    REG_TC4_INTENCLR = TC_INTENCLR_OVF;
    hal_gpio_write(GFX_DISPLAY_EXTCOMIN, LOW);
#elif defined (ESP32)
	if(timer)
	{
		timerEnd(timer);
		timer = NULL;
	}
#endif
}

#if defined (_VARIANT_ARDUINO_ZERO_)
/**
 * @brief Global function to toggle EXTCOMIN pin specific for ARDUINO_ZERO
 */
void TC4_Handler(void)
{
  //static int state=0;
  // Check for overflow (OVF) interrupt
  if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF)             
  {
    // Put your timer overflow (OVF) code here:     
    // ...
    //hal_gpio_write(GFX_DISPLAY_EXTCOMIN, (state)?HIGH:LOW);
    //state = !state;
	hal_extcom_toggle();
    REG_TC4_INTFLAG = TC_INTFLAG_OVF;         // Clear the OVF interrupt flag
  }    
}
#endif

/**
 * brief	HAL function to toggle EXTCOMIN pin
 */
#if defined (ESP32)
void IRAM_ATTR hal_extcom_toggle(void)
#else
void hal_extcom_toggle(void)
#endif
{
	if(digitalRead(GFX_DISPLAY_EXTCOMIN)==HIGH)
		hal_gpio_write(GFX_DISPLAY_EXTCOMIN, LOW);
	else
		hal_gpio_write(GFX_DISPLAY_EXTCOMIN, HIGH);
}

