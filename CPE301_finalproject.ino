// Names: Christopher Long,Emerson Fleming,Shane Petree,Isabelle Amanuel
// Class: CPE 301 Section 1001
// Date: December 15,2023
// Assignment: Final Project

//Preprocessing Directives
#include <dht_nonblocking.h>
#include <TimeLib.h>
#include <Stepper.h>
#include <LiquidCrystal.h>
#define DHT_SENSOR_TYPE DHT_TYPE_11

int steps = 0;
const int stepsRev = 25;
Stepper myStepper(stepsRev,22,24,26,28);
LiquidCrystal lcd(7,8,9,10,11,12);
int hisVal = 0;
int adcId = 0;
int start = 0;
int error = 0;
int printed = 0;
int idlePrinted = 0;
char printBuffer[128];
static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN,DHT_SENSOR_TYPE);
//Ports
volatile unsigned char* pB = (unsigned char*) 0x25;
volatile unsigned char* pD = (unsigned char*) 0x2B;
volatile unsigned char* pE = (unsigned char*) 0x2E;
volatile unsigned char* pL = (unsigned char*) 0x10B;
//DDRs
volatile unsigned char* ddrB  = (unsigned char*) 0x24;
volatile unsigned char* ddrC  = (unsigned char*) 0x27;
volatile unsigned char* ddrE  = (unsigned char*) 0x2D;
volatile unsigned char* ddrL  = (unsigned char*) 0x10A;
//Pins
volatile unsigned char* pinC  = (unsigned char*) 0x26;

//Initialize values at startup
void setup(){
  Serial.begin(9600);
  lcd.begin(16,2);
  *ddrL |= 0x82;
  *ddrE |= 0x28;
  *ddrB |= 0xA;
  *ddrC &= 0xF0;
  *pL &= 0x0;
  *pB &= 0x0;
  *pL |= 0x80;
  *pE &= 0x0;
  *pE |= 0b00000100;
}

//Repeat this until machine is done being used
void loop(){
  //Idle
  if(*pinC & 0x1){
    Serial.print("Idle," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
    printed = 0;
    idlePrinted = 0;
    *pB &= 0x0;
    *pL &= 0x0;
    *pB |= 0x8;
    *pE &= 0x0;
    start = 1;
  }
  //Machine starts
  while(start==1){
    int val = analogRead(adcId);
    if(((hisVal>=val)&&((hisVal-val)>10))||((hisVal<val)&&((val-hisVal)>10))){
      if(val < 200){
        Serial.print("Error," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
        printed = 0;
        idlePrinted = 0;
        *pB &= 0x0;
        *pD &= 0x0;
        *pL |= 0x2;
        *pE &= 0x0;
        error = 1;
        start = 0;
      }
      hisVal = val;
    }
    int val2 = analogRead(adcId);
    float temp;
    float hum;
    static unsigned long time = millis();
    //Show temperature and humidity
    if(millis() - time > 3000ul){
      if(dht_sensor.measure(temp,hum)==1){
        time = millis();
        lcd.setCursor(0,0);
        lcd.print("Temperature: " + String(temp,1) + char(176) + "C");
        lcd.setCursor(0,1);
        lcd.print("Humidity: " + String(hum,1) + "%");
      }
    }
    //Idle if temperature is low enough
    if(temp < 32){
      if(idlePrinted != 0){
        Serial.print("Idle," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
        idlePrinted = 1;
      }
      printed = 0;
      *pB &= 0x0;
      *pL &= 0x0;
      *pB |= 0x8;
      *pE &= 0x0;
    }
    //Run if temperature is high enough
    if(temp > 32 && val > 200){
      if(printed != 1){
        Serial.print("Running," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
        printed = 1;
      }
      idlePrinted = 0;
      *pB &= 0x0;
      *pL &= 0x0;
      *pB |= 0x2;
      *pE |= 0b00101000;
    }
    //Take steps continuously
    while(*pinC & 0x4){
      myStepper.step(1);
      steps++;
      Serial.print("Take step," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
      delay(50);
    }
    //Disable
    if(*pinC & 0x2){
      Serial.print("Disabled," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');   
      printed = 0;
      idlePrinted = 0;
      *pB &= 0x0;
      *pL &= 0x0;
      *pL |= 0x80;
      *pE &= 0x0;
      start = 0;
    }
  }
    //Needs more water
    if(error==1){
      lcd.setCursor(0,0);
      lcd.print("Needs more water");
    }
    //Disable
    if(*pinC & 0x2){
      Serial.print("Disabled," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
      printed = 0;
      idlePrinted = 0;    
      *pB &= 0x0;
      *pL &= 0x0;
      *pL |= 0x80;
      error = 0;
    }
    //Disable
    if(*pinC & 0x8 && error==1){
      Serial.print("Diabled," + String(hour(),DEC) + ':' + String(minute(),DEC) + ':' + String(second(),DEC) + '\n');
      printed = 0;  
      idlePrinted = 0;  
      *pB &= 0x0;
      *pL &= 0x0;
      *pB |= 0x8;
      error = 0;
      start = 1;
    }
}
