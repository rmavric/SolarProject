#include <Wire.h>
#include <TFT.h>    //including library for drawing on display
#include <SoftwareSerial.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>

//TFT 
#define cs 10
#define dc 9
#define rst 8
TFT tftScreen = TFT(cs, dc, rst);

//this will be displayed on TFT
char voltageArray[7];
char currentArray[7];
char powerArray[8];
char dateArray[11];
char timeArray[9];
char stepperVoltageArray[7];
char stepperCurrentArray[7];
char stepperPowerArray[8];

void setup() {
  Serial.begin(9600);
  setupDefinition();          // TFT
  pinMode(2, INPUT_PULLUP);   // pin 2 is used as interrupt pin, ARDUINO wakes up when pin 2 is set to LOW
  pinMode(3, OUTPUT);         // when pin 3 is set to HIGH then current starts to flow between COLLECTOR and EMITTER on BJT 2N3094
                              // this is used to switch backlight of display ON and OFF
}

int flag = 0;
char message[100];
int hoursForDimmingDisplay = -1;

void loop() {

  digitalWrite(3, HIGH);
  //ARDUINO receives serial communication from WEMOS
  //when message is correct set flag to 1, exit while loop and print everything on display
  if(Serial.available()>0){
      String espToArduinoMessage;
      while(espToArduinoMessage.indexOf("T:")<0 && flag==0){
          espToArduinoMessage = readSerialMessage();
          delay(1000);
          if(espToArduinoMessage.indexOf("T:")>=0&&espToArduinoMessage.indexOf("??")>=0){
            flag = 1;
          }
      }
      
      if(flag==1){
          delay(1000);
          
          tftScreen.stroke(0,0,0);
          cleanTFTScreen();
          tftScreen.stroke(0,0,255); 
         
          //this part of code is for displaying Date&Time on TFT
          String dateDisp = dateDisplay(espToArduinoMessage);
          String timeDisp = timeDisplay(espToArduinoMessage);
          String voltage = voltageDisplay(espToArduinoMessage);
          String current = currentDisplay(espToArduinoMessage);
          String power = powerDisplay(espToArduinoMessage);
          String stepperVoltage = stepperVoltageDisplay(espToArduinoMessage);
          String stepperCurrent = stepperCurrentDisplay(espToArduinoMessage);
          String stepperPower = stepperPowerDisplay(espToArduinoMessage);

          voltage.toCharArray(voltageArray,7);
          current.toCharArray(currentArray,7);
          power.toCharArray(powerArray,8);
          dateDisp.toCharArray(dateArray,11);
          timeDisp.toCharArray(timeArray,9);
          stepperVoltage.toCharArray(stepperVoltageArray,7);
          stepperCurrent.toCharArray(stepperCurrentArray,7);
          stepperPower.toCharArray(stepperPowerArray,8);

          delay(100);

          cleanTFTScreen();

          flag = 0;
          Serial.flush();

          //ARDUINO goes to sleep after 8PM and wakes up again at 10AM
          hoursForDimmingDisplay = hours(timeDisp);
          if((hoursForDimmingDisplay>=20 || hoursForDimmingDisplay <=10)){
            sleepNow();
          }
      } 
      flag = 0;
  }
  
}

//setting TFT display 
void setupDefinition(){
  tftScreen.begin();
  
  tftScreen.background(0,0,0);
  tftScreen.stroke(0,0,255);
  tftScreen.setTextSize(1);
  tftScreen.text("Vpanel: ",5,4);    //(text, xPos, ypos)
  tftScreen.text("V", 86, 4);
  tftScreen.text("Vstepp: ",5,16);    //(text, xPos, ypos)
  tftScreen.text("V", 86, 16);

  tftScreen.text("Cpanel: ",5,33);    
  tftScreen.text("mA", 88, 33);
  tftScreen.text("Cstepp: ",5,45);    
  tftScreen.text("mA", 88, 45);

  tftScreen.text("Ppanel: ",5,62);   
  tftScreen.text("mW", 91, 62);
  tftScreen.text("Pstepp: ",5,74);   
  tftScreen.text("mW", 91, 74);

  tftScreen.text("Date: ", 5,97);    

  tftScreen.text("Time: ",5,120);   
  tftScreen.text("h", 76, 120);

}

//for reading message sent via serial from WEMOS
String readSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(Serial.available()>0){
    value[index_count]=Serial.read();
    if(value[index_count] == '#'){
      value[index_count] = '\0';
      break; 
    }
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
  }
  String str(value);
  str.trim();
  return str;
}

