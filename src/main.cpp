//libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

int sign(int x);

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

bool governor = false; //if true, limit speed
int speedLimit = 10;


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Websocket disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("Websocket connected");
      break;
    case WStype_TEXT:
      //Serial.println("raw payload: " + String((char*)payload));

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

      JsonObject state = doc["state"];

      if(state["governor"] != governor) {
        vel = {0, 0, 0};
        governor = state["governor"];
      }

      governor = state["governor"];

      accel.x = x;
      accel.y = y;
      accel.z = z;

      //Serial.printf("x: %d, y: %d, z: %d, gov: %d\n", x, y, z, governor);

      //send ack
      ws.sendTXT(num, "ACK");

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
  WiFi.begin("free-sad", "rocksatx");

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

  if(millis() - startTime > 100) { //velocity update loop
    if(governor) { //speed limiting enabled
      //x axis
      if(accel.x != 0) {
        //only accelerate if less than speed limit
        if(abs(vel.x + accel.x) < speedLimit) {
          vel.x += accel.x;

          if(accel.x < 0) {
            digitalWrite(L, HIGH);
            digitalWrite(R, LOW);
          } else if(accel.x > 0) {
            digitalWrite(L, LOW);
            digitalWrite(R, HIGH);
          }

        } else {
          digitalWrite(L, LOW);
          digitalWrite(R, LOW);
        }
      } else {
        digitalWrite(L, LOW);
        digitalWrite(R, LOW);

        //slow down if no input
        if(abs(vel.x) > 0) {
          vel.x += -sign(vel.x);

          if(vel.x < 0) {
            digitalWrite(L, LOW);
            digitalWrite(R, HIGH);
          } else if(vel.x > 0) {
            digitalWrite(L, HIGH);
            digitalWrite(R, LOW);
          }
        }
      }

      //y axis
      if(accel.y != 0) {
        //only accelerate if less than speed limit
        if(abs(vel.y + accel.y) < speedLimit) {
          vel.y += accel.y;

          if(accel.y < 0) {
            digitalWrite(D, HIGH);
            digitalWrite(U, LOW);
          } else if(accel.y > 0) {
            digitalWrite(D, LOW);
            digitalWrite(U, HIGH);
          }

        } else {
          digitalWrite(D, LOW);
          digitalWrite(U, LOW);
        }
      } else {
        digitalWrite(D, LOW);
        digitalWrite(U, LOW);

        //slow down if no input
        if(abs(vel.y) > 0) {
          vel.y += -sign(vel.y);

          if(vel.y < 0) {
            digitalWrite(D, LOW);
            digitalWrite(U, HIGH);
          } else if(vel.y > 0) {
            digitalWrite(D, HIGH);
            digitalWrite(U, LOW);
          }
        }
      }

      //z axis
      if(accel.z != 0) {
        //only accelerate if less than speed limit
        if(abs(vel.z + accel.z) < speedLimit) {
          vel.z += accel.z;

          if(accel.z < 0) {
            digitalWrite(F, HIGH);
            digitalWrite(B, LOW);
          } else if(accel.z > 0) {
            digitalWrite(F, LOW);
            digitalWrite(B, HIGH);
          }

        } else {
          digitalWrite(F, LOW);
          digitalWrite(B, LOW);
        }
      } else {
        digitalWrite(F, LOW);
        digitalWrite(B, LOW);

        //slow down if no input
        if(abs(vel.z) > 0) {
          vel.z+= -sign(vel.z);

          if(vel.z < 0) {
            digitalWrite(F, LOW);
            digitalWrite(B, HIGH);
          } else if(vel.z > 0) {
            digitalWrite(F, HIGH);
            digitalWrite(B, LOW);
          }
        }
      }

    } else {
      vel.x += accel.x;
      vel.y += accel.y;
      vel.z += accel.z;

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
    }

    startTime = millis();

    Serial.printf("velocity: x: %d, y: %d, z: %d\n", vel.x, vel.y, vel.z);
  }

  ws.loop();
}

int sign(int x) {
  if(x < 0) {
    return -1;
  } else if(x > 0) {
    return 1;
  } else {
    return 0;
  }
}
