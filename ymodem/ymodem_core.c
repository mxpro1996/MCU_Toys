#include "ymodem.h"

// seek by SOH,STX,SMX
const uint16_t unitTab[] = {
	0, 128, 1024, 27
};

void YModem_InitTrans(YModem_Trans *trans, const char *name, uint8_t *fptr, const uint32_t flen, uint8_t magicChar, recv_func_t recv, send_func_t send){
	trans->fw_base = fptr;
	trans->fw_size = flen;
	strncpy(trans->fw_name,name,MAX_FNAME_LEN);
	
	trans->expectNo = 0;
	trans->last_rx_jiffies = 0;
	trans->unitType = magicChar;
	trans->lastPackSize = flen % unitTab[magicChar];
	trans->packCnt = flen / unitTab[magicChar] + (trans->lastPackSize!=0);
	
	trans->recv = recv;
	trans->send = send;
}


// assume txlen>5
_Bool YModem_PreparePack(YModem_Trans *trans, uint8_t *payload, uint16_t plen){
	uint16_t unitSize = typeToPayLen(trans);
	uint16_t crc;
	uint8_t* const dptr = trans->ymodem_tx + 3;
	// const ptr
	
	if(plen>unitSize)
		return FALSE;
	
	// clear Buf
	memset(dptr,0,unitSize);
	// cur Header
	trans->ymodem_tx[0] = trans->unitType;
	trans->ymodem_tx[1] = trans->expectNo % 0xff;
	trans->ymodem_tx[2] = ~trans->ymodem_tx[1];
	// add payload as real-plen
	memcpy(dptr,payload,plen);
	
	// calc include the Unit-padding
	crc = CRC16_Get(dptr,unitSize);
	
	dptr[unitSize] = crc>>8;
	dptr[unitSize+1] = crc&0xff;
	
	return TRUE;
}
