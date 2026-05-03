#define MAKE_INSTANCE
#include "ymodem.h"
#include "stdint.h"
#include "pthread.h"
#include "semaphore.h"

typedef struct {
    // waiting for
    uint32_t totalSize;
    uint32_t scatter_size;
}fw_infoBlock_t;

typedef struct {
    // waiting for
    uint8_t *fw_data;
    uint32_t scatter_no;
    uint32_t scatter_cnt;
    uint32_t scatter_lastSize;
}fw_staBlock_t;

typedef struct {
    fw_infoBlock_t manifest;
    fw_staBlock_t status;
}firmware_t;

// assume using FIFO, as physic-layer
uint8_t mock_cbuf[32], mock_len;
static sem_t tx_sem, rx_sem;

static const uint16_t tx_recv(YModem_Trans *self){
	uint16_t tmp = mock_len;
	memset(self->ymodem_rx, 0, typeToPackSize(self));
	memcpy(self->ymodem_rx, mock_cbuf, typeToPackSize(self));
	mock_len = 0;
	return tmp;
}

static void tx_send(YModem_Trans *self, const uint16_t len){
	memcpy(mock_cbuf, self->ymodem_tx, len);
	mock_len = len;
}

void fw_recvProc(firmware_t *fw){
	YModem_Trans curTrans;
	uint8_t nameBuf[32];
	
    while(1){
        sprintf(nameBuf,"block_%u",fw->status.scatter_no);
        puts(nameBuf);
        // fill
        if(fw->status.scatter_no==0)
	        YModem_InitTrans(&curTrans,nameBuf,(uint8_t*)&fw->manifest,sizeof(fw->manifest),YMODEM_SMX,tx_recv,tx_send);
        else if(fw->status.scatter_no==fw->status.scatter_cnt)
            YModem_InitTrans(&curTrans,nameBuf,fw->status.fw_data+ (fw->status.scatter_no-1) * fw->manifest.scatter_size, fw->status.scatter_lastSize,YMODEM_SMX,tx_recv,tx_send);
        else
            YModem_InitTrans(&curTrans,nameBuf,fw->status.fw_data+ (fw->status.scatter_no-1)* fw->manifest.scatter_size, fw->manifest.scatter_size,YMODEM_SMX,tx_recv,tx_send);

        // async
        while(!isTransFin(&curTrans)){
            sem_wait(&rx_sem);
            YModem_DeviceProc(&curTrans);
            sem_post(&tx_sem);
        }

        // onFinish
        if(fw->status.scatter_no==0){
            fw->status.scatter_lastSize = fw->manifest.totalSize % fw->manifest.scatter_size;
            fw->status.scatter_cnt = fw->manifest.totalSize / fw->manifest.scatter_size + (fw->status.scatter_lastSize!=0);
        }

        fw->status.scatter_no++;
        if(fw->status.scatter_no==fw->status.scatter_cnt+1)
            break;
    }
}


void fw_sendProc(firmware_t *fw){
	YModem_Trans curTrans;
	uint8_t nameBuf[32];
	
    while(1){
        sprintf(nameBuf,"block_%u",fw->status.scatter_no);

        // onBeforeSend
        if(fw->status.scatter_no==0){
            fw->status.scatter_lastSize = fw->manifest.totalSize % fw->manifest.scatter_size;
            fw->status.scatter_cnt = fw->manifest.totalSize / fw->manifest.scatter_size + (fw->status.scatter_lastSize!=0);
        }

        // fill
        if(fw->status.scatter_no==0)
	        YModem_InitTrans(&curTrans,nameBuf,(uint8_t*)&fw->manifest,sizeof(fw->manifest),YMODEM_SMX,tx_recv,tx_send);
        else if(fw->status.scatter_no==fw->status.scatter_cnt)
            YModem_InitTrans(&curTrans,nameBuf,fw->status.fw_data + (fw->status.scatter_no-1) * fw->manifest.scatter_size, fw->status.scatter_lastSize,YMODEM_SMX,tx_recv,tx_send);
        else
            YModem_InitTrans(&curTrans,nameBuf,fw->status.fw_data + (fw->status.scatter_no-1) * fw->manifest.scatter_size, fw->manifest.scatter_size,YMODEM_SMX,tx_recv,tx_send);

        // async
        while(!isTransFin(&curTrans)){
            sem_wait(&tx_sem);
            YModem_HostProc(&curTrans);
            sem_post(&rx_sem);
        }

        fw->status.scatter_no++;
        if(fw->status.scatter_no==fw->status.scatter_cnt+1)
            break;
    }
}

typedef void* (*pthread_fptr)(void *);
int main(){
    static firmware_t fw1,fw2;
    uint8_t mock_txFile[1024], mock_rxFile[1024];
    pthread_t tx_proc,rx_proc;

    sem_init(&rx_sem,0,1);
	sem_init(&tx_sem,0,0);
    
    fw1.status.fw_data = mock_txFile;
    fw2.status.fw_data = mock_rxFile;
    fw1.manifest.totalSize = fw2.manifest.totalSize = 1024;
    fw1.manifest.scatter_size = fw2.manifest.scatter_size = 128;

	pthread_create(&tx_proc,NULL,(pthread_fptr)fw_sendProc,&fw1);
	pthread_create(&rx_proc,NULL,(pthread_fptr)fw_recvProc,&fw2);

    // while (1){
    //     /* code */
    // }

    pthread_join(tx_proc,NULL);
    pthread_join(rx_proc,NULL);
    puts("end here");

    return 0;
}
