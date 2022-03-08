#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include <ArduinoJson.h>
const int input = 4;
String recievedMessage = "";

const char* ssid = "Hacker@123";
const char* password = "AmichandDugar@#9810193900";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* AWS_endpoint = "a1l34b8dami2sc-ats.iot.ap-south-1.amazonaws.com"; //MQTT broker ip
DHT dht(input, DHT11);
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    recievedMessage += (char)payload[i];// Pring payload content
  }
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, recievedMessage);
  String device = doc["device"];
  String deviceStatus = doc["state"];

  if(strcmp(device.c_str(),"light") == 0 && strcmp(deviceStatus.c_str(),"on") == 0)
  {
    digitalWrite(2,HIGH);
    Serial.println("Light on"); 
  }
  else if(strcmp(device.c_str(),"light") == 0 && strcmp(deviceStatus.c_str(),"off") == 0)
  {
    digitalWrite(2,LOW);
    Serial.println("Light off"); 
  }
  else if(strcmp(device.c_str(),"fan") == 0 && strcmp(deviceStatus.c_str(),"on") == 0)
  {
    digitalWrite(5,HIGH);
    Serial.println("Fan on"); 
  }
  else if(strcmp(device.c_str(),"fan") == 0 && strcmp(deviceStatus.c_str(),"off") == 0)
  {
    digitalWrite(5,LOW);
    Serial.println("Fan off"); 
  }
  else
  {
    Serial.println(device.c_str());
    Serial.println(deviceStatus.c_str());
  }
  
  Serial.println();
  recievedMessage = "";
}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  espClient.setX509Time(timeClient.getEpochTime());

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      client.publish("outTopic", "hello world");
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);
  dht.begin();
  Serial.setDebugOutput(true);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(2,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(4,INPUT);
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  // Load certificate file
  File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load private key file
  File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");

  // Load CA file
  File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
  if (!ca) {
    Serial.println("Failed to open ca ");
  }
  else
    Serial.println("Success to open ca");

  delay(1000);

  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
}
int k = 0;
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  float temp = dht.readTemperature(); // return temperature in °C
  float humidity = dht.readHumidity(); // return humidity in %
  ESP.wdtFeed();
  if(k == 100 || k==0){
    snprintf(msg, 75, "{\"device\": \"DHT11\",\"Temperature\": \"%.0f\",\"Humidity\": \"%.0f\"}", temp, humidity);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
    if(k == 100){
      k = 1;
    }
  }
  k++;
  delay(50);
  client.loop();
}
