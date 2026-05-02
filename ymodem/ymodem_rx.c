#include "ymodem.h"

// powered by 
void YModem_DeviceProc(YModem_Trans *trans){
	static _Bool wakeup_tx1;
	if(!trans->recv || !trans->send){
		return;
	}

	// RX
	if(SELF_CALL(trans, recv) > 0){
		wakeup_tx1 = 1;
		DEBUG_LOG("RX: R %02x\n", trans->ymodem_rx[0]);
		switch(trans->ymodem_rx[0]){
			case YMODEM_SOH: case YMODEM_STX: case YMODEM_SMX:{
				char* const dptr = trans->ymodem_rx+3;			
				uint16_t crc16 = CRC16_Get(dptr,typeToPayLen(trans));
				crc16 = crc16>>8 | (crc16&0xff)<<8;
				// little to Big

				// CRC16
				trans->rollNoOccur = TRUE;
				trans->rollNoOccur &= (trans->ymodem_rx[1] == trans->expectNo%0xff);
				trans->rollNoOccur &= (memcmp(&trans->ymodem_rx[typeToPackSize(trans)-2],&crc16,2)==0);
				if( trans->rollNoOccur ){	// label-no
					// re-trans occurred
					
					trans->ymodem_sta = DATA;
					if(trans->expectNo==0){	// INFO
						uint32_t dsize = strlen(dptr);
						strncpy(trans->fw_name,dptr,16);
						trans->fw_size = atoi(dptr+dsize+1);
						
						trans->lastPackSize = trans->fw_size % typeToPayLen(trans);
						trans->packCnt = trans->fw_size / typeToPayLen(trans) + (trans->lastPackSize!=0);
					}else{
						if(trans->expectNo  == trans->packCnt && trans->lastPackSize!=0){
							// THE LAST pack
							//puts("rx: last packet");
							memcpy(trans->fw_base + (trans->expectNo-1) * typeToPayLen(trans) , dptr, trans->lastPackSize);
						}else{
							memcpy(trans->fw_base + (trans->expectNo-1) * typeToPayLen(trans) , dptr, typeToPayLen(trans));
						}


						DEBUG_LOG("RX: ");
						for(int i=0;i<typeToPackSize(trans);i++)
							DEBUG_LOG("%02x ",trans->ymodem_rx[i]);
						DEBUG_LOG("\n");
						
					}
					trans->expectNo++;
				}
			}break;
			
			case YMODEM_EOT:{
				if(trans->ymodem_sta == DATA || trans->ymodem_sta == EOT1){
					trans->ymodem_sta++;
				}
			}break;
			
			default:{
				wakeup_tx1 = 0;
			}break;
		}
		
		trans->last_rx_jiffies = get_jiffies();
	}else {
		if(get_jiffies() - trans->last_rx_jiffies > MAX_TURN_WAIT ){
			wakeup_tx1 = 1;
			trans->last_rx_jiffies = get_jiffies();
			DEBUG_LOG("DEVICE: Forced woken\n");
		}
		// crack deadLock
	}
	
	
	
	if(wakeup_tx1){
		wakeup_tx1 = 0;
		
		// TX and repeat try, RX&TX not async?
		switch(trans->ymodem_sta){
			case IDLE: {
				trans->ymodem_tx[0] = YMODEM_REQ;
				SELF_CALL(trans, send, 1);
				// sEND REQUEST
			}break;
			
//			case INFO: {
//				trans->ymodem_tx[0] = YMODEM_REQ;
//				SELF_CALL(trans, send, 1);
//				// sEND REQUEST
//			}break;
			

			case DATA: {
				trans->ymodem_tx[0] = trans->rollNoOccur?YMODEM_ACK:YMODEM_NACK;
				SELF_CALL(trans, send, 1);
				
				if(trans->expectNo == 1){
					// soft-bootlin
					trans->ymodem_sta = IDLE;
				}
			}break;

			case EOT1: {
				trans->ymodem_tx[0] = YMODEM_NACK;
				SELF_CALL(trans, send, 1);
			}break;	
			
			case EOT2:{
				trans->ymodem_tx[0] = YMODEM_ACK;
				SELF_CALL(trans, send, 1);
				// puts("called EOT2 retreen");
				trans->ymodem_sta ++;
			}break;
			
			default:{
				// EMPLACE Sleep
			}break;
				
		}
	}
}
