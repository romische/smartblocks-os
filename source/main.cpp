
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Firmware Headers */
#include "system.h"
#include "timer.h"
#include "huart_controller.h"
#include "tw_controller.h"
#include "port_controller.h"
#include "led_controller.h"



void TestAccelerometer(int arg);
const char* GetPortString(CPortController::EPort ePort);
void TestPortController();
void SetAllColorsOnFace(uint8_t unRed, uint8_t unGreen, uint8_t unBlue);
void SetAllModesOnFace(CLEDController::EMode e_mode);
void TestLEDs();
void dummy5();


/******************************************** Accelerometer ******************************************/

/* I2C Address Space */
#define MPU6050_ADDR 0x68

enum class EMPU6050Register : uint8_t {
   /* MPU6050 Registers */
   PWR_MGMT_1     = 0x6B, // R/W
   PWR_MGMT_2     = 0x6C, // R/W
   ACCEL_XOUT_H   = 0x3B, // R  
   ACCEL_XOUT_L   = 0x3C, // R  
   ACCEL_YOUT_H   = 0x3D, // R  
   ACCEL_YOUT_L   = 0x3E, // R  
   ACCEL_ZOUT_H   = 0x3F, // R  
   ACCEL_ZOUT_L   = 0x40, // R  
   TEMP_OUT_H     = 0x41, // R  
   TEMP_OUT_L     = 0x42, // R  
   WHOAMI         = 0x75  // R
};
	
void TestAccelerometer(int arg) {
	while(true){
	   /* Array for holding accelerometer result */
	   uint8_t punMPU6050Res[8];
		CHUARTController::instance().lock();
		printf("task %d locking the TW\r\n",arg);
		CHUARTController::instance().unlock();
	   CTWController::GetInstance().lock();
	   CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
	   CTWController::GetInstance().Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
	   CTWController::GetInstance().EndTransmission(false);
	   CTWController::GetInstance().Read(MPU6050_ADDR, 8, true);
	   /* Read the requested 8 bytes */
	   for(uint8_t i = 0; i < 8; i++) {
		  punMPU6050Res[i] = CTWController::GetInstance().Read();
	   }
	   
		CHUARTController::instance().lock();
		printf("task %d UNlocking the TW\r\n",arg);
		CHUARTController::instance().unlock();
		
	   CTWController::GetInstance().unlock();
	   
	   CHUARTController::instance().lock();
	   printf( "---------------%d\t"
	   		   "Acc[x] = %i\t"
		       "Acc[y] = %i\t"
		       "Acc[z] = %i\t"
		       "Temp = %i\t"
		       "Time = %d\r\n",
		       arg,
		       int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
		       int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
		       int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
		       (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340,
		       CTimer::instance().GetMilliseconds());  
	   CHUARTController::instance().Flush();
	   CHUARTController::instance().unlock();
	}
}

/******************************************** PortController ******************************************/


CPortController::EPort m_peAllPorts[NUM_PORTS] {
      CPortController::EPort::NORTH,
      CPortController::EPort::EAST,
      CPortController::EPort::SOUTH,
      CPortController::EPort::WEST,
      CPortController::EPort::TOP,
      CPortController::EPort::BOTTOM,
   };

CPortController::EPort m_peConnectedPorts[NUM_PORTS] {
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
      CPortController::EPort::NULLPORT,
   };


const char* GetPortString(CPortController::EPort ePort) {
   switch(ePort) {
   case CPortController::EPort::NORTH:
      return "NORTH";
      break;
   case CPortController::EPort::EAST:
      return "EAST";
      break;
   case CPortController::EPort::SOUTH:
      return "SOUTH";
      break;
   case CPortController::EPort::WEST:
      return "WEST";
      break;
   case CPortController::EPort::TOP:
      return "TOP";
      break;
   case CPortController::EPort::BOTTOM:
      return "BOTTOM";
      break;
   default:
      return "INVALID";
      break;
   }
}


void TestPortController() {

   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   System::instance().sleep(10);
   CPortController::instance().Init();


   for(CPortController::EPort& ePort : m_peAllPorts) {
      if(CPortController::instance().IsPortConnected(ePort)) {
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort == CPortController::EPort::NULLPORT) {
               eConnectedPort = ePort;
               break;
            }
         }         
      }
   }
   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   
   //print result
   CHUARTController::instance().lock();
   printf("Connected Ports: ");
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         printf("%s ", GetPortString(eConnectedPort));
      }
   }
   printf("\r\n");
   CHUARTController::instance().unlock();
   
   System::instance().schedule_task((void*) TestLEDs, nullptr);
   System::instance().exit_task();
}


