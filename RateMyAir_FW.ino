// MIT License
//
// Copyright(c) 2021 Giovanni Bertazzoni <nottheworstdev@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "src/Config.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <NonBlockingDallas.h>
#include <WarmTheSDS011.h>

//====================================================================
//                        MACHINE STATE
//====================================================================
typedef enum states 
{ 
  IDLING,             //0
  INITIAL_READING,    //1
  UPDATING_SENSORS,   //2
  INITIALIZING_WIFI,  //3
  CONNECTING_WIFI,    //4
  SENDING_DATA        //5
} state;

state _currentState;  //Current state
//====================================================================

float _temperatureOutdoor;
float _temperatureIndoor;
float _humidity;
float _pressure;
float _pm25;
float _pm10;

bool bme280Found = false;
unsigned long _lastSendingMillis;
unsigned long _lastBme280ReadingMillis;
unsigned long _startWifiConnectionMillis;

//==================================================
// DS18B20 Digital Temperature sensors
//==================================================
OneWire oneWire(DS18B20_BUS);
DallasTemperature dallasTemp(&oneWire);
NonBlockingDallas nonBlockingDallas(&dallasTemp);

//==================================================
// SDS011 Particulate matter sensor
//==================================================
WarmTheSDS011 sds011(SDS_RX, SDS_TX);

//==================================================
// BME280 Temperature, Pressure, Humidity sensor
//==================================================
Adafruit_BME280 bme;


//====================================================================
//                        SETUP
//====================================================================
void setup() {

#ifdef DEBUG_INFO
  Serial.begin(9600);
  while (!Serial)
    ;
#endif

  _lastSendingMillis = 0;
  _lastBme280ReadingMillis = 0;
  _startWifiConnectionMillis = 0;
  _temperatureOutdoor = 9999;
  _temperatureIndoor = 9999;
  _humidity = 9999;
  _pressure = 9999;
  _pm25 = 9999;
  _pm10 = 9999;

  //Setup sensors
  SetupBME280();
  delay(100);
  SetupDS18B20();
  delay(100);
  SetupSDS011();
  delay(100);
  
  SetupOverTheAirUpdate();

  //This is necessary to reset the Wifi status
  WiFi.disconnect();

  //Query the sensors to get the initial values 
  //without waiting for their time interval to expire
  sds011.requestPollution();
  nonBlockingDallas.requestTemperature();
  ReadBME208();
  
  //Machine state initial value
  _currentState = INITIAL_READING;
}


//====================================================================
//                        LOOP
//====================================================================
void loop() {
  switch(_currentState){
      case IDLING:
        LoopIdling();
      break;
      case INITIAL_READING:
       LoopInitialReading();
      break;
      case UPDATING_SENSORS:
        LoopUpdatingSensors();
      break;
      case INITIALIZING_WIFI:
        LoopInitializingWifi();
      break;
      case CONNECTING_WIFI:
        LoopConnectingWifi();
      break;
      case SENDING_DATA:
        LoopSendingData();
      break;
    }

    ArduinoOTA.handle();
}


//====================================================================
//                        MACHINE STATE LOOPS
//====================================================================
void LoopIdling(){ }


void LoopInitialReading(){

  UpdateSensors();

  //Once all sensors reading are complete send data to server
  //so I don't have to wait SEND_DATA_INTERVAL to get the first values
  if(_temperatureOutdoor != 9999 &&
    _temperatureIndoor != 9999 &&
    _humidity != 9999 &&
    _pressure != 9999 &&
    _pm25 != 9999 &&
    _pm10 != 9999){
      _currentState = INITIALIZING_WIFI;
    }
}

void LoopUpdatingSensors(){
  UpdateSensors();
  
  //Time to read the BME280
  if(millis() - _lastBme280ReadingMillis >= BME_INTERVAL || _lastBme280ReadingMillis == 0){
    ReadBME208();
    _lastBme280ReadingMillis = millis();
  }
  
  //Time to send new data?
  if((millis() - _lastSendingMillis >= SEND_DATA_INTERVAL) && sds011.isBusy() == false){
    _currentState = SENDING_DATA;
  }
}

void LoopInitializingWifi(){
  SetupWifi();
  _startWifiConnectionMillis = millis();
  _currentState = CONNECTING_WIFI;
}

void LoopConnectingWifi(){

  if(WiFi.status() == WL_CONNECTED){
    _currentState = SENDING_DATA;
  
#ifdef DEBUG_INFO
    Serial.println("Wifi connected!");
#endif 
  }
  else {
    //Check if the Wifi connection time has expired.
    //This is necessary to avoid getting stuck forever in LoopConnectingWifi
    if(millis() - _startWifiConnectionMillis >= WIFI_CONNECTION_TIMEOUT){
      _currentState = UPDATING_SENSORS;

#ifdef DEBUG_INFO
      Serial.println("Wifi connection failed!");
#endif
    } 
  }
}

void LoopSendingData(){
  if(WiFi.status() != WL_CONNECTED){
    _currentState = INITIALIZING_WIFI;
    return;
  }

  SendSensorData();
  _lastSendingMillis = millis();
  _currentState = UPDATING_SENSORS;

#ifdef DEBUG_INFO
  Serial.println("Data sent!");
#endif
}

