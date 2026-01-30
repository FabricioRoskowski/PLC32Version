#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ======================================================
   WIFI
====================================================== */
#define WIFI_SSID     "GRAHL_CASA"
#define WIFI_PASSWORD "anyyetroll1234"

/* ======================================================
   THINGSBOARD MQTT BASIC
====================================================== */
#define TB_SERVER     "192.168.0.25"     // IP do ThingsBoard
#define TB_PORT       1883

#define MQTT_CLIENT_ID "ybfdbatzp3k0qpdjf16g"
#define MQTT_USERNAME  "frj58cin41bme5d395oa"
#define MQTT_PASSWORD  "il7h6b96g79tpjlz1i7h"

/* ======================================================
   OBJETOS
====================================================== */
WiFiClient espClient;
PubSubClient client(espClient);

/* ======================================================
   PROTÓTIPOS
====================================================== */
void connectWiFi();
void connectMQTT();
void sendTelemetry();
void mqttCallback(char* topic, byte* payload, unsigned int length);
/* ======================================================
   SETUP
====================================================== */
void setup() {

  Serial.begin(115200);
  delay(1000);

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
  delay(2000);   // envia a cada 2 segundos
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
  Serial.print("IP ESP32: ");
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
      Serial.println("Conectado!");
    }
    else {
      Serial.print("Falhou rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

/* ======================================================
   ENVIO TELEMETRIA
====================================================== */
void sendTelemetry() {

  float temperatura = random(200, 350) / 10.0;
  float umidade     = random(400, 800) / 10.0;
  int   estadoIO    = digitalRead(2);

  StaticJsonDocument<128> json;

  json["temperatura"] = temperatura;
  json["umidade"] = umidade;
  json["estado"] = estadoIO;

  char payload[128];
  serializeJson(json, payload);

  client.publish("v1/devices/me/telemetry", payload);

  Serial.print("Enviado: ");
  Serial.println(payload);
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String msg;

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("RPC recebido: ");
  Serial.println(msg);

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
