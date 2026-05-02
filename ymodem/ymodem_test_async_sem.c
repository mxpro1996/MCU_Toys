#define MAKE_INSTANCE
#include "ymodem.h"
#include "unistd.h"
// emulate the communicate Pipe
#include "pthread.h"	
#include "semaphore.h"

static pthread_t tx_proc, rx_proc;
static sem_t tx_sem, rx_sem;
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

// Ping-Pong
void *YModem_HostProc_wrapper(void* args){
	while(1){
		sem_wait(&tx_sem);
		YModem_HostProc(args);
		sem_post(&rx_sem);
	}
}
void *YModem_DeviceProc_wrapper(void* args){
	while(1){
		sem_wait(&rx_sem);
		YModem_DeviceProc(args);
		sem_post(&tx_sem);
	}
}

int main(){	
	static YModem_Trans rx_session, tx_session;
	static uint8_t rx_mockBuf[1024], tx_mockBuf[1024];
	
	for(int i=0;i<1024;i++)
	 	tx_mockBuf[i] = i;
	YModem_InitTrans(&tx_session,"real",tx_mockBuf,128,YMODEM_SMX,tx_recv,tx_send);
	YModem_InitTrans(&rx_session,"dummy",rx_mockBuf,128,YMODEM_SMX,tx_recv,tx_send);

	// Ping-Pong
	sem_init(&rx_sem,0,1);
	sem_init(&tx_sem,0,0);

	pthread_create(&tx_proc,NULL,YModem_HostProc_wrapper,&tx_session);
	pthread_create(&rx_proc,NULL,YModem_DeviceProc_wrapper,&rx_session);

	while(1){
		if(rx_session.expectNo==rx_session.packCnt+1 && rx_session.expectNo==tx_session.expectNo){
			// for(int i=0;i<tx_session.fw_size;i++)
			// 	printf("%d=%hhu %hhu\n",i,tx_mockBuf[i],rx_mockBuf[i]);
			puts("transfer end");
			break;
		}
	}	

	return 0;
}