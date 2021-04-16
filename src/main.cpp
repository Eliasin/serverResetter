#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Pinger.h>

constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MAX_PASSWORD_LENGTH = 63;
constexpr size_t MAX_HOST_LENGTH = 32;

void setup() {
	pinMode(0, OUTPUT);
	pinMode(2, OUTPUT);

	digitalWrite(0, LOW);
	digitalWrite(2, LOW);
}

void loop() {
	Serial.begin(115200);
	WiFi.softAP("serverResetter", "123456789");

	WiFiServer initial_server{80};
	initial_server.begin();
	Serial.println("Beginning to await clients...");

	char ssid[MAX_SSID_LENGTH + 1] = "";
	char pass[MAX_PASSWORD_LENGTH + 1] = "";
	char host[MAX_HOST_LENGTH + 1] = "";

	bool credentials_initialized = false;
	while (!credentials_initialized) {
		WiFiClient client = initial_server.available();

		if (!client) {
			delay(100);
			continue;
		}
		Serial.println("Client connected.");

		const size_t ssid_length = client.readBytes(ssid, MAX_SSID_LENGTH);
		const size_t pass_length = client.readBytes(pass, MAX_PASSWORD_LENGTH);
		const size_t host_length = client.readBytes(host, MAX_HOST_LENGTH);

		ssid[ssid_length] = '\0';
		pass[pass_length] = '\0';
		host[host_length] = '\0';

		constexpr size_t SUCCESS_MESSAGE_LENGTH = 32;
		constexpr char SUCCESS_MESSAGE[SUCCESS_MESSAGE_LENGTH] = "Success uploading credentials";
		Serial.println("Success uploading credentials.");
		client.write(SUCCESS_MESSAGE, SUCCESS_MESSAGE_LENGTH);

		client.stop();
	}

	initial_server.stop();
	WiFi.softAPdisconnect(true);

	if (!WiFi.begin(ssid, pass)) {
		return;
	}

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}

    digitalWrite(2, LOW);
	Pinger pinger;

	uint8_t missed_responses = 0;
	constexpr uint8_t MAX_MISSED_RESPONSES = 4;

	pinger.OnReceive([&] (const PingerResponse& response) {
		if (!response.ReceivedResponse) {
			missed_responses += 1;
		} else {
			missed_responses = 0;
		}
		return true;
	});

	while (true) {
		if (missed_responses > MAX_MISSED_RESPONSES) {
			missed_responses = 0;
			digitalWrite(2, HIGH);
			delay(1000);
			digitalWrite(2, LOW);
			delay(1000 * 60 * 5);
		}
		pinger.Ping(host);
		delay(1000 * 15);
	}

}

