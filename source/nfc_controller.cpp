
#include "nfc_controller.h"

uint8_t ack[6]={
    0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00
};
uint8_t nfc_version[6]={
    0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03
};
uint8_t default_response[1] = {'T'};
uint8_t default_response_len = 1;


/***********************************************************/
/***********************************************************/

/*
 * msg is a filled buffer
 * send is validated if gotten default_response as response
 */
bool CNFCController::Send(CPortController::EPort e_port, uint8_t* msg, uint8_t len){
	uint8_t unRxCount = 0;
	uint8_t punInboundBuffer[default_response_len];
	
	if(P2PInitiatorInit(e_port)) {
		unRxCount = P2PInitiatorTxRx(e_port, msg, len, punInboundBuffer, default_response_len);
	}
	PowerDown(e_port);
	System::instance().sleep(100);
	
	return (unRxCount == default_response_len && punInboundBuffer[0]==default_response[0]); //or just unRxCount>0 ?
}

/*
 * msg is an empty buffer
 * send default_response as response
 */
bool CNFCController::Receive(CPortController::EPort e_port, uint8_t* msg, uint8_t len){
	uint8_t unRxCount = 0;
	if(P2PTargetInit(e_port)) {
		unRxCount = P2PTargetTxRx(e_port,default_response, default_response_len, msg, len);
	}
	System::instance().sleep(60);
	PowerDown(e_port);
	System::instance().sleep(100);
	return (unRxCount>0);
}

/*
 * init nfc on face e_port
 */
bool CNFCController::Init(CPortController::EPort e_port){
	bool success = false;
         //3 attempts to init NFC
    for(uint8_t unAttempts = 3; unAttempts > 0; unAttempts--) {
	   	if(Probe(e_port) == true) {
		  if(ConfigureSAM(e_port) == true) {
			 if(PowerDown(e_port) == true) {
			 	success=true;
	            break;
			 }
		  }   
	   	}
        System::instance().sleep(100);
    }
    return success;
}



/***********************************************************/
/***********************************************************/

bool CNFCController::PowerDown(CPortController::EPort e_port) {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::POWERDOWN);
   m_punIOBuffer[1] = 0x88; // Wake up on RF field & I2C
   m_punIOBuffer[2] = 0x01; // Generate an IRQ on wake up
   /* write command and check ack frame */
   if(!write_cmd_check_ack(e_port, m_punIOBuffer, 3)) {
      return false;
   }

   /* read the rest of the reply */
   read_dt(e_port, m_punIOBuffer, 8);
   /* verify that the recieved data was a reply frame to given command */
   if(m_punIOBuffer[NFC_FRAME_DIRECTION_INDEX] != PN532_PN532TOHOST ||
      m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::POWERDOWN)) {
      return false;
   }
   /* check the lower 6 bits of the status byte, error code 0x00 means success */
   if((m_punIOBuffer[NFC_FRAME_STATUS_INDEX] & 0x3F) != 0x00) {
      return false;
   }
   return true;
}

/***********************************************************/
/***********************************************************/

bool CNFCController::Probe(CPortController::EPort e_port) {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::GETFIRMWAREVERSION);
   /* write command and check ack frame */
   if(!write_cmd_check_ack(e_port, m_punIOBuffer, 1)) {
      return false;
   }
   /* read the rest of the reply */
   read_dt(e_port, m_punIOBuffer, 12);
   /* verify that the recieved data was a reply frame to given command */
   if(m_punIOBuffer[NFC_FRAME_DIRECTION_INDEX] != PN532_PN532TOHOST ||
      m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::GETFIRMWAREVERSION)) {
      return false;
   }
   /* check the firmware version */
   return (strncmp((char *)m_punIOBuffer, (char *)nfc_version, 6) == 0);
}

/*****************************************************************************/
/*!
	@brief  Configures the SAM (Secure Access Module)
	@param  mode - set mode, normal mode default
	@param  timeout - Details in NXP's PN532UM.pdf
	@param  irq - 0 unused (default), 1 used
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::ConfigureSAM(CPortController::EPort e_port, ESAMMode e_mode, uint8_t un_timeout, bool b_use_irq) {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::SAMCONFIGURATION);
    m_punIOBuffer[1] = static_cast<uint8_t>(e_mode);
    m_punIOBuffer[2] = un_timeout; // timeout = 50ms * un_timeout
    m_punIOBuffer[3] = b_use_irq?1u:0u; // use IRQ pin
    /* write command */
    if(!write_cmd_check_ack(e_port, m_punIOBuffer, 4)){
       return false;
    }
    /* read response */
    read_dt(e_port, m_punIOBuffer, 8);
    /* return whether the reply was as expected */
    return  (m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 == static_cast<uint8_t>(ECommand::SAMCONFIGURATION));
}


