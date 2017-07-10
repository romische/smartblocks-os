#include "firmware.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

void dummy1(){
	for(int i=0; ; i++)
		if(i%200==0)
			Firmware::GetInstance().GetHUARTController().Write('-');
}

void dummy2(){
	for(int i=0; ; i++)
		if(i%200==0)
			Firmware::GetInstance().GetHUARTController().Write('.');
}

/* main function that runs the firmware */
int main(void)
{
   /* FILE structs for fprintf */
   FILE huart;                         
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


	
   /* Execute the firmware */
   fprintf(&huart, "\r\n\r\nStarting the program...\r\n");
	System::instance().run();

   return Firmware::GetInstance().Exec();
   
}

/***********************************************************/
/***********************************************************/
