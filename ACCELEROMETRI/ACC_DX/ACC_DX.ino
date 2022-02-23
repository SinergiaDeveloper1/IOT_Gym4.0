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
#define WIFI_SSID       "Vodafone-A47203440"
#define WIFI_PASSWORD   "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL    "http://93.186.254.118:8086"
#define INFLUXDB_ORG    "uniurb"
#define INFLUXDB_BUCKET "test"
#define INFLUXDB_TOKEN  "7q44Rz0f0IZYM4SYguqyPB5RPafXPEagZUpRuIUBp3aoDT3HVQzFg5c0Hg_RY8Khk8cH8MjuApdyQsKrFyaF4w=="

#define D_TARA    100
#define D_MISURE  25

/* tare accelerometro SX */
float taraX,
      taraY,
      taraZ;

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL,
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN,
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

void setup() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "Accelerometro_DX");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  /* testo la connessione */
  client.validateConnection();

  if (!mpu.begin()) {
    while (1);
  }

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(1000);

}

/* variabili globali di elaborazione dati */
float perTaraX[D_TARA];
float perTaraY[D_TARA];
float perTaraZ[D_TARA];
float AccX[D_MISURE];
float AccY[D_MISURE];
float AccZ[D_MISURE];

float aX, aY, aZ; /* valori accelerazione */

/* contatori */
int cTara = 0,
    cRaccoltaDati = 0;

bool flgTaraInCorso = true;
bool flgInizioDiscesa = false;
bool flgInizioSalita = false;

/* INIZIO LOOP */
void loop() {

  readSensors();

  /* taro i sensori in base alla loro posizione */
  if (flgTaraInCorso) {

    impostaTara();

  } else {

    if (cRaccoltaDati < D_MISURE) {

      /* raccolgo i dati della discesa */
      AccX[cRaccoltaDati] = aX;
      AccY[cRaccoltaDati] = aY;
      AccZ[cRaccoltaDati] = aZ;

      cRaccoltaDati++;

    } else {

      /* elaboro i dati e li invio a InfluxDB */
      cRaccoltaDati = 0;
      writeToInfluxDb(elaboraDato());

    }

    delay(10);

  }

}

void impostaTara() {

  if (cTara < D_TARA) {

    perTaraX[cTara] = aX;
    perTaraY[cTara] = aY;
    perTaraZ[cTara] = aZ;

    cTara++;

  } else {

    /* calcolo la tara come valore medio di quelli registrati */
    for (byte i = 0; i < D_TARA; i++) {
      taraX += perTaraX[i];
      taraY += perTaraY[i];
      taraZ += perTaraZ[i];
    }

    taraX = taraX / D_TARA;
    taraY = taraY / D_TARA;
    taraZ = taraZ / D_TARA;

    flgTaraInCorso = false;

  }

}

float elaboraDato() {

  float accelerazioneMediaTot = 0.0;
  float accX = 0.0;
  float accY = 0.0;
  float accZ = 0.0;

  /* calcolo la tara come valore medio di quelli registrati */
  for (byte i = 0; i < D_MISURE; i++) {
    accX += AccX[i];
    accY += AccY[i];
    accZ += AccZ[i];
  }

  accX /= D_MISURE;
  accY /= D_MISURE;
  accZ /= D_MISURE;

  accelerazioneMediaTot = sqrt((accX * accX) + (accY * accY) + (accZ * accZ));
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;

  return (accelerazioneMediaTot);

}

/* funzione che legge dal sensore */
void readSensors() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  aX = a.acceleration.x - taraX;
  aY = a.acceleration.y - taraY;
  aZ = a.acceleration.z - taraZ;

}

void writeToInfluxDb(float a) {

  sensor.clearFields();
  sensor.addField("Accelerazione Dx", a);

  client.writePoint(sensor);

}
