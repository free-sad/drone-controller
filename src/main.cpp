//libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

MDNSResponder mdns;
WebSocketsServer ws = WebSocketsServer(81);

//solenoid pins (up, down, left, right, front, back)
//Axes: (axis: neg_dir/pos_dir) x: L/R, y: D/U, z: F/B
#define U 13 //D7
#define D 12 //D6
#define L 14 //D5
#define R 0  //D3
#define F 4  //D2
#define B 5  //D1

typedef struct Vec3 {
  int x;
  int y;
  int z;
} Vec3;


Vec3 vel = {0, 0, 0};
Vec3 accel = {0, 0, 0};

time_t startTime = 0;


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Websocket disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("Websocket connected");
      break;
    case WStype_TEXT:
      Serial.println("raw payload: " + String((char*)payload));

      StaticJsonDocument<500> doc;

      DeserializationError error = deserializeJson(doc, (char *) payload, length);
      if(error) {
        Serial.println("deserialize error");
        Serial.println(error.c_str());
        break;
      }

      JsonObject controllerInput = doc["controller"];
      int x = controllerInput["x"];
      int y = controllerInput["y"];
      int z = controllerInput["z"];

      accel.x = x;
      accel.y = y;
      accel.z = z;

      Serial.printf("x: %d, y: %d, z: %d", x, y, z);

      break;
  }
}

void setup() {
  pinMode(U, OUTPUT);
  pinMode(D, OUTPUT);
  pinMode(L, OUTPUT);
  pinMode(R, OUTPUT);
  pinMode(F, OUTPUT);
  pinMode(B, OUTPUT);


  digitalWrite(U, LOW);
  digitalWrite(D, LOW);
  digitalWrite(L, LOW);
  digitalWrite(R, LOW);
  digitalWrite(F, LOW);
  digitalWrite(B, LOW);

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  Serial.println("Connecting to network");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("Connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  if(mdns.begin("drone")) {
    Serial.println("mdns started at drone.local!");
  }

  ws.begin();
  ws.onEvent(webSocketEvent);
}

void loop() {

  if(millis() - startTime > 100) {
    vel.x += accel.x;
    vel.y += accel.y;
    vel.z += accel.z;

    startTime = millis();

    Serial.printf("velocity: x: %d, y: %d, z: %d\n", vel.x, vel.y, vel.z);
  }

  //actuate solenoids
  //x axis
  if(accel.x < 0) {
    digitalWrite(L, HIGH);
    digitalWrite(R, LOW);
  } else if(accel.x == 0) {
    digitalWrite(L, LOW);
    digitalWrite(R, LOW);
  } else if(accel.x > 0) {
    digitalWrite(L, LOW);
    digitalWrite(R, HIGH);
  }

  //y axis
  if(accel.y < 0) {
    digitalWrite(D, HIGH);
    digitalWrite(U, LOW);
  } else if(accel.y == 0) {
    digitalWrite(D, LOW);
    digitalWrite(U, LOW);
  } else if(accel.y > 0) {
    digitalWrite(D, LOW);
    digitalWrite(U, HIGH);
  }

  //z axis
  if(accel.z < 0) {
    digitalWrite(F, HIGH);
    digitalWrite(B, LOW);
  } else if(accel.z == 0) {
    digitalWrite(F, LOW);
    digitalWrite(B, LOW);
  } else if(accel.z > 0) {
    digitalWrite(F, LOW);
    digitalWrite(B, HIGH);
  }


  ws.loop();
}
