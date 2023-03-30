#include <Arduino.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "BH1750.h"
#include "DHTesp.h"

#define LED_RED 18
#define LED_YELLOW 15
#define LED_GREEN 21

#define PIN_DHT 19

#define PIN_SCL 22
#define PIN_SDA 23

#define HIGH 1
#define LOW 0

const char* ssid = "Lab-Eng";
const char* password = "Lab-Eng123!";
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH "KuisIotJeremy/data"
#define MQTT_TOPIC_SUBSCRIBE "KuisIotJeremy/cmd"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Ticker timerTemp, timerHumid, timerLux;
DHTesp dht;
BH1750 lightMeter;

char g_szDeviceId[30];
void WifiConnect();
boolean mqttConnect();
void onPublishMessage();

void tempPublishMessage();
void humidPublishMessage();
void luxPublishMessage();

void taskTemp(void* arg);
void taskHumid(void* arg);
void taskLux(void* arg);
void taskLed(void* arg);

float temperature;
float humidity;

void setup() {
Serial.begin(9600);

pinMode(LED_RED, OUTPUT);
pinMode(LED_GREEN, OUTPUT);
pinMode(LED_YELLOW, OUTPUT);


dht.setup(PIN_DHT, DHTesp::DHT11);
Wire.begin(PIN_SDA, PIN_SCL);
lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

WifiConnect();
mqttConnect();

xTaskCreatePinnedToCore(taskTemp, "taskTemp", 2048, NULL, 1, NULL, 0);
xTaskCreatePinnedToCore(taskHumid, "taskHumid", 2048, NULL, 1, NULL, 0);
xTaskCreatePinnedToCore(taskLux, "taskLux", 2048, NULL, 1, NULL, 0);
xTaskCreatePinnedToCore(taskLed, "taskLed", 2048, NULL, 1, NULL, 0);

}

void loop() {
mqtt.loop();
}

void taskTemp(void* arg){
  for(;;){
    tempPublishMessage();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}


void taskHumid(void* arg){
  for(;;){
    humidPublishMessage();
    vTaskDelay(6000 / portTICK_PERIOD_MS);
  }
}

void taskLux(void* arg){
  for(;;){
    luxPublishMessage();
    vTaskDelay(4000 / portTICK_PERIOD_MS);
  }
}

void taskLed(void* arg){
  for(;;){
    if (temperature >= 28 && humidity >80)
    {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
    }
    else if (temperature >= 28 && humidity >60 && humidity<=80)
    {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_GREEN, LOW);
    }
    else if (temperature < 28 && humidity <60)
    {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }
    //else if (humidity >60)
    //{
    //  digitalWrite(LED_RED,HIGH);
    //}
    //Used to test other LED karena ruangannya dingin banget
    
    else{
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}




//Message arrived [esp32_test/cmd/led1]: 0
void mqttCallback(char* topic, byte* payload, unsigned int len) {

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();


}

void tempPublishMessage()
{
  char szData[50];

  temperature = dht.getTemperature();

  if (dht.getStatus()==DHTesp::ERROR_NONE)
  {
    Serial.printf("Temperature: %.2f C\n", temperature);
    sprintf(szData, "Temperature: %.2f C", temperature);
    mqtt.publish(MQTT_TOPIC_PUBLISH, szData);
  }
}

void humidPublishMessage(){
  char szData[50];

  humidity = dht.getHumidity();

  if (dht.getStatus()==DHTesp::ERROR_NONE)
  {
    Serial.printf("Humidity: %.2f %\n", humidity);
    sprintf(szData, "Humidity: %.2f %", humidity);
    mqtt.publish(MQTT_TOPIC_PUBLISH, szData);
  }
}

void luxPublishMessage(){

  char szData[50];

  float lux = lightMeter.readLightLevel();

  

  if (lux > 400)
  {
    Serial.printf("Warning!! Door open, Light: %.2f lx\n", lux);
    sprintf(szData, "Door open, Light %.2f lx", lux);

  }
  else if (lux <= 400)
  {
    Serial.printf("Door closed, Light: %.2f lx\n", lux);
    sprintf(szData, "Door closed, Light %.2f lx", lux);

  }
  
  mqtt.publish(MQTT_TOPIC_PUBLISH, szData);
}



boolean mqttConnect() {

  sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCallback);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

  boolean fMqttConnected = false;
  for (int i=0; i<3 && !fMqttConnected; i++) {
    Serial.print("Connecting to mqtt broker...");
    fMqttConnected = mqtt.connect(g_szDeviceId);
    if (fMqttConnected == false) {
      Serial.print(" fail, rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }

  if (fMqttConnected)
  {
    Serial.println(" success");
    mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
    Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
  }
  return mqtt.connected();
}

void WifiConnect()
{
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
while (WiFi.waitForConnectResult() != WL_CONNECTED) {
Serial.println("Connection Failed! Rebooting...");
delay(5000);
ESP.restart();
}
Serial.print("System connected with IP address: ");
Serial.println(WiFi.localIP());
Serial.printf("RSSI: %d\n", WiFi.RSSI());
}