/*****************************************************************************/
/*!
	@brief  Configure PN532 as Initiator
	@param  NONE
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::P2PInitiatorInit(CPortController::EPort e_port) {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::INJUMPFORDEP);
    m_punIOBuffer[1] = 0x01; // avtive mode
    m_punIOBuffer[2] = 0x02; // 201Kbps
    m_punIOBuffer[3] = 0x01;

    m_punIOBuffer[4] = 0x00;
    m_punIOBuffer[5] = 0xFF;
    m_punIOBuffer[6] = 0xFF;
    m_punIOBuffer[7] = 0x00;
    m_punIOBuffer[8] = 0x00;

    if(!write_cmd_check_ack(e_port, m_punIOBuffer, 9)) {
       return false;
    }

    read_dt(e_port, m_punIOBuffer, 25);

    if(m_punIOBuffer[5] != PN532_PN532TOHOST) {
       //        Serial.println("InJumpForDEP sent read failed");
       return false;
    }

    if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::INJUMPFORDEP)) {
       return false;
    }
    if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
       return false;
    }

    return true;
}

/*****************************************************************************/
/*!
	@brief  Configure PN532 as Target.
	@param  NONE.
	@return 0 - failed
            1 - successfully
*/
/*****************************************************************************/
bool CNFCController::P2PTargetInit(CPortController::EPort e_port) {
    m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGINITASTARGET);
    /** 14443-4A Card only */
    m_punIOBuffer[1] = 0x00;
    /** SENS_RES */
    m_punIOBuffer[2] = 0x04;
    m_punIOBuffer[3] = 0x00;
    /** NFCID1 */
    m_punIOBuffer[4] = 0x12;
    m_punIOBuffer[5] = 0x34;
    m_punIOBuffer[6] = 0x56;
    /** SEL_RES - DEP only mode */
    m_punIOBuffer[7] = 0x40; 
    /**Parameters to build POL_RES (18 bytes including system code) */
    m_punIOBuffer[8] = 0x01;
    m_punIOBuffer[9] = 0xFE;
    m_punIOBuffer[10] = 0xA2;
    m_punIOBuffer[11] = 0xA3;
    m_punIOBuffer[12] = 0xA4;
    m_punIOBuffer[13] = 0xA5;
    m_punIOBuffer[14] = 0xA6;
    m_punIOBuffer[15] = 0xA7;
    m_punIOBuffer[16] = 0xC0;
    m_punIOBuffer[17] = 0xC1;
    m_punIOBuffer[18] = 0xC2;
    m_punIOBuffer[19] = 0xC3;
    m_punIOBuffer[20] = 0xC4;
    m_punIOBuffer[21] = 0xC5;
    m_punIOBuffer[22] = 0xC6;
    m_punIOBuffer[23] = 0xC7;
    m_punIOBuffer[24] = 0xFF;
    m_punIOBuffer[25] = 0xFF;
    /** NFCID3t */
    m_punIOBuffer[26] = 0xAA;
    m_punIOBuffer[27] = 0x99;
    m_punIOBuffer[28] = 0x88;
    m_punIOBuffer[29] = 0x77;
    m_punIOBuffer[30] = 0x66;
    m_punIOBuffer[31] = 0x55;
    m_punIOBuffer[32] = 0x44;
    m_punIOBuffer[33] = 0x33;
    m_punIOBuffer[34] = 0x22;
    m_punIOBuffer[35] = 0x11;
    /** Length of general bytes  */
    m_punIOBuffer[36] = 0x00;
    /** Length of historical bytes  */
    m_punIOBuffer[37] = 0x00;

    if(!write_cmd_check_ack(e_port, m_punIOBuffer, 38)) {
       return false;
    }
    read_dt(e_port, m_punIOBuffer, 24);

    if(m_punIOBuffer[5] != PN532_PN532TOHOST){
        return false;
    }

    if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGINITASTARGET)) {
        return false;
    }

    return true;
}

