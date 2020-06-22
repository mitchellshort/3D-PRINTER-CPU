
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>


//THERMISTORS
#define THERMISTOR_T1 A0    //enclosure temp
#define THERMISTOR_T2 A1    //connector temp
#define FLAME_IR A2         //ir flame detector

//RELAYS
#define RELAY_K1 3  //MAINS POWER
#define RELAY_K2 2  //12v LIGHTS
#define RELAY_K3 6  //CO2 RELEASE

//PINOUT
#define BUTTON_LIGHTS 4 //TOGGLE SWITCH FOR LIGHTS PIN
#define TRIP_LED 5 // LED TRIP PIN

//THERMISTOR DEFINES
#define THERMISTORNOMINAL 8000    //resistance at 25 degrees C  
#define TEMPERATURENOMINAL 25   // temp. for nominal resistance
#define NUMSAMPLES 5          // Samples to create average
#define BCOEFFICIENT 2500   // Beta Coefficient of the Thermistor
#define SERIESRESISTOR 9000      //R2 Value
 
int samples[NUMSAMPLES];
int T1_OFF_TEMP = 50; //HIGHHIGH
int T2_OFF_TEMP = 60; //HIGHHIGH
int T1_WARN = 40; //HIGH
int T2_WARN = 50; //HIGH
int T_REASON = 0; //0 no trip, 1 overheat, 2 flame
int T_TRIPPED; //Which thermistor tripped
float T1_MAX; //record max temp of thermistor 1
float T2_MAX; //record max temp of thermmistor 2
float T1_TEMP; //current temp
float T2_TEMP; //current temp
bool LIGHTS_STATE = 0;
bool FLAME_TRIP = false;
bool TEMP_TRIP = false;
bool TEMP_WARN = false;
bool CO2_TIMER_START = false;

const long interval = 1000;
const long CO2_DELAY = 8000;
unsigned long CO2_TIME = 0;
unsigned long previousMillis = 0;
unsigned long previousWarnMillis = 0;

const int sensorMin = 0;     // flame sensor minimum
const int sensorMax = 1024;  // flame sensor maximum


U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0); 

//RUN
void setup(void) {
  Serial.begin(57600);
  delay(1000);
  Serial.println("Booting...");
  analogReference(DEFAULT);
  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
  u8g2.drawStr(10,29,"HELLO.");  // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display
  delay(2000);
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.sendBuffer();
  pinMode(RELAY_K1, OUTPUT);
  pinMode(RELAY_K2, OUTPUT);
  pinMode(RELAY_K3, OUTPUT);
  pinMode(BUTTON_LIGHTS, INPUT_PULLUP);
  pinMode(TRIP_LED, OUTPUT);

  digitalWrite(RELAY_K1, HIGH);
  digitalWrite(RELAY_K2, LOW);
  digitalWrite(RELAY_K3, LOW);
}
 
void loop(void) {
  
  if (!digitalRead(BUTTON_LIGHTS)) {
      digitalWrite(RELAY_K2, !digitalRead(RELAY_K2));
      delay(1000); //DEBOUNCE
  }
  
  u8g2.clearBuffer();          // clear the internal memory
  
  checkHighTemp();

  if(TEMP_TRIP == true || FLAME_TRIP == true){
    alarm();
    
  } 
  if(TEMP_WARN == true){
    warning();
  }

  checkFlame();
  CO2();

  delay(10);
}

void CO2(){
  if(CO2_TIMER_START == true){
    
    unsigned long currentMillis = millis();
    if(currentMillis - CO2_TIME > CO2_DELAY){
      digitalWrite(RELAY_K3, HIGH);
      Serial.println("CO2 RELEASED");
    }
  }
  
}

void checkFlame(){
  int sensorReading = analogRead(FLAME_IR);
  //Serial.println(sensorReading);
  int range = map(sensorReading, sensorMin, sensorMax, 0, 3);
  if(FLAME_TRIP != true){
    // range value:
    switch (range) {
    case 0:    // A fire closer than 1.5 feet away.
      FLAME_TRIP = true;
      T_REASON = 2;
      digitalWrite(RELAY_K1, LOW);
      CO2_TIMER_START = true;
      CO2_TIME = millis();
      Serial.println("CO2 Release Timer Active!");
      break;
    case 1:    // A fire between 1-3 feet away.
      FLAME_TRIP = true;
      T_REASON = 2;
      digitalWrite(RELAY_K1, LOW);
      CO2_TIMER_START = true;
      CO2_TIME = millis();
      Serial.println("CO2 Release Timer Active!");
      break;
    case 2:    // No fire detected.
      FLAME_TRIP = false;
      break;
    }
  }
  
}
void printVars(float T1C, float T2C){
  if(FLAME_TRIP != true){
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb16_mf);
    u8g2.setCursor(0,22);
    u8g2.print(T1C);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(73,10);
    u8g2.print(F("'C Enclosure"));
    u8g2.setCursor(73,20);
    u8g2.print(F("Temperature"));
  }else{
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_inb16_mf);
    u8g2.setCursor(0,22);
    u8g2.print(F("FIRE"));
  }
  u8g2.sendBuffer();          // transfer internal memory to the display
}


