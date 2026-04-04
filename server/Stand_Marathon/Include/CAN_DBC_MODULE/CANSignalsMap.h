#ifndef CANSIGNALMAP_H
#define CANSIGNALMAP_H

#define SIGNAL_MAP_SIZE 32

#ifdef __cplusplus
extern "C" {
#endif

typedef union
{
    struct
    {
        float* timeMs;
        float* IqRef;
        float* IqReg;
        float* IdRef;
        float* IdReg;
        float* Udc;
        float* Umod;
        float* Udz;
        float* Uqz;
        float* PidIq_ref;
        float* PidIq_fdb;
        float* PidIq_out;
        float* PidId_ref;
        float* PidId_fdb;
        float* PidId_out;
        float* PidFw_ref;
        float* PidFw_fdb;
        float* PidFw_out;
        float* ElSpeedRads;
        float* MeSpeedRads;
        float* Pel;
        float* PelFilt;
        float* Idc;
        float* IdcFilt;
        float* Tmotor;
        float* Tigbt;
        float* Tsink;
        float* MotorEMF;
        float* MotorFlusErr;
        float* MotorLd;
        float* MotorLq;
        float* MotorTrq;
    }Signals;
    float* Table[SIGNAL_MAP_SIZE];
}T_SIGNALS_MAP;

extern void InitSignalMap(T_SIGNALS_MAP* map);

extern T_SIGNALS_MAP SignalsMap;

#ifdef __cplusplus
}
#endif

#endif
