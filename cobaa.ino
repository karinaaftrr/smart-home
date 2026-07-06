#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =========================
// WIFI
// =========================
const char* ssid = "karina";
const char* password = "12345678";

// =========================
// HIVEMQ CLOUD
// =========================
const char* mqtt_server = "a19780eddbd848a5b6696ea6691df1ec.s1.eu.hivemq.cloud";
const char* mqtt_user = "karinaaftrr";
const char* mqtt_pass = "Karina1105";
const int mqtt_port = 8883;

// =========================
// PIN
// =========================
#define DHTPIN 4
#define DHTTYPE DHT11

#define MQ2_PIN 34
#define BUZZER_PIN 23
#define RELAY_PIN 18

// =========================
// THRESHOLD
// =========================
float suhuThreshold = 30.0;
int gasThreshold = 800;

// =========================
// OBJECT
// =========================
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// =========================
// WIFI CONNECT
// =========================
void setupWiFi() {

  Serial.println();
  Serial.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());
}

// =========================
// MQTT CONNECT
// =========================
void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Connecting MQTT...");

    String clientId = "ESP32-";
    clientId += String(random(1000));

    if (client.connect(clientId.c_str(),
                       mqtt_user,
                       mqtt_pass)) {

      Serial.println("Connected");

    } else {

      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry...");
      delay(5000);
    }
  }
}

// =========================
// SETUP
// =========================
void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);

  dht.begin();

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART HOME");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);

  setupWiFi();

  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(2000);
}

// =========================
// LOOP
// =========================
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  int gasValue = analogRead(MQ2_PIN);

  bool suhuPanas = suhu >= suhuThreshold;
  bool gasBocor = gasValue >= gasThreshold;

  String status = "SISTEM AMAN";

  int fanState = 0;
  int buzzerState = 0;

  // =========================
  // LOGIKA SISTEM
  // =========================

  if (suhuPanas && gasBocor) {

    status = "BAHAYA";

    fanState = 1;
    buzzerState = 1;

    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(suhu, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("BAHAYA!");
  }

  else if (gasBocor) {

    status = "GAS BOCOR";

    fanState = 0;
    buzzerState = 1;

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(suhu, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("GAS BOCOR!");
  }

  else if (suhuPanas) {

    status = "SUHU PANAS";

    fanState = 1;
    buzzerState = 0;

    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(suhu, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("KIPAS ON");
  }

  else {

    status = "SISTEM AMAN";

    fanState = 0;
    buzzerState = 0;

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(suhu, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("SISTEM AMAN");
  }

  // =========================
  // MQTT PUBLISH
  // =========================

  client.publish(
      "iot/suhu",
      String(suhu, 1).c_str());

  client.publish(
      "iot/kelembapan",
      String(kelembapan, 1).c_str());

  client.publish(
      "iot/gas",
      String(gasValue).c_str());

  client.publish(
      "iot/status",
      status.c_str());

  client.publish(
      "iot/fan",
      String(fanState).c_str());

  client.publish(
      "iot/buzzer",
      String(buzzerState).c_str());

  // =========================
  // SERIAL MONITOR
  // =========================

  Serial.println("==========");

  Serial.print("Suhu : ");
  Serial.println(suhu);

  Serial.print("Kelembapan : ");
  Serial.println(kelembapan);

  Serial.print("Gas : ");
  Serial.println(gasValue);

  Serial.print("Status : ");
  Serial.println(status);

  Serial.println("==========");

  delay(2000);
}
