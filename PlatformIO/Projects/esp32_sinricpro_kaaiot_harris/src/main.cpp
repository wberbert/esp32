#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFiClient.h>

/*SINRICPRO*/
#include <SinricPro.h>
#include <SinricProTemperaturesensor.h>

/*KAAIOT*/
#include <PubSubClient.h>
#include <time.h>

#include "esp32.h"
//#include "sinrincpro_device.h"
//#include "dweet_device.h"

//#define DEBUG_ESP_PORT Serial
//#define NODEBUG_WEBSOCKETS
//#define NDEBUG

#define TEMPO_ESPERA_EVENTO       5000    //  5000 Milisegundos.
#define SERIAL_TAXA_TRANSSISSAO   115200  //  115200 Velocidade da porta serial.

typedef struct {
  char ssid[32];
  char pass[32];
} wificon;

 wificon gWIFIConexao[2] = {
                                  {"Iron Maiden","onlythegooddieyoung"}, 
                                  {"Killers","onlythegooddieyoung"}
                                };


float gfltTemperatura;            //Temperatura atual.
float gfltHumidade;               //Humidade atual.
float gfltSensacaoTermica;        //Sensação térmica.

float gfltUltimaTemperatura;      //Ultima temperatura lida do sensor.
float gfltUltimaHumidade;         //Ultima humidade lida do sensor.
float gfltUltimaSensacaoTermica;  //Ultima sensação térmica lida do sensor.

bool  gblnSensorDHTLigado=true;   //Sensor de temperatura está ligado (sim/não).

unsigned long glngUltimoEvento=0; //Última comunicação com o endpoint IOT.


DHT dht (GPIO04, DHT11);

/*SINRICPRO*/

#define APP_KEY           "46dd3ffc-d9b6-4242-9437-163c7ebf316a"                                      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "3a794083-7056-4b93-bd43-25031948e6e9-150670c1-3379-476e-973d-f13fb3e7a539" //"YOUR-APP-SECRET"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define TEMP_SENSOR_ID    "61782f89eb3dca1828226e1c"                                                  //"YOUR-DEVICE-ID"    // Should look like "5dc1564130xxxx

/*Kaaiot*/
//MQQT server para ser utilzado com método publish subscribler
const char *mqtt_server = "mqtt.cloud.kaaiot.com";
const String TOKEN = "kpwYDv9vh5";                    // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "c5rgoth04t2n6tdig60g-sensor"; // Application version - you specify it during device provisioning
char *client_id = "harris";

WiFiClient kaaiot_client;
PubSubClient client(kaaiot_client);

/* bool onPowerState(String deviceId, bool &state) 
 *
 * Callback for setPowerState request
 * parameters
 *  String deviceId (r)
 *    contains deviceId (useful if this callback used by multiple devices)
 *  bool &state (r/w)
 *    contains the requested state (true:on / false:off)
 *    must return the new state
 * 
 * return
 *  true if request should be marked as handled correctly / false if not
 */

bool onPowerState(const String &deviceId, bool &state) {

  Serial.printf("Sensor de temperatura %s (via SinricPro) \r\n", state?"on":"off");
  gblnSensorDHTLigado = state; // turn on / off temperature sensor
  return true; // request handled properly

}

bool sinrincpro_enviar_evento(float *fltTempreatura, float *fltHumidade) {

  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];  // get temperaturesensor device
  bool success = mySensor.sendTemperatureEvent(*fltTempreatura, *fltHumidade); // send event

  return success;
  
}

void sinrincpro_setup() {
  
  // add device to SinricPro
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  mySensor.onPowerState(onPowerState);
  

  // setup SinricPro
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true); // get latest known deviceState from server (is device turned on?)

  SinricPro.onConnected([](){Serial.printf("Conectado ao SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });



}

/*------------------------------------------------------------------------------------------------------*/

void configurarWIFI() {

  Serial.printf ("[Wifi]: Conectando a ");
  
  Serial.printf (gWIFIConexao[0].ssid);

  WiFi.begin(gWIFIConexao[0].ssid,gWIFIConexao[0].pass);

  while (WiFi.status() != WL_CONNECTED) {
    
    delay(250);
    Serial.printf(".");
    digitalWrite(GPIO14, LOW); 
    delay(250);
    digitalWrite(GPIO14, HIGH); 

  }

  IPAddress localIP = WiFi.localIP();
  Serial.printf("Conectado!\r\n[WiFi]: Endereco IP é %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);

}

void ler_sensor_temperatura() {
  

  gfltTemperatura = (float) dht.readTemperature();                // Ler a temperatura em graus celcius.
  gfltHumidade = (float) dht.readHumidity();                      // Ler a humidade em percentual.
  gfltSensacaoTermica = (float) dht.computeHeatIndex(false);      // Calcula sensação térmica em graus celcius.

  if (isnan(gfltTemperatura) || isnan(gfltHumidade)) {
    
    Serial.printf("Falha na leitura DHT!\r\n");
    return;

  }
  

  Serial.printf ("Temperatura: %2.1f Celsius\tHumidade: %2.1f%%\tSensação Termica: %2.1f Celsius\tEstado: Sensor %s\t", gfltTemperatura, gfltHumidade, gfltSensacaoTermica, gblnSensorDHTLigado ? "Ligado" : "Desligado");

  if (gblnSensorDHTLigado == false) return;                       // Dispositivo desligado, não faça nada.
  if (gfltTemperatura == gfltUltimaTemperatura && gfltHumidade == gfltUltimaHumidade) {
    
    Serial.printf("\n");
    return;   // Nenhum valor modificado, não há necessidade de enviar dados.
  
  } 

  bool blnSucesso = false;
  unsigned int intRetentar = 0;


  Serial.printf("Enviando eventos");
  while (!blnSucesso && intRetentar < 3) {
    blnSucesso = sinrincpro_enviar_evento(&gfltTemperatura, &gfltHumidade);
    Serial.printf(".");
    delay(250);
    intRetentar++;
  }
  Serial.printf("%s\r\n", blnSucesso ? "sucesso" : "falha");

  //dweet_loop(&gfltTemperatura, &gfltHumidade, &gfltSensacaoTermica);
  

  if (blnSucesso) {
    gfltUltimaTemperatura = gfltTemperatura;
    gfltUltimaHumidade = gfltHumidade;
    gfltUltimaSensacaoTermica = gfltSensacaoTermica;
  }


}


void setup()
{
  Serial.begin(SERIAL_TAXA_TRANSSISSAO); 
  pinMode(GPIO12, OUTPUT);
  pinMode(GPIO14, OUTPUT);

  Serial.printf("\r\n\r\n");
  
  dht.begin();
  
  configurarWIFI();
  
  delay(250);

  sinrincpro_setup();

  // put your setup code here, to run once:

}

void loop()
{
  unsigned long lngEventoAtual = millis();
  
  digitalWrite(GPIO12, LOW);
  
  SinricPro.handle();
   
  // put your main code here, to run repeatedly:
  if (lngEventoAtual - glngUltimoEvento < TEMPO_ESPERA_EVENTO) return;  // Não faça nada caso o tempo de espera não seja atingido.
  
  digitalWrite(GPIO12, HIGH);
  ler_sensor_temperatura();

  glngUltimoEvento = lngEventoAtual;
    

}