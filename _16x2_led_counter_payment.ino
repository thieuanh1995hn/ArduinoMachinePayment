#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);
const char* ssid = "dinhanh";
const char* pass = "tayta123";
const char* in_topic_lamp = "bedroom/desktop-lamp-in";
const char* mqtt_server = "192.168.100.5";
const char* clientID = "machine1_lcd";
int timeCounter = -1;
WiFiClient espClient;
PubSubClient client(espClient);

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


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientID)) {
      Serial.println("connected to mQTT broker");
      client.subscribe(in_topic_lamp);
    } else {
      Serial.print("failed, rc=...");
      Serial.println(client.state());
      Serial.println("try again in 1 sec");
      delay(1000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char state = (char)payload[0];
  String topic_name(topic);
  if (topic_name == in_topic_lamp && state == '1') {
    String dointTime = String((char*)(payload+1));
    timeCounter = dointTime.toInt();
  }

}

void disPlayDefault() {
  lcd.setCursor(0, 0);
  // Print HELLO to the screen, starting at 5,0.
  lcd.print("Dia chi vi Iota");
  lcd.setCursor(0, 1);
  lcd.print("    ben duoi");

}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  disPlayDefault();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  while (timeCounter > 0) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Thoi gian:");
    lcd.setCursor(5, 1);
    lcd.print(timeCounter);
    timeCounter -- ;
    delay(1000);
  }
  if ( timeCounter == 0 ) {
    lcd.clear();
    lcd.setCursor(0, 0);
    // Print HELLO to the screen, starting at 5,0.
    lcd.print(" Xin chan thanh");
    lcd.setCursor(0, 1);
    lcd.print("     cam on!");
    delay (4000);
    lcd.clear();
    disPlayDefault();
    timeCounter = -1;
  }

  client.loop();
  delay(500);
}
