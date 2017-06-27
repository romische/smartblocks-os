#include "firmware.h"
#include "os.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;


/* main function that runs the firmware */
int main(void)
{
   /* FILE structs for fprintf */
   FILE huart;

   /* Set up FILE structs for fprintf */                           
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
   Firmware::GetInstance().GetHUARTController().Flush();

	Firmware::GetInstance().Exec();
   
   //OS::GetInstance().switch_context((uint16_t*) Execute1);
   
   fprintf(&huart, "...done\r\n");
   Firmware::GetInstance().GetHUARTController().Flush();
   
   return 1; 
   //return Firmware::GetInstance().Exec();
}

/***********************************************************/
/***********************************************************/
