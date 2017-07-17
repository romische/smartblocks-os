#include "firmware.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

FILE huart;    

void init_huart_file(){
   /* FILE structs for fprintf */                     
   fdev_setup_stream(&huart, 
                     [](char c_to_write, FILE* pf_stream) {
                        Firmware::GetInstance().GetHUARTController().Write(c_to_write);
                        return 1;
                     },
                     [](FILE* pf_stream) {
                        return int(Firmware::GetInstance().GetHUARTController().Read());
                     },
                     _FDEV_SETUP_RW);
   Firmware::GetInstance().SetFilePointer(&huart);
}


void dummy1(){
	for(int i=0; ;i++)
		fprintf(&huart, ".%d.",i);
			//Firmware::GetInstance().GetHUARTController().Write('#');
}

void dummy2(void* arg){
	char c = *((char*) arg);
	while(true)
			Firmware::GetInstance().GetHUARTController().Write(c);
}



/* main function that runs the firmware */
int main(void){

   init_huart_file();
	
   System::instance().schedule_task((void*) dummy1, nullptr);
   char arg = '-';
   System::instance().schedule_task((void*) dummy2, (void*) &arg );
   
   fprintf(&huart, "\r\n\r\nStarting the program...\r\n");
   return System::instance().run();  
   
}

/***********************************************************/
/***********************************************************/