/*****************************************************************************/
/*!
	@brief  Initiator send and reciev data.
    @param  tx_buf --- data send buffer, user sets
            tx_len --- data send legth, user sets.
            rx_buf --- data recieve buffer, returned by P2PInitiatorTxRx
            rx_len --- data receive length, returned by P2PInitiatorTxRx
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/
uint8_t CNFCController::P2PInitiatorTxRx(CPortController::EPort e_port, 
										 uint8_t* pun_tx_buffer,
                                         uint8_t  un_tx_buffer_len,
                                         uint8_t* pun_rx_buffer,
                                         uint8_t  un_rx_buffer_len) {
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::INDATAEXCHANGE);
   m_punIOBuffer[1] = 0x01; // logical number of the relevant target

   /* transfer the tx data into the IO buffer as the command parameter */
   memcpy(m_punIOBuffer + 2, pun_tx_buffer, un_tx_buffer_len);

   if(!write_cmd_check_ack(e_port, m_punIOBuffer, un_tx_buffer_len + 2)){
      return 0;
   }

   read_dt(e_port, m_punIOBuffer, 60);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST){
      return 0;
   }

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::INDATAEXCHANGE)){
      return 0;
   }

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
      return 0;
   }

   /* return number of read bytes */
   uint8_t unRxDataLength = m_punIOBuffer[3] - 3;
   memcpy(pun_rx_buffer, m_punIOBuffer + 8, (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength);
   return (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength;
}

/*****************************************************************************/
/*!
	@brief  Target sends and recievs data.
    @param  tx_buf --- data send buffer, user sets
            tx_len --- data send legth, user sets.
            rx_buf --- data recieve buffer, returned by P2PInitiatorTxRx
            rx_len --- data receive length, returned by P2PInitiatorTxRx
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/

uint8_t CNFCController::P2PTargetTxRx(CPortController::EPort e_port, 
									  uint8_t* pun_tx_buffer, 
                                      uint8_t  un_tx_buffer_len,
                                      uint8_t* pun_rx_buffer,
                                      uint8_t  un_rx_buffer_len) {

   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGGETDATA);
   if(!write_cmd_check_ack(e_port, m_punIOBuffer, 1)){
      return 0;
   }
   read_dt(e_port, m_punIOBuffer, 60);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST){
      return 0;
   }

   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGGETDATA)) {
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
      return 0;
   }

   /* read data */
   uint8_t unRxDataLength = m_punIOBuffer[3] - 3;
   memcpy(pun_rx_buffer, m_punIOBuffer + 8, (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength);

   /* send reply */
   m_punIOBuffer[0] = static_cast<uint8_t>(ECommand::TGSETDATA);
   memcpy(m_punIOBuffer + 1, pun_tx_buffer, un_tx_buffer_len);

   if(!write_cmd_check_ack(e_port, m_punIOBuffer, un_tx_buffer_len + 1)) {
      return 0;
   }
   read_dt(e_port, m_punIOBuffer, 26);
   if(m_punIOBuffer[5] != PN532_PN532TOHOST) {
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX] - 1 != static_cast<uint8_t>(ECommand::TGSETDATA)) {
      return 0;
   }
   if(m_punIOBuffer[NFC_FRAME_ID_INDEX + 1]) {
      return 0;
   }
   /* return the number of bytes in the rx buffer (or maximum buffer capacity) */
   return (unRxDataLength > un_rx_buffer_len) ? un_rx_buffer_len : unRxDataLength;
}

/*****************************************************************************/
/*!
	@brief  send frame to PN532 and wait for ack
	@param  cmd - pointer to frame buffer
	@param  len - frame length
	@return 0 - send failed
            1 - send successfully
*/
/*****************************************************************************/


uint8_t CNFCController::write_cmd_check_ack(CPortController::EPort e_port, uint8_t *cmd, uint8_t len) {
    write_cmd(e_port, cmd, len);

    // read acknowledgement
    if (!read_ack(e_port)) {
       return false;
    }
    return true; // ack'd command
}


