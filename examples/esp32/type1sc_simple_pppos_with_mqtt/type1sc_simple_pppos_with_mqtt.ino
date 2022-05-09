#include <PPPOS.h>
#include <PPPOSClient.h>
#include <PubSubClient.h>

#include "TYPE1SC.h"

#define SERIAL_BR 115200
#define GSM_SERIAL 1
#define GSM_RX 16
#define GSM_TX 17
#define GSM_BR 115200

#define PWR_PIN 5
#define RST_PIN 18
#define WAKEUP_PIN 19

#define DebugSerial Serial
#define M1Serial Serial2 // ESP32

char *server = "example.com";
char *ppp_user = "codezoo";
char *ppp_pass = "codezoo";
String APN = "simplio.apn";
char *SUB_TOPIC = "type1sc/100/test";
char *PUB_TOPIC = "type1sc/100/status";

#define MQTT_SERVER "broker.hivemq.com"
String buffer = "";
char *data = (char *)malloc(1024);

PPPOSClient ppposClient;
PubSubClient client(ppposClient);
bool atMode = true;

TYPE1SC TYPE1SC(M1Serial, DebugSerial, PWR_PIN, RST_PIN, WAKEUP_PIN);

void callback(char *topic, byte *payload, unsigned int length) {
  // Allocate the correct amount of memory for the payload copy
  byte *p = (byte *)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);

  //  DebugSerial.print("Message arrived [");
  //  DebugSerial.print(topic);
  //  DebugSerial.print("] ");
  //  for (int i = 0; i < length; i++) {
  //    DebugSerial.print((char)payload[i]);
  //  }
  //  DebugSerial.println();

  if (strstr((char *)p, "on")) {
    digitalWrite(25, HIGH);
    client.publish(PUB_TOPIC, p, length);

  } else if (strstr((char *)p, "off")) {
    digitalWrite(25, LOW);
    client.publish(PUB_TOPIC, p, length);

  } else if (strstr((char *)p, "dis")) {
    PPPOS_stop();
    atMode = true;
    if (TYPE1SC.setAT() == 0) {
      DebugSerial.println("Command Mode");
    } else {
      atMode = false;
    }
  }
  //  client.publish(PUB_TOPIC, p, length);
  free(p);
}

bool startPPPOS() {
  PPPOS_start();
  unsigned long _tg = millis();
  while (!PPPOS_isConnected()) {
    DebugSerial.println("ppp Ready...");
    if (millis() > (_tg + 30000)) {
      PPPOS_stop();
      return false;
    }
    delay(3000);
  }

  DebugSerial.println("PPPOS Started");
  return true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DebugSerial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("catm1Client")) {
      DebugSerial.println("connected");
      client.subscribe(SUB_TOPIC);
      // Once connected, publish an announcement...
      client.publish(PUB_TOPIC, "Device Ready");
      // ... and resubscribe
    } else {
      DebugSerial.print("failed, rc=");
      DebugSerial.print(client.state());
      DebugSerial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  /* CATM1 Modem PowerUp sequence */
  pinMode(PWR_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  pinMode(WAKEUP_PIN, OUTPUT);

  digitalWrite(PWR_PIN, HIGH);
  digitalWrite(WAKEUP_PIN, HIGH);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
  delay(2000);
  /********************************/
  // put your setup code here, to run once:
  M1Serial.begin(SERIAL_BR);
  DebugSerial.begin(SERIAL_BR);

  DebugSerial.println("TYPE1SC Module Start!!!");

  /* TYPE1SC Module Initialization */
  if (TYPE1SC.init()) {
    DebugSerial.println("TYPE1SC Module Error!!!");
  }

  /* Network Regsistraiton Check */
  while (TYPE1SC.canConnect() != 0) {
    DebugSerial.println("Network not Ready !!!");
    delay(2000);
  }

  /* Get Time (GMT, (+36/4) ==> Korea +9hour) */
  char szTime[32];
  if (TYPE1SC.getCCLK(szTime, sizeof(szTime)) == 0) {
    DebugSerial.print("Time : ");
    DebugSerial.println(szTime);
  }
  delay(1000);

  int rssi, rsrp, rsrq, sinr;
  for (int i = 0; i < 3; i++) {
    /* Get RSSI */
    if (TYPE1SC.getRSSI(&rssi) == 0) {
      DebugSerial.println("Get RSSI Data");
    }
    delay(1000);

    /* Get RSRP */
    if (TYPE1SC.getRSRP(&rsrp) == 0) {
      DebugSerial.println("Get RSRP Data");
    }
    delay(1000);

    /* Get RSRQ */
    if (TYPE1SC.getRSRQ(&rsrq) == 0) {
      DebugSerial.println("Get RSRQ Data");
    }
    delay(1000);

    /* Get SINR */
    if (TYPE1SC.getSINR(&sinr) == 0) {
      DebugSerial.println("Get SINR Data");
    }
    delay(1000);
    int count = 3 - (i + 1);
    DebugSerial.print(count);
    DebugSerial.println(" left..");
  }

  if (TYPE1SC.setPPP() == 0) {
    DebugSerial.println("PPP mode change");
    atMode = false;
  }

  String RF_STATUS = "[RF Status] RSSI: " + String(rssi) +
                     " RSRP:" + String(rsrp) + " RSRQ:" + String(rsrq) +
                     " SINR:" + String(sinr);
  DebugSerial.println(RF_STATUS);
  DebugSerial.println("TYPE1SC Module Ready!!!");

  pinMode(25, OUTPUT);

  /* PPPOS Setup */
  PPPOS_init(GSM_TX, GSM_RX, GSM_BR, GSM_SERIAL, ppp_user, ppp_pass);
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  DebugSerial.println("Starting PPPOS...");

  if (startPPPOS()) {
    DebugSerial.println("Starting PPPOS... OK");
  } else {
    DebugSerial.println("Starting PPPOS... Failed");
  }
}

void loop() {
  if (PPPOS_isConnected() && !atMode) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}
