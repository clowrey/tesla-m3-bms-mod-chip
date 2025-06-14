#ifndef BATMAN_H
#define BATMAN_H

#include <stdint.h>
#include "Param.h"
#include "driver/spi_master.h"


// ESP32 SPI Configuration
#define BMB_SPI_HOST    SPI2_HOST
#define BMB_MISO        GPIO_NUM_34  // Adjust these pins based on your wiring
#define BMB_MOSI        GPIO_NUM_18
#define BMB_SCK         GPIO_NUM_5
#define BMB_CS          GPIO_NUM_32

// Helper function to reverse 16-bit value
static inline uint16_t rev16(uint16_t x) {
    return ((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8);
}

class BATMan {
public:
    BATMan();
    void BatStart();
    void loop();
    void StateMachine();
    void IdleWake();
    void GetData(uint8_t ReqID);
    void WriteCfg();
    void GetTempData();
    void WakeUP();
    void Generic_Send_Once(uint16_t Command[], uint8_t len);
    void upDateCellVolts();
    void upDateAuxVolts();
    void upDateTemps();
    uint8_t calcCRC(uint8_t *inData, uint8_t Length);
    void crc14_bytes(uint8_t len_B, uint8_t *bytes, uint16_t *crcP);
    void crc14_bits(uint8_t len_b, uint8_t inB, uint16_t *crcP);
    uint16_t spi_xfer(spi_host_device_t host, uint16_t data);
    bool checkSPIConnection();

private:
    spi_device_handle_t spi_dev;
    uint8_t ChipNum;
    uint16_t Voltage[8][15];
    uint16_t CellBalCmd[8];
    uint16_t Temps[8];
    uint16_t Temp1[8];
    uint16_t Temp2[8];
    uint16_t Volts5v[8];
    uint16_t ChipV[8];
    uint16_t Cfg[8][2];
    float CellVMax;
    float CellVMin;
    float TempMax;
    float TempMin;
    bool BalanceFlag;
    bool BmbTimeout;
    uint16_t LoopState;
    uint16_t LoopRanCnt;
    uint8_t WakeCnt;
    uint8_t WaitCnt;
    uint16_t IdleCnt;
    uint16_t SendDelay;
    uint32_t lasttime;
    bool BalEven;
    float Cell1start;
    float Cell2start;
};

#endif // BATMAN_H 