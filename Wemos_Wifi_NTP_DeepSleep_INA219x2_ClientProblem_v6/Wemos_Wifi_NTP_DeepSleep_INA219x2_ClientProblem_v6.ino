#include "ESP8266WiFi.h"
#include <NTPtimeESP.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SPI.h>

//two INA219 devices are used, and they should have different addresses
Adafruit_INA219 ina219_A;           //default address
Adafruit_INA219 ina219_B(0x41);     //this address is achieved with jumper (A0) on device 

NTPtime NTPhr("hr.pool.ntp.org");   //Choose server pool as required
strDateTime dateTime;

const char* ssid = "xxxxxxxxxx"; //SECRET_SSID; //Enter SSID
const char* password = "xxxxxxxxxx" ; //SECRET_PASS; //Enter Password 
char host[] = "xxx.xxx.x.x"; //IPv4 address
const int port = 8888 ;   //port on which server listens

//variables for ina219_A -> values for generation
static float currentSumA = 0;
static int countA = 0;
static float powerSumA = 0;
static float voltageSumA = 0;

//variables for ina219_B -> values for consumption
static float currentSumB = 0;
static int countB = 0;
static float powerSumB = 0;
static float voltageSumB = 0;

WiFiClient client;    //use for TCP connections

void setup() {
  Serial.begin(9600); 
  ina219_A.begin();
  ina219_B.begin();

  pinMode(12, OUTPUT);      //it is used for waking up arduino nano, this is pin D6 on WEMOS
  
  digitalWrite(12, LOW);    //LOW signal triggers interrupt on arduino nano
    
  WiFi.begin(ssid, password);   // Connect to WiFi
  
  measurementA();       //ina219_A measurements
  measurementB();       //ina219_B measurements
  
  while(WiFi.waitForConnectResult() != WL_CONNECTED) //while(WiFi.status() != WL_CONNECTED)
  {
    measurementA();
    measurementB();
    delay(100);
    if(millis()>25000){         
      break;
    }
  }

  //if does not connect to WiFi in 25 seconds then go to sleep
  if(WiFi.status() != WL_CONNECTED)
  {
    while(millis()<40000)
    {
      delay(10);
    }
    digitalWrite(12, HIGH);
    delay(500);
    ESP.deepSleep(92069e4);
  }

  //read date and time
  while(!(dateTime = NTPhr.getNTPtime(1.0, 1)).valid)
  {
    delay(100);
  }
  
  measurementA();
  measurementB();
  
  while(millis()<30000)
  {
    delay(10);
  }
  
  measurementA();
  measurementB();

  String timeAndDate = timeCalculation(dateTime);

  measurementA();
  measurementB();
  
  while(millis()<40000)
  {
    delay(10);
  }
  
  measurementA();
  String allMeasurementsA = printMeasurementsA(voltageSumA, currentSumA, powerSumA, countA, millis());

  measurementB();
  String allMeasurementsB = printMeasurementsB(voltageSumB, currentSumB, powerSumB, countB, millis());

  //finalString will be sent to server, parsed, and values will be stored in database
  String finalString = timeAndDate + allMeasurementsA + allMeasurementsB;

  int indexA = allMeasurementsA.indexOf("Co:");
  int indexB = allMeasurementsB.indexOf("CCo:");
  String serialMeasurement = timeAndDate + allMeasurementsA.substring(0, indexA) + allMeasurementsB.substring(0, indexB) + "??#" ;   

  delay(10);

  Serial.println(serialMeasurement);

  delay(10);

  digitalWrite(12, HIGH);
  
  delay(10);

  //connection to client
  if(client.connect(host, port))
  {
    client.print(finalString); 

    delay(300);
  
    delay(300);

    client.stop();
  }
  else{
    delay(300);
    ESP.deepSleep(92069e4);
    
  }

  delay(300);

  ESP.deepSleep(92369e4);     //internal clock is not to precise, so it will go to sleep to approximately 14m20s
                              //92369e4 us -> 15m23s, but it will actually be around 14m20s, I didn't want to use DS3231 for this (but that would be more precise)
}


void loop() 
{
}


String timeCalculation(strDateTime dateAndTime)
{
      strDateTime dateTime = dateAndTime; 
      String message = "";
      if(dateTime.valid)
      {   
        byte actualHour = dateTime.hour;
        byte actualMinute = dateTime.minute;
        byte actualSecond = dateTime.second;
        int actualYear = dateTime.year;
        byte actualMonth = dateTime.month;
        byte actualDay = dateTime.day;
  
        String Hour = actualHour < 10 ? "0" + String(actualHour) : String(actualHour);
        String Minute = actualMinute < 10 ? "0" + String(actualMinute) : String(actualMinute);
        String Second = actualSecond < 10 ? "0" + String(actualSecond) : String(actualSecond);
        String Year = actualYear < 10 ? "0" + String(actualYear) : String(actualYear);
        String Month = actualMonth < 10 ? "0" + String(actualMonth) : String(actualMonth);
        String Day = actualDay < 10 ? "0" + String(actualDay) : String(actualDay);
  
        message = Year + "." + Month + "." + Day + "." + " " + Hour + ":" + Minute + ":" + Second;
        message = "T:" + message;
      }
      else
      {
        Serial.print("dateTime is not valid");
        message = "dateTime is not valid";
      }
      return message;
} 

void measurementA()
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  shuntvoltage = ina219_A.getShuntVoltage_mV();
  busvoltage = ina219_A.getBusVoltage_V();
  current_mA = ina219_A.getCurrent_mA();
  power_mW = ina219_A.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  voltageSumA += loadvoltage;
  currentSumA += current_mA;
  powerSumA += currentSumA*voltageSumA;
  countA++;
}

void measurementB()
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  shuntvoltage = ina219_B.getShuntVoltage_mV();
  busvoltage = ina219_B.getBusVoltage_V();
  current_mA = ina219_B.getCurrent_mA();
  power_mW = ina219_B.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  voltageSumB += loadvoltage;
  currentSumB += current_mA;
  powerSumB += currentSumB*voltageSumB;
  countB++;
}

String printMeasurementsA(float volt, float curr, float powr, int count, unsigned long currentMillis)
{
  String message = "";
  float voltage = volt / count;
  voltage = (voltage < 0) ? 0.0 : voltage;
  
  float current = curr / count;
  current = (current < 0) ? 0.0 : current;
  
  float power = voltage * current;
  
  float consumption = (power * currentMillis) / 3600000;      // consumption is now in mWh

  message = "V:"+String(voltage)+"Cu:"+String(current)+"Co:"+String(consumption)+"??";
  return message;
}

String printMeasurementsB(float volt, float curr, float powr, int count, unsigned long currentMillis)
{
  String message = "";
  float voltage = volt / count;
  voltage = (voltage < 0) ? 0.0 : voltage;
  
  float current = curr / count;
  current = (current < 0) ? 0.0 : current;
  
  float power = voltage * current;
  
  float consumption = (power * currentMillis) / 3600000;      // consumption is now in mWh

  message = "CV:"+String(voltage)+"CCu:"+String(current)+"CCo:"+String(consumption)+"END#";
  return message;
}
