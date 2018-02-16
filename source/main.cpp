
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

/*
#define DEBUG(msg){ \
  CHUARTController::instance().lock(); \
  printf("[%d]\t", CTimer::instance().GetMilliseconds());
  printf("%s\r\n", msg);
  CHUARTController::instance().unlock();
  }
*/


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
   CTWController tw  = CTWController::GetInstance();
   
   /* Probe */
   tw.BeginTransmission(MPU6050_ADDR);
   tw.Write(static_cast<uint8_t>(EMPU6050Register::WHOAMI));
   tw.EndTransmission(false);
   tw.Read(MPU6050_ADDR, 1, true);      
   if(tw.Read() != MPU6050_ADDR) 
      return false;
      
   /* select internal clock, disable sleep/cycle mode, enable temperature sensor*/
   tw.BeginTransmission(MPU6050_ADDR);
   tw.Write(static_cast<uint8_t>(EMPU6050Register::PWR_MGMT_1));
   tw.Write(0x00);
   tw.EndTransmission(true);

   return true;
}
	
void TestAccelerometer(int arg) {
	while(true){
	   /* Array for holding accelerometer result */
	   uint8_t punMPU6050Res[8];
	   CTWController tw  = CTWController::GetInstance();
	   tw.lock();
	   tw.BeginTransmission(MPU6050_ADDR);
	   tw.Write(static_cast<uint8_t>(EMPU6050Register::ACCEL_XOUT_H));
	   tw.EndTransmission(false);
	   tw.Read(MPU6050_ADDR, 8, true);
	   
	   /* Read the requested 8 bytes */
	   for(uint8_t i = 0; i < 8; i++) {
		  punMPU6050Res[i] = tw.Read();
	   }
	   tw.unlock();
	   
	   CHUARTController huart = CHUARTController::instance();
	   huart.lock();
	   printf( "Acc[x] = %i\t"
		       "Acc[y] = %i\t"
		       "Acc[z] = %i\t"
		       "Temp = %i\t"
		       "Time = %d\r\n",
		       int16_t((punMPU6050Res[0] << 8) | punMPU6050Res[1]),
		       int16_t((punMPU6050Res[2] << 8) | punMPU6050Res[3]),
		       int16_t((punMPU6050Res[4] << 8) | punMPU6050Res[5]),
		       (int16_t((punMPU6050Res[6] << 8) | punMPU6050Res[7]) + 12412) / 340,
		       CTimer::instance().GetMilliseconds());  
	   huart.Flush();
	   huart.unlock();
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

   CHUARTController::instance().lock();
   printf("[%d]\tCreating list of connected ports\r\n", CTimer::instance().GetMilliseconds());
   CHUARTController::instance().unlock();
	
   CPortController p = CPortController::instance();
	
   /* Init the CPortController */
   p.lock();
   p.SelectPort(CPortController::EPort::NULLPORT);
   System::instance().sleep(10);
   p.Init();

   // disable tw before??

   /* Fill the list m_peConnectedPorts */
   for(CPortController::EPort& ePort : m_peAllPorts) {
      if(p.IsPortConnected(ePort)) {
      	 //if port is connected then look at all port in connectedPorts and replace the first NULLport encountered 
         for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
            if(eConnectedPort == CPortController::EPort::NULLPORT) {
               eConnectedPort = ePort;
               break;
            }
         }         
      }
   }
   p.SelectPort(CPortController::EPort::NULLPORT);
   p.unlock();
   
   /* Print the list m_peConnectedPorts */
   CHUARTController::instance().lock();
   printf("Connected Ports: ");
   for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         printf("%s ", p.GetPortString(eConnectedPort));
      }
   }
   printf("\r\n");
   CHUARTController::instance().unlock();
   System::instance().exit_task();
}


/******************************************** Leds ******************************************/


