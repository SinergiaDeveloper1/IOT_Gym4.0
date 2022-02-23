#include <WiFi.h>             /* inclusione della libreria per il WiFi */
#include <DHT.h>              /* inclusione della libreria per il sensore */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */

/* costanti per le connessioni */
#define WIFI_SSID       "Vodafone-A47203440"
#define WIFI_PASSWORD   "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL    "http://93.186.254.118:8086"
#define INFLUXDB_ORG    "uniurb"
#define INFLUXDB_BUCKET "test"
#define INFLUXDB_TOKEN  "7q44Rz0f0IZYM4SYguqyPB5RPafXPEagZUpRuIUBp3aoDT3HVQzFg5c0Hg_RY8Khk8cH8MjuApdyQsKrFyaF4w=="

/* costanti per l'accesso a DH11 */
#define DHTPIN  4
#define DHTTYPE DHT22

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL, 
                      INFLUXDB_ORG, 
                      INFLUXDB_BUCKET, 
                      INFLUXDB_TOKEN, 
                      InfluxDbCloud2CACert);

/* definisco la tabella dove inserire i dati */
Point sensor("progetto_LC");
/* creo l'istanza di DHT22 */
DHT dht(DHTPIN, DHTTYPE);

/* variabili delle misure */
double t = 0.0;
double h = 0.0;

void setup() {

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
  while (WiFi.status() != WL_CONNECTED) {
    
    delay(1000);
    
  }

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "ESP_LC");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  client.validateConnection();
  
}

void loop() {
  
  readSensors();
  writeToInfluxDb();
  delay(30000);
  
}

/* funzione che legge dal sensore DHT11 */
void readSensors(){
  t = dht.readTemperature();
  h = dht.readHumidity();
}

void writeToInfluxDb() {
  
  sensor.clearFields();
  
  sensor.addField("Temperatura_Interna_2", t);
  sensor.addField("Umidità_Interna_2", h);

  client.writePoint(sensor);
  
}
