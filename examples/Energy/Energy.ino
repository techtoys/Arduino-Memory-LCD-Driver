/**
 * @brief   This sketch uses INA226 shunt amplifier to measure the LCD voltage and shunt current across a 100Ohm shunt resistor.<br>
 *          The shunt resistor is wired to VSSA and VSS at one end and the other tied to the power ground.<br>
 *          Therefore what the shunt voltage measuring is the current in return path of the Memory LCD. <br>
 *          After power up press SW3 key onboard, you will see the power consumption data on LCD.<br>
 *          This was calculated by the equation :<br>
 *          Power = LCD voltage * shunt current * time to update<br>
 *          Example, LCD voltage = 5.00V, shunt current measured to be 181uA with update time = 58ms for display a vertical strip<br>
 *          Power = 5*181*58/100 uW = 52uW. <br>
 *          This is the power drawn when the LCD is updated.<br>
 *          When the LCD is not refreshing any content, the shunt current is too weak for measurement. Please refer to user guide for an anaylsis on this.<br>
 *          This demo also print the data via Serial Monitor. You may open it from Tools->Serial Monitor. Set baud rate to 115200 baud with Newline as terminator.<br>
 *        
 *          Select the right LCD model from MemoryLCD.h by uncomment the model to test with<br>
 *          e.g. we are testing 3.2" Memory model.<br>
 *          //#define   LS027B7DH01
 *            #define  LS032B7DD02
 *          //#define   LS044Q7DH01
 *        
 * @note    Programmer : John Leung <br>
 *          Date:        15th June 2018          
 */

  // Unify tone generation for ESP32 to tone(pin,freq,duration) and for other platforms to buzz(pin,freq,duration)
  // Date: 16th March 2025
  // Programmer: John Leung
  
#include "MemoryLCD.h"
#include <INA226.h>

// Declare global variables and instantiate classes
INA226 ina;                                                         // INA class instantiation

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

uint32_t consolasHeight = 0;
uint32_t updateTime = 0;
float shuntVoltage_uV = 0;
float busVoltage_V = 0;
float shuntCurrent_uA = 0;
float power_uW = 0;
  
void buzz(int pin, unsigned int freq, unsigned long duration)
{
  tone(pin,freq,duration);
}

void setup() {
  hal_bsp_init();
  
  GFXDisplayPowerOn();
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(BUZZ, OUTPUT);

  USE_SERIAL.begin(115200);
  delay(1000);

  ina.begin();
  ina.configure(INA226_AVERAGES_4, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
  consolasHeight = GFXDisplayGetFontHeight(&fontConsolas24h);
 
  uint16_t w = GFXDisplayGetStringWidth(&fontConsolas24h, "Press SW3 to measure power");
  GFXDisplayPutString((GFXDisplayGetLCDWidth()-w)/2,120, &fontConsolas24h, "Press SW3 to measure power", BLACK, WHITE);
  
  w = GFXDisplayGetStringWidth(&fontConsolas24h, "Press SW2 to clear LCD");
  GFXDisplayPutString((GFXDisplayGetLCDWidth()-w)/2,
                      120+consolasHeight, 
                      &fontConsolas24h, "Press SW2 to clear LCD", BLACK, WHITE);
}

/** 
 *  INA226 to measure Bus voltage and Shunt Voltage.
 */
void ina226_measure(void)
{
  busVoltage_V = (float)ina.readBusVoltage(); //in V
  shuntVoltage_uV = (float)ina.readShuntVoltage()*1000000.0;    //in uV
}

void result_printout(void)
{
  Serial.print("Bus Voltage:   "); Serial.print(busVoltage_V); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntVoltage_uV); Serial.println(" uV");
  Serial.print("Update time:   "); Serial.print(updateTime); Serial.println(" ms");
  Serial.print("Power:         "); Serial.print(power_uW); Serial.println(" uW");  
}
uint32_t sMillis=0, eMillis=0;

void loop() {  
  /*
    eMillis = millis();
    if(eMillis-sMillis > 2000)
    {
      sMillis = eMillis;
      updateTime = 0;
      ina226_measure();
      if(shuntVoltage_uV > 0) {
        shuntCurrent_uA = shuntVoltage_uV/100;
        power_uW = shuntCurrent_uA*busVoltage_V;

       GFXDisplayPutString(0,GFXDisplayGetLCDHeight()-2*consolasHeight, &fontConsolas24h, "Idle current: ", BLACK, WHITE);
       GFXDisplayPutString(220,GFXDisplayGetLCDHeight()-2*consolasHeight,&fontConsolas24h,(String(shuntCurrent_uA) + "uA    ").c_str(), BLACK, WHITE);
       GFXDisplayPutString(0,GFXDisplayGetLCDHeight()-consolasHeight,&fontConsolas24h, "Idle power: ", BLACK, WHITE);
       GFXDisplayPutString(220,GFXDisplayGetLCDHeight()-consolasHeight,&fontConsolas24h, (String(power_uW) + "uW    ").c_str(), BLACK, WHITE);
       
       result_printout();
      }
      else
      {
        USE_SERIAL.println("Measurement out of range in LCD maintain mode!");
      }
    }
    */
    if(digitalRead(SW3)==LOW)
    {
      delay(10);  //key debounce time for 10ms

      if(digitalRead(SW3)==LOW)
      {
        digitalWrite(GFX_DISPLAY_DISP, HIGH); //DISP = '1'
        buzz(BUZZ,1000,100);
        while(digitalRead(SW3)==LOW)
          ;
          
          USE_SERIAL.println("Energy measurement when display is updated ::");
          updateTime = GFXDisplayTestPattern(0xF0, &ina226_measure);

          shuntCurrent_uA = shuntVoltage_uV/100;
          power_uW = shuntCurrent_uA*busVoltage_V*updateTime/1000;
          delay(1000);
          GFXDisplayPutString(10,10,&fontConsolas24h, "<During display update>", WHITE, BLACK);
          
          GFXDisplayPutString(0,2*consolasHeight,&fontConsolas24h, "LCD voltage: ", BLACK, WHITE);
          GFXDisplayPutString(220,2*consolasHeight,&fontConsolas24h, (String(busVoltage_V) + "V").c_str(), BLACK, WHITE);
          
          GFXDisplayPutString(0,3*consolasHeight,&fontConsolas24h, "Shunt current: ", BLACK, WHITE);
          GFXDisplayPutString(220,3*consolasHeight,&fontConsolas24h, (String(shuntCurrent_uA) + "uA").c_str(), BLACK, WHITE);

          GFXDisplayPutString(0,4*consolasHeight,&fontConsolas24h, "Update time: ", BLACK, WHITE);
          GFXDisplayPutString(220,4*consolasHeight,&fontConsolas24h, (String(updateTime) + "ms").c_str(), BLACK, WHITE);
          
          GFXDisplayPutString(0,5*consolasHeight,&fontConsolas24h, "Power consumption: ", BLACK, WHITE);
          GFXDisplayPutString(220,5*consolasHeight,&fontConsolas24h, (String(power_uW) + "uW").c_str(), BLACK, WHITE);
          
          result_printout();
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



