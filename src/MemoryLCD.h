/**
* @brief 		Header file for Sharp Memory LCD
* @processor	ESP32 / Arduino M0 PRO
*/

/**
 * Revision	: 	Add suport for two new modules 
 * 				- LS006B7DH03 (64x64) & LS011B7DH03 (160x68)
 * Date :		29-11-2018
 * */

#ifndef MEMORY_LCD_H
#define MEMORY_LCD_H

#include <stdint.h>	  //for uint8_t etc.
#include <stdbool.h>  //for bool type
#include "bfcFontMgr.h"
#include "tImage.h"
/**
 * @note  Define any model below and recompile<br>
 *        	LS027B7DH01 = 2.7" Memory LCD with 400*240 pixels<br>
 *        	LS032B7DD02 = 3.16" Memory LCD with 336*536 pixels<br>
 *        	LS044Q7DH01 = 4.4" Memory LCD with 320*240 pixels<br>
 * 			LS006B7DH03 = 0.56" Memory LCD with 64*64 pixels, 3V input voltage<br>
 * 			LS011B7DH03 = 1.08" Memory LCD with 160*68, 3V input voltage(br>)
 */
#define   LS027B7DH01
//#define	LS032B7DD02
//#define 	LS044Q7DH01
//#define 	LS006B7DH03
//#define 	LS011B7DH03

//@note pin number definition, hardware dependent
#if defined (ARDUINO)
#include "Arduino.h"
#if defined (_VARIANT_ARDUINO_ZERO_)
	//Arduino M0 PRO :  IO11(MOSI) IO13(SCK) IO10(SCS) IO9(EXTCOMIN) IO8(DISP)
	//Memory LCD   :    MOSI       SCK       SCS       EXTCOMIN      DISP
	#define GFX_DISPLAY_SCS       10
	#define GFX_DISPLAY_EXTCOMIN  9
	#define GFX_DISPLAY_DISP      8
	#define GFX_5V0_EN            3
	#define USE_SERIAL            Serial
#else if defined (ESP32)
	//ESP32    :  IO23(VSPI_MOSI) IO18(VSPI_SCK) IO5(SCS) IO25(EXTCOMIN) IO26(DISP)
	//Memory LCD: MOSI            SCK            SCS      EXTCOMIN       DISP
	#define GFX_DISPLAY_SCS       5
	#define GFX_DISPLAY_EXTCOMIN  25
	#define GFX_DISPLAY_DISP      26
	#define GFX_5V0_EN            2
	#define USE_SERIAL            Serial
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN
#define MIN(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A,B)    ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })
#endif

#ifdef LS027B7DH01
	#define DISP_HOR_RESOLUTION	400
	#define DISP_VER_RESOLUTION	240
#elif defined LS032B7DD02
	#define DISP_HOR_RESOLUTION	336
	#define DISP_VER_RESOLUTION	536
#elif defined LS044Q7DH01
	#define DISP_HOR_RESOLUTION	320
	#define DISP_VER_RESOLUTION	240
#elif defined LS006B7DH03
	#define DISP_HOR_RESOLUTION	64
	#define DISP_VER_RESOLUTION	64
#elif defined LS011B7DH03
	#define DISP_HOR_RESOLUTION	160
	#define DISP_VER_RESOLUTION	68
#else
	#error You need to define the horizontal and vertical resolution for a new model
#endif

//@note Horizontal screen size in byte count
#define GFX_FB_CANVAS_W	((DISP_HOR_RESOLUTION + 7) / 8)
//@note Vertical screen size in line number
#define GFX_FB_CANVAS_H	DISP_VER_RESOLUTION
//@note EXTCOMIN pulse frequency in hal_extcom_start(hz) fcn. -> GFXDisplayOn()
#define EXTCOMIN_FREQ 1 

extern uint8_t frameBuffer[GFX_FB_CANVAS_H][GFX_FB_CANVAS_W];

typedef enum
{
	BLACK = 0,
	WHITE,
	TRANSPARENT	//means leaving original color
} COLOR;

/**
 * @note	HAL functions to be implemented by individual hardware platform
 */
void		hal_bsp_init(void);
inline void	hal_gpio_write(uint8_t pin, bool level);
void		hal_delayMs(uint32_t ms);
void		hal_delayUs(uint32_t us);
inline void hal_spi_start_transaction(void);
inline void hal_spi_end_transaction(void);
inline void hal_spi_write_byte(uint8_t val);
void    hal_extcom_start(uint8_t hz);
void    hal_extcom_stop(void);
void	hal_extcom_toggle(void);

/**
********************************************************************************************************
* @note	API functions
********************************************************************************************************
*/
void GFXDisplayAllClear(void);
void GFXDisplayPowerOn(void);
void GFXDisplayOn(void);
void GFXDisplayPowerOff(void);
void GFXDisplayOff(void);
void GFXDisplayPutPixel(uint16_t x, uint16_t y, COLOR color);
void GFXDisplayLineDrawH(uint16_t x1, uint16_t x2, uint16_t y, COLOR color, uint8_t thick);
void GFXDisplayLineDrawV(uint16_t x, uint16_t y1, uint16_t y2, COLOR color, uint8_t thick);
void GFXDisplayDrawRect(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, COLOR color);
//void GFXDisplayPutPicture(uint16_t left, uint16_t top, const uint8_t* data, bool invert);
void GFXDisplayPutImage(uint16_t left, uint16_t top, const tImage* image, bool invert);
uint32_t GFXDisplayTestPattern(uint8_t pattern, void (*pfcn)(void));
uint16_t GFXDisplayPutChar(uint16_t x, uint16_t y, const BFC_FONT* pFont, const uint16_t ch, COLOR color, COLOR bg);
uint16_t GFXDisplayPutString(uint16_t x, uint16_t y, const BFC_FONT* pFont, const char *str, COLOR color, COLOR bg);
uint16_t GFXDisplayPutWString(uint16_t x, uint16_t y, const BFC_FONT* pFont, const uint16_t *str, COLOR color, COLOR bg);

uint16_t GFXDisplayGetLCDWidth(void);
uint16_t GFXDisplayGetLCDHeight(void);
uint16_t GFXDisplayGetCharWidth(const BFC_FONT *pFont, const uint16_t ch);
uint16_t GFXDisplayGetFontHeight(const BFC_FONT *pFont);
uint16_t GFXDisplayGetStringWidth(const BFC_FONT *pFont, const char *str);
uint16_t GFXDisplayGetWStringWidth(const BFC_FONT *pFont, const uint16_t *str);

#ifdef	__cplusplus
}
#endif

#endif


