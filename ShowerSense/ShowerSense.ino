/*
       File: ShowerSense.ino 
    Purpose: Sense waterflow and alert a listener
Application: Float on shower usage detector informing a node that signals the Helm node. Eventual use might be also automatically reading sensors in the tank.
     Author: Sean Connell
     Design: A finite state machine which collects a data during a collection window, applies transforms to the data, then notifies any listeners with that data.
     
power on init-> DATA_GATHERING-----<timer overflow>----[-> DATA_TRANSFORM]----<transform completes>  
                    ^                     |                                            |
                    |             (!DATA_GATHERING?)                                   |
                    |                     |                                            |
                    |                 [-> RESET]                             [-> NOTIFY_LISTENER]
                    |                                                                  |
                    |                                                                  |  
                    |---------------------------------[CLEAR_STATE <-]---------<notify completes>
                    
TODO: Add in timer stop/start and watchdog timer initialize/cleanup and use to reset
     
*/

#define IN_USE_FLOW_RATE 50
#define TIMER_PERIOD 1000
#define SHOWER_IN_USE(X) (return X > IN_USE_FLOW_RATE)
#define FLOW_SENSOR_PIN 2
#define CLEAR_PENDING_INTERRUPTS EIFR = 0
#define ENABLE_INTERRUPTS sei()
#define DISABLE_INTERRUPTS cli()
#define STOP_TIMER TCCR1B &= 0B11111000 //Clear CS12,11,10 I think this is clearer than doing the shift over invert thing
#define START_TIMER TCCR1B |= (1 << CS10) | (1 << CS12); //Set 1024 prescaler bits

//States for the system
#define INITIAL         B00000000
#define DATA_TRANSFORM  B00000001
#define NOTIFY_LISTENER B00000010
#define DATA_GATHERING  B00000100
#define CLEAR_STATE     B00001000
#define RESTART  B10000000

//Volatile stateful variables
volatile uint16_t  pulse_count;
volatile uint8_t STATE = INITIAL;

//Data to be transformed register
#define FLOW_RATE_DATA  B00000001
#define SALINITY_DATA   B00000010
#define PH_DATA         B00000100

//less state critical variables
uint16_t flow_rate = 0;

//The data gathering window has ended and we should move on to calculating reporting values
ISR(TIMER1_COMPA_vect){
  if(STATE != DATA_GATHERING){
    /* We somehow got to the end of our data gathering window without resetting state
    Might be a bad calc somewhere in the DATA_TRANSFORM step
    Try to restart and hope that it doesn't happen again */ 
    STATE = RESTART; 
    DISABLE_INTERRUPTS;//We're going to bail out and restart. Don't need to worry about the world
  }
  else{
    STATE = DATA_TRANSFORM;
    STOP_TIMER;
    DISABLE_INTERRUPTS;//Don't want to collect any data while we copy some values
  }
}

void setup() {
   clear_watchdog();
   Serial.begin(115200); //initialize uart
   pinMode(FLOW_SENSOR_PIN, INPUT); 
   digitalWrite(FLOW_SENSOR_PIN, HIGH);
   attachInterrupt(FLOW_SENSOR_PIN, pulses_counter, CHANGE);
   initialize_timer();
   STATE = DATA_GATHERING;
   ENABLE_INTERRUPTS;
}

void loop()
{ 
  /* State machine transitions and actions that 
  aren't time critical or are too heavy to do
  in interrupts are done here. */
  //TODO look into: implement this as a fn pointer to STATE
  //such that each state has a function that either
  //returns itself or next state, then the main falls
  //out into while(1){p_state = *p_state();} as per
  //Ben's suggestion. Might require implementing
  //some sort of struct to represent magic interrupt
  //based variables
  switch(STATE){
    
    //Not concerned with DATA_GATHERING 
    case DATA_GATHERING:
      break;
      
    //If the data is ready to process do the neccesary
    //steps to transform it into output form
    case DATA_TRANSFORM:
      flow_rate = calculate_flow_rate(pulse_count);
      STATE = NOTIFY_LISTENER;
      break;
    
    //Notify whatever is listening by whatever method
    case NOTIFY_LISTENER:
      Serial.print("Flow Rate:\t"); 
      Serial.println(flow_rate);
      STATE = CLEAR_STATE;
      break;
    
    //reset everything to be ready for the next cycle
    case CLEAR_STATE:
      flow_rate = 0;
      CLEAR_PENDING_INTERRUPTS;
      STATE = DATA_GATHERING;
      START_TIMER;
      ENABLE_INTERRUPTS;
      break;
      
    //currently hard coded error message for reset. Generalize later
    case RESTART:
      Serial.println("Reset not yet implemented. Would if it could though...");
      Serial.println("|-------------------------------------RESETTING------------------------------------|"); 
      Serial.println("| ILLEGAL STATE: Did not return to DATA_GATHERING state before time window elapsed |"); 
      Serial.println("|-------------------------------------RESETTING------------------------------------|"); 
      reset();
      break;
  }
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
  OCR1A = 46874; //compare mode with 3 second intervals at this clock rate of 16mhz
  TCCR1B |= (1 << WGM12); //CTC mode
  TIMSK1 |= (1 << OCIE1A); //enable timer compare interrupt
}

uint16_t calculate_flow_rate(uint16_t pulses){
    return (pulses/TIMER_PERIOD); 
}

void clear_watchdog(){
    //TODO
}

void reset(){
   initialize_watchdog();
   //TODO
   //set watchdog to explode here
}