void alarm(){
    int i;
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(0,32);
    if(T_REASON == 1){ //HEAT TRIP
      if(T_TRIPPED == 1){
        u8g2.print(F("TEMP TRIP::ENCLOSURE"));
      }else if(T_TRIPPED == 2){
        u8g2.print(F("TEMP TRIP::XT90"));
      }
    }else if(T_REASON == 2){
        u8g2.print(F("FLAME DETECTED - CO2 ACTIVATED"));
    }
    
    u8g2.sendBuffer();  
      
    if (TEMP_TRIP == true)
    {
        for(i = 0; i < 255; i = i + 2) { analogWrite(TRIP_LED, i); delay(10); } for(i = 255; i > 1; i = i - 2)
        {
            analogWrite(TRIP_LED, i);
            delay(5);
        }
        for(i = 1; i <= 10; i++)
        {
            analogWrite(TRIP_LED, 200);
            delay(100);
            analogWrite(TRIP_LED, 25);
            delay(100);
        }
    }
    if(FLAME_TRIP == true){
      for(i = 1; i <= 2; i++)
        {
            analogWrite(TRIP_LED, 100);
            delay(250);
            analogWrite(TRIP_LED, 500);
            delay(250);
        }
    }
}

void warning(){
  int i;
  for(i = 0; i < 150; i = i+2){
    analogWrite(TRIP_LED, i);
    delay(5);
  }
  analogWrite(TRIP_LED, 0);

  TEMP_WARN = false;
}

float checkTemp (int T){

  uint8_t i;
  float average;
  //Serial.print("T = ");
  //Serial.println(T);
  if(T == 1){
    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++) {
     samples[i] = analogRead(A0);
     //Serial.println(analogRead(A0));
     delay(10);
    }
  }
  if(T == 2){
    for (i=0; i< NUMSAMPLES; i++) {
     samples[i] = analogRead(A1);
     delay(10);
    }  
  }
  
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  //Serial.print("Average analog reading "); 
  //Serial.println(average);
  
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  //Serial.print("Thermistor resistance "); 
  //Serial.println(average);
  
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  String serialT = T + " Temperature: ";
  if(T == 1){
    Serial.print("Enclosure Temperature: ");
  }else if(T == 2){
    Serial.print("Connector Temperature: ");
  }else{
    Serial.print("Temperature: "); 
  }
  Serial.print(steinhart);
  Serial.println(" *C");

  return steinhart;
  
}

void checkHighTemp() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.println();
    T1_TEMP = checkTemp(1);
    
    if (T1_TEMP >= T1_OFF_TEMP) {
      digitalWrite(RELAY_K1, LOW);

      if(TEMP_TRIP != true){
        Serial.println("T2 - HIGH TEMP WARNING! :: ENCLOSURE");
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.setCursor(0,32);
        u8g2.print(F("ENCLOSURE TEMP - HIGHHIGH"));
        u8g2.sendBuffer();          // transfer internal memory to the display
      }
      
      TEMP_TRIP = true;
      T_TRIPPED = 1;
      T_REASON = 1;
      
    }else if (T1_TEMP >= T1_WARN) {
      TEMP_WARN = true;
      if(TEMP_TRIP != true){
        Serial.println("T1 - HIGH TEMP WARNING! :: ENCLOSURE");
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.setCursor(0,32);
        u8g2.print(F("ENCLOSURE TEMP - HIGH"));
        u8g2.sendBuffer();          // transfer internal memory to the display
      }
    }
    if(T1_MAX < T1_TEMP){
      T1_MAX = T1_TEMP;
    }

    T2_TEMP = checkTemp(2);
    if (T2_TEMP >= T2_OFF_TEMP) {
      
      digitalWrite(RELAY_K1, LOW);
      
      if(TEMP_TRIP != true){
        Serial.println(F("T2 - HIGH TEMP WARNING! :: CONNECTOR"));
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.setCursor(0,32);
        u8g2.print(F("XT90 TEMP - HIGHHIGH"));
        u8g2.sendBuffer();          // transfer internal memory to the display
      }
      
      T_TRIPPED = 2;
      T_REASON = 1;
      TEMP_TRIP = true;
      
    }else if (T2_TEMP >= T2_WARN) {
      TEMP_WARN = true;
      if(TEMP_TRIP != true){
        Serial.println(F("T2 - HIGH TEMP WARNING! :: CONNECTOR"));
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.setCursor(0,32);
        u8g2.print(F("XT90 TEMP - HIGH"));
        u8g2.sendBuffer();          // transfer internal memory to the display
      }
    }
    if(T2_MAX < T2_TEMP){
      T2_MAX = T2_TEMP;
    }

    printVars(T1_TEMP,T2_TEMP);
  }
}
