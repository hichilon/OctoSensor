
/*
  Include Libraries
*/
#include <SoftwareSerial.h>
#include "Arduino.h"
#include <DHT.h>
#include <DataPacket.h>
#include "AirQuality.h"

/*
  Define Constants
*/
#define POWER_ON_STATE             0
#define READ_SENSOR_STATE          1
#define SAVE_DATA_STATE            2
#define PACKET_LENGTH              5
#define D                          1000
#define DUST_SENSOR_PIN            8
#define TEMP_HUMIDITY_SENSOR_PIN   4
#define AIRQUALITY_PIN             14

#define STATE(s, fun) if (guard && state == s) { guard = false; state = fun;} //State Macro

DHT DHT_SENSOR (TEMP_HUMIDITY_SENSOR_PIN, 22);
AirQuality AIRQUALITY_SENSOR;
DataPacket PACKET;
SoftwareSerial OpenLog(2, 3); // RX, TX

int current_airquality = -1;
unsigned long starttime;
unsigned long duration;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

void setup () {
  Serial.begin (9600);
  
  //Temp and Humidity
  DHT_SENSOR.begin();
  
  //Dust Sensor
  pinMode(DUST_SENSOR_PIN,INPUT);
  starttime = millis();//get the current time;
  
  //Air Quality
  AIRQUALITY_SENSOR.init(AIRQUALITY_PIN);
  
  OpenLog.begin(9600);
}

//Get sensor data
void get_temp(DataPacket& PACKET){
    float temp = DHT_SENSOR.readTemperature();
    PACKET.set_temp(temp);
}
void get_humid(DataPacket& PACKET){
    float humid = DHT_SENSOR.readHumidity();
    PACKET.set_humid(humid);
  }
  
void get_dust(DataPacket& PACKET){
  duration = pulseIn(DUST_SENSOR_PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;

  if ((millis()-starttime) > sampletime_ms)//if the sampel time == 30s
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    
    PACKET.set_lowpulseoccupancy(lowpulseoccupancy);
    PACKET.set_ratio(ratio);
    PACKET.set_concentration(concentration);
    
    lowpulseoccupancy = 0;
    starttime = millis();
  }
}
void get_airquality(DataPacket& PACKET,AirQuality& AIRQUALITY_SENSOR){
    current_airquality=AIRQUALITY_SENSOR.slope();
    if (current_airquality >= 0){// if a valid data returned.
        if (current_airquality==0)
            PACKET.set_airquality(0);
        else if (current_airquality==1)
            PACKET.set_airquality(1);
        else if (current_airquality==2)
            PACKET.set_airquality(2);
        else if (current_airquality ==3)
            PACKET.set_airquality(3);
    }
}
//Function Definitions
int init_error_state () {
  Serial.println("We should never be here.");
  OpenLog.println("We should never be here.");
  return -2;
}


int power_on () {
  Serial.println("Powering up.");
  OpenLog.println("Powering up.");
  return READ_SENSOR_STATE;
}

int read_sensor (DataPacket& PACKET,AirQuality& AIRQUALITY_SENSOR) {
  Serial.println("Reading data.");
  OpenLog.println("Reading data.");
  delay(D);
  get_temp(PACKET);
  get_humid(PACKET);
  get_dust(PACKET);
  get_airquality(PACKET,AIRQUALITY_SENSOR);
  
  return SAVE_DATA_STATE;
}

int show_data (DataPacket& PACKET,SoftwareSerial& OpenLog) {
  PACKET.print_packet(OpenLog);
  return POWER_ON_STATE;
}

//The Main Loop
void loop () {
  // Only initialize the first time through the loop
  static int state = POWER_ON_STATE;
  boolean guard = true;
  
  STATE (-1, init_error_state());
  STATE (POWER_ON_STATE,     power_on());
  STATE (READ_SENSOR_STATE,   read_sensor(PACKET,AIRQUALITY_SENSOR));
  STATE (SAVE_DATA_STATE,  show_data(PACKET,OpenLog));
}

ISR(TIMER2_OVF_vect){
  if(AIRQUALITY_SENSOR.counter==122){//set 2 seconds as a detected duty
      AIRQUALITY_SENSOR.last_vol=AIRQUALITY_SENSOR.first_vol;
      AIRQUALITY_SENSOR.first_vol=analogRead(A0);
      AIRQUALITY_SENSOR.counter=0;
      AIRQUALITY_SENSOR.timer_index=1;
      PORTB=PORTB^0x20;
  }else{
      AIRQUALITY_SENSOR.counter++;
  }
}
