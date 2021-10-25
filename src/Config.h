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

#ifndef CONFIG_H
#define CONFIG_H

//#define DEBUG_INFO

#define SDS_RX D1									//Particulate matter sensor RX Pin
#define SDS_TX D2									//Particulate matter sensor TX Pin
#define BME280_SDA D3								//Bme280 I2C data Pin
#define BME280_SCL D4								//Bme280 I2C clock Pin
#define DS18B20_BUS 14								//One Wire bus Pin D5 (14)

//Timing
const long BME_INTERVAL = 60 * 1000;				//60 seconds
const long DS18B20_INTERVAL = 60 * 1000;			//60 seconds
const long SDS_INTERVAL = 10 * 60 * 1000;			//10 minutes
const long SDS_WARMUP_INTERVAL = 30 * 1000;			//30 seconds
const long WIFI_CONNECTION_TIMEOUT = 30 * 1000;		//30 seconds
const long SEND_DATA_INTERVAL = 10 * 60 * 1000;		//10 minutes

//Network configuration
const char* WIFI_SSID = "YOUR-WIFI-SSID";
const char* WIFI_PASSWORD = "YOUR-WIFI-PASSWORD";
const char* HOSTNAME = "RateMyAir_Sensor_1";

//RateMyAir_API URL
const char* API_URL = "http://192.168.1.10/ratemyair/api/airquality";

//Get authentication ApiKey from RateMyAir_API repository => RateMyAir.API => appsettings.json
const char* API_KEY = "YOUR-API-KEY";

#endif