void InitLEDs(){

  CHUARTController::instance().lock();
  printf("[%d]\tInitiating LEDs\r\n", CTimer::instance().GetMilliseconds());
  CHUARTController::instance().unlock();
 
  CPortController p = CPortController::instance(); 

  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
      	 
         p.lock();
         p.SelectPort(eConnectedPort);
     	 
         CLEDController::Init();
         CLEDController::SetAllModesOnFace(CLEDController::EMode::PWM);
         CLEDController::SetAllColorsOnFace(0x01,0x01,0x02);
         
         p.unlock();
      }
   }
   
   System::instance().sleep(10);
   System::instance().exit_task();
}


void setFaceColor(uint8_t unColor, uint8_t unVal, CPortController::EPort eConnectedPort){

   CPortController p = CPortController::instance();
   
   p.lock();
   p.SelectPort(eConnectedPort);
   switch(unColor) {
   		case 0 :
   			CLEDController::SetAllColorsOnFace(unVal,0x00,0x00);
   			break;
   		case 1 :
   			CLEDController::SetAllColorsOnFace(0x00,unVal,0x00);
   			break;
   		case 2 :
   			CLEDController::SetAllColorsOnFace(0x00,0x00,unVal);
   			break;
   		case 3 :
   			CLEDController::SetAllColorsOnFace(unVal,unVal,unVal);
   			break;
   		default :
   			break;
   }
   p.unlock();
   System::instance().sleep(1);
}

void VariateLEDsOnPort(CPortController::EPort eConnectedPort){
   
   CPortController p = CPortController::instance();

   CHUARTController::instance().lock();
   printf("[%d]\tVariating LEDs on port", CTimer::instance().GetMilliseconds());
   printf(" %s \r\n", p.GetPortString(eConnectedPort));  //WHY ONLY ONE ARGUMENT??!
   CHUARTController::instance().unlock();
   	  
   //foreach color  (red 0, green 1, blue 2, white 3 )
   for(uint8_t unColor = 0; unColor < 4; unColor++) {
   	  
   	  //set color and increase brightness
      for(uint8_t unVal = 0x00; unVal < 0x40; unVal++) {
      	setFaceColor(unColor, unVal, eConnectedPort);
      }
      
   	  //then decrease brightness
      for(uint8_t unVal = 0x40; unVal > 0x00; unVal--) {
      	setFaceColor(unColor, unVal, eConnectedPort);
      }
   }
   
   //finally set all colors on a low value
   p.lock();
   p.SelectPort(eConnectedPort);
   CLEDController::SetAllColorsOnFace(0x01,0x01,0x02);
   p.unlock();
   
   System::instance().exit_task();
}



void LEDtask(){
	System sys = System::instance();
	
	sys.schedule_task((void*) InitPortController, nullptr);
    sys.sleep(1000);
	sys.schedule_task((void*) InitLEDs, nullptr);
    sys.sleep(1000);
    
	sys.schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::TOP);
	sys.schedule_task((void*) VariateLEDsOnPort, (void*) CPortController::EPort::BOTTOM);
	
	sys.exit_task();
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


/******************************************** NFC ******************************************/


void InitNFC(){

  CHUARTController::instance().lock();
  printf("[%d]\tInitiating NFC\r\n", CTimer::instance().GetMilliseconds());
  CHUARTController::instance().unlock();
 
 
  CPortController p = CPortController::instance(); 
  CNFCController nfc;

  for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
      if(eConnectedPort != CPortController::EPort::NULLPORT) {
         p.lock();
         p.SelectPort(eConnectedPort);
         p.EnablePort(eConnectedPort);
         //3 attempts to init NFC
     	 for(uint8_t unAttempts = 3; unAttempts > 0; unAttempts--) {
		   	if(nfc.Probe() == true) {
			  if(nfc.ConfigureSAM() == true) {
				 if(nfc.PowerDown() == true) {
		            setFaceColor(3, 0x04, eConnectedPort);
		            break;
				 }
			  }   
		   	}
            System::instance().sleep(100);
         }
         
         p.unlock();
      }
   }
   
   System::instance().sleep(10);
   System::instance().exit_task();
}


