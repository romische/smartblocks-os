
#include "led_controller.h"

#define PCA963X_RST_ADDR  0x03 
#define PCA963X_RST_BYTE1 0xA5
#define PCA963X_RST_BYTE2 0x5A
#define PCA963X_DEV_ADDR  0x18
#define PCA963X_LEDOUTX_MASK 0x03


#define RGB_RED_OFFSET    0
#define RGB_GREEN_OFFSET  1
#define RGB_BLUE_OFFSET   2
#define RGB_UNUSED_OFFSET 3

#define RGB_LEDS_PER_FACE 4

void CLEDController::Init() {
   /* Send the reset sequence */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_RST_ADDR);
   CTWController::GetInstance().Write(PCA963X_RST_BYTE1);
   CTWController::GetInstance().Write(PCA963X_RST_BYTE2);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();   

   /* Wake up the internal oscillator, disable group addressing and auto-increment */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(ERegister::MODE1));
   CTWController::GetInstance().Write(0x00);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();

   /* Enable group blinking */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(ERegister::MODE2));
   CTWController::GetInstance().Write(0x25);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();

   /* Default blink configuration 1s period, 50% duty cycle */
   SetBlinkRate(0x18, 0x80);

   /* Default all leds to off */
   for(uint8_t unLED = 0; unLED < 4; unLED++) {
      SetMode(unLED, EMode::OFF);
      SetBrightness(unLED, 0x00);
   }
}

void CLEDController::SetMode(uint8_t un_led, EMode e_mode) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::LEDOUT0) + (un_led / 4u);
   /* read current register value */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(unRegisterAddr);
   CTWController::GetInstance().EndTransmission(false);
   CTWController::GetInstance().Read(PCA963X_DEV_ADDR, 1, true);
   uint8_t unRegisterVal = CTWController::GetInstance().Read();
   CTWController::GetInstance().unlock();
   
   /* clear and set target bits in unRegisterVal */
   unRegisterVal &= ~(PCA963X_LEDOUTX_MASK << ((un_led % 4) * 2));
   unRegisterVal |= (static_cast<uint8_t>(e_mode) << ((un_led % 4) * 2));
   
   /* write back */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(unRegisterAddr);
   CTWController::GetInstance().Write(unRegisterVal);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();
}

void CLEDController::SetBrightness(uint8_t un_led, uint8_t un_val) {
   /* get the register responsible for LED un_led */
   uint8_t unRegisterAddr = static_cast<uint8_t>(ERegister::PWM0) + un_led;
   /* write value */
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(unRegisterAddr);
   CTWController::GetInstance().Write(un_val);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();
}

void CLEDController::SetColor(uint8_t un_led_pack, uint8_t unRed, uint8_t unGreen, uint8_t unBlue){
      SetBrightness(un_led_pack * RGB_LEDS_PER_FACE + RGB_RED_OFFSET, unRed);
      SetBrightness(un_led_pack * RGB_LEDS_PER_FACE + RGB_GREEN_OFFSET, unGreen);
      SetBrightness(un_led_pack * RGB_LEDS_PER_FACE + RGB_BLUE_OFFSET, unBlue);
}

void CLEDController::SetBlinkRate(uint8_t un_period, uint8_t un_duty_cycle) {
   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(ERegister::GRPFREQ));
   CTWController::GetInstance().Write(un_period);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();

   CTWController::GetInstance().lock();
   CTWController::GetInstance().BeginTransmission(PCA963X_DEV_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(ERegister::GRPPWM));
   CTWController::GetInstance().Write(un_duty_cycle);
   CTWController::GetInstance().EndTransmission(true);
   CTWController::GetInstance().unlock();
}


void CLEDController::SetAllColorsOnFace(uint8_t unRed, uint8_t unGreen, uint8_t unBlue){
	for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++)
		CLEDController::SetColor(unLedIdx, unRed, unGreen, unBlue);
}
   
void CLEDController::SetAllModesOnFace(EMode e_mode){
   for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE + RGB_RED_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE + RGB_GREEN_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE + RGB_BLUE_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE + RGB_UNUSED_OFFSET, CLEDController::EMode::OFF);
   }

}

