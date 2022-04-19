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
#define WIFI_SSID "Vodafone-A47203440" //"STRONG_353C_2.4GHz"  // "OPPO Reno4 Z 5G" "Vodafone-A47203440"
#define WIFI_PASSWORD "HtmMgyffEM4Mf4cH" //"737558353C"      //"ZIOBANANA" "HtmMgyffEM4Mf4cH"

#define INFLUXDB_URL "http://93.186.254.118:8086"
#define INFLUXDB_ORG "uniurb"
#define INFLUXDB_BUCKET "esercitazioni"
#define INFLUXDB_TOKEN "0s5ZuEkoUpmjulLxgpZfnXeiN1RU5tafsDkZ_bSdB-DzeDXQeN8k03ylXMREBwqYKd46oLq0Se8Qc13IjOuF-A=="

#define D_MISURE 50

/* in questa versione ho rimosso la tara sugli accelerometri */

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL,
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN,
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

TaskHandle_t Task1;
TaskHandle_t Task2;

/* variabili globali di elaborazione dati */
float AccX[D_MISURE];
float AccY[D_MISURE];
float AccZ[D_MISURE];

/* valori accelerazione */
float aX, aY, aZ;   
float aMedia, aMax;

/* contatori e flag */
int cRaccoltaDati = 0;
bool flgInvia = false;

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
    while (1)
      ;
  }
  Serial.println("MPU6050 Found!");

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("");

  /* da qui la parte multicore: divido i due task che verranno eseguiti su un core separatamente */

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task1code,   /* Task function. */
                          "Task1",     /* name of task. */
                          10000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &Task1,      /* Task handle to keep track of created task */
                          0);          /* pin task to core 0 */      

  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task2code,   /* Task function. */
                          "Task2",     /* name of task. */
                          10000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &Task2,      /* Task handle to keep track of created task */
                          1);          /* pin task to core 1 */

  /* prima di passare al loop, aspetto 5 secondi */
  delay(5000);

}

void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  /* loop del primo task */
  for(;;){
    
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

      /* elaboro i dati e li invio a InfluxDB */
      cRaccoltaDati = 0;

      aMedia = elaboraDatoMedio();
      aMax = elaboraDatoMax();

      flgInvia = true;

      delay(10);

    }

  } 
}

void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  /* loop del secondo task */
  for(;;){
    
    if (flgInvia)
    {
      writeToInfluxDb(aMedia, 0);
      writeToInfluxDb(aMax, 1);
      flgInvia = false;
    }
    
    delay(10);
    
  }
}

/* INIZIO LOOP */
void loop()
{


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

  accelerazioneMediaTot = sqrt((accX * accX) + (accY * accY) + (accZ * accZ));
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;

  Serial.println("Accelerazione media bilanciere SX: " + String(accelerazioneMediaTot, 4));

  return (accelerazioneMediaTot);
}

float elaboraDatoMax()
{

  float accelerazioneMax = 0.0;
  float perMax[D_MISURE];

  for (byte i = 0; i < D_MISURE; i++)
  {

    perMax[i] = sqrt((AccX[i] * AccX[i]) + (AccY[i] * AccY[i]) + (AccZ[i] * AccZ[i]));

    if (perMax[i] > accelerazioneMax)
    {
      accelerazioneMax = perMax[i];
    }
  }

  // Serial.println("accX: " + String(accX, 4));
  // Serial.println("accY: " + String(accY, 4));
  // Serial.println("accZ: " + String(accZ, 4));

  accelerazioneMax = roundf(accelerazioneMax * 10000) / 10000;

  Serial.println("Accelerazione Max bilanciere SX: " + String(accelerazioneMax, 4));

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
void writeToInfluxDb(float a, int flgMedia0_Max1)
{

  if (flgMedia0_Max1 == 0)
  {

    sensor.clearFields();
    sensor.addField("Accelerazione_Media_SX", a);
  }
  else
  {

    sensor.clearFields();
    sensor.addField("Accelerazione_Max_SX", a);
  }

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed ");
    Serial.println(client.getLastErrorMessage());
  }
}