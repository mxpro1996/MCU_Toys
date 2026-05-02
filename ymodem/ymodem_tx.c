#include "ymodem.h"

void YModem_HostProc(YModem_Trans *trans){
	static uint8_t nackSemaphor = 0;
	static _Bool wakeup_tx;
	if(!trans->recv || !trans->send){
		return;
	}

	// RX
	if(SELF_CALL(trans, recv) > 0){
		wakeup_tx = 1;
		DEBUG_LOG("TX: R %02x\n", trans->ymodem_rx[0]);
		switch(trans->ymodem_rx[0]){
			case YMODEM_REQ:{
				if(trans->ymodem_sta == IDLE){
					if(trans->expectNo == 1){
						trans->ymodem_sta = DATA;				
					}else{
						trans->ymodem_sta = INFO;
					}
				}
				// switch to 
			}break;
			
			case YMODEM_ACK:{
				if(trans->ymodem_sta == EOT2){
					trans->ymodem_sta++;				
					break;
				}
				// END of here
				
				trans->expectNo++;
				if(trans->ymodem_sta == DATA && trans->expectNo == trans->packCnt+1){
					trans->ymodem_sta = EOT1;
				}else if(trans->ymodem_sta == INFO){
					trans->ymodem_sta = IDLE;				
				}
			}break;
			
			case YMODEM_NACK:{
				nackSemaphor++;
				if(trans->ymodem_sta == EOT1){
					trans->ymodem_sta = EOT2;
				}else if(trans->ymodem_sta <= DATA && nackSemaphor>10){
					nackSemaphor = 0;
					trans->ymodem_sta = IDLE;
				}
			}break;
			
			case YMODEM_CAN:{
				nackSemaphor ++;
			}break;				
			
			default:{
				wakeup_tx = 0;
			}break;
		}

		trans->last_rx_jiffies = get_jiffies();
	}else if(get_jiffies() - trans->last_rx_jiffies > MAX_TURN_WAIT && trans->ymodem_sta!=IDLE ){
			wakeup_tx = 1;
			trans->last_rx_jiffies = get_jiffies();
			DEBUG_LOG("Host: Forced woken\n");
		// crack deadLock
	}
	

	if(wakeup_tx){
		wakeup_tx = 0;
		
		// TX and repeat try, RX&TX not async?
		switch(trans->ymodem_sta){
			case IDLE:{
			}break;
			
			case INFO:{
				char infoLoad[32];
				uint8_t sizeLen;
				sizeLen = snprintf(infoLoad,sizeof(infoLoad),"%.16s%c%u", trans->fw_name,'\0',trans->fw_size);
				
				// ASSUME two NUL '\0'
				YModem_PreparePack(trans,infoLoad,sizeLen+1);
				SELF_CALL(trans, send, typeToPackSize(trans));
			}break;
			
			case DATA:{
				if(trans->expectNo == trans->packCnt && trans->lastPackSize!=0){
					// THE LAST pack, not include info-0
					// puts("tx: last packet");
					YModem_PreparePack(trans, trans->fw_base + (trans->expectNo-1) * typeToPayLen(trans) , trans->lastPackSize);
				}else{
					YModem_PreparePack(trans, trans->fw_base + (trans->expectNo-1) * typeToPayLen(trans) , typeToPayLen(trans));
				}

				DEBUG_LOG("TX: ");
				for(int i=0;i<typeToPackSize(trans);i++)
					DEBUG_LOG("%02x ",trans->ymodem_tx[i]);
				DEBUG_LOG("\n");				

				SELF_CALL(trans, send, typeToPackSize(trans));
				// 27 or 1
			}break;

			case EOT1: case EOT2:{
				trans->ymodem_tx[0] = YMODEM_EOT;
				SELF_CALL(trans, send, 1);
			}break;	
			
			default:{
			}break;
		}
	}
}
