
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
#include "nfc_controller.h"

#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3

#define LOG(msg1, msg2){ \
  CHUARTController::instance().lock(); \
  fprintf(CHUARTController::instance().get_file(), "[%u]\t", CTimer::instance().GetMilliseconds()); \
  fprintf(CHUARTController::instance().get_file(), "%s", msg1);\
  fprintf(CHUARTController::instance().get_file(), "%s\r\n", msg2);\
  CHUARTController::instance().unlock();\
  }

stackPointer stackTopCheck;

bool InitMPU6050();
void TestAccelerometer(int arg);

void InitPortController();

void InitLEDs();
void setFaceColor();
void VariateLEDsOnPort(CPortController::EPort eConnectedPort);
void LEDtask();

void dummy5();

void InitNFC();
void NFCTransmit();
void NFCReact();
void NFCtask();


/****************************************** RAM Debug ******************************************/

/*
 * prints RAM usage
 * [HEAP] 256 -> x 	 y <- 2303 [Stack] 	 tot xx yy 
 * where x is the address of a newly created pointer (not the actual heap top!!)
 * and y is the top of the stack
 * xx and yy are the corresponding total sizes
 * needs_to_lock must be set to true if we are in the multitask environment
 */
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
	   CTWController::GetInstance().lock();
	   CTWController::GetInstance().BeginTransmission(MPU6050_ADDR);
	   CTWController::GetInstance().Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
	   CTWController::GetInstance().EndTransmission(false);
	   CTWController::GetInstance().Read(MPU6050_ADDR, 8, true);
	   
	   /* Read the requested 8 bytes */
	   for(uint8_t i = 0; i < 8; i++) {
		  punMPU6050Res[i] = CTWController::GetInstance().Read();
	   }
	   CTWController::GetInstance().unlock();
	   
	   CHUARTController::instance().lock();
	   printf( "Acc[x] = %i\t"
		       "Acc[y] = %i\t"
		       "Acc[z] = %i\t"
		       "Temp = %i\t"
		       "Time = %u\r\n",
		       int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
		       int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
		       int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
		       (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340,
		       CTimer::instance().GetMilliseconds());  
	   CHUARTController::instance().Flush();
	   CHUARTController::instance().unlock();
	   System::instance().sleep(1000);
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


/* Construct the list m_peConnectedPorts and print the connected ports */
void InitPortController() {

   //LOG("Creating list of connected ports", "")
	
	
   /* Init the CPortController */
   CPortController::instance().lock();
   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   System::instance().sleep(10);
   //CPortController::instance().Init();

   /* Fill the list m_peConnectedPorts */
   for(CPortController::EPort& ePort : m_peAllPorts) {
      if(CPortController::instance().IsPortConnected(ePort)) {
      	 //if port is connected then look at all port in connectedPorts and replace the first NULLport encountered 
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort == CPortController::EPort::NULLPORT) {
               eConnectedPort = ePort;
               break;
            }
         }         
      }
   }
   CPortController::instance().SelectPort(CPortController::EPort::NULLPORT);
   CPortController::instance().unlock();
   
   /* Print the list m_peConnectedPorts */
   CHUARTController::instance().lock();
   printf("Connected Ports: ");
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         printf("%s ", CPortController::instance().GetPortString(eConnectedPort));
      }
   }
   printf("\r\n");
   CHUARTController::instance().unlock();
   System::instance().exit_task();
}


/******************************************** Leds ******************************************/


void InitLEDs(){

  //LOG("Init LEDs", "")

  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) { 
         CLEDController::Init(eConnectedPort);
         CLEDController::SetAllModesOnFace(eConnectedPort, CLEDController::EMode::PWM);
         CLEDController::SetAllColorsOnFace(eConnectedPort, 0x01,0x01,0x00);
      }
   }
   
   LOG("LED ok", "")
   System::instance().sleep(10);
   System::instance().exit_task();
}


