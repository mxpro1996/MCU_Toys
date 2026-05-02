#define MAKE_INSTANCE
#include "ymodem.h"
#include "unistd.h"
// emulate the communicate Pipe

#include "pthread.h"	

static pthread_t tx_proc, rx_proc;
typedef struct {
	uint8_t buf[1024];
	uint32_t len;
	pthread_mutex_t lock;
	/* data */
}rxfifo_t;
rxfifo_t tfifo,rfifo;

static const uint16_t tx_recv(struct YModem_Trans_t *self){
	uint16_t tmp;
	
	pthread_mutex_lock(&tfifo.lock);
	{
		tmp = tfifo.len;
		memset(self->ymodem_rx, 0, typeToPackSize(self));
		memcpy(self->ymodem_rx, tfifo.buf, typeToPackSize(self));
		tfifo.len = 0;
	}
	pthread_mutex_unlock(&tfifo.lock);

	return tmp;
}

static void tx_send(struct YModem_Trans_t *self, const uint16_t len){
	pthread_mutex_lock(&rfifo.lock);
	{
		memcpy(rfifo.buf, self->ymodem_tx, len);
		rfifo.len = len;
	}
	pthread_mutex_unlock(&rfifo.lock);
}


static const uint16_t rx_recv(struct YModem_Trans_t *self){
	uint16_t tmp;
	
	pthread_mutex_lock(&rfifo.lock);
	{
		tmp = rfifo.len;
		memset(self->ymodem_rx, 0, typeToPackSize(self));
		memcpy(self->ymodem_rx, rfifo.buf, typeToPackSize(self));
		rfifo.len = 0;
	}
	pthread_mutex_unlock(&rfifo.lock);

	return tmp;
}

static void rx_send(struct YModem_Trans_t *self, const uint16_t len){
	pthread_mutex_lock(&tfifo.lock);
	{
		memcpy(tfifo.buf, self->ymodem_tx, len);
		tfifo.len = len;
	}
	pthread_mutex_unlock(&tfifo.lock);
}


void *YModem_HostProc_wrapper(void* args){
	while(1){
		YModem_HostProc(args);
	}
}
void *YModem_DeviceProc_wrapper(void* args){
	while(1){
		YModem_DeviceProc(args);
	}
}

//void test_Ymodem(){
int main(){	
	static YModem_Trans rx_session, tx_session;
	static uint8_t rx_mockBuf[1024], tx_mockBuf[1024];
	
	for(int i=0;i<1024;i++)
	 	tx_mockBuf[i] = i;
	YModem_InitTrans(&tx_session,"real",tx_mockBuf,128,YMODEM_SMX,tx_recv,tx_send);
	YModem_InitTrans(&rx_session,"real",rx_mockBuf,128,YMODEM_SMX,rx_recv,rx_send);

	pthread_mutex_init(&tfifo.lock,NULL);
	pthread_mutex_init(&rfifo.lock,NULL);

	pthread_create(&tx_proc,NULL,YModem_HostProc_wrapper,&tx_session);
	pthread_create(&rx_proc,NULL,YModem_DeviceProc_wrapper,&rx_session);

	while(1){
			if(isTransFin(&rx_session)){
				// for(int i=0;i<tx_session.fw_size;i++)
 				// 	printf("%d=%hhu %hhu\n",i,tx_mockBuf[i],rx_mockBuf[i]);
				puts("transfer end");
				break;
			}
	}	

	return 0;
}
