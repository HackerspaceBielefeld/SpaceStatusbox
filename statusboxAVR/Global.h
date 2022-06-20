/*
 * Global.h
 *
 */ 


#ifndef GLOBAL_H_
#define GLOBAL_H_

#define MHZCPU 16

// CPU-Takt Default 
#ifndef MHZCPU
	#warning "MHZCPU not set. Setting it to 8MHz"
	#define MHZCPU     8
#endif

// Timer Prescaler
#define TIMER1_PRESCALER  0x02

// Setzen der Timer auf einen 10ms Interrupt
#if MHZCPU == 16
	#define F_CPU             16000000UL
	#define TIMER1_COMPA_VAL  0x4E1F
#elif MHZCPU == 8
	#define F_CPU             8000000UL
	#define TIMER1_COMPA_VAL  0x270F
#else
	#error "Unknown MHZCPU"
#endif

#define UART_BAUD 9600UL

#define LED1 D,4
#define LED2 D,5
#define LED3 D,6
#define LED4 D,7
#define LED5 B,1

#define BTN1 C,0
#define BTN2 C,1
#define BTN3 C,2
#define BTN4 C,3
#define BTN5 B,0

#include <util/delay.h>

static inline void Delay_ms(uint8_t d){
	uint8_t i;
	for(i = 0; i < d; i++){
		_delay_ms(1);
	}
}

static inline void Delay_us(uint_fast16_t d){
	_delay_us(d);
}


#endif /* GLOBAL_H_ */