#include <PPPOS.h>
#include <PPPOSClient.h>

#include "TYPE1SC.h"
#include <Arduino.h>
#include <U8x8lib.h>

#define SERIAL_BR 115200
#define GSM_SERIAL 1
#define GSM_RX 16
#define GSM_TX 17
#define GSM_BR 115200

#define PWR_PIN 5
#define RST_PIN 18
#define WAKEUP_PIN 19
#define EXT_ANT 4

#define DebugSerial Serial
#define M1Serial Serial2

char *ppp_user = "codezoo";
char *ppp_pass = "codezoo";
char *msg = "hello world ppp";
String APN = "simplio.apn";

#define TCP_SERVER "echo.mbedcloudtesting.com"
const uint16_t port = 7;

String buffer = "";
char *data = (char *)malloc(1024);

PPPOSClient ppposClient;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
TYPE1SC TYPE1SC(M1Serial, DebugSerial, PWR_PIN, RST_PIN, WAKEUP_PIN);

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8X8LOG u8x8log;

/* EXT_ANT_ON 0 : Use an internal antenna.
 * EXT_ANT_ON 1 : Use an external antenna.
 */
#define EXT_ANT_ON 0

void extAntenna(){
	if (EXT_ANT_ON)
	{
		pinMode(EXT_ANT, OUTPUT);
		digitalWrite(EXT_ANT, HIGH);
		delay(100);
	}
}

bool startPPPOS() {
	PPPOS_start();
	unsigned long _tg = millis();
	while (!PPPOS_isConnected()) {
		DebugSerial.println("ppp Ready...");
		u8x8log.print("ppp Ready...\n");
		if (millis() > (_tg + 30000)) {
			PPPOS_stop();
			return false;
		}
		delay(3000);
	}

	DebugSerial.println("PPPOS Started");
	u8x8log.print("PPPOS Started\n");
	return true;
}

void reconnect() {
	// Loop until we're reconnected
	while (!ppposClient.connected()) {
		DebugSerial.print("Attempting TCP connection...");
		u8x8log.print("\f");      // \f = form feed: clear the screen
		u8x8log.print("Attempting TCP connection...");

		// Attempt to connect
		if (ppposClient.connect(TCP_SERVER, port)) {
			DebugSerial.println("connected");
			u8x8log.print("connected\n");
			// Once connected, socket data send to server...
			ppposClient.println("Device Ready.");
		} else {
			DebugSerial.print("socket failed");
			DebugSerial.println(" try again in 5 seconds");
			u8x8log.print("socket failed");
			u8x8log.print(" try again in 5 seconds\n");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void setup() {
	u8x8.begin();
	u8x8.setFont(u8x8_font_chroma48medium8_r);

	u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
	u8x8log.setRedrawMode(1);    // 0: Update screen with newline, 1: Update screen for every char

	// put your setup code here, to run once:
	M1Serial.begin(SERIAL_BR);
	DebugSerial.begin(SERIAL_BR);

	DebugSerial.println("TYPE1SC Module Start!!!");
	u8x8log.print("TYPE1SC Module Start!!!\n");

	extAntenna();

	/* TYPE1SC Module Initialization */
	if (TYPE1SC.init()) {
		DebugSerial.println("TYPE1SC Module Error!!!");
		u8x8log.print("TYPE1SC Module Error!!!\n");
	}

	/* Network Regsistraiton Check */
	while (TYPE1SC.canConnect() != 0) {
		DebugSerial.println("Network not Ready !!!");
		u8x8log.print("Network not Ready!!!\n");
		delay(2000);
	}

	/* Get Time (GMT, (+36/4) ==> Korea +9hour) */
	char szTime[32];
	if (TYPE1SC.getCCLK(szTime, sizeof(szTime)) == 0) {
		DebugSerial.print("Time : ");
		DebugSerial.println(szTime);
		u8x8log.print("Time : ");
		u8x8log.print(szTime);
		u8x8log.print("\n");		
	}
	delay(1000);

	int rssi, rsrp, rsrq, sinr;
	for (int i = 0; i < 3; i++) {
		/* Get RSSI */
		if (TYPE1SC.getRSSI(&rssi) == 0) {
			DebugSerial.println("Get RSSI Data");
			u8x8log.print("Get RSSI Data\n");
		}
		/* Get RSRP */
		if (TYPE1SC.getRSRP(&rsrp) == 0) {
			DebugSerial.println("Get RSRP Data");
			u8x8log.print("Get RSRP Data\n");
		}
		/* Get RSRQ */
		if (TYPE1SC.getRSRQ(&rsrq) == 0) {
			DebugSerial.println("Get RSRQ Data");
			u8x8log.print("Get RSRQ Data\n");
		}
		/* Get SINR */
		if (TYPE1SC.getSINR(&sinr) == 0) {
			DebugSerial.println("Get SINR Data");
			u8x8log.print("Get SINR Data\n");
		}
		delay(1000);
	}

	if (TYPE1SC.setPPP() == 0) {
		DebugSerial.println("PPP mode change");
		u8x8log.print("PPP mode change\n");
	}

	String RF_STATUS = "RSSI: " + String(rssi) +
		" RSRP:" + String(rsrp) + " RSRQ:" + String(rsrq) +
		" SINR:" + String(sinr);
	DebugSerial.println("[RF_STATUS]");
	DebugSerial.println(RF_STATUS);
	u8x8log.print("[RF_STATUS]\n");
	u8x8log.print(RF_STATUS);
	u8x8log.print("\n");

	DebugSerial.println("TYPE1SC Module Ready!!!");
	u8x8log.print("TYPE1SC Module Ready!!!\n");

	PPPOS_init(GSM_TX, GSM_RX, GSM_BR, GSM_SERIAL, ppp_user, ppp_pass);
	DebugSerial.println("Starting PPPOS...");
	u8x8log.print("Starting PPPOS...\n");

	if (startPPPOS()) {
		DebugSerial.println("Starting PPPOS... OK");
		u8x8log.print("Starting PPPOS... OK\n");
		u8x8log.print("\f");      // \f = form feed: clear the screen
	} else {
		DebugSerial.println("Starting PPPOS... Failed");
		u8x8log.print("Starting PPPOS... Failed\n");
	}
}

/* Test Property */
#define TEST_CNT 10
int cnt = 0;

void loop() {
	if (PPPOS_isConnected()) {
		if (!ppposClient.connected()) {
			reconnect();
		}

		// Data Receive with TCP Socket
		// Read all the lines of the reply from Server and print them to Serial
		while (ppposClient.available() > 0) {
			String line = ppposClient.readStringUntil('\r');
			DebugSerial.print(line);
			u8x8log.print(line);
		}

		// Data Transfer with TCP Socket
		if (cnt < TEST_CNT) {
			ppposClient.println(msg);
			delay(1000);
		} else if (cnt == TEST_CNT) {
			DebugSerial.println("socket Test complete.");
			u8x8log.print("socket Test complete.\n");
		}
		cnt++;
	}
}
