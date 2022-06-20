/*
 * BTN.c
 *
 * Created: 25.11.2018 15:40:12
 *  Author: Bjoern
 */ 

#include <avr/io.h>
#include "../Global.h"
#include "../EasyIO.h"
#include "BTN.h"
#include "../timedJob/timedJob.h"
#include "../UART/UART.h"



uint8_t btnJob;
static btn_t btnList[BTN_MAX_ENTRIES];
static btn_event_t btnEventList[BTN_EVENT_MAX_ENTRIES];

uint8_t btn_get_optimal_event(uint8_t btnid, uint8_t time){
	uint8_t bestValue = 0;
	uint8_t bestIndex = 0xFF;
	
	for(uint8_t i = 0; i < BTN_EVENT_MAX_ENTRIES; i++){
		if((btnEventList[i].bOpts & (1<<BTN_EVENT_FLAG_INUSE)) && (btnEventList[i].bOpts & (1<<BTN_EVENT_FLAG_ENABLED)) && (btnEventList[i].btnid == btnid)){
			if((btnEventList[i].bMSeconds <= time) && (btnEventList[i].bMSeconds > bestValue)){
				bestValue = btnEventList[i].bMSeconds;
				bestIndex = i;
			}
		}
	}	
	
	return bestIndex;	
}

void btn_tick(uint8_t dummy){
	for(uint8_t i = 0; i < BTN_MAX_ENTRIES; i++){
		// Button ist registriert
		if(btnList[i].bOpts & (1<<BTN_FLAG_INUSE)){
			// Gesperrte Buttons behandeln
			if(btnList[i].bOpts & (1<<BTN_FLAG_LOCKED)){
				if((btnList[i].bLockTime != BTN_LOCK_INFINITE) && (btnList[i].bLockTime > 0)){
					btnList[i].bLockTime--;
				}	
				if(btnList[i].bLockTime == 0){
					btnList[i].bOpts &= ~(1<<BTN_FLAG_LOCKED);
				}
			}
			
			// Schau auf Masse
			//if(!(*btnList[i].pinregister & (1<<btnList[i].bitmask))){
			// Schau auf VCC
			if(*btnList[i].pinregister & (1<<btnList[i].bitmask)){
				// BTN liegt (noch) auf Masse
				if(!(btnList[i].bOpts & (1<<BTN_FLAG_LOCKED))){
					btnList[i].bOpts |= (1<<BTN_FLAG_PRESSED);
					btnList[i].bTimeCount++;
				}
			} else {		
				// BTN ist auf VCC gezogen
				// Prüfe, ob der Button mal gedrückt war
				if(btnList[i].bOpts & (1<<BTN_FLAG_PRESSED)){
					// Nur agieren, wenn der Button nicht gesperrt ist
					// Code wird hier angewendet, damit es auch greift, während des drückens gesperrt wird.
					if(!(btnList[i].bOpts & (1<<BTN_FLAG_LOCKED))){
						//uart_putc(btnList[i].bTimeCount);
						uint8_t bestEventIndex = btn_get_optimal_event(i, btnList[i].bTimeCount);
						//uart_putc(bestEventIndex);
						if(bestEventIndex < 0xFF){
							btnEventList[bestEventIndex].bOpts |= (1<<BTN_EVENT_FLAG_EXEC);
						}
						
						btnList[i].bTimeCount = 0;
						btnList[i].bOpts &= ~(1<<BTN_FLAG_PRESSED);
					}
				}
			}
			
			
		}
	}
}

void btn_init(){
	btnJob = timedjob_add(TIMEDJOB_INFINITE, TIMEDJOB_TIME_100MS, TIMEDJOB_OPT_ACTIVE, &btn_tick);

	for(uint8_t i = 0; i < BTN_MAX_ENTRIES; i++)
	{
		btnList[i].bOpts = 0;
	}
	
	for(uint8_t i = 0; i < BTN_MAX_ENTRIES; i++)
	{
		btnEventList[i].bOpts = 0;
	}

}

uint8_t btn_add(volatile uint8_t *port, volatile uint8_t *pin, uint8_t mask){
	for(uint8_t i = 0; i < BTN_MAX_ENTRIES; i++)
	{
		if(~btnList[i].bOpts & (1<<BTN_FLAG_INUSE))
		{
			btnList[i].bOpts = (1<<BTN_FLAG_INUSE);
			btnList[i].portregister = port;
			btnList[i].pinregister = pin;
			btnList[i].bitmask = mask;
			btnList[i].bTimeCount = 0;
			btnList[i].bLockTime = 0;
			return i;
		}
	}
	
	return 0xFF;
}

void btn_lock(uint8_t btnid, uint8_t mseconds){
	if(btnid >= BTN_MAX_ENTRIES)
		return;
	  
	if(btnList[btnid].bOpts & (1<<BTN_FLAG_INUSE)){
		if(mseconds == 0){
			btnList[btnid].bLockTime = BTN_LOCK_INFINITE;
		} else {
			btnList[btnid].bLockTime = mseconds;
		}
		btnList[btnid].bOpts |= (1<<BTN_FLAG_LOCKED);
	}
}

void btn_unlock(uint8_t btnid){
	if(btnid >= BTN_MAX_ENTRIES)
		return;
	
	if(btnList[btnid].bOpts & (1<<BTN_FLAG_INUSE)){
		btnList[btnid].bOpts &= ~(1<<BTN_FLAG_LOCKED);
		btnList[btnid].bLockTime = 0;
	}
}


uint8_t btn_event_add(uint8_t buttonid, uint8_t mseconds, btnf_t cb){
	for(uint8_t i = 0; i < BTN_EVENT_MAX_ENTRIES; i++){
		if(~btnEventList[i].bOpts & (1<<BTN_EVENT_FLAG_INUSE)){
			btnEventList[i].btnid = buttonid;
			btnEventList[i].bOpts = (1<<BTN_EVENT_FLAG_INUSE) | (1<<BTN_EVENT_FLAG_ENABLED);
			btnEventList[i].bMSeconds = mseconds;
			btnEventList[i].bFunction = cb;
			return i;
		}
	}
	
	return 0xFF;
}

void btn_event_enable(uint8_t eventid){
	if(eventid >= BTN_EVENT_MAX_ENTRIES){
		return;
	}
	
	btnEventList[eventid].bOpts |= (1<<BTN_EVENT_FLAG_ENABLED);
}

void btn_event_disable(uint8_t eventid){
	if(eventid >= BTN_EVENT_MAX_ENTRIES){
		return;
	}
	
	btnEventList[eventid].bOpts &= ~(1<<BTN_EVENT_FLAG_ENABLED);
}

void btn_perform_actions(void){
	for(uint8_t i = 0; i < BTN_EVENT_MAX_ENTRIES; i++){
		if((btnEventList[i].bOpts & (1<<BTN_EVENT_FLAG_INUSE)) && (btnEventList[i].bOpts & (1<<BTN_EVENT_FLAG_EXEC))){
			btnEventList[i].bOpts &= ~(1<<BTN_EVENT_FLAG_EXEC);
			
			btnEventList[i].bFunction(btnEventList[i].btnid, i);
		}		
	}
}
