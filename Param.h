#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>

class Param {
public:
    enum PARAM_NUM {
        numbmbs,
        LoopCnt,
        LoopState,
        CellMax,
        CellMin,
        CellsPresent,
        CellsBalancing,
        Chip1_5V,
        Chip2_5V,
        u1,  // First cell voltage
        u2,  // Second cell voltage
        umax,
        umin,
        deltaV,
        udc,
        uavg,
        chargeVlim,
        dischargeVlim,
        balance,
        CellVmax,
        CellVmin,
        Chipt0,
        Cellt0_0,
        Cellt0_1,
        TempMax,
        TempMin,
        ChipV1,
        ChipV2,
        ChipV3,
        ChipV4,
        ChipV5,
        ChipV6,
        ChipV7,
        ChipV8
    };

    static int GetInt(PARAM_NUM param);
    static void SetInt(PARAM_NUM param, int value);
    static float GetFloat(PARAM_NUM param);
    static void SetFloat(PARAM_NUM param, float value);
};

#endif // PARAM_H 