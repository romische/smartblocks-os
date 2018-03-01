
#include <stdint.h>
#include <string.h>
#include "tw_controller.h" 
#include "system.h" //for sleep(int) in write_cmd and read_dt
					//!!\ this implies that CPortController can only be used in the multitasking context
#include "huart_controller.h" //for printing in puthex
#include "port_controller.h" 

#ifndef NFC_CONTROLLER_H
#define NFC_CONTROLLER_H

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_I2C_ADDRESS                   (0x48 >> 1)
#define PN532_I2C_READBIT                   (0x01)
#define PN532_I2C_BUSY                      (0x00)
#define PN532_I2C_READY                     (0x01)
#define PN532_I2C_READYTIMEOUT              (20)

enum class ESAMMode : uint8_t {
   NORMAL       = 0x01,
   VIRTUAL_CARD = 0x02,
   WIRED_CARD   = 0x03,
   DUAL_CARD    = 0x04
};

#define NFC_CMD_BUF_LEN                     64

#define NFC_FRAME_DIRECTION_INDEX           5
#define NFC_FRAME_ID_INDEX                  6
#define NFC_FRAME_STATUS_INDEX              7

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

class CNFCController {

public:
   enum class ECommand : uint8_t {
      DIAGNOSE              = 0x00,
      GETFIRMWAREVERSION    = 0x02,
      GETGENERALSTATUS      = 0x04,
      READREGISTER          = 0x06,
      WRITEREGISTER         = 0x08,
      READGPIO              = 0x0C,
      WRITEGPIO             = 0x0E,
      SETSERIALBAUDRATE     = 0x10,
      SETPARAMETERS         = 0x12,
      SAMCONFIGURATION      = 0x14,
      POWERDOWN             = 0x16,
      RFCONFIGURATION       = 0x32,
      RFREGULATIONTEST      = 0x58,
      INJUMPFORDEP          = 0x56,
      INJUMPFORPSL          = 0x46,
      INLISTPASSIVETARGET   = 0x4A,
      INATR                 = 0x50,
      INPSL                 = 0x4E,
      INDATAEXCHANGE        = 0x40,
      INCOMMUNICATETHRU     = 0x42,
      INDESELECT            = 0x44,
      INRELEASE             = 0x52,
      INSELECT              = 0x54,
      INAUTOPOLL            = 0x60,
      TGINITASTARGET        = 0x8C,
      TGSETGENERALBYTES     = 0x92,
      TGGETDATA             = 0x86,
      TGSETDATA             = 0x8E,
      TGSETMETADATA         = 0x94,
      TGGETINITIATORCOMMAND = 0x88,
      TGRESPONSETOINITIATOR = 0x90,
      TGGETTARGETSTATUS     = 0x8A
   };


   CNFCController() {}  //remove and make everything static as in led_controller?
   
   bool Init(CPortController::EPort e_port);
   
   bool Send(CPortController::EPort e_port, uint8_t* msg, uint8_t  len);
   
   bool Receive(CPortController::EPort e_port, uint8_t* msg, uint8_t len);

   bool Probe(CPortController::EPort e_port);

   bool ConfigureSAM(CPortController::EPort e_port, ESAMMode e_mode = ESAMMode::NORMAL, uint8_t un_timeout = 20, bool b_use_irq = false);

   bool P2PInitiatorInit(CPortController::EPort e_port);

   bool P2PTargetInit(CPortController::EPort e_port);

   uint8_t P2PInitiatorTxRx(CPortController::EPort e_port, 
   							uint8_t* pun_tx_buffer, 
                            uint8_t  un_tx_buffer_len, 
                            uint8_t* pun_rx_buffer,
                            uint8_t  un_rx_buffer_len);

   uint8_t P2PTargetTxRx(CPortController::EPort e_port, 
   						 uint8_t* pun_tx_buffer, 
                         uint8_t  un_tx_buffer_len, 
                         uint8_t* pun_rx_buffer,
                         uint8_t  un_rx_buffer_len);

   bool PowerDown(CPortController::EPort e_port);
   
private:
   void write_cmd(CPortController::EPort e_port, uint8_t *cmd, uint8_t len);
   uint8_t write_cmd_check_ack(CPortController::EPort e_port, uint8_t *cmd, uint8_t len);
   bool read_dt(CPortController::EPort e_port, uint8_t *buf, uint8_t len);
   bool read_ack(CPortController::EPort e_port);

   void puthex(uint8_t data);
   void puthex(uint8_t *buf, uint32_t len);

   /* data buffer for reading / writing commands */
   uint8_t m_punIOBuffer[NFC_CMD_BUF_LEN];

};

#endif
