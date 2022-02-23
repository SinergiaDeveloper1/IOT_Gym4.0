#include "header.h"

#include <ESP8266WiFiMulti.h>
 
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "Vodafone-A47203440"
#define WIFI_PASSWORD "HtmMgyffEM4Mf4cH"
#define INFLUXDB_URL "http://93.186.254.118:8086"

#define INFLUXDB_TOKEN "7q44Rz0f0IZYM4SYguqyPB5RPafXPEagZUpRuIUBp3aoDT3HVQzFg5c0Hg_RY8Khk8cH8MjuApdyQsKrFyaF4w=="
#define INFLUXDB_ORG "uniurb"
#define INFLUXDB_BUCKET "test"

ESP8266WiFiMulti wifiMulti;
InfluxDBClient client(INFLUXDB_URL, 
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET, 
                      INFLUXDB_TOKEN, 
                      InfluxDbCloud2CACert);

Point sensor("progetto_LC");

void initInfluxdb() {
  
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  /* ciclo i tentativi di connessione */
  while (wifiMulti.run() != WL_CONNECTED) {
    
    delay(2000);
    
  }
    
  sensor.addTag("host", "ESP8266_Co2");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "palestra");
  
}

void sendInfluxdb() {
  
  sensor.clearFields();

  sensor.addField("Temperatura_Interna_1", getT());
  sensor.addField("Umidità_Interna_1", getH());
  
  if(getEtvco() > 10)
    sensor.addField("etvco", getEtvco());
  if(getEco2() > 10)
    sensor.addField("eco2", getEco2());

  /* invio il dato */
  client.writePoint(sensor);

}