void NFCTransmit(){
	CPortController p = CPortController::instance(); 
	CHUARTController huart = CHUARTController::instance();
	
	huart.lock();
	printf("NFC transmission\r\n");
	huart.unlock();

	for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
		if(eConnectedPort != CPortController::EPort::NULLPORT) {

		   huart.lock();
		   printf("On port %s : ", p.GetPortString(eConnectedPort) );
		   huart.unlock();		   
		   
		   uint8_t punOutboundBuffer[] = {'S','M','A','R','T','B','L','K','0','2'};
		   uint8_t punInboundBuffer[20];
		   uint8_t unRxCount = 0;
		   
		   p.lock();
		   p.SelectPort(eConnectedPort);
					 	 
		   CNFCController nfc;
		   if(nfc.P2PInitiatorInit()) {
			  unRxCount = nfc.P2PInitiatorTxRx(punOutboundBuffer,10,punInboundBuffer,20);
		   }
		   nfc.PowerDown();

		   System::instance().sleep(100);
		   p.ClearInterrupts();
		   p.unlock();
		 
		   if (unRxCount > 0){ //success
				setFaceColor(2, 0x04, eConnectedPort);
		   }
		   else{  //failure
		   		setFaceColor(0, 0x04, eConnectedPort);
		   }

		}
	}
}



void NFCReact(){
	CPortController p = CPortController::instance(); 
	CHUARTController huart = CHUARTController::instance();
	
	p.lock();
    p.SynchronizeInterrupts();
	if(p.HasInterrupts()) {
		uint8_t unIRQs = p.GetInterrupts();
		p.unlock();
				    
		huart.lock();
		printf("NFC : received irq(0x%02X)\r\n", unIRQs);
		huart.unlock();
					
		for(CPortController::EPort eRxPort : m_peConnectedPorts) {
        	if(eRxPort != CPortController::EPort::NULLPORT) {
		    	if((unIRQs >> static_cast<uint8_t>(eRxPort)) & 0x01) {
					uint8_t punOutboundBuffer[] = {'S','B','0','1'};
				    uint8_t punInboundBuffer[8];
				    uint8_t unRxCount = 0;
				             
				    p.lock();
				    p.SelectPort(eRxPort);
				             
				    CNFCController nfc;
				    if(nfc.P2PTargetInit()) {
				    	unRxCount = nfc.P2PTargetTxRx(punOutboundBuffer, 4, punInboundBuffer, 8);
				    }
				    System::instance().sleep(60);
				    nfc.PowerDown();
				    System::instance().sleep(100);
				    p.ClearInterrupts();
				    p.unlock();

				    for(CPortController::EPort& eConnectedPort : m_peConnectedPorts) {
				    	uint8_t unColor = (uint8_t) punInboundBuffer[0];
				    			    
						huart.lock();
						printf("Asked for color %d\r\n", unColor);
						huart.unlock();
				    	setFaceColor(1, 0x20, eConnectedPort);
					}
				}//if
			}//if
	    }//for
	}
	else
		p.unlock();
}


void NFCtask(){

	System sys = System::instance();
	CHUARTController huart = CHUARTController::instance();
	
	sys.schedule_task((void*) InitPortController, nullptr);
    sys.sleep(1000);
    huart.lock();
    printf("printing doesnt work from there ah!\r\n");
    huart.unlock();
	sys.schedule_task((void*) InitLEDs, nullptr);
    sys.sleep(1000);
	sys.schedule_task((void*) InitNFC, nullptr);
    sys.sleep(1000);
    
    
    
    
    while(true){
    	 uint8_t unInput = 0;
    	 
    	 //check input from user
    	 huart.lock();
         if(huart.Available()) {
            unInput = huart.Read();
            //flush
            while(huart.Available())
               huart.Read();
         }
    	 huart.unlock();
    	 
    	 //transmit something
    	 if(unInput == 't'){
			  NFCTransmit();
    	 }
    	 else {
    	 	NFCReact();
    	 }
    } //while
    
    
    
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
     
   
   //Accelerometer test
   //InitMPU6050();
   //System::instance().schedule_task((void*) TestAccelerometer, nullptr);   

   
   //NFC test
   System::instance().schedule_task((void*) NFCtask, nullptr); 
   
   //printRAMUsage("start", false);
   printf("\r\nStarting the program...\r\n");   
   return System::instance().run();  
}


