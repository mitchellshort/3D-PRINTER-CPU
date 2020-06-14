#include <Arduino.h>

//THERMISTORS
#define THERMISTOR_T1 A0    //enclosure temp
#define THERMISTOR_T2 A1    //connector temp

//RELAYS
#define RELAY_K1 2  //MAINS POWER
#define RELAY_K2 3  //12v LIGHTS

//PINOUT
#define BUTTON_LIGHTS 4 //TOGGLE SWITCH FOR LIGHTS PIN
#define TRIP_LED 5 // LED TRIP PIN

//THERMISTOR DEFINES
#define THERMISTORNOMINAL 14000    //resistance at 25 degrees C  
#define TEMPERATURENOMINAL 25   // temp. for nominal resistance
#define NUMSAMPLES 5          // Samples to create average
#define BCOEFFICIENT 3950   // Beta Coefficient of the Thermistor
#define SERIESRESISTOR 10000      //R2 Value
 
int samples[NUMSAMPLES];
int T1_OFF_TEMP = 45; //HIGHHIGH
int T2_OFF_TEMP = 60; //HIGHHIGH
int T1_WARN = 35; //HIGH
int T2_WARN = 50; //HIGH
float T1_TEMP;
float T2_TEMP;
bool LIGHTS_STATE = 0;
bool TEMP_TRIP = false;
bool TEMP_WARN = false;

const long interval = 1000;
unsigned long previousMillis = 0;
unsigned long previousWarnMillis = 0;

//FUNCTIONS
void alarm();
void warning();
void checkHighTemp();
float checkTemp (char T);

//RUN
void setup(void) {
  Serial.begin(115200);
  delay(1000);
  analogReference(EXTERNAL);
  pinMode(RELAY_K1, OUTPUT);
  pinMode(RELAY_K2, OUTPUT);
  pinMode(BUTTON_LIGHTS, INPUT_PULLUP);
  pinMode(TRIP_LED, OUTPUT);
  digitalWrite(RELAY_K1, HIGH);
  digitalWrite(RELAY_K2, LOW);
}
 
void loop(void) {
  if (!digitalRead(BUTTON_LIGHTS)) {
      digitalWrite(RELAY_K2, !digitalRead(RELAY_K2));
      delay(1000); //DEBOUNCE
  }
  checkHighTemp();
  if(TEMP_TRIP == true){
    alarm();
  } 
  if(TEMP_WARN == true){
    warning();
  }
  delay(50);
}

void alarm(){
    int i;
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

float checkTemp (char T){

  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(T);
   delay(10);
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
  if(T == THERMISTOR_T1){
    Serial.print("Enclosure Temperature: ");
  }else if(T == THERMISTOR_T2){
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

    T1_TEMP = checkTemp(THERMISTOR_T1);
    if (T1_TEMP >= T1_OFF_TEMP) {
      digitalWrite(RELAY_K1, LOW);
      TEMP_TRIP = true;
    }else if (T1_TEMP >= T1_WARN) {
      TEMP_WARN = true;
      Serial.println("T1 - HIGH TEMP WARNING! :: ENCLOSURE");
    }

    T2_TEMP = checkTemp(THERMISTOR_T2);
    if (T2_TEMP >= T2_OFF_TEMP) {
      digitalWrite(RELAY_K1, LOW);
      TEMP_TRIP = true;
    }else if (T2_TEMP >= T2_WARN) {
      TEMP_WARN = true;
      Serial.println("T2 - HIGH TEMP WARNING! :: CONNECTOR");
    }


  }

}
