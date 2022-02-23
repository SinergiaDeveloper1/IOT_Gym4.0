#include <WiFi.h> /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */
#define WIFI_SSID       "Vodafone-A47203440" //"Vodafone-A47203440_2GEXT"
#define WIFI_PASSWORD   "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL    "http://93.186.254.118:8086"
#define INFLUXDB_ORG    "uniurb" 
#define INFLUXDB_BUCKET "test"
#define INFLUXDB_TOKEN  "7q44Rz0f0IZYM4SYguqyPB5RPafXPEagZUpRuIUBp3aoDT3HVQzFg5c0Hg_RY8Khk8cH8MjuApdyQsKrFyaF4w=="

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL, 
                      INFLUXDB_ORG, 
                      INFLUXDB_BUCKET, 
                      INFLUXDB_TOKEN, 
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Connecting to WiFi ..");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  
  Serial.println(WiFi.localIP());
  Serial.println("Connesso");

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "Accelerometro_SX");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  /* testo la connessione */
  if (client.validateConnection()) {
    Serial.println("Connected to influxDb");
  } else {
    Serial.println("Connection failed");
    Serial.println(client.getLastErrorMessage());
  }

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  Serial.println("MPU6050 Found!");

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("");
  delay(100);
  
}

/* variabili globali di elaborazione dati */
float arrayTrigger[10];

/* tare accelerometro SX */
float taraX = 0.16;
float taraY = 0.09;
float taraZ = 10.02;

/* valori accelerazione */
float aX, aY, aZ;

/* contatori */
int c1 = 0;

void loop() {

  readSensors();  /* legge i sensori */

  
  
  //writeToInfluxDb();
  delay(70);
  
}

/* funzione che legge dal sensore */
void readSensors(){

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  aX = a.acceleration.x - taraX;
  aY = a.acceleration.y - taraY;
  aZ = a.acceleration.z - taraZ;




  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(aX);
  
  Serial.print(", Y: ");
  Serial.print(aY);
  
  Serial.print(", Z: ");
  Serial.print(aZ);
  
  Serial.println(" m/s^2");

/*
  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC"); */
  
}

void writeToInfluxDb() {

  /*
  sensor.clearFields();
  sensor.addField("temperature", t);
  sensor.addField("humidity", h);
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if(!client.writePoint(sensor)){
    Serial.print("InfluxDB write failed ");
    Serial.println(client.getLastErrorMessage());
  }
  */
  
}
