# 1 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino"


/** Biblioteca WIFI.
    Biblioteca DHT para sensor de temperatura e humidade.
    Biblioteca time para utilizar temporização.
**/
# 8 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2
# 9 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2
# 10 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2

# 12 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2
# 13 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2


//Includes expecíficos do projeto
# 17 "/home/wberbert/IoTWorkbenchProjects/projects/sensor_MQQT_token/Device/device.ino" 2

#define INTERVAL 10000
#define MESSAGE_MAX_LEN 258

#define TEMPERATURA_MAX_VALOR 50
#define TEMPERATURA_MIN_VALOR 0
#define HUMIDADE_MAX_VALOR 100
#define HUMIDADE_MIN_VALOR 20

// Please input the SSID and password of WiFi
const char *ssid = "Iron Maiden";
const char *password = "onlythegooddieyoung";

//MQQT server para ser utilzado com método publish subscribler
const char *mqtt_server = "mqtt.cloud.kaaiot.com";
const String TOKEN = "kpwYDv9vh5"; // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "c5rgoth04t2n6tdig60g-sensor"; // Application version - you specify it during device provisioning
char *client_id = "esp32";

const unsigned long fiveSeconds = 1 * 5 * 1000UL;
static unsigned long lastPublish = 0 - fiveSeconds;



DHT dht(4, 11 /**< DHT TYPE 11 */);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("\nHandling command message on topic: %s\n", topic);

  DynamicJsonDocument doc(1023);
  deserializeJson(doc, payload, length);
  JsonVariant json_var = doc.as<JsonVariant>();

  DynamicJsonDocument commandResponse(1023);
  for (int i = 0; i < json_var.size(); i++)
  {
    unsigned int command_id = json_var[i]["id"].as<unsigned int>();
    commandResponse.createNestedObject();
    commandResponse[i]["id"] = command_id;
    commandResponse[i]["statusCode"] = 200;
    commandResponse[i]["payload"] = "done";
  }

  String responseTopic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/result/SWITCH";
  client.publish(responseTopic.c_str(), commandResponse.as<String>().c_str());
  Serial.println("Published response to SWITCH command on topic: " + responseTopic);
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    //char *client_id = "client-id-123ab";
    if (client.connect(client_id))
    {
      Serial.println("Connected to WiFi");
      // ... and resubscribe
      subscribeToCommand();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void subscribeToCommand()
{

  String topic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/command/SWITCH/status";
  client.subscribe(topic.c_str());
  Serial.println("Subscribed on topic: " + topic);

}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(12, 0x02);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  dht.begin();
}

void loop()
{
  // put your main code here, to run repeatedly:

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  if (now - lastPublish >= fiveSeconds)
  {

    digitalWrite(12, 0x1); // turn the LED on (HIGH is the voltage level)

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float temperature = (float)dht.readTemperature();

    // Read temperature as Celsius (the default)
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float humidity = (float)dht.readHumidity();

    float heatindex = (float)dht.computeHeatIndex(false);

    lastPublish += fiveSeconds;

    DynamicJsonDocument telemetry(1023);

    telemetry.createNestedObject();
    telemetry[0]["temperatura"] = temperature;
    telemetry[0]["humidade"] = humidity;
    telemetry[0]["heatindex"] = heatindex;

    //telemetry[0]["temperature"] = random(18, 23);
    //telemetry[0]["humidity"] = random(40, 60);
    //telemetry[0]["co2"] = random(900, 1200);

    String topic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";
    client.publish(topic.c_str(), telemetry.as<String>().c_str());

    Serial.println("Published on topic: " + topic);

    digitalWrite(12, 0x0); // turn the LED off by making the voltage LOW
  }
}