void setFaceColor(uint8_t unColor, uint8_t unVal, CPortController::EPort eConnectedPort){
   switch(unColor) {
   		case 0 :
   			CLEDController::SetAllColorsOnFace(eConnectedPort, unVal,0x00,0x00);
   			break;
   		case 1 :
   			CLEDController::SetAllColorsOnFace(eConnectedPort, 0x00,unVal,0x00);
   			break;
   		case 2 :
   			CLEDController::SetAllColorsOnFace(eConnectedPort, 0x00,0x00,unVal);
   			break;
   		case 3 :
   			CLEDController::SetAllColorsOnFace(eConnectedPort, unVal,unVal,unVal);
   			break;
   		default :
   			break;
   }
   System::instance().sleep(1);
}

void VariateLEDsOnPort(CPortController::EPort eConnectedPort){
   
   uint8_t unColor = static_cast<uint8_t>(eConnectedPort)%4; 
   while(true){	  
   	  //set color and increase brightness
      for(uint8_t unVal = 0x00; unVal < 0x30; unVal++) { 
      	setFaceColor(unColor, unVal, eConnectedPort);
      }
      
   	  //then decrease brightness
      for(uint8_t unVal = 0x30; unVal > 0x00; unVal--) {
      	setFaceColor(unColor, unVal, eConnectedPort);
      }
      
      
	   //finally set all colors on a low value
	   CLEDController::SetAllColorsOnFace(eConnectedPort,0x01,0x01,0x02);
	   
	   System::instance().sleep(1000);
   }
   
   System::instance().exit_task();
}



void LEDtask(){
	
	System::instance().schedule_task((void*) InitPortController, nullptr);
    System::instance().sleep(1000);
	System::instance().schedule_task((void*) InitLEDs, nullptr);
    System::instance().sleep(1000);
    
	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::NORTH);
	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::EAST);
	
	System::instance().exit_task();
}

void LEDoff(){
  System::instance().schedule_task((void*) InitPortController, nullptr);
  System::instance().sleep(1000);
  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) { 
         CLEDController::Init(eConnectedPort); //just init turn them off
      }
   }
   System::instance().sleep(10);
   System::instance().exit_task();
  
}

/******************************************** Others ******************************************/

void dummy5(){
	int millis;
	while(true){
		CHUARTController::instance().lock();
		printf("Uptime = %u\r\n", CTimer::instance().GetMilliseconds());
		CHUARTController::instance().unlock();
		System::instance().sleep(1000);
	}
}


/******************************************** NFC ******************************************/


void InitNFC(){

  //LOG("Init NFC", "")

  // for each face
  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
      	 //enable port
         CPortController::instance().lock();
		 CPortController::instance().SelectPort(eConnectedPort);
         CPortController::instance().EnablePort(eConnectedPort);
         CPortController::instance().unlock();
         
         //init nfc
         CNFCController nfc;
         bool success = nfc.Init(eConnectedPort);
         
         //if success then light up face in violet
         if(success){
			 CLEDController::SetAllColorsOnFace(eConnectedPort,0x01,0x01,0x02); 
         }
      }
   }
   
   LOG("NFC ok", "")
   System::instance().sleep(10);
   System::instance().exit_task();
}


void NFCTransmit(){
	
	LOG("send msg TM", "")
	
    //on each face
	for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
		if(eConnectedPort != CPortController::EPort::NULLPORT) {
		   
		   uint8_t punOutboundBuffer[] = {'T','M'};
		   uint8_t unRxCount = 0;
		   
		   // send punOutboundBuffer
		   CNFCController nfc;
		   bool success = nfc.Send(eConnectedPort,punOutboundBuffer,2);
		   
		   //clear interrupts
		   CPortController::instance().lock();
		   CPortController::instance().SelectPort(eConnectedPort);
		   CPortController::instance().ClearInterrupts(); //why clear interrupts?
		   CPortController::instance().unlock();
		   
		   //if success light up faces in green, else in red		 
		   if (success){
				setFaceColor(GREEN, 0x04, eConnectedPort);
		   }
		   else{
		   		setFaceColor(RED, 0x04, eConnectedPort);
		   }

		}
	}
}


