#define MAKE_INSTANCE
#include "ymodem.h"

// for MCU mainloop, we can use commin_buf shared_ptr
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


int main(){	
	static YModem_Trans rx_session, tx_session;
	static uint8_t rx_mockBuf[1024], tx_mockBuf[1024];
	
	for(int i=0;i<1024;i++)
	 	tx_mockBuf[i] = i;
	YModem_InitTrans(&tx_session,"real",tx_mockBuf,512,YMODEM_SMX,tx_recv,tx_send);
	YModem_InitTrans(&rx_session,"dummy",rx_mockBuf,512,YMODEM_SMX,tx_recv,tx_send);

	while(1){
			YModem_HostProc(&tx_session);
			YModem_DeviceProc(&rx_session);

			if(rx_session.expectNo==rx_session.packCnt+1 && rx_session.expectNo==tx_session.expectNo){
				// for(int i=0;i<tx_session.fw_size;i++)
 				// 	printf("%d=%hhu %hhu\n",i,tx_mockBuf[i],rx_mockBuf[i]);
				puts("transfer end");
				break;
			}
	}	

	return 0;
}
