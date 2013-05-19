#ifndef sensor_bridge_h
#define sensor_bridge_h

#include "Arduino.h"

//Logging formatting
#define ERROR "ERROR"
#define WARN  "WARN"
#define DEBUG "DEBUG"
#define INFO  "INFO"

//Readability defines
#define CLEAR_PENDING_INTERRUPTS EIFR = 0
#define ENABLE_INTERRUPTS sei()
#define DISABLE_INTERRUPTS cli()
#define STOP_TIMER TCCR1B &= 0B11111000 //Clear CS12,11,10 I think this is clearer than doing the shift over invert thing
#define CLEAR_TIMER TCNT1 = 0B00000000
#define START_TIMER TCCR1B |= (1 << CS10) | (1 << CS12); //Set prescaler to 1024, and start timer

//States for the system
#define INITIAL         1
#define DATA_TRANSFORM  2
#define NOTIFY_LISTENER 3
#define DATA_GATHERING 4
#define CLEAR_STATE     5
#define CHECK_MESSAGES  6
#define RESTART        7

//message command bytes
#define MAX_MESSAGE_CHECKS 20 // only check 20 messages at a time to keep reporting interval consistent
#define START_CMD 33 // ! character
#define RESTART_CMD 38 // & character

//Data to be transformed register
#define FLOW_RATE_DATA  B00000001
#define SALINITY_DATA   B00000010
#define PH_DATA         B00000100

#endif