/*
 * BTN.h
 *
 * Created: 25.11.2018 15:40:03
 *  Author: Bjoern
 */ 


#ifndef BTN_H_
#define BTN_H_

#define BTN_MAX_ENTRIES 16
#define BTN_EVENT_MAX_ENTRIES 32

#define BTN_FLAG_INUSE   0
#define BTN_FLAG_LOCKED  1
#define BTN_FLAG_PRESSED 2

#define BTN_EVENT_TIME_100MS 1
#define BTN_EVENT_TIME_1S    10

#define BTN_EVENT_FLAG_INUSE   0
#define BTN_EVENT_FLAG_ENABLED 1
#define BTN_EVENT_FLAG_EXEC    2

#define BTN_LOCK_INFINITE   0xFF
#define BTN_LOCK_TIME_100ms 1
#define BTN_LOCK_TIME_1S    10
#define BTN_LOCK_TIME_2S    20
#define BTN_LOCK_TIME_3S    30
#define BTN_LOCK_TIME_4S    40
#define BTN_LOCK_TIME_5S    50

typedef void (*btnf_t)(uint8_t, uint8_t);

typedef struct{
	volatile uint8_t *portregister;
	volatile uint8_t *pinregister;
	uint8_t bitmask;
	uint8_t bTimeCount;
	uint8_t bLockTime;
	uint8_t bOpts;
} btn_t;

typedef struct{
	uint8_t btnid;
	uint8_t bOpts;
	uint8_t bMSeconds;	
	btnf_t bFunction;
} btn_event_t;

extern void btn_init();
extern uint8_t btn_add(volatile uint8_t *port, volatile uint8_t *pin, uint8_t mask);
extern void btn_lock(uint8_t btnid, uint8_t mseconds);
extern void btn_unlock(uint8_t btnid);
extern uint8_t btn_event_add(uint8_t buttonid, uint8_t mseconds, btnf_t cb);
extern void btn_event_enable(uint8_t eventid);
extern void btn_event_disable(uint8_t eventid);

extern void btn_perform_actions(void);


#endif /* BTN_H_ */