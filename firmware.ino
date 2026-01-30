#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>

/* ======================================================
   WIFI
====================================================== */
#define WIFI_SSID     "GRAHL_CASA"
#define WIFI_PASSWORD "anyyetroll1234"

/* ======================================================
   THINGSBOARD MQTT BASIC
====================================================== */
#define TB_SERVER     "192.168.0.25"
#define TB_PORT       1883

#define MQTT_CLIENT_ID "ybfdbatzp3k0qpdjf16g"
#define MQTT_USERNAME  "frj58cin41bme5d395oa"
#define MQTT_PASSWORD  "il7h6b96g79tpjlz1i7h"


String CURRENT_FW_VERSION = "1.1";

/* ======================================================
   OBJETOS
====================================================== */
WiFiClient espClient;
PubSubClient client(espClient);

/* ======================================================
   OTA VARIÁVEIS
====================================================== */
String fw_title;
String fw_version;
String fw_url;

/* ======================================================
   PROTÓTIPOS
====================================================== */
void connectWiFi();
void connectMQTT();
void sendTelemetry();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void processAttributes(String json);
void checkForOTA();
void performOTA(String url);

/* ======================================================
   SETUP
====================================================== */
void setup() {

  Serial.begin(115200);

  pinMode(5, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(2, INPUT);

  connectWiFi();

  client.setServer(TB_SERVER, TB_PORT);
  client.setCallback(mqttCallback);
}

/* ======================================================
   LOOP
====================================================== */
void loop() {

  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();

  sendTelemetry();
  delay(2000);
}

/* ======================================================
   WIFI
====================================================== */
void connectWiFi() {

  Serial.print("Conectando ao WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());
}

/* ======================================================
   MQTT
====================================================== */
void connectMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando ao ThingsBoard...");

    if (client.connect(
          MQTT_CLIENT_ID,
          MQTT_USERNAME,
          MQTT_PASSWORD
        )) {

      client.subscribe("v1/devices/me/rpc/request/+");
      client.subscribe("v1/devices/me/attributes");
      client.publish("v1/devices/me/attributes/request/1", "{}");
      Serial.println("OK");
    }
    else {
      Serial.print("Falha rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

/* ======================================================
   TELEMETRIA
====================================================== */
void sendTelemetry() {

  float temperatura = random(200, 350) / 10.0;
  float umidade     = random(400, 800) / 10.0;
  int estadoIO = digitalRead(2);

  StaticJsonDocument<128> json;

  json["temperatura"] = temperatura;
  json["umidade"] = umidade;
  json["estado"] = estadoIO;

  char payload[128];
  serializeJson(json, payload);

  client.publish("v1/devices/me/telemetry", payload);

  Serial.println(payload);
}

/* ======================================================
   CALLBACK MQTT
====================================================== */
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Msg: ");
  Serial.println(msg);

  // ---- ATRIBUTOS OTA ----
  if (String(topic).indexOf("attributes") >= 0) {
    processAttributes(msg);
    return;
  }

  // ---- RPC ----
  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);

  const char* metodo = doc["method"];
  bool valor = doc["params"];

  if (strcmp(metodo, "setRelay1") == 0) {
    digitalWrite(5, valor ? HIGH : LOW);
  }

  if (strcmp(metodo, "setRelay2") == 0) {
    digitalWrite(18, valor ? HIGH : LOW);
  }
}

/* ======================================================
   PROCESSA ATRIBUTOS OTA
====================================================== */
void processAttributes(String json) {

  StaticJsonDocument<256> doc;
  deserializeJson(doc, json);

  if (doc["fw_title"])   fw_title   = doc["fw_title"].as<String>();
  if (doc["fw_version"]) fw_version = doc["fw_version"].as<String>();
  if (doc["fw_url"])     fw_url     = doc["fw_url"].as<String>();

  Serial.println("Firmware recebido:");
  Serial.println(fw_title);
  Serial.println(fw_version);
  Serial.println(fw_url);

  checkForOTA();
}

/* ======================================================
   VERIFICA OTA
====================================================== */
void checkForOTA() {

  if (fw_version.length() == 0) return;

  if (fw_version != CURRENT_FW_VERSION) {
    Serial.println("Nova versao encontrada!");
    performOTA(fw_url);
  }
}


/* ======================================================
   EXECUTA OTA
====================================================== */
void performOTA(String url) {

  Serial.println("Baixando firmware...");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {

    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    if (Update.begin(len)) {

      size_t written = Update.writeStream(*stream);

      if (written == len && Update.end()) {

        Serial.println("Update OK. Reiniciando...");
        ESP.restart();
      }
    }
  }

  http.end();
}
