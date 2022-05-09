#include <PPPOS.h>
#include <PPPOSClient.h>

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

char *ppp_user = "codezoo";
char *ppp_pass = "codezoo";
char *msg = "hello world ppp";
String APN = "simplio.apn";

#define TCP_SERVER "echo.mbedcloudtesting.com"
const uint16_t port = 7;

String buffer = "";
char *data = (char *)malloc(1024);

PPPOSClient ppposClient;

TYPE1SC TYPE1SC(M1Serial, DebugSerial, PWR_PIN, RST_PIN, WAKEUP_PIN);

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
  while (!ppposClient.connected()) {
    DebugSerial.print("Attempting TCP connection...");
    // Attempt to connect
    if (ppposClient.connect(TCP_SERVER, port)) {
      DebugSerial.println("connected");
      // Once connected, socket data send to server...
      ppposClient.println("Device Ready.");
      // ppposClient.write(msg, strlen(msg));
    } else {
      DebugSerial.print("socket failed");
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
  }

  String RF_STATUS = "[RF Status] RSSI: " + String(rssi) +
                     " RSRP:" + String(rsrp) + " RSRQ:" + String(rsrq) +
                     " SINR:" + String(sinr);
  DebugSerial.println(RF_STATUS);
  DebugSerial.println("TYPE1SC Module Ready!!!");

  PPPOS_init(GSM_TX, GSM_RX, GSM_BR, GSM_SERIAL, ppp_user, ppp_pass);
  DebugSerial.println("Starting PPPOS...");

  if (startPPPOS()) {
    DebugSerial.println("Starting PPPOS... OK");
  } else {
    DebugSerial.println("Starting PPPOS... Failed");
  }
}

/* Test Property */
#define TEST_CNT 5
int cnt = 0;
void loop() {
  if (PPPOS_isConnected()) {
    if (!ppposClient.connected()) {
      reconnect();
    }

    // Read all the lines of the reply from Server and print them to Serial
    while (ppposClient.available() > 0) {
      String line = ppposClient.readStringUntil('\r');
      DebugSerial.print(line);
    }

    // Data transfer with TCP Socket
    if (cnt < TEST_CNT) {
      ppposClient.println(msg);
    } else if (cnt == TEST_CNT) {
      DebugSerial.println("socket Test complete.");
      /* Force an exception case (disconnect from server)*/
      // ppposClient.stop();
      // cnt = 0;
    }
    cnt++;
  }
  delay(1000);
}
