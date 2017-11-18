
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>  //for malloc

/* Firmware Headers */
#include "system.h"
#include "timer.h"
#include "huart_controller.h"
#include "tw_controller.h"
#include "port_controller.h"
#include "led_controller.h"

stackPointer stackTopCheck;

bool InitMPU6050();
void TestAccelerometer(int arg);

void TestPortController();

void InitLEDs();
void TestLEDs();
void VariateLEDsOnPort(CPortController::EPort eConnectedPort);

void dummy5();
void LEDtask();


/****************************************** RAM Debug ******************************************/

void printRAMUsage(char* func_name, bool needs_to_lock){
	//set the stackTopCheck pointer
   asm volatile("in r0, __SP_L__\nsts stackTopCheck, r0\nin r0, __SP_H__\nsts stackTopCheck+1, r0");
   
   //creates a pointer that may point to the top of the stack (better use a bigger one...)
   int* p = (int*) malloc(sizeof(int));
   
   //print
   if(needs_to_lock) CHUARTController::instance().lock();
   printf("%s : \t [HEAP] 256 -> %u \t %u <- 2303 [Stack] \t tot %u %u \r\n", func_name, p, stackTopCheck, ((uint16_t)p-256), (2303-(uint16_t) stackTopCheck));
   if(needs_to_lock) CHUARTController::instance().unlock();
   
   //free memory
   free(p);
}

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


bool InitMPU6050() {
   /* Probe */
   CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(EMPU6050Register::WHOAMI));
   CTWController::GetInstance().EndTransmission(false);
   CTWController::GetInstance().Read(MPU6050_ADDR, 1, true);
         
   if(CTWController::GetInstance().Read() != MPU6050_ADDR) 
      return false;
   /* select internal clock, disable sleep/cycle mode, enable temperature sensor*/
   CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
   CTWController::GetInstance().Write(static_cast<uint8_t>(EMPU6050Register::PWR_MGMT_1));
   CTWController::GetInstance().Write(0x00);
   CTWController::GetInstance().EndTransmission(true);

   return true;
}
	
void TestAccelerometer(int arg) {
	while(true){
	   /* Array for holding accelerometer result */
	   uint8_t punMPU6050Res[8];
		CHUARTController::instance().lock();
		printf("task %d try to lock the TW\r\n",arg);
		CHUARTController::instance().unlock();
	   CTWController::GetInstance().lock();
		CHUARTController::instance().lock();
		printf("task %d locked the TW\r\n",arg);
		CHUARTController::instance().unlock();
	   CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
	   CTWController::GetInstance().Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
	   CTWController::GetInstance().EndTransmission(false);
	   CTWController::GetInstance().Read(MPU6050_ADDR, 8, true);
	   
	   /* Read the requested 8 bytes */
	   for(uint8_t i = 0; i < 8; i++) {
		  punMPU6050Res[i] = CTWController::GetInstance().Read();
	   }
	   
		
		CHUARTController::instance().lock();
		printf("task %d UNlock the TW\r\n",arg);
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
		//System::instance().sleep(1000);
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

void TestPortController() {
	
   // Init the CPortController
   CPortController::instance().lock();
   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   System::instance().sleep(10);
   CPortController::instance().Init();
   CPortController::instance().unlock();


   // Construct the list m_peConnectedPorts
   for(CPortController::EPort& ePort : m_peAllPorts) {
   	  CPortController::instance().lock();
      if(CPortController::instance().IsPortConnected(ePort)) {
      	 CPortController::instance().unlock();
      	 //if port is connected then look at all port in connectedPorts and replace the first NULLport encountered 
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort == CPortController::EPort::NULLPORT) {
               eConnectedPort = ePort;
               break;
            }
         }         
      }
      else
      	CPortController::instance().unlock();
   }
   
   CPortController::instance().lock();
   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   CPortController::instance().unlock();
   
   //print the list m_peConnectedPorts
   CHUARTController::instance().lock();
   printf("Connected Ports: ");
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         printf("%s ", CPortController::instance().GetPortString(eConnectedPort));
      }
   }
   printf("\r\n");
   CHUARTController::instance().unlock();
}


/******************************************** Leds ******************************************/


