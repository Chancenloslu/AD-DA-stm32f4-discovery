#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdint.h>
#include "stm32f429i_discovery_lcd.h"
#include <stdio.h>

void Display_Init(void);
void Display(uint32_t uwADC1ConvertedVoltage);
void Axis_init(void);
void Vertex_record(uint32_t ADCVoltageConvertedValue);

#endif /* __DISPLAY_H */