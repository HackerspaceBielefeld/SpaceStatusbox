#include <avr/io.h>
#include <avr/interrupt.h>

#include "EasyIO.h"
#include "Global.h"
#include "timedJob/timedJob.h"
#include "UART/UART.h"
#include "BTN/BTN.h"

uint8_t btns[5];
uint8_t blinkJobs[5] = {0xff, 0xff, 0xff, 0xff, 0xff};
	

void _btnPressShort(uint8_t btnid, uint8_t btneventid){
	uart_putc(0xbf);
	uart_putc(btnid);
	uart_putc(1);
}

void _btnPressMedium(uint8_t btnid, uint8_t btneventid){
	uart_putc(0xbf);
	uart_putc(btnid);
	uart_putc(5);
}

void _btnPressLong(uint8_t btnid, uint8_t btneventid){
	uart_putc(0xbf);
	uart_putc(btnid);
	uart_putc(10);
}

void _btnPressVeryLong(uint8_t btnid, uint8_t btneventid){
	uart_putc(0xbf);
	uart_putc(btnid);
	uart_putc(30);
}

void init(void){
	uart_init();
	
	uart_puts("xy");
	
	SPIN(BTN1);
	SOFF(BTN1);
	SPIN(BTN2);
	SOFF(BTN2);
	SPIN(BTN3);
	SOFF(BTN3);
	SPIN(BTN4);
	SOFF(BTN4);
	SPIN(BTN5);
	SOFF(BTN5);
	
	SPORT(LED1);
	SOFF(LED1);
	SPORT(LED2);
	SOFF(LED2);
	SPORT(LED3);
	SOFF(LED3);
	SPORT(LED4);
	SOFF(LED4);
	SPORT(LED5);
	SOFF(LED5);
	
	timedjob_init();
	btn_init();
	
	sei();
	
	btns[0] = btn_add(&DPORT(BTN1), &DPIN(BTN1), DPNUM(BTN1));
	btns[1] = btn_add(&DPORT(BTN2), &DPIN(BTN2), DPNUM(BTN2));
	btns[2] = btn_add(&DPORT(BTN3), &DPIN(BTN3), DPNUM(BTN3));
	btns[3] = btn_add(&DPORT(BTN4), &DPIN(BTN4), DPNUM(BTN4));
	btns[4] = btn_add(&DPORT(BTN5), &DPIN(BTN5), DPNUM(BTN5));
	
	for(uint8_t i = 0; i < 5; i++){
		btn_event_add(btns[i], BTN_EVENT_TIME_100MS, &_btnPressShort);
		btn_event_add(btns[i], BTN_EVENT_TIME_100MS * 5, &_btnPressMedium);
		btn_event_add(btns[i], BTN_EVENT_TIME_100MS * 10, &_btnPressLong);
		btn_event_add(btns[i], BTN_EVENT_TIME_100MS * 30, &_btnPressVeryLong);
	}
}


void _blinkCBLED1(uint8_t callcount){
	if(callcount % 2){ SON(LED1); } else { SOFF(LED1); }
}

void _blinkCBLED2(uint8_t callcount){
	if(callcount % 2){ SON(LED2); } else { SOFF(LED2); }
}

void _blinkCBLED3(uint8_t callcount){
	if(callcount % 2){ SON(LED3); } else { SOFF(LED3); }
}

void _blinkCBLED4(uint8_t callcount){
	if(callcount % 2){ SON(LED4); } else { SOFF(LED4); }
}

void _blinkCBLED5(uint8_t callcount){
	if(callcount % 2){ SON(LED5); } else { SOFF(LED5); }
}

int main(void)
{
    init();
    while (1) 
    {
		
		if(uart_data_ready()){
			uint8_t header = uart_getc_wait();
			if(header == 0xbf){
				uint8_t lednum = uart_getc_wait();
				uint8_t time = uart_getc_wait();
				
				uart_putc(lednum);
				uart_putc(time);
				
				if(lednum >= 5) {
					continue;
				}
				
				timedjob_remove(blinkJobs[lednum]);
				if(time == 0x00){
					switch(lednum){
						case 0: { SOFF(LED1); break;}
						case 1: { SOFF(LED2); break;}
						case 2: { SOFF(LED3); break;}
						case 3: { SOFF(LED4); break;}
						case 4: { SOFF(LED5); break;}
					}
				} else if(time == 0xFF){
					switch(lednum){
						case 0: { SON(LED1); break;}
						case 1: { SON(LED2); break;}
						case 2: { SON(LED3); break;}
						case 3: { SON(LED4); break;}
						case 4: { SON(LED5); break;}
					}
				} else {
					switch(lednum){
						case 0: {blinkJobs[lednum] = timedjob_add(TIMEDJOB_INFINITE, time * 10, TIMEDJOB_OPT_ACTIVE, &_blinkCBLED1); break;}
						case 1: {blinkJobs[lednum] = timedjob_add(TIMEDJOB_INFINITE, time * 10, TIMEDJOB_OPT_ACTIVE, &_blinkCBLED2); break;}
						case 2: {blinkJobs[lednum] = timedjob_add(TIMEDJOB_INFINITE, time * 10, TIMEDJOB_OPT_ACTIVE, &_blinkCBLED3); break;}
						case 3: {blinkJobs[lednum] = timedjob_add(TIMEDJOB_INFINITE, time * 10, TIMEDJOB_OPT_ACTIVE, &_blinkCBLED4); break;}
						case 4: {blinkJobs[lednum] = timedjob_add(TIMEDJOB_INFINITE, time * 10, TIMEDJOB_OPT_ACTIVE, &_blinkCBLED5); break;}
					}
				}
				
			}
		}
    
		btn_perform_actions();
	}
	
}