/*****************************************************************************/
/*!
	@brief  Send hexadecimal data through Serial with specified length.
	@param  buf - pointer of data buffer.
	@param  len - length need to send.
	@return NONE
*/
/*****************************************************************************/
void CNFCController::puthex(uint8_t data) {
	CHUARTController::instance().lock();
    fprintf(CHUARTController::instance().get_file(), "0x%02x ", data);
	CHUARTController::instance().unlock();
}


void CNFCController::puthex(uint8_t *buf, uint32_t len) {
    for(uint32_t i = 0; i < len; i++)
    {
    	CHUARTController::instance().lock();
        fprintf(CHUARTController::instance().get_file(), "0x%02x ", buf[i]);
		CHUARTController::instance().unlock();
    }
}

/*****************************************************************************/
/*!
	@brief  Write data frame to PN532.
	@param  cmd - Pointer of the data frame.
	@param  len - length need to write
	@return NONE
*/
/*****************************************************************************/
void CNFCController::write_cmd(CPortController::EPort e_port, uint8_t *cmd, uint8_t len)
{
    uint8_t checksum;

    len++;

    System::instance().sleep(2);    // or whatever the delay is for waking up the board

    // I2C START
    CPortController::instance().lock();
    CPortController::instance().SelectPort(e_port);
    CTWController::GetInstance().lock();
    CTWController::GetInstance().BeginTransmission(PN532_I2C_ADDRESS);
    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    CTWController::GetInstance().Write(PN532_PREAMBLE);
    CTWController::GetInstance().Write(PN532_PREAMBLE);
    CTWController::GetInstance().Write(PN532_STARTCODE2);

    CTWController::GetInstance().Write(len);
    CTWController::GetInstance().Write(~len + 1);

    CTWController::GetInstance().Write(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

    for (uint8_t i=0; i<len-1; i++)
    {
        if(CTWController::GetInstance().Write(cmd[i])){
            checksum += cmd[i];
        } else {
            i--;
            System::instance().sleep(1);
        }
    }

    CTWController::GetInstance().Write(~checksum);
    CTWController::GetInstance().Write(PN532_POSTAMBLE);

    // I2C STOP
    uint8_t err = CTWController::GetInstance().EndTransmission();
    CTWController::GetInstance().unlock();
    CPortController::instance().unlock();
    System::instance().sleep(100); 

}

/*****************************************************************************/
/*!
	@brief  Read data frame from PN532.
	@param  buf - pointer of data buffer
	@param  len - length need to read
	@return true if read was successful.
*/
/*****************************************************************************/
bool CNFCController::read_dt(CPortController::EPort e_port, uint8_t *buf, uint8_t len) {
   uint8_t unStatus = PN532_I2C_BUSY;
   // attempt to read response twenty times
   for(uint8_t i = 0; i < 25; i++) {
   
      // Start read (n+1 to take into account leading 0x01 with I2C)
      CPortController::instance().lock();
      CPortController::instance().SelectPort(e_port);
      
      CTWController::GetInstance().lock();
      CTWController::GetInstance().Read(PN532_I2C_ADDRESS, len + 2, true);
      // Read the status byte
      unStatus = CTWController::GetInstance().Read();
    
      if(unStatus == PN532_I2C_READY) {
      	 for(uint8_t i=0; i<len; i++) {
		    buf[i] = CTWController::GetInstance().Read();
		 }
		 CTWController::GetInstance().unlock(); //!
		 CPortController::instance().unlock();
         return true;
      }
      else {
         // flush the buffer
         for(uint8_t i=0; i<len; i++) {
            CTWController::GetInstance().Read();
         }
      }
      CTWController::GetInstance().unlock();
      CPortController::instance().unlock();
      
      System::instance().sleep(10);
   }

   // Discard trailing 0x00 0x00
   // receive();
   return (unStatus == PN532_I2C_READY);
}

/*****************************************************************************/
/*!
	@brief  read ack frame from PN532
	@param  NONE
	@return 0 - ack failed
            1 - Ack OK
*/
/*****************************************************************************/
bool CNFCController::read_ack(CPortController::EPort e_port)
{
   uint8_t ack_buf[6] = {0};

   read_dt(e_port, ack_buf, 6);

   //    puthex(ack_buf, 6);
   //    Serial.println();
   return (memcmp(ack_buf, ack, 6) == 0);
}
