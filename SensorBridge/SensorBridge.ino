/*
       File: ShowerSense.ino 
    Purpose: Sense waterflow and alert a listener
Application: Float on shower usage detector informing a node that signals the Helm node. Eventual use might be also automatically reading sensors in the tank.
     Author: Sean Connell
     Design: A finite state machine which collects a data during a collection window, applies transforms to the data, then notifies any listeners with that data.
     
power on init and start char-> DATA_GATHERING-<timer overflow>--[-> DATA_TRANSFORM]-<transform completes>  
                                      ^                     |                                  |
                                      |             (!DATA_GATHERING?)                         |
                                      |                     |                                  |
                                      |                 [-> RESET]                   [-> NOTIFY_LISTENER]
                                      |                                                        |
                                      |                                                        |  
                                      |-------------------[CLEAR_STATE <-]---------<notify completes>
                    
TODO: Put MCU to sleep while not gathering data via interrupts
TODO: Add reporting windows for each data type, not just global reports
*/

#include "avr/wdt.h"

//User Settings
#define FLOW_SENSOR_PIN 2
//Be careful about setting this, it might overflow variables tracking various data if too long
#define REPORT_INTERVAL_SECONDS 5
//Higher rates make my usb port unhappy. Might be the usb/uart converter or something else in the chain
#define BAUD_RATE 38400

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
#define INITIAL         B00000000
#define DATA_TRANSFORM  B00000001
#define NOTIFY_LISTENER B00000010
#define DATA_GATHERING  B00000100
#define CLEAR_STATE     B00001000
#define CHECK_MESSAGES  B00010000
#define RESTART         B10000000

//message command bytes
#define MAX_MESSAGE_CHECKS 20 // only check 20 messages at a time to keep reporting interval consistent
#define START_CMD 33 // ! character
#define RESTART_CMD 38 // & character

//Volatile stateful variables
volatile uint16_t  pulse_count;
volatile uint8_t   STATE = INITIAL;
volatile uint8_t   second_counter = 0;

//Data to be transformed register
#define FLOW_RATE_DATA  B00000001
#define SALINITY_DATA   B00000010
#define PH_DATA         B00000100

//less state critical variables
uint16_t flow_rate = 0;
unsigned long start_time = 0;

//message iteration and value variables
uint8_t m_i;
uint8_t msg = 0;

//The data gathering window has ended and we should move on to calculating reporting values
ISR(TIMER1_COMPA_vect){  
  second_counter++;
  
  //wait to transition state until we hit the end of our observation window
  if(REPORT_INTERVAL_SECONDS == second_counter){
    second_counter = 0; 
    
    if(STATE != DATA_GATHERING){
      /* We somehow got to the end of our data gathering window without resetting state
      Might be a bad calc somewhere in the DATA_TRANSFORM step
      Try to restart and hope that it doesn't happen again */ 
      STOP_TIMER;
      STATE = RESTART; 
      DISABLE_INTERRUPTS;//We're going to bail out and restart. Don't need to worry about the world
    }
    
    else{
      STOP_TIMER;
      STATE = DATA_TRANSFORM;
      DISABLE_INTERRUPTS;//Don't want to collect any data while we copy some values
    }
  }
}

void setup() {
   clear_watchdog();
   Serial.begin(BAUD_RATE); //initialize uart
   //Wait for start byte to enter cyclic state machine
   while(Serial.read() != START_CMD);
   print_startup_message();
   attachInterrupt(0, pulses_counter, CHANGE);
   initialize_timer();
   STATE = DATA_GATHERING;
   ENABLE_INTERRUPTS;
   log_message(INFO, "Entering DATA_GATHERING state and running...");
   start_time = millis();
   START_TIMER;
}

