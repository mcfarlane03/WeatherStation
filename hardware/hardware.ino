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



// MQTT CLIENT CONFIG
static const char* pubtopic = "620156144";                         // Add your ID number here
static const char* subtopic[] = { "620156144_sub", "/elet2415" };  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server = "www.yanacreations.com";             // Broker IP address or Domain name as a String
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
bool isNumber(double number);

void displayValues(double dhtTemperature, double bmpTemperature, double pressure, double humidity, int moisture);


//Object Instantation 
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);


//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif

//Setup Function
void setup(){

  Serial.begin(115200);
  dht.begin();            // Initialise DHT
  bmp.begin(0x76);        // Initialise DHT SPI

  //Initialise tft display 
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setRotation(1);
  img.createSprite(320,120);
  img.setTextSize(2);

  /* ENABLE PULL-UP RESISTORS */
  pinMode(BTN1,INPUT_PULLUP);
  pinMode(BTN2,INPUT_PULLUP);
  pinMode(BTN3,INPUT_PULLUP);

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  // vButtonCheckFunction(); // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION
  initialize();           // INIT WIFI, MQTT & NTP 

}

void loop(){

  vTaskDelay(1000 / portTICK_PERIOD_MS); 
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
      double temperature     = dht.readTemperature();
      double pressure        = bmp.readPressure()/100;
      double humidity        = dht.readHumidity();

      int moisture = map(analogRead(SoilPin), 3300, 1500, 0, 100);
      

      if (moisture >= 100){
        moisture = 100;
      }

      else if (moisture <= 0){
        moisture = 0;
      }

      displayValues(temperature, pressure, humidity, moisture);


      if (isNumber(temperature)){
        

        // PUBLISH to topic every second.  
        StaticJsonDocument<1000> doc;
        char message[1000] = {0};

        // Add key:value pairs to JSon object
        doc["id"] = "620156144";
        doc["timestamp"] = getTimeStamp();
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["pressure"] = pressure;
        doc["moisture"] = moisture;


        serializeJson(doc, message);  // Seralize / Covert JSon object to JSon string and store in char* array

        if (mqtt.connected()) {
          publish(pubtopic, message);
        }

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

void displayValues(double dhtTemperature, double pressure, double humidity, int moisture) {
  img.fillRect(0, 0, 320, 120, TFT_BLACK);
  img.setCursor(0, 0);
  img.printf("DHT sensor temp: %.2f C\n", dhtTemperature);
  img.printf("Pressure: %.2lf hPa\n", pressure);
  img.printf("Humidity: %.2f %%\n", humidity);
  img.printf("Moisture: %d %%\n", moisture);
  
  img.pushSprite(0,0);
}

bool isNumber(double number) {
  char item[20];
  snprintf(item, sizeof(item), "%f\n", number);
  if (isdigit(item[0]))
    return true;
  return false;
}
