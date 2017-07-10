#include "firmware.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

   FILE huart;    

void dummy1(){
	for(int i=0; ;i++)
		fprintf(&huart, ".%d.",i);
			//Firmware::GetInstance().GetHUARTController().Write('#');
}

void dummy2(){
	while(true)
			Firmware::GetInstance().GetHUARTController().Write('~');
}

/* main function that runs the firmware */
int main(void)
{
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


	
   System::instance().schedule_task((void*) dummy1, nullptr);
   System::instance().schedule_task((void*) dummy2, nullptr);
   
   fprintf(&huart, "\r\n\r\nStarting the program...\r\n");
   System::instance().run();

   return Firmware::GetInstance().Exec();
  
}

/***********************************************************/
/***********************************************************/
