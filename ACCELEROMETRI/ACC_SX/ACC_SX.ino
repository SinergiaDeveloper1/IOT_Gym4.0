#include <Stepper.h>

#include <WiFi.h> /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */
#define WIFI_SSID       "Vodafone-A47203440_2GEXT"
#define WIFI_PASSWORD   "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL    "http://93.186.254.118:8086"
#define INFLUXDB_ORG    "uniurb"
#define INFLUXDB_BUCKET "test"
#define INFLUXDB_TOKEN  "7q44Rz0f0IZYM4SYguqyPB5RPafXPEagZUpRuIUBp3aoDT3HVQzFg5c0Hg_RY8Khk8cH8MjuApdyQsKrFyaF4w=="

#define D_MISURE 10

/* in questa versione ho rimosso la tara sugli accelerometri */

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL,
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN,
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

void setup()
{

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "Accelerometro_SX");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  /* testo la connessione */
  client.validateConnection();

  if (!mpu.begin())
  {
    while (1);
  }

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  /* prima di passare al loop, aspetto 5 secondi */
  delay(5000);
}

/* variabili globali di elaborazione dati */
float AccX[D_MISURE];
float AccY[D_MISURE];
float AccZ[D_MISURE];

float aX, aY, aZ; /* valori accelerazione */

/* contatori */
int cRaccoltaDati = 0;

/* INIZIO LOOP */
void loop()
{

  readSensors();

  /* raccolgo i dati */
  if (cRaccoltaDati < D_MISURE)
  {

    /* raccolgo i dati della discesa */
    AccX[cRaccoltaDati] = aX;
    AccY[cRaccoltaDati] = aY;
    AccZ[cRaccoltaDati] = aZ;

    cRaccoltaDati++;
  }
  else
  {

    /* elaboro i dati e li invio */
    cRaccoltaDati = 0;
    writeToInfluxDb(elaboraDatoMedio(), 0);
    writeToInfluxDb(elaboraDatoMax(), 1);

    delay(10);
  }
}

float elaboraDatoMedio()
{

  float accelerazioneMediaTot = 0.0;
  float accX = 0.0;
  float accY = 0.0;
  float accZ = 0.0;

  for (byte i = 0; i < D_MISURE; i++)
  {
    accX += AccX[i];
    accY += AccY[i];
    accZ += AccZ[i];
  }

  accX /= D_MISURE;
  accY /= D_MISURE;
  accZ /= D_MISURE;
  // Serial.println("accX: " + String(accX, 4));
  // Serial.println("accY: " + String(accY, 4));
  // Serial.println("accZ: " + String(accZ, 4));

  accelerazioneMediaTot = sqrt((accX * accX) + (accY * accY) + (accZ * accZ)) - 9.81;
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;

  return (accelerazioneMediaTot);
}

float elaboraDatoMax()
{

  float accelerazioneMax = 0.0;
  float perMax[D_MISURE];

  for (byte i = 0; i < D_MISURE; i++)
  {

    perMax[i] = sqrt((AccX[i] * AccX[i]) + (AccY[i] * AccY[i]) + (AccZ[i] * AccZ[i])) - 9.81;

    if (perMax[i] > accelerazioneMax)
    {
      accelerazioneMax = perMax[i];
    }
  }

  // Serial.println("accX: " + String(accX, 4));
  // Serial.println("accY: " + String(accY, 4));
  // Serial.println("accZ: " + String(accZ, 4));

  accelerazioneMax = roundf(accelerazioneMax * 10000) / 10000;

  return (accelerazioneMax);
}

/* funzione che legge dal sensore */
void readSensors()
{

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  aX = a.acceleration.x;
  aY = a.acceleration.y;
  aZ = a.acceleration.z;
}

/* una volta scrivo il valore medio, una volta il max */
void writeToInfluxDb(float a, int flgMedia_Max)
{

  if (flgMedia_Max == 0)
  {

    sensor.clearFields();
    sensor.addField("Accelerazione_Media_SX", a);
  }
  else
  {

    sensor.clearFields();
    sensor.addField("Accelerazione_Max_SX", a);
  }

  client.writePoint(sensor);
  
}