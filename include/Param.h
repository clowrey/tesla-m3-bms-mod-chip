#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>
#include <Arduino.h>

class Param {
public:
    enum PARAM_NUM {
        // System parameters
        numbmbs,
        LoopCnt,
        LoopState,
        CellsPresent,
        CellsBalancing,
        
        // Cell voltage parameters (must be consecutive)
        u1, u2, u3, u4, u5, u6, u7, u8, u9, u10,
        u11, u12, u13, u14, u15, u16, u17, u18, u19, u20,
        u21, u22, u23, u24, u25, u26, u27, u28, u29, u30,
        u31, u32, u33, u34, u35, u36, u37, u38, u39, u40,
        u41, u42, u43, u44, u45, u46, u47, u48, u49, u50,
        u51, u52, u53, u54, u55, u56, u57, u58, u59, u60,
        u61, u62, u63, u64, u65, u66, u67, u68, u69, u70,
        u71, u72, u73, u74, u75, u76, u77, u78, u79, u80,
        u81, u82, u83, u84, u85, u86, u87, u88, u89, u90,
        u91, u92, u93, u94, u95, u96, u97, u98, u99, u100,
        u101, u102, u103, u104, u105, u106, u107, u108,
        
        // Voltage statistics
        CellMax,
        CellMin,
        umax,
        umin,
        deltaV,
        udc,
        uavg,
        chargeVlim,
        dischargeVlim,
        
        // Balance control
        balance,
        CellVmax,
        CellVmin,
        
        // Temperature parameters
        Chipt0,
        Cellt0_0,
        Cellt0_1,
        TempMax,
        TempMin,
        
        // Chip voltages
        ChipV1,
        ChipV2,
        ChipV3,
        ChipV4,
        ChipV5,
        ChipV6,
        ChipV7,
        ChipV8,
        
        // Chip supplies
        Chip1_5V,
        Chip2_5V,
        
        // Cell counts per chip
        Chip1Cells,
        Chip2Cells,
        Chip3Cells,
        Chip4Cells
    };

    static int GetInt(PARAM_NUM param);
    static void SetInt(PARAM_NUM param, int value);
    static float GetFloat(PARAM_NUM param);
    static void SetFloat(PARAM_NUM param, float value);
    
    // Serial API methods
    static const char* GetParamName(PARAM_NUM param);
    static PARAM_NUM GetParamFromName(const char* name);
    static void PrintAllParams();
    static void PrintParam(PARAM_NUM param);
    static bool SetParamFromString(const char* name, const char* value);
    static void PrintParamHelp();
    
    // Overloaded methods for specific serial ports
    static void PrintAllParams(HardwareSerial& serialPort);
    static void PrintParam(PARAM_NUM param, HardwareSerial& serialPort);
    static bool SetParamFromString(const char* name, const char* value, HardwareSerial& serialPort);
    static void PrintParamHelp(HardwareSerial& serialPort);
};

#endif // PARAM_H 