//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##                                                                                                              ##
//##################################################################################################################

// LIBRARY IMPORTS
#include <rom/rtc.h>
#include <math.h>  // https://www.tutorialspoint.com/c_standard_library/math_h.htm
#include <ctype.h>


#ifndef _WIFI_H
#include <WiFi.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif

#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
#endif



#include "SPI.h"
#include <TFT_eSPI.h> // Hardware-specific library


#include "DHT.h"

#include <Wire.h>
#include <Adafruit_BMP280.h>

// DEFINE VARIABLES
#define ARDUINOJSON_USE_DOUBLE      1 

#define DHTPIN 32
#define DHTTYPE DHT22

#define SoilPin A0


#define BTN1      25
#define BTN2      26
#define BTN3      27

//Object Instantation 
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// MQTT CLIENT CONFIG
static const char* pubtopic = "620156144";                         // Add your ID number here
static const char* subtopic[] = { "620156144_sub", "/elet2415" };  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server = "dbs.msjrealtms.com";             // Broker IP address or Domain name as a String
static uint16_t mqtt_port = 1883;

// WIFI CREDENTIALS
const char* ssid = "MonaConnect";        // Add your Wi-Fi ssid
const char* password = "";  // Add your Wi-Fi password

// TASK HANDLES
TaskHandle_t xMQTT_Connect = NULL;
TaskHandle_t xNTPHandle = NULL;
TaskHandle_t xLOOPHandle = NULL;
TaskHandle_t xUpdateHandle = NULL;
TaskHandle_t xButtonCheckeHandle = NULL;

// FUNCTION DECLARATION   
void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 


//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif

//Global Variables
float  dhtTemperature = 0;
double  bmpTemperature = 0;
float  humidity = 0;
double  pressure = 0;
int     moisture = 0;

//Setup Function
void setup(){

  Serial.begin(115200);
  // initialize();           // INIT WIFI, MQTT & NTP 
  dht.begin();            // Initialise DHT
  bmp.begin(0x76);        // Initialise DHT SPI

  //Initialise tft display 
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setRotation(1);

  /* ENABLE PULL-UP RESISTORS */
  pinMode(BTN1,INPUT_PULLUP);
  pinMode(BTN2,INPUT_PULLUP);
  pinMode(BTN3,INPUT_PULLUP);

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  vButtonCheckFunction(); // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION
  // divideDisplay();

}

void loop(){

  dhtTemperature  = dht.readTemperature();
  bmpTemperature  = bmp.readTemperature(); 
  pressure        = bmp.readPressure()/100;
  humidity        = dht.readHumidity();

  float val   = analogRead(SoilPin);

  moisture    = map(val, 3300, 1500, 0, 100);
      

  if (moisture >= 100) {
    moisture = 100;
  } 
  
  else if (moisture <= 0) {
    moisture = 0;
  }

  Serial.printf("DHT sensor temp: %.2f C\n", dhtTemperature);
  Serial.printf("BMP sensor temp: %.2f C\n", bmpTemperature);
  Serial.printf("Pressure: %.2f hPa\n",     pressure);
  Serial.printf("Humidity: %.2f %\n",       humidity);
  Serial.printf("Moisture: %d %\n",         moisture);

  displayValues();

  // vTaskDelay(1000 / portTICK_PERIOD_MS); 
}

//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );     
      
    for( ;; ) {
        // Add code here to check if a button(S) is pressed
        // then execute appropriate function if a button is pressed  

        // 1. Implement button1  functionality

        // 2. Implement button2  functionality

        // 3. Implement button3  functionality
       
        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }
}

void vUpdate( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
 
    for( ;; ) {


      // Task code goes here.   
      dhtTemperature  = dht.readTemperature();
      bmpTemperature  = bmp.readTemperature();
      pressure        = bmp.readPressure();
      humidity        = dht.readHumidity();

      float val = analogRead(SoilPin);

      moisture = map(val, 3300, 1500, 0, 100);
      

      if (moisture >= 100){
        moisture = 100;
      }

      else if (moisture <= 0){
        moisture = 0;
      }

      // PUBLISH to topic every second.  
      StaticJsonDocument<1000> doc;
      char message[1000] = {0};

      // Add key:value pairs to JSon object
      doc["id"] = "620156144";
      doc["timestamp"] = getTimeStamp();
      doc["dhtTemperature"] = dhtTemperature;
      doc["bmpTemperature"] = bmpTemperature;
      doc["humidity"] = humidity;
      doc["pressure"] = pressure;
      doc["moisture"] = moisture;


      serializeJson(doc, message);  // Seralize / Covert JSon object to JSon string and store in char* array

      if (mqtt.connected()) {
        publish(pubtopic, message);
      }


      vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}

unsigned long getTimeStamp(void) {
          // RETURNS 10 DIGIT TIMESTAMP REPRESENTING CURRENT TIME
          time_t now;         
          time(&now); // Retrieve time[Timestamp] from system and save to &now variable
          return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // ############## MQTT CALLBACK  ######################################
  // RUNS WHENEVER A MESSAGE IS RECEIVED ON A TOPIC SUBSCRIBED TO
  
  Serial.printf("\nMessage received : ( topic: %s ) \n",topic ); 
  char *received = new char[length + 1] {0}; 
  
  for (int i = 0; i < length; i++) { 
    received[i] = (char)payload[i];    
  }

  // PRINT RECEIVED MESSAGE
  Serial.printf("Payload : %s \n",received);

 
  // CONVERT MESSAGE TO JSON


  // PROCESS MESSAGE

}

bool publish(const char *topic, const char *payload){   
     bool res = false;
     try{
        res = mqtt.publish(topic,payload);
        // Serial.printf("\nres : %d\n",res);
        if(!res){
          res = false;
          throw false;
        }
     }
     catch(...){
      Serial.printf("\nError (%d) >> Unable to publish message\n", res);
     }
  return res;
}

void displayValues() {
  tft.fillRect(0, 0, 320, 120, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.printf("DHT sensor temp: %.2f C\n", dhtTemperature);
  tft.printf("BMP sensor temp: %.2f C\n", bmpTemperature);
  tft.printf("Pressure: %.2lf hPa\n", pressure);
  tft.printf("Humidity: %.2f %%\n", humidity);
  tft.printf("Moisture: %d %%\n", moisture);
  delay(675);
}

// void divideDisplay(){
//   tft.drawLine(0,120,320,120,TFT_BLUE);

//   for(unsigned char i = 1; i<3; i++){
//     tft.drawRoundRect(0,0,160,120,TFT_BLUE);
//   }

//   for(unsigned char i = 1; i<3; i++){
//     tft.drawLine(106*i,120,106*i,240,TFT_BLUE);
//   }
// }