void loop()
{ 
  /* State machine transitions and actions that 
  aren't time critical or are too heavy to do
  in interrupts are done here. */
  switch(STATE){
    
    //Not concerned with DATA_GATHERING 
    case DATA_GATHERING:
      break;
      
    //If the data is ready to process do the neccesary
    //steps to transform it into output form
    case DATA_TRANSFORM:
      flow_rate = calculate_flow_rate(pulse_count);
      STATE = CHECK_MESSAGES;
      break;
    
    //Check for any buffered commands and act on them
    case CHECK_MESSAGES:
      for(m_i=0; m_i<MAX_MESSAGE_CHECKS; m_i++){
         //only check for messages if we have some buffered
         if(Serial.available() <= 0){
           break;//done with checking because we don't have any messages 
         }
         switch(msg = Serial.read()){
           case RESTART_CMD:
             reset();
             break;
         }
      }
      msg = 0;
      STATE = NOTIFY_LISTENER;
      break;
    
    //Notify whatever is listening with a json report
    case NOTIFY_LISTENER:
      serialize_as_json_report();
      STATE = CLEAR_STATE;
      break;
    
    //reset everything to be ready for the next cycle
    case CLEAR_STATE:
      clear_data_counters();
      CLEAR_PENDING_INTERRUPTS;
      STATE = DATA_GATHERING;
      CLEAR_TIMER;
      start_time = millis();
      START_TIMER;
      ENABLE_INTERRUPTS;
      break;
      
    case RESTART:
      log_message(ERROR, "Illegal state: Did not return to DATA_GATHERING state before time window elapsed");
      reset();
      
    default:
      log_message(ERROR, "Illegal state: STATE register corrupted into unrecognized state");
      reset();
  }
}

//All data variables go here for clearing
void clear_data_counters(){
  flow_rate = 0;
  pulse_count = 0;
}

//State safe counter
void pulses_counter(){
  if(STATE == DATA_GATHERING){
    pulse_count++;
  }
}

void initialize_timer(){
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 15624; //compare mode with 1 second intervals at this clock rate of 16mhz and prescale of 1024
  TCCR1B |= (1 << WGM12); //CTC mode
  TIMSK1 |= (1 << OCIE1A); //enable timer compare interrupt
}

/*
 Sensor:https://www.adafruit.com/products/828
 2.25 ml per pulse rougly speaking
 so *(9/4), div 4 of course being 2 
 bitshifts right. Not sure if compiler
 is smart enough to know that though
 */
uint16_t calculate_flow_rate(uint16_t pulses){
    return ((pulses*9)>>2); 
}

/*
 Print which version of the firmware is running, which sensors are expected and where
*/
void print_startup_message(){
   Serial.print("!{\"firmware_version\":\"1.0\", \"sensors_manifest\":");
   Serial.println("[{\"sensor_name\":\"flow_sensor\",\"sensor_url\":\"https://www.adafruit.com/products/828\",\"connection\":\"PIN2\"}]}");
   log_message(INFO, "Initializing data gathering subsystems...");
}

void serialize_as_json_report(){
  Serial.print("!{");
    Serial.print("\"DATA\":[");
      Serial.print("{");
        Serial.print("\"period\":"); Serial.print(millis() - start_time); Serial.print(",");
        Serial.print("\"data_type\":\"water_flow\",");
        Serial.print("\"units\":\"mL\",");
        Serial.print("\"value\":"); Serial.print(flow_rate);
      Serial.print("}");
    Serial.print("],");
    Serial.print("\"TIME\":"); Serial.print(millis());
  Serial.println("}");
}

void log_message(String level, String message){
  Serial.print("!{\""); Serial.print(level); Serial.print("\":\""); Serial.print(message); Serial.print("\",");
  Serial.print("\"TIME\":"); Serial.print(millis()); Serial.println("}");
}

void clear_watchdog(){
    // Clear the reset bit
    MCUSR &= ~_BV(WDRF);
    // Disable the WDT
    WDTCSR |= _BV(WDCE) | _BV(WDE); 
    WDTCSR = 0;
}

void reset(){
  wdt_enable(WDTO_1S); 
  while(1);//wait for the watchdog to kill us
}
