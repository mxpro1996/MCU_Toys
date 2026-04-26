#define 	MAKE_INSTANCE
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


#define EMU_LOOPBACK
#ifdef EMU_LOOPBACK
#include "unistd.h"
// emulate the communicate Pipe
uint8_t mock_cbuf[32], mock_len;
static const uint16_t tx_recv(struct YModem_Trans_t *self){
	uint16_t tmp = mock_len;
	memset(self->ymodem_rx, 0, typeToPackSize(self));
	memcpy(self->ymodem_rx, mock_cbuf, typeToPackSize(self));
	mock_len = 0;
	return tmp;
}

static void tx_send(struct YModem_Trans_t *self, const uint16_t len){
	memcpy(mock_cbuf, self->ymodem_tx, len);
	mock_len = len;
}

//void test_Ymodem(){
int main(){	
	static YModem_Trans rx_session, tx_session;
	static uint8_t rx_mockBuf[1024], tx_mockBuf[1024];
	
	for(int i=0;i<1024;i++)
	 	tx_mockBuf[i] = i;
	YModem_InitTrans(&tx_session,"real",tx_mockBuf,64,YMODEM_SMX,tx_recv,tx_send);
	YModem_InitTrans(&rx_session,"dummy",rx_mockBuf,64,YMODEM_SMX,tx_recv,tx_send);
	while(1){
			YModem_HostProc(&tx_session);
			YModem_DeviceProc(&rx_session);
			if(rx_session.expectNo==rx_session.packCnt+1 && rx_session.expectNo==tx_session.expectNo){
				// for(int i=0;i<tx_session.fw_size;i++)
 				// 	printf("%d=%hhu %hhu\n",i,tx_mockBuf[i],rx_mockBuf[i]);
				puts("transfer end");
				break;
			}
			usleep(50*1000);
	}	

	return 0;
}
#endif