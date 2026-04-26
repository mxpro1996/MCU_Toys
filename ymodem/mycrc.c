#include "mycrc.h"
// crc多项式最高位总为1，可以省略（对CRC-16说是16位）
#define CRC16_XMODEM_POLY   0x1021
#define CRC16_XMODEM_TAB    crc16_xmodem_table
static uint16_t crc16_xmodem_table[256];
static _Bool isInit = 0;
/*
    总结此处的思路：
    | dats[0] |
              | dats[1] |
              |  CRC1H  |  CRC1L  |
                        |  CRC2H  |  CRC2L |
    首先CRC(0) = dats[0], 即第一个字节
    CRC(2) = 消去(CRC1H+dats[1])而生成的余数table[(crc>>8)^dats[1]]
            + 补全CRC1L这个没有使用的字节 (crc<<8)
        ==> crc(n+1) = (crc(n)<<8) ^ table[(crc(n)>>8)^dats[n]]
*/

static void CRC16_XModem_Init(uint16_t* dats){
    uint16_t crc;
    // 提前生成表查询
    for(uint16_t i=0;i<=0xff;i++){
        crc = (i<<8);
        for(uint8_t j=0;j<8;j++){
            if(crc&0x8000){
                crc = (crc<<1)^CRC16_XMODEM_POLY;
            }else{
                crc<<=1;
            }
        }
        dats[i] = crc;
    }
    //return crc;
}

uint16_t CRC16_Get(const uint8_t* dats, int len){
    uint16_t crc = 0;
    if(!isInit){
        CRC16_XModem_Init(CRC16_XMODEM_TAB);
        isInit = 1;
    }
    for(int i=0;i<len;i++){
        crc = CRC16_XMODEM_TAB[(crc>>8)^(dats[i])] ^ (crc<<8);
    }
    return crc;
}

uint16_t CRC16_Get_Legacy(const uint8_t* dats, int len){
    uint16_t crc = 0;
    for(int i=0 ;i<len;i++){
        // 补齐确定余数
        crc ^= (dats[i]<<8);
        // clz可以直接移位
        for(int j=0;j<8;j++){
            // crc舍弃首1位，注意做竖式除法
            if(crc&0x8000){
                crc = (crc<<1)^CRC16_XMODEM_POLY;
            }else{
                crc<<=1;
            }
            //printf("res: %04x\n",crc);
        }
        // 假设存在无限长整数，该crc可以通过偏移提取计算
    }
    return crc;
}

// int main(){
//     static uint8_t test[]={
//         "((crc >> 8) ^ (0xff & bytes[i]))"
//     };
//     CRC16_XModem_Init(CRC16_XMODEM_TAB);
//     printf("%04x\n",CRC_Get((uint8_t*)test,sizeof(test)-1));

//     return 0;
// }

