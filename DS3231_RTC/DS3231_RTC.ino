#include <avr/sleep.h>
#define interruptPin 2 //pin we are going to use to wake up the Arduino
#include <DS3232RTC.h>  
#include<AccelStepper.h>

unsigned long time_value;

unsigned long int previousMillis = 0;

// Stepper motor
#define dirPin 3    //direction pin
#define stepPin 4   //pin which determines steps
#define motorInterfaceType 1  // when we are using driver 
                              // motor interface type needs to be set to 1
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
              
void setup() {
  Serial.begin(9600); //start Serial Comunication
  stepperSetup();
  pinMode(LED_BUILTIN,OUTPUT);        //we use the led on pin 13 to indicate when Arduino is asleep
  pinMode(interruptPin,INPUT_PULLUP); //set pin d2 to input using the builtin pullup resistor
  digitalWrite(LED_BUILTIN,HIGH);     //turning LED on
  
  //initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
    
//    uncomment the block block to set the time on your RTC. Remember to comment it again 
//    otherwise you will set the time at everytime you upload the sketch
//    Begin block
    
//    tmElements_t tm;
//    tm.Hour = 05;               //set the RTC to an arbitrary time
//    tm.Minute = 36;
//    tm.Second = 30;
//    tm.Day = 26;
//    tm.Month = 4;
//    tm.Year = 2020 - 1970;      //tmElements_t.Year is the offset from 1970
//    RTC.write(tm);              //set the RTC from the tm structure
    
//    Block end
    
    time_t t;     //create temporary time variable so we can set the time and read the time from the RTC
    t=RTC.get();  //gets the current time of the RTC

    time_value=10UL;
    
    RTC.setAlarm(ALM1_MATCH_HOURS, 0, 0, time_value, 0);
    //clear the alarm flag
    RTC.alarm(ALARM_1);
    //configure the INT/SQW pin for "interrupt" operation (disable square wave output)
    RTC.squareWave(SQWAVE_NONE);
    //enable interrupt output for Alarm 1
    RTC.alarmInterrupt(ALARM_1, true);
}

void loop() {
  time_t value = RTC.get();   

  //from April to September(included) stepper should run for 6 hours and then go to sleep 
  if(month(value)>=4 && month(value)<=9){
    if(hour(value)>15){
      Going_To_Sleep();  
    }
    else{
      previousMillis=millis();
      //it will run for 6hrs 
      while(millis()-previousMillis<=21600000UL){
        stepper.runSpeed();
      }
    } 
  }

  //from January to March(included) and from October to December(included) stepper should run for 4 hours and then go to sleep
  if((month(value)>=1 && month(value)<=3) || (month(value)>=10 && month(value)<=12)){
    if(hour(value)>13){
      Going_To_Sleep();  
    } 
    else{
      previousMillis=millis();
      //it will run for 4hrs
      while(millis()-previousMillis<=14400000UL){
        stepper.runSpeed();
      }
    } 
  }

}

void wakeUp(){
  sleep_disable();      //disable sleep mode
  detachInterrupt(0);   //removes the interrupt from pin 2;
}

void stepperSetup() {
  stepper.setMaxSpeed(1000);
  stepper.setSpeed(40);
}

void Going_To_Sleep(){
    sleep_enable(); //enabling sleep mode
    attachInterrupt(0, wakeUp, LOW);  //attaching a interrupt to pin d2
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  //setting the sleep mode, in our case full sleep
    digitalWrite(LED_BUILTIN,LOW);  //turning LED off
    time_t t;     //creates temp time variable
    t=RTC.get();  //gets current time from rtc
    delay(1000);  //wait a second to allow the led to be turned off before going to sleep
    sleep_cpu();  //activating sleep mode
    
//  ------------------------ HERE ARDUINO WAKES UP ------------------------    
    
    digitalWrite(LED_BUILTIN,HIGH); //turning LED on
    t=RTC.get();

    time_value=10UL;

    RTC.setAlarm(ALM1_MATCH_HOURS , 0, 0, time_value, 0);
    //clear the alarm flag
    RTC.alarm(ALARM_1);
}
