/*
  This applies for the outside node
  sensor: photon, DHT22

  16-6:
  out_topic: "outside/light-sensing-front-door"
              "outside/temperature-sensing-front-door"

   22/6:
    DHT22: D0
    photon-resistor: A0

*/

#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>

#define DHTPIN D5
#define RELAY_PIN D2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE, 15);

const char* ssid = "dinhanh";
const char* pass = "tayta123";

const char* mqtt_server = "192.168.100.5";
const char* out_topic_temperature = "outside/temperature-sensing-front-door";
const char* out_topic_humidity = "outside/humidity-sensing-front-door";
const char* in_topic_all_sensor = "outside/in_stop_all_service";
const char* out_topic_all_sensor = "outside/out_stop_all_service";
const char* in_topic_lamp = "bedroom/desktop-lamp-in";
const char* out_topic_lamp = "bedroom/desktop-lamp-out";
const char* clientID = "machine1_eco";
const char* notificationRestURL = "http://192.168.100.5:8123/api/services/notify/notification_all_platforms";
float tempCelsius;
float humidity;
float msg_photon;

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
boolean sensorsOff = true;
boolean machineOff = true;
boolean firstTimeRun = true;
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("connecting to..");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to....");
  Serial.println(ssid);
  Serial.print(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topic_name(topic);
  char state = (char)payload[0];
  if (topic_name == in_topic_all_sensor) {
    if (state == '0') {
      sensorsOff = true;
      client.publish(out_topic_all_sensor, "0");
      client.publish(out_topic_temperature, "");
      client.publish(out_topic_humidity, "");
      Serial.print("Stop all services \n");
    } else if (state == '1') {
      sensorsOff = false;
      client.publish(out_topic_all_sensor, "1");
      Serial.print("Start all services \n");
    }
  } else if (topic_name == in_topic_lamp) {
    if (state == '0') {
      machineOff = true;
      digitalWrite(RELAY_PIN, HIGH);
      client.publish(out_topic_lamp, "0");
      Serial.println("Tat den....");

    } else if (state == '1') {
      machineOff = false;
      client.publish(out_topic_lamp, "1");
      digitalWrite(RELAY_PIN, LOW);
      Serial.print("Bat den....");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientID)) {
      Serial.println("connected to mQTT broker");
      client.subscribe(in_topic_all_sensor);
      client.subscribe(in_topic_lamp);
    } else {
      Serial.print("failed, rc=...");
      Serial.println(client.state());
      Serial.println("try again in 1 sec");
      delay(1000);
    }
  }
}

void publish_value() {

  //convert F to C
  tempCelsius = dht.convertFtoC(tempCelsius);
  dtostrf(tempCelsius, 3, 3, msg);

  //temperature
  Serial.print("temperature = ");
  client.publish(out_topic_temperature, msg);
  Serial.println(msg);

  //humidity
  Serial.print("humidity = ");
  dtostrf(humidity, 3, 3, msg);
  client.publish(out_topic_humidity, msg);
  Serial.println(msg);
  //================================================
  Serial.print("====================== \n");

}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(DHTPIN, INPUT_);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if (firstTimeRun) {
    client.publish(in_topic_all_sensor, "1");
    firstTimeRun = false;
    HTTPClient http;
    http.begin(notificationRestURL);
    http.addHeader("Content-Type", "application/json", true , false);
    http.addHeader("x-ha-access", "welcome", false , false);
    int httpCode = http.POST("{\"title\": \"Turn on machine 1\" ,\"message\":\"Thiết bị 1 đã sẵn sàng hoạt động và chờ thanh toán\"}");
    Serial.print("Start notification http result:");
    Serial.println(httpCode);
  }

  if (!sensorsOff) {
    //msg_photon = analogRead(A0);
    tempCelsius = dht.readTemperature(DHTPIN);
    humidity = dht.readHumidity(DHTPIN);
    publish_value();
    if ( tempCelsius > 35 && !machineOff) {
      digitalWrite(RELAY_PIN, HIGH);
      client.publish(out_topic_lamp, "0");
      machineOff = true;
      Serial.println("Nhiệt độ vượt quá cho phép, tắt thiết bị....");
      HTTPClient http;
      http.begin(notificationRestURL);
      http.addHeader("Content-Type", "application/json", true , false);
      http.addHeader("x-ha-access", "welcome", false , false);
      int httpCode = http.POST("{\"title\": \"Nhiệt độ quá cao\" ,\"message\":\"Thiết bị 1: Nhiệt độ không cho phép, tắt thiết bị\"}");
      Serial.print("Temperature notification http result:");
      Serial.println(httpCode);
    }
  }
  client.loop();
  delay(500);
}
