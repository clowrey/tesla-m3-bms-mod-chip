#ifndef BATMAN_H
#define BATMAN_H

#include <stdint.h>
#include "Param.h"
#include "driver/spi_master.h"


// ESP32 SPI Configuration
// Adjust these pins based on your wiring
#define BMB_SPI_HOST    SPI2_HOST    // Use HSPI controller - LCD disabled
#define BMB_ENABLE      GPIO_NUM_21
#define BMB_MISO        GPIO_NUM_17  // Keep original BMB pins
#define BMB_MOSI        GPIO_NUM_2   // Keep original BMB pins
#define BMB_SCK         GPIO_NUM_15  // Keep original BMB pins
#define BMB_CS          GPIO_NUM_22  // Keep original BMB pins


/* Tesla HVC Batman Debug Header Pinout

#1 - SCK  (Square pin)
#2 - MOSI
#3 - MISO
#4 - CS 
#5 - Batman Enable
#6 - GND

*/
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
    void updateIndividualCellVoltageParameters(void);  // Update individual cell voltage parameters during all phases
    uint8_t calcCRC(uint8_t *inData, uint8_t Length);
    void crc14_bytes(uint8_t len_B, uint8_t *bytes, uint16_t *crcP);
    void crc14_bits(uint8_t len_b, uint8_t inB, uint16_t *crcP);
    uint16_t spi_xfer(spi_host_device_t host, uint16_t data);
    bool checkSPIConnection();

    // New getter methods for display
    float getMinVoltage() const { return CellVMin; }
    float getMaxVoltage() const { return CellVMax; }
    
    // Helper method to get hardware register position for a cell
    struct CellPosition {
        int chip;
        int register_pos;
        bool valid;
    };
    
    CellPosition getCellHardwarePosition(int sequentialCellNum) const {
        CellPosition pos = {0, 0, false};
        int cellCount = 0;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 15; j++) {
                if (Voltage[i][j] > 10) { // Cell is present
                    cellCount++;
                    if (cellCount == sequentialCellNum) {
                        pos.chip = i;
                        pos.register_pos = j;
                        pos.valid = true;
                        return pos;
                    }
                }
            }
        }
        return pos;
    }
    
    // Get sequential cell number from hardware position
    int getSequentialCellNumber(int chip, int register_pos) const {
        int cellCount = 0;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 15; j++) {
                if (Voltage[i][j] > 10) { // Cell is present
                    cellCount++;
                    if (i == chip && j == register_pos) {
                        return cellCount;
                    }
                }
            }
        }
        return 0;
    }
    
    // Debug method to print hardware register mapping
    void printHardwareMapping() const;
    
    // Add debug control for detailed register analysis
    static void setRegisterDebug(bool enable) { _registerDebugEnabled = enable; }
    static bool getRegisterDebug() { return _registerDebugEnabled; }

    int getMinCell() const {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 15; j++) {
                if (Voltage[i][j] == CellVMin) {
                    return getSequentialCellNumber(i, j);
                }
            }
        }
        return 0;
    }
    int getMaxCell() const {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 15; j++) {
                if (Voltage[i][j] == CellVMax) {
                    return getSequentialCellNumber(i, j);
                }
            }
        }
        return 0;
    }

    // Get balancing information for display
    struct BalancingInfo {
        int totalCells;
        int balancingCells;
        int balancingCellNumbers[108]; // Array to store cell numbers being balanced
    };
    
    BalancingInfo getBalancingInfo() const;
    
    // Get voltage from specific chip and register position
    uint16_t getVoltage(int chip, int register_pos) const {
        if (chip >= 0 && chip < 8 && register_pos >= 0 && register_pos < 15) {
            return Voltage[chip][register_pos];
        }
        return 0;
    }

private:
    spi_device_handle_t spi_dev;
    static bool _registerDebugEnabled;  // Debug flag for detailed register output
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
    uint8_t BalancePhase;  // 0=measurement only (no balancing), 1=even cells, 2=odd cells
    uint16_t LastCellBalancing;  // Preserve balancing count across phases
    float Cell1start;
    float Cell2start;
};

#endif // BATMAN_H 