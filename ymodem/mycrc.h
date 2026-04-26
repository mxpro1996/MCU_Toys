#ifndef __MYCRC_H
#define __MYCRC_H

#include "main.h"

static void CRC16_XModem_Init(uint16_t* dats);
uint16_t CRC16_Get(const uint8_t* dats, int len);
uint16_t CRC16_Get_Legacy(const uint8_t* dats, int len);

#endif
