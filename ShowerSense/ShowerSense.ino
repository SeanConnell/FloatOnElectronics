/*
Written by Sean Connell (Hacked together is a better description) on 5/13/2012, and then rewritten with different sensors on 8/13 for Float On.
Uses demo code from LadyAda's flowrate sensor test
Detects shower usage by temperature gradiant and sound level thresholding. Indicates this via three LEDs.
This allows the pump operator to begin running the pumps before the floater has even left the room.
*/

//hook up some indicator leds to these pins
#define LED1 9
#define LED2 10
#define LED3 11
#define FLOW_THRESH 50
#define SMOOTHING_DELAY 2

// pin to use for reading the flow rate sensor
#define FLOWSENSORPIN 2

//Led brightness for the animation
int brightness1 = 0;    // how bright the LED is
int brightness2 = 83;
int brightness3 = 166;
//Animation scheme is constant brightness amount, different time between changes
int fadeAmount1 = 1;    // Direction to fade LEDs
int fadeAmount2 = 1;
int fadeAmount3 = 1;
//Use a logarythmic lookup table to keep the fade smooth by only increasing the brightness 1 each time, but varying the amount of time between increase/decreases
int fadeTable[] = {  1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 };
int di = 0; //index variable for fadetable
#define FADE_LENGTH 256 //length of table

//the end goal is to set this when somebody is showering
int showerInUse = 0;
//Tells us whether to fade leds up or continue animating
boolean onTransition = 1;
//LadyAda's app note for reading the sensor with a constant timed interrupt
volatile uint16_t pulses = 0; // count how many pulses!
volatile uint8_t lastflowpinstate; // track the state of the pulse pin
volatile uint32_t lastflowratetimer = 0; // you can try to keep time of how long it is between pulses
volatile float flowrate; // and use that to calculate a flow rate
// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

void setup() {
   // initialize serial communication with computer
   Serial.begin(115200);  
   pinMode(FLOWSENSORPIN, INPUT);
   digitalWrite(FLOWSENSORPIN, HIGH);
   lastflowpinstate = digitalRead(FLOWSENSORPIN);
   useInterrupt(true);
   // LEDs are outputs
   pinMode(LED1, OUTPUT);
   pinMode(LED2, OUTPUT);
   pinMode(LED3, OUTPUT);
}

void loop()
{ 
  Serial.print("Freq: "); Serial.print(flowrate);
  Serial.print("\tIn Use: "); Serial.println(showerInUse);

  if(flowrate > FLOW_THRESH){//Threshold exceeded, shower is being used
      showerInUse = 1;
    }
  else{//Shower not in use
      showerInUse = 0;
    }
  if(showerInUse){
      if(onTransition){ //just started being used, fade up the LEDs
          LEDFadeUp(); 
          onTransition = 0; 
      }
      LEDAnimate(); //LEDs have faded into appropriate brightness, animate!
    }//Send the signal for In Use
  else{//Turn LEDs off, shower not in use
      LEDFadeDown();//return to off smoothly
      onTransition = 1;//get ready to fade up next use
  }
  //float liters = pulses;
  //liters /= 7.5; // liters/s
  delay(10);//chill a little while
}

void LEDFadeUp(){//fades LEDs down to initial brightness for a smoother transition. I know it's blocking, too bad.
    analogWrite(LED1, 0);//this guy is just off
    while(brightness2 < 83){
        brightness2++; 
        brightness3++; 
        analogWrite(LED2, brightness2);
        analogWrite(LED3, brightness3);
        delay(SMOOTHING_DELAY);}
    while(brightness3 < 166){
        brightness3++; 
        delay(SMOOTHING_DELAY);
    }
}

void LEDFadeDown(){//fades LEDs down to no brightness for a smoother transition. I know it's blocking, too bad.
    while((brightness1+brightness2+brightness3) > 0){
        if(brightness1 >0 ){brightness1--; analogWrite(LED1, brightness1);}
        if(brightness2 >0 ){brightness2--; analogWrite(LED2, brightness2);}
        if(brightness3 >0 ){brightness3--; analogWrite(LED3, brightness3);}
        delay(SMOOTHING_DELAY);
    }
}

void LEDAnimate()  { 
    analogWrite(LED1, brightness1);
    analogWrite(LED2, brightness2);
    analogWrite(LED3, brightness3);
    // change the brightness for next time through the loop:
    brightness1 = brightness1 + fadeAmount1;
    brightness2 = brightness2 + fadeAmount2;
    brightness3 = brightness3 + fadeAmount3;
    // reverse the direction of the fading at the ends of the fade: 
    if (brightness1 <= 0 || brightness1 >= 250) {
      fadeAmount1 = -fadeAmount1 ; 
    }     
    if (brightness2 <= 0 || brightness2 >= 250) {
      fadeAmount2 = -fadeAmount2 ; 
    }     
    if (brightness3 <= 0 || brightness3 >= 250) {
      fadeAmount3 = -fadeAmount3 ; 
    }        
    delay(fadeTable[wait_index()]);                
}

int wait_index( void ){//iterates through the fade table
  return (di = ++di %FADE_LENGTH); 
}