String dateDisplay(String receivedMessage){
  String solution;
  char message [50];
  int index = receivedMessage.indexOf("T:"); 
  receivedMessage.remove(0, index+2);
  receivedMessage.toCharArray(message, 50);
  char numbers [8];
  int k = 0;
  int i = 0;
  while(k<8 && i<50){
    if(isDigit(message[i])){
      numbers[k] = message[i];
      k++;  
    }
    i++;
  }
  for(int i = 0; i < 8; i++){
    if(i<4){
      solution += numbers[i];  
    }
    else{
      if(i%2==0){
        solution += '.';  
      }  
      solution += numbers[i];
    }
  }
  solution += '.';
  return solution;
}

String timeDisplay(String receivedMessage){
  String solution;
  char message [50];
  int index = receivedMessage.indexOf("T:");
  receivedMessage.remove(0, index+2);
  receivedMessage.toCharArray(message, 50);
  char numbers [4];
  int k = 0;
  int i = 11;
  while(k<4 && i<50){
    if(isDigit(message[i])){
      numbers[k] = message[i];
      k++;  
    }
    i++;
  }
  for(int i = 0; i < 4; i++){
    if(i==2){
      solution += ':';  
    }
    solution += numbers[i];
  }
  return solution;
}

int hours(String timeDisplay){
  int solution;
  char hrsField [2];
  String hrs = timeDisplay.substring(0, 2);
  hrs.toCharArray(hrsField, 2);
  if(hrsField[0]=="0"){
    if(isDigit(hrsField[1])){
      solution = hrs.substring(1).toInt();
    } 
  }
  else{
    solution = hrs.toInt();
  }
  return solution;
}

String voltageDisplay(String receivedMessage){
  String solution;
  char message [50];
  int startIndex = receivedMessage.indexOf("V:") + 2; 
  int endIndex = receivedMessage.indexOf("Cu:");
  solution = receivedMessage.substring(startIndex, endIndex);
  return solution;
}

String currentDisplay(String receivedMessage){
  String solution;
  char message [50];
  int startIndex = receivedMessage.indexOf("Cu:") + 3; 
  int endIndex = receivedMessage.indexOf("CV:");
  solution = receivedMessage.substring(startIndex, endIndex);
  return solution;
}

String powerDisplay(String receivedMessage){

  String voltage = voltageDisplay(receivedMessage);
  String current = currentDisplay(receivedMessage);
  float power = voltage.toFloat() * current.toFloat();
  String solution = String(power);
  return solution;
}

String stepperVoltageDisplay(String receivedMessage){
  String solution;
  char message [50];
  int startIndex = receivedMessage.indexOf("CV:") + 3; 
  int endIndex = receivedMessage.indexOf("CCu:");
  solution = receivedMessage.substring(startIndex, endIndex);
  return solution;
}

String stepperCurrentDisplay(String receivedMessage){
  String solution;
  char message [50];
  int startIndex = receivedMessage.indexOf("CCu:") + 4; 
  int endIndex = receivedMessage.indexOf("??");
  solution = receivedMessage.substring(startIndex, endIndex);
  return solution;
}

String stepperPowerDisplay (String receivedMessage){
  String voltage = stepperVoltageDisplay(receivedMessage);
  String current = stepperCurrentDisplay(receivedMessage);
  float power = voltage.toFloat() * current.toFloat();
  String solution = String(power);
  return solution;
}

void cleanTFTScreen(){
  tftScreen.text(voltageArray,53,4);
  tftScreen.text(stepperVoltageArray,53,16);
  tftScreen.text(currentArray,53,33);
  tftScreen.text(stepperCurrentArray,53,45);
  tftScreen.text(powerArray,53,62);
  tftScreen.text(stepperPowerArray,53,74);
  tftScreen.text(dateArray,39,97);
  tftScreen.text(timeArray,39,120);
}

// here we put arduino to sleep
void sleepNow(){         
    digitalWrite(3, LOW);
    Serial.flush();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();    
    attachInterrupt(digitalPinToInterrupt(2), wakeUpNow, LOW);
    sei();
    sleep_cpu();
// ------------------ HERE ARDUINO WAKES UP ------------------
    sleep_disable(); 
    digitalWrite(3, HIGH);
}

void wakeUpNow(){
  detachInterrupt(digitalPinToInterrupt(2));  
}