/******************************************** Leds ******************************************/

#define RGB_RED_OFFSET    0
#define RGB_GREEN_OFFSET  1
#define RGB_BLUE_OFFSET   2
#define RGB_UNUSED_OFFSET 3

#define RGB_LEDS_PER_FACE 4


void SetAllColorsOnFace(uint8_t unRed, uint8_t unGreen, uint8_t unBlue) {

   for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_RED_OFFSET, unRed);
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_GREEN_OFFSET, unGreen);
      CLEDController::SetBrightness(unLedIdx * RGB_LEDS_PER_FACE +
                                    RGB_BLUE_OFFSET, unBlue);
   }
}


void SetAllModesOnFace(CLEDController::EMode e_mode) {

   for(uint8_t unLedIdx = 0; unLedIdx < RGB_LEDS_PER_FACE; unLedIdx++) {
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE 
                              + RGB_RED_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_GREEN_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_BLUE_OFFSET, e_mode);
      CLEDController::SetMode(unLedIdx * RGB_LEDS_PER_FACE +
                              RGB_UNUSED_OFFSET, CLEDController::EMode::OFF);
   }
}


void TestLEDs() {
	//Init leds
  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         CPortController::instance().SelectPort(eConnectedPort);
         CLEDController::Init();
         SetAllModesOnFace(CLEDController::EMode::PWM);
         SetAllColorsOnFace(0x01,0x01,0x00);
         CPortController::instance().EnablePort(eConnectedPort);  //whut why?
      }
   }
	

	//foreach color
   for(uint8_t unColor = 0; unColor < 3; unColor++) {
   	
	   CHUARTController::instance().lock();
	   printf("color %d\r\n", unColor);
	   CHUARTController::instance().unlock();
	   
   	  //set color and increase brightness
      for(uint8_t unVal = 0x00; unVal < 0x40; unVal++) {
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               CPortController::instance().SelectPort(eConnectedPort);
               CLEDController::SetBrightness(unColor + 0, unVal);
               CLEDController::SetBrightness(unColor + 4, unVal);
               CLEDController::SetBrightness(unColor + 8, unVal);
               CLEDController::SetBrightness(unColor + 12, unVal);
            }
         }
         System::instance().sleep(1);
      }
   	  //then decrease brightness
      for(uint8_t unVal = 0x40; unVal > 0x00; unVal--) {
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort != CPortController::EPort::NULLPORT) {
               CPortController::instance().SelectPort(eConnectedPort);
               CLEDController::SetBrightness(unColor + 0, unVal);
               CLEDController::SetBrightness(unColor + 4, unVal);
               CLEDController::SetBrightness(unColor + 8, unVal);
               CLEDController::SetBrightness(unColor + 12, unVal);
            }  
         }
         System::instance().sleep(1);
      }
      //and finally turn it off
      for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
         if(eConnectedPort != CPortController::EPort::NULLPORT) {
            CLEDController::SetBrightness(unColor + 0, 0x00);
            CLEDController::SetBrightness(unColor + 4, 0x00);
            CLEDController::SetBrightness(unColor + 8, 0x00);
            CLEDController::SetBrightness(unColor + 12, 0x00);
         }
      }
   }
   
   //why is it still on the default color while we just turn them on? 
   
   
   System::instance().exit_task();
}


/******************************************** Others ******************************************/

void dummy5(){
	CTimer timer = CTimer::instance();
	int millis;
	while(true){
		CHUARTController::instance().lock();
		printf("Uptime = %d\r\n", timer.GetMilliseconds());
		CHUARTController::instance().unlock();
		System::instance().sleep(1000);
	}
}


/******************************************** Main ******************************************/

int main(void){
	
   stdout = CHUARTController::instance().get_file();
   
   
   System::instance().schedule_task((void*) TestPortController, nullptr);
   
   
   printf("\r\n\r\nStarting the program...\r\n");
   return System::instance().run();  
}


