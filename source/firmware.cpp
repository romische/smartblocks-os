#include "firmware.h"
#include "os.h"

/* initialisation of the static singleton */
Firmware Firmware::_firmware;

   void Execute1(void* arg) {
   	  FILE* m_psHUART = Firmware::GetInstance().m_psHUART;
   
      uint8_t unInput = 0;
      
      fprintf(m_psHUART, "Ready>\r\n");

      for(;;) {
         if(Firmware::GetInstance().GetHUARTController().Available()) {
            unInput = Firmware::GetInstance().GetHUARTController().Read();
            /* flush */
            while(Firmware::GetInstance().GetHUARTController().Available()) {
               Firmware::GetInstance().GetHUARTController().Read();
            }
         }
         else {
            unInput = 0;
         }

         switch(unInput) {
         case 'u':
            fprintf(m_psHUART, "Uptime context 1 \r\n");
            break;
         default:
            break;
         }
      }
   }

   void Execute2(void* arg) {
   	  FILE* m_psHUART = Firmware::GetInstance().m_psHUART;
   
      uint8_t unInput = 0;
      
      fprintf(m_psHUART, "Ready>\r\n");

      for(;;) {
         if(Firmware::GetInstance().GetHUARTController().Available()) {
            unInput = Firmware::GetInstance().GetHUARTController().Read();
            /* flush */
            while(Firmware::GetInstance().GetHUARTController().Available()) {
               Firmware::GetInstance().GetHUARTController().Read();
            }
         }
         else {
            unInput = 0;
         }

         switch(unInput) {
         case 'u':
            fprintf(m_psHUART, "Uptime context 2 \r\n");
            break;
         default:
            break;
         }
      }
   }



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

	Execute1(nullptr);
   
   //OS::GetInstance().switch_context((uint16_t*) Execute1);
   
   fprintf(&huart, "...done\r\n");
   Firmware::GetInstance().GetHUARTController().Flush();
   
   return 1; 
   //return Firmware::GetInstance().Exec();
}

/***********************************************************/
/***********************************************************/