void InitLEDs(){


   
   printRAMUsage("InitLEDs", true);

  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
      	 
      	 //CHUARTController::instance().lock();
      	 //printf("Initiating LEDs on port %s \r\n", CPortController::instance().GetPortString(eConnectedPort));
      	 //CHUARTController::instance().unlock();
      	 
         CPortController::instance().lock();
         CPortController::instance().SelectPort(eConnectedPort);
     	 
         CLEDController::Init();
         CLEDController::SetAllModesOnFace(CLEDController::EMode::PWM);
         CLEDController::SetAllColorsOnFace(0x01,0x01,0x02);
         
         CPortController::instance().EnablePort(eConnectedPort);  //whut why?
         CPortController::instance().unlock();
      }
   }
   
   
   
   printRAMUsage("InitLEDs", true);
   
   System::instance().sleep(10);
}


void VariateLEDsOnPort(CPortController::EPort eConnectedPort){

	
   printRAMUsage("Var LEDs", true);
   	  
	//foreach color
	//red 0, green 1, blue 2, white 3
   for(uint8_t unColor = 0; unColor < 4; unColor++) {
   	  
   	  //set color and increase brightness
      for(uint8_t unVal = 0x00; unVal < 0x40; unVal++) {
         CPortController::instance().lock();
         CPortController::instance().SelectPort(eConnectedPort);
         if(unColor==0)
         	CLEDController::SetAllColorsOnFace(unVal,0x00,0x00);
         else if (unColor==1)
         	CLEDController::SetAllColorsOnFace(0x00,unVal,0x00);
         else if (unColor==2)
         	CLEDController::SetAllColorsOnFace(0x00,0x00,unVal);
         else 
         	CLEDController::SetAllColorsOnFace(unVal,unVal,unVal);
         CPortController::instance().unlock();
         System::instance().sleep(1);
      }
      
   	  //then decrease brightness
      for(uint8_t unVal = 0x40; unVal > 0x00; unVal--) {
         CPortController::instance().lock();
         CPortController::instance().SelectPort(eConnectedPort);
         if(unColor==0)
         	CLEDController::SetAllColorsOnFace(unVal,0x00,0x00);
         else if (unColor==1)
         	CLEDController::SetAllColorsOnFace(0x00,unVal,0x00);
         else if (unColor==2)
         	CLEDController::SetAllColorsOnFace(0x00,0x00,unVal);
         else 
         	CLEDController::SetAllColorsOnFace(unVal,unVal,unVal);
         CPortController::instance().unlock();
         System::instance().sleep(1);
      }
   }
   
   //finally set all colors on a low value
   CPortController::instance().lock();
   CPortController::instance().SelectPort(eConnectedPort);
   CLEDController::SetAllColorsOnFace(0x01,0x01,0x01);
   CPortController::instance().unlock();
   
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

void LEDtask(){
	TestPortController();
	InitLEDs();
	
	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::NORTH);
	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::EAST);
	//System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::SOUTH);
	//System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::WEST);
	//System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::TOP);
	//System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::BOTTOM);
	
	/*for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
    	if(eConnectedPort != CPortController::EPort::NULLPORT) {
    		CPortController::EPort port = eConnectedPort;
			CHUARTController::instance().lock();
			printf("Lauching task on %s\r\n", CPortController::instance().GetPortString(port));
			CHUARTController::instance().unlock();
			System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) port);
		}
	}*/
	
	System::instance().exit_task();
}


/******************************************** Main ******************************************/

extern stackPointer stackTop;
extern stackPointer tasksStackTop;
extern task_definition tasks[MAX_TASKS];

int main(void){
	
   printRAMUsage("main", false);
	
   stdout = CHUARTController::instance().get_file();
   
   //InitMPU6050();
   
   
   printRAMUsage("Scheduling", false);
   System::instance().schedule_task((void*) LEDtask, nullptr);   
   /*
   System::instance().schedule_task((void*) TestAccelerometer, (void*) 2);   
   System::instance().schedule_task((void*) TestAccelerometer, (void*) 7);
   */
   
   //
   printRAMUsage("Starting", false);
   printf("StackTop : %u\r\n", stackTop);
   printf("tasksStackTop : %u\r\n", tasksStackTop);
   printf("task %d SP : %u\r\n", 0, tasks[0].sp);
   printf("task %d SP : %u\r\n", 1, tasks[2].sp);
   printf("task %d SP : %u\r\n", 2, tasks[2].sp);
   printf("\r\n\r\nStarting the program...\r\n");   
   
   return System::instance().run();  
}


