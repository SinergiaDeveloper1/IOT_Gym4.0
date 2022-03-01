#include <Stepper.h>

#include <WiFi.h>           /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h> /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>  /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */
#define WIFI_SSID "Vodafone-A47203440"
#define WIFI_PASSWORD "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL "http://93.186.254.118:8086"
#define INFLUXDB_ORG "uniurb"
#define INFLUXDB_BUCKET "esercitazioni"
#define INFLUXDB_TOKEN "0s5ZuEkoUpmjulLxgpZfnXeiN1RU5tafsDkZ_bSdB-DzeDXQeN8k03ylXMREBwqYKd46oLq0Se8Qc13IjOuF-A=="

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

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED)
  {
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
  if (client.validateConnection())
  {
    Serial.println("Connected to influxDb");
  }
  else
  {
    Serial.println("Connection failed");
    Serial.println(client.getLastErrorMessage());
  }

  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  Serial.println("MPU6050 Found!");

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("");

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

    /* stampo sulla seriale i valori raccolti */
    Serial.print("Accelerazione X: ");

    for (byte i = 0; i < D_MISURE; i++)
    {
      Serial.print(AccX[i]);
      Serial.print(" ");
    }

    Serial.println("");

    Serial.print("Accelerazione Y: ");

    for (byte i = 0; i < D_MISURE; i++)
    {
      Serial.print(AccY[i]);
      Serial.print(" ");
    }

    Serial.println("");

    Serial.print("Accelerazione Z: ");
    for (byte i = 0; i < D_MISURE; i++)
    {
      Serial.print(AccZ[i]);
      Serial.print(" ");
    }

    Serial.println("");

    /* elaboro i dati e li invio a InfluxDB */
    cRaccoltaDati = 0;
    writeToInfluxDb(elaboraDato());

    delay(10);
  }
}

float elaboraDato()
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
  Serial.println("accX: " + String(accX, 4));
  Serial.println("accY: " + String(accY, 4));
  Serial.println("accZ: " + String(accZ, 4));

  accelerazioneMediaTot = sqrt((accX * accX) + (accY * accY) + (accZ * accZ)) - 9.81;
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;

  Serial.println("Accelerazione bilanciere Sx: " + String(accelerazioneMediaTot, 4));

  return (accelerazioneMediaTot);
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

void writeToInfluxDb(float a)
{

  sensor.clearFields();
  sensor.addField("Accelerazione Sx", a);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed ");
    Serial.println(client.getLastErrorMessage());
  }
}
