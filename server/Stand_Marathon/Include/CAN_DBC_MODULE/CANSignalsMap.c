#include "CANSignalsMap.h"

T_SIGNALS_MAP SignalsMap;

float MapDefaultVariable = 0;

void InitSignalMap(T_SIGNALS_MAP* map)
{
    int i;
    for (i = 0; i < SIGNAL_MAP_SIZE; i++)
    {
        map->Table[i] = &MapDefaultVariable;
    }
}