void NFCReact(){
	
	CPortController::instance().lock();
    CPortController::instance().SynchronizeInterrupts();
	if(CPortController::instance().HasInterrupts()) {
		//fetch the interrupt code
		uint8_t unIRQs = CPortController::instance().GetInterrupts();
		CPortController::instance().ClearInterrupts(unIRQs);
		CPortController::instance().unlock();
		
		//print it
		CHUARTController::instance().lock();
		printf("irq %u\r\n", unIRQs);
		CHUARTController::instance().unlock();
		
		//for each face
		for(CPortController::EPort eConnectedPort : m_peConnectedPorts) {
        	if(eConnectedPort != CPortController::EPort::NULLPORT) {
        		//on the face corresponding to the interrupt
		    	if((unIRQs >> static_cast<uint8_t>(eConnectedPort)) & 0x01) {
		    	
				   	uint8_t punInboundBuffer[2];
				    uint8_t unRxCount = 0;
				    
				    //receive message in punInboundBuffer
				    CNFCController nfc;
				    if(nfc.Receive(eConnectedPort, punInboundBuffer, 2)){
				    	
				    	//light up the face in green
				    	setFaceColor(GREEN, 0x04, eConnectedPort);
				    	
				    	//print the face
				    	LOG("msg on ", CPortController::instance().GetPortString(eConnectedPort))
				    	
				    	//print the msg
				    	CHUARTController::instance().lock();
						printf("(msg is %c%c)\r\n",punInboundBuffer[0],punInboundBuffer[1]);
						CHUARTController::instance().unlock();
				    }
				}//if
			}//if
	    }//for
	}
	else
		CPortController::instance().unlock();
}

void InteractiveMode(){

	LOG("intmod","")

    //loop
    while(true){
    	 uint8_t unInput = 0;
    	 
    	 //check input from user
    	 CHUARTController::instance().lock();
         if(CHUARTController::instance().Available()) {
            unInput = CHUARTController::instance().Read();
            //flush
            while(CHUARTController::instance().Available())
               CHUARTController::instance().Read();
         }
    	 CHUARTController::instance().unlock();
    	 
    	 //if t transmit
    	 if(unInput == 't'){
			  NFCTransmit();
    	 }
    	 
    	 else if(unInput=='u'){
    	 	System::instance().schedule_task((void*) dummy5, nullptr);
    	 }
    	 
    	 else if (unInput=='n'){
    	 	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::NORTH);
    	 }
    	 else if (unInput=='e'){
    	 	System::instance().schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::EAST);
    	 }
    	 else if (unInput=='i'){
    	 	System::instance().schedule_task((void*) InitNFC, nullptr);
    	 }
    	 
    	 //else listen to eventual nfc messages
    	 else {
    	 	NFCReact();
    	 }
    }
}

void NFCtask(){
	
	//init list of connected ports
	System::instance().schedule_task((void*) InitPortController, nullptr);
    System::instance().sleep(1000);
    
    //init leds
	System::instance().schedule_task((void*) InitLEDs, nullptr);
    System::instance().sleep(1000);
    
    //init nfc
	System::instance().schedule_task((void*) InitNFC, nullptr);
    System::instance().sleep(5000);
    
    //interactive mode 
    System::instance().schedule_task((void*) InteractiveMode, nullptr);
    //System::instance().schedule_task((void*) InteractiveMode, nullptr);

    System::instance().exit_task();
}


/******************************************** Main ******************************************/

extern stackPointer stackTop;
extern stackPointer tasksStackTop;
extern task_definition tasks[MAX_TASKS];

int main(void){
	
   stdout = CHUARTController::instance().get_file();
   
   //Led test
   //System::instance().schedule_task((void*) LEDtask, nullptr); 
   //System::instance().schedule_task((void*) LEDoff, nullptr); 
     
     
   //System::instance().schedule_task((void*) dummy5, nullptr);   
   
   
   //Accelerometer test
   //InitMPU6050();
   //System::instance().schedule_task((void*) TestAccelerometer, nullptr);   
   
   //NFC test
   System::instance().schedule_task((void*) NFCtask, nullptr); 
   
   //printRAMUsage("start", false);
   printf("\r\nStarting...\r\n");   
   return System::instance().run();  
}