void UpdateSensors(){
  nonBlockingDallas.update();
  sds011.update();
}


//====================================================================
//                        SETUP DS18B20 TEMPERATURE SENSORS
//====================================================================
void SetupDS18B20(){
  nonBlockingDallas.begin(NonBlockingDallas::resolution_12, NonBlockingDallas::unit_C, DS18B20_INTERVAL);
  nonBlockingDallas.onTemperatureChange(OnTemperatureChange);
}

//====================================================================
//                        SETUP PARTICULATE MATTER SDS011 SENSOR
//====================================================================
void SetupSDS011(){
  sds011.begin(SDS_INTERVAL, SDS_WARMUP_INTERVAL);
  sds011.onIntervalElapsed(OnSdsIntervalElapsed);
}

//====================================================================
//                        SETUP BME280 SENSOR
//====================================================================
void SetupBME280(){
  Wire.begin(BME280_SDA, BME280_SCL);
  Wire.setClock(100000);
  delay(1000);
  
  bme280Found = bme.begin();
  if(bme280Found == false){

#ifdef DEBUG_INFO
  Serial.println("Could not find a valid BME280 sensor, check wiring!");
#endif  
  }
}

//====================================================================
//                        SETUP WIFI
//====================================================================
void SetupWifi(){
  
  // Connecting to WiFi network
#ifdef DEBUG_INFO
  Serial.print("Connecting to Wifi network: ");
  Serial.println(WIFI_SSID);
#endif

  //Available options are:
  //WIFI_AP (only Access Point)
  //WIFI_STA  (only client)
  //WIFI_AP_STA (dual, Access Point and Client. This is the default)
  //WIFI_OFF (turn off wifi).
  WiFi.mode(WIFI_STA);
 
  //Static IP Address
  IPAddress ip(192,168,1,11);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  WiFi.config(ip, gateway, subnet);

  //Start Wifi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

//====================================================================
//                        SETUP OVER THE AIR PROGRAMMING
//====================================================================
void SetupOverTheAirUpdate(){
  
  // Default port is 8266
  // ArduinoOTA.setPort(8266);

  // Default Hostname is esp8266-[ChipID]
   ArduinoOTA.setHostname(HOSTNAME);
  
  // Set a password
  // ArduinoOTA.setPassword("admin");
  // Set a MD5 encrypted password
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    if (ArduinoOTA.getCommand() == U_FLASH) {
    
    } else { // U_SPIFFS
    
    }
  });
  
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
    
    ESP.restart();
  });
  
  ArduinoOTA.begin();
}


//====================================================================
//                        SENDING SENSORS DATA TO SERVER
//====================================================================
void SendSensorData(){
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("ApiKey", API_KEY);
  
  char *json = EncodeJson();
  int httpCode = http.POST(json);
  free(json);                               // !!!!! DO NOT FORGET TO FREE THE MEMORY !!!!!

  //FOR DEBUG
  //String payload = http.getString();
  //Serial.println(httpCode);               //Print HTTP return code
  //Serial.println(payload);                //Print request response payload
  http.end();
}



char* EncodeJson(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& jsonEncoder = jsonBuffer.createObject();

  jsonEncoder["TemperatureOutdoor"] = _temperatureOutdoor;
  jsonEncoder["TemperatureIndoor"] = _temperatureIndoor;
  jsonEncoder["Humidity"] = _humidity;
  jsonEncoder["Pressure"] = _pressure;
  jsonEncoder["Pm25"] = _pm25;
  jsonEncoder["Pm10"] = _pm10;
  
  int jsonLenght = jsonEncoder.measureLength();
  char *buf = (char *)malloc(jsonLenght + 1);
  jsonEncoder.printTo(buf, jsonLenght + 1);
  return buf;
}

void ReadBME208(){
  if(bme280Found == false) return;
  
  float h = bme.readHumidity();
  float p = bme.readPressure();
  bool valid = !isnan(h) && !isnan(p);
  
  if(valid){

    _humidity = h;
    _pressure = p / 100.0F;

#ifdef DEBUG_INFO
    Serial.print("Humidity ");
    Serial.print(_humidity);
    Serial.print(" % Pressure ");
    Serial.print(_pressure);
    Serial.println(" hPa");
#endif
  }
  else {
    
#ifdef DEBUG_INFO
    Serial.println("Failed to read BME280!");
#endif

  }
}

//====================================================================
//                        SENSORS CALLBACKS
//====================================================================

//Invoked ONLY when the temperature changes between two sensor readings
void OnTemperatureChange(float temperature, bool valid, int deviceIndex){

#ifdef DEBUG_INFO
  Serial.print("Temperature sensor ");
  Serial.print(deviceIndex);
  Serial.print(" ");
  Serial.print(temperature);
  Serial.println(" °C");
#endif

  switch(deviceIndex){
    case 0:
      _temperatureIndoor = temperature;
    break;
    case 1:
      _temperatureOutdoor = temperature;
    break; 
  }
}

void OnSdsIntervalElapsed(float pm25, float pm10, bool valid){
    _pm25 = pm25;
    _pm10 = pm10;
}
