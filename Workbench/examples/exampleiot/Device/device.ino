/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#include <WiFi.h>
#include "Esp32MQTTClient.h"
#include "DHT.h"
#include "time.h"

#include "esp32_ports.h"

#define INTERVAL 10000
#define MESSAGE_MAX_LEN 258

#define TEMPERATURA_MAX_VALOR   50
#define TEMPERATURA_MIN_VALOR   0
#define HUMIDADE_MAX_VALOR      100
#define HUMIDADE_MIN_VALOR      20

// Please input the SSID and password of WiFi
const char *ssid = "Iron Maiden";
const char *password = "onlythegooddieyoung";

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

//Azure IOT HUB
//static const char *connectionString = "HostName=ih-tst-berbert-brazilsouth.azure-devices.net;DeviceId=esp32_01;SharedAccessKey=Kutdbv+oTv4nr+b7IH379clVSUfMN408pXvCTYnIJXc=";

//Azure IOT Central
static const char *connectionString = "HostName=iotc-4b513a0a-3a6f-4718-b6ec-7d037539e3d6.azure-devices.net;DeviceId=e7ucvftf92;SharedAccessKey=PKxpimdB250NgFvCJI3d6LH+L6JksugwRMCHnRPi2UQ=";

//Kaa IOT
//static const char *connectionString = "HostName=mqtt.cloud.kaaiot.com:1883;DeviceId=c5pn5f904t2n6tdiafb0-v1;SharedAccessKey=LpEg2FVIqu";

//const char *messageData = "{\"MessageId\":%d, \"DeviceID\": \"%s\", \"Temperature\":%f, \"Humidity\":%f, \"Tag\":\"%s\"}";

//Formatar payload que será enviado.
//const char *messageData = "{\"dispositivo\":\"%s\",\"hora\": \"%s\", \"temperatura\": {\"valor\": %f, \"max\": %f, \"min\": %f}, \"humidade\":{\"valor\": %f, \"max\": %f, \"min\": %f}, \"marcador\": \"%s\"}";
//Criando um json simples porque json complexos ainda não são suportados pelo iot central.
const char *messageData = "{\"dispositivo\": \"%s\",\"hora\": \"%s\", \"temperatura_valor\": %f, \"temperatura_max\": %f, \"temperatura_min\": %f, \"humidade_valor\": %f, \"humidade_max\": %f, \"humidade_min\": %f, \"marcador\": \"%s\"}";

static bool hasIoTHub = false;
static bool hasWifi = false;
int messageCount = 1;
static bool messageSending = true;
static uint64_t send_interval_ms;

static char MacAddress[18];

DHT dht(GPIO04, DHT11);


static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char *payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  Serial.println(" > WiFi");
  Serial.println("Starting connecting WiFi.");

  pinMode(GPIO12, OUTPUT);

  delay(10);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    hasWifi = false;
  }
  hasWifi = true;

  WiFi.macAddress().toCharArray(MacAddress, 18);

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(" > IoT Hub");
  if (!Esp32MQTTClient_Init((const uint8_t *)connectionString, true))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
  }
  hasIoTHub = true;
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
  Serial.println("Start sending events.");
  randomSeed(analogRead(0));
  send_interval_ms = millis();

  dht.begin();
  configTime(0, 3600, "pool.ntp.org");

}

void loop()
{
  if (hasWifi && hasIoTHub)
  {
    if (messageSending && (int)(millis() - send_interval_ms) >= INTERVAL)
    {
      digitalWrite(GPIO12, HIGH); // turn the LED on (HIGH is the voltage level)

      // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      //float temperature = (float)random(0, 500) / 10;
      
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float temperature = (float)dht.readTemperature();

      //float humidity = (float)random(0, 1000) / 10;
      
      // Read temperature as Celsius (the default)
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float humidity = (float)dht.readHumidity();
      
      //struct tm timeinfo;
      
      //unsigned int t;

      time_t now = time(0);
      tm *ltm = localtime(&now);
      char cm[16];

      strftime(cm,16, "%Y%m%d%H%M%S", ltm);
      //snprintf(cm, 4, "%s", &ltm->tm_year);

      //getLocalTime(&timeinfo);

      //t = snprintf(cm,5,"%s", timeinfo.tm_year);

      //time_t tempo;
      //char* c_hora_atual;
     
      //tempo = time(NULL);
      //c_hora_atual = ctime(&tempo);

      //snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, MacAddress, temperature, humidity, "berbert");
      
      snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, MacAddress, cm, temperature, (float) TEMPERATURA_MAX_VALOR, (float) TEMPERATURA_MIN_VALOR, humidity, (float) HUMIDADE_MAX_VALOR, (float) HUMIDADE_MIN_VALOR, "esp32");
      Serial.println(messagePayload);

      EVENT_INSTANCE *message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      Esp32MQTTClient_SendEventInstance(message);
      send_interval_ms = millis();

      digitalWrite(GPIO12, LOW); // turn the LED off by making the voltage LOW
    }
    else
    {
      Esp32MQTTClient_Check();
    }
  }
  delay(10);
}