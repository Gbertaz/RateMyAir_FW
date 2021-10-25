# RateMyAir_FW

Firmware for the air quality monitoring system project. Runs on the Esp8266 reading the temperature, humidity, air pressure and particulate matter.  
The measurements are stored in an SQLite database by calling the [RateMyAir API](https://github.com/Gbertaz/RateMyAir_API) hosted on a Raspberry Pi 4 which act as a web server. Check out the [step by step guide](https://github.com/Gbertaz/RateMyAir_API#hosting-a-net-core-5-application-on-a-raspberry-pi) on how to set up the Raspberry to host the API.

This is the [flowchart](https://github.com/Gbertaz/RateMyAir_FW/blob/master/images/Flowchart.png) of the implementation. Please check it out to see the details of the machine state algorithm.

# Features

* The sensors measurements are not buffered. That means it always sends to server the last sensor reading
* Sends the measurements to the server by calling a HTTP Post Rest API passing the values as a json string
* Sends the first measurement as soon as it is availble without waiting *SEND_DATA_INTERVAL* to elapse
* The Wifi connection status is checked and re-initialized if necessary before sending data to server (see [flowchart](https://github.com/Gbertaz/RateMyAir_FW/blob/master/images/Flowchart.png))
* Supports Over The Air update
* Doesn't make use of the DNS. You have to assing a static *IP Address*
* All the timings, intervals, Wifi parameters are configurable in [Config.h](https://github.com/Gbertaz/RateMyAir_FW/blob/master/src/Config.h)
* Schematics are coming soon

# Prerequisites

This project uses a couple of libraries that I wrote with the aim to wrap a machine state into an easy and reusable code:

* [NonBlockingDallas](https://github.com/Gbertaz/NonBlockingDallas)
* [WarmTheSDS011](https://github.com/Gbertaz/WarmTheSDS011)

The following libraries are also required:

* ESP8266HTTPClient
* ESP8266mDNS
* WiFiUdp
* ArduinoOTA
* ESP8266WiFi
* OneWire
* Wire
* DallasTemperature
* ArduinoJson
* Adafruit_Sensor
* Adafruit_BME280

# Usage

Edit the following parameters in [Config.h](https://github.com/Gbertaz/RateMyAir_FW/blob/master/src/Config.h) to match your LAN configuration:

```
WIFI_SSID
WIFI_PASSWORD
API_URL
API_KEY
```

The *API_KEY* can be found in the [RateMyAir API](https://github.com/Gbertaz/RateMyAir_API) [*appsettings.json*](https://github.com/Gbertaz/RateMyAir_API/blob/master/RateMyAir/RateMyAir.API/appsettings.Production.json) configuration file. Replace it with your own key.

Finally, in [RateMyAir_FW.ino](https://github.com/Gbertaz/RateMyAir_FW/blob/master/RateMyAir_FW.ino) *SetupWifi()* function, edit the IP, Gateway and Subnet mask according to your LAN configuration

```
  IPAddress ip(192,168,1,11);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
```

# Hardware

* Esp8266 Wemos D1 Mini
* BME280 I2C Humidity, Temperature and Pressure sensor
* Number 2 DS18B20 Digital Temperature sensors
* [Nova Fitness SDS011](http://inovafitness.com/en/a/chanpinzhongxin/95.html) particulate matter sensor
* Resistor 4.7 K-Ohm
