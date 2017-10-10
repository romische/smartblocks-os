
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Firmware Headers */
#include "system.h"
#include "timer.h"
#include "huart_controller.h"
#include "tw_controller.h"
#include "port_controller.h"

/* I2C Address Space */
#define MPU6050_ADDR 0x68


/******************************************** Accelerometer ******************************************/

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
   
   CHUARTController::instance().lock();
   printf("Connected Ports: ");
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         printf("%s ", GetPortString(eConnectedPort));
      }
   }
   CHUARTController::instance().unlock();
   
   
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


