#include <PPPOS.h>
#include <PPPOSClient.h>

#define SERIAL_BR           115200
#define GSM_SERIAL          1
#define GSM_RX              16
#define GSM_TX              17
#define GSM_BR              115200

#define PWR_PIN 5
#define RST_PIN 18
#define WAKEUP_PIN 19

char* ppp_user = "codezoo";
char* ppp_pass = "codezoo";
char* msg = "hello world ppp";
String APN = "simplio.apn";

#define TCP_SERVER "echo.mbedcloudtesting.com"
const uint16_t port = 7;

String buffer = "";
char *data = (char *) malloc(1024);

PPPOSClient ppposClient;

bool sendCommandWithAnswer(String cmd, String ans) {
	PPPOS_write((char *)cmd.c_str());
	unsigned long _tg = millis();
	while (true) {
		data = PPPOS_read();
		if (data != NULL) {
			char* command = strtok(data, "\n");
			while (command != 0)
			{
				buffer = String(command);
				buffer.replace("\r", "");
				command = strtok(0, "\n");
				if (buffer != "") {
					Serial.println(buffer);
				}
				if (buffer == ans) {
					buffer = "";
					return true;
				}
				buffer = "";
			}
		}
		if (millis() > (_tg + 5000)) {
			buffer = "";
			return false;
		}
	}
	buffer = "";
	return false;
}

bool startPPPOS() {
	String apnSet = "AT%APNN=\"" + APN + "\"\r";
	if (!sendCommandWithAnswer(apnSet, "OK")) {
		return false;
	}

	if (!sendCommandWithAnswer("ATD*99***1#\r", "CONNECT")) {
		return false;
	}
	PPPOS_start();
	unsigned long _tg = millis();
	while (!PPPOS_isConnected()) {
		Serial.println("ppp Ready...");
		if (millis() > (_tg + 30000)) {
			PPPOS_stop();
			return false;
		}
		delay(3000);
	}

	Serial.println("PPPOS Started");
	return true;
}

void reconnect() {
	// Loop until we're reconnected
	while (!ppposClient.connected()) {
		Serial.print("Attempting TCP connection...");
		// Attempt to connect
		if (ppposClient.connect(TCP_SERVER, port)) {
			Serial.println("connected");
			// Once connected, socket data send to server...
			ppposClient.println("Device Ready.");
			//ppposClient.write(msg, strlen(msg));
		} else {
			Serial.print("socket failed");
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void setup()
{
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

	pinMode(25, OUTPUT);
	Serial.begin(SERIAL_BR);
	PPPOS_init(GSM_TX, GSM_RX, GSM_BR, GSM_SERIAL, ppp_user, ppp_pass);
	Serial.println("Starting PPPOS...");

	if (startPPPOS()) {
		Serial.println("Starting PPPOS... OK");
	} else {
		Serial.println("Starting PPPOS... Failed");
	}

}

/* Test Property */
#define TEST_CNT 5
int cnt = 0;
void loop()
{  
	if (PPPOS_isConnected()) {
		if (!ppposClient.connected()) {
			reconnect();
		}

		// Read all the lines of the reply from server and print them to Serial
		while(ppposClient.available( ) > 0) {
			String line = ppposClient.readStringUntil('\r');
			Serial.print(line);
		}
		if(cnt<TEST_CNT){
			ppposClient.println(msg);
		}
		else if(cnt==TEST_CNT){
			Serial.println("socket Test complete.");
			/* Force an exception case (disconnect from server)*/
			//ppposClient.stop(); 
			//cnt = 0;			
		}
		cnt++;    
	}
	delay(1000);
}
