#ifndef __YMODEM_H
#define __YMODEM_H

#include "main.h"
#include "mycrc.h"
#include "string.h"
#include "stdlib.h"
#include <time.h>

// LOGCAT
#define _DEBUG
#ifdef _DEBUG
	#define DEBUG_LOG(fmt, ...)		printf(fmt, ##__VA_ARGS__)
#else
	#define DEBUG_LOG(fmt, ...)
#endif

// CHARS
#define YMODEM_REQ  'C'
#define YMODEM_SOH  0x01
#define YMODEM_STX  0X02
#define YMODEM_SMX	0x03
#define YMODEM_EOT  0x04
#define YMODEM_ACK  0x06
#define YMODEM_NACK 0x15
#define YMODEM_CAN  0x18

// SMX/STX/SOH LABEL ~LABEL [PAY] CRCH CRCL
#define typeToPayLen(obj)	(unitTab[(obj)->unitType])
#define typeToPackSize(obj)	(unitTab[(obj)->unitType]+5)
#define SELF_CALL(obj, func, ...)	obj->func(obj, ##__VA_ARGS__)

// SESSION_ON
#define MAX_FNAME_LEN	16
#define	MAX_TURN_WAIT	460
#define YMODEM_FIFOSIZE	32
#define get_jiffies() (clock() * 1000 / CLOCKS_PER_SEC)

// enumerater
#define SPECIAL_ENUM_META \
	SPECIAL_ENUM(IDLE),	SPECIAL_ENUM(INFO), \
	SPECIAL_ENUM(DATA),	SPECIAL_ENUM(EOT1), \
	SPECIAL_ENUM(EOT2),	SPECIAL_ENUM(END) 
#define SPECIAL_ENUM_NAME	YMODEM_STA	
#include "special_enum.h"

struct YModem_Trans_t{
	enum CONCAT(enum_,SPECIAL_ENUM_NAME) ymodem_sta;
	
	char fw_name[MAX_FNAME_LEN+1];
	uint32_t fw_size;

	uint8_t *fw_base;
	// uint8_t *firmware_pos;
	uint32_t packCnt,expectNo;
	uint32_t lastPackSize;
	uint8_t rollNoOccur;
	
	uint32_t last_rx_jiffies;
	uint8_t unitType;	// SOH,STX,SMX
	uint8_t ymodem_rx[YMODEM_FIFOSIZE],ymodem_tx[YMODEM_FIFOSIZE];
	
	const uint16_t (*recv)(struct YModem_Trans_t *);
	void (*send)(struct YModem_Trans_t *, uint16_t);
};
typedef struct YModem_Trans_t YModem_Trans;

typedef const uint16_t (*recv_func_t)(YModem_Trans *self);
typedef void (*send_func_t)(YModem_Trans *self, uint16_t len);


extern const uint16_t unitTab[];

static inline _Bool isTransFin(YModem_Trans *session){
	return session->expectNo==session->packCnt+1;
}
// core-function
void YModem_InitTrans(YModem_Trans *trans, const char *name, uint8_t *fptr, const uint32_t flen, uint8_t magicChar, recv_func_t recv, send_func_t send);
_Bool YModem_PreparePack(YModem_Trans *trans, uint8_t *payload, uint16_t plen);
void YModem_HostProc(YModem_Trans *trans);
void YModem_DeviceProc(YModem_Trans *trans);
#endif
