#include <PPPOS.h>
#include <PPPOSClient.h>
#include <PubSubClient.h>

#define SERIAL_BR           115200
#define GSM_SERIAL          1
#define GSM_RX              16
#define GSM_TX              17
#define GSM_BR              115200

#define PWR_PIN 5
#define RST_PIN 18
#define WAKEUP_PIN 19

char* server = "example.com";
char* ppp_user = "codezoo";
char* ppp_pass = "codezoo";
String APN = "simplio.apn";
char* SUB_TOPIC = "type1sc/100/test";
char* PUB_TOPIC = "type1sc/100/status";

#define MQTT_SERVER "broker.hivemq.com"
String buffer = "";
char *data = (char *) malloc(1024);

PPPOSClient ppposClient;
PubSubClient client(ppposClient);

void callback(char* topic, byte* payload, unsigned int length) {
	// Allocate the correct amount of memory for the payload copy
	byte* p = (byte*)malloc(length);
	// Copy the payload to the new buffer
	memcpy(p,payload,length);

	//  Serial.print("Message arrived [");
	//  Serial.print(topic);
	//  Serial.print("] ");
	//  for (int i = 0; i < length; i++) {
	//    Serial.print((char)payload[i]);
	//  }
	//  Serial.println();

	if (strstr((char *)p, "on")){
		digitalWrite(25, HIGH);
	}else  if (strstr((char *)p, "off")){
		digitalWrite(25, LOW);
	}

	client.publish(PUB_TOPIC, p, length);
	free(p);
}

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
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect("catm1Client")) {
			Serial.println("connected");
			client.subscribe(SUB_TOPIC);
			// Once connected, publish an announcement...
			client.publish(PUB_TOPIC, "Device Ready");
			// ... and resubscribe
		} else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
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
	client.setServer(MQTT_SERVER, 1883);
	client.setCallback(callback);
	Serial.println("Starting PPPOS...");

	if (startPPPOS()) {
		Serial.println("Starting PPPOS... OK");
	} else {
		Serial.println("Starting PPPOS... Failed");
	}

}

void loop()
{
	if (PPPOS_isConnected()) {
		if (!client.connected()) {
			reconnect();
		}
		client.loop();
	}
}
