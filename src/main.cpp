#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Pinger.h>

constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MAX_PASSWORD_LENGTH = 63;
constexpr size_t MAX_HOST_LENGTH = 32;

void setup() {
	pinMode(0, OUTPUT);
	pinMode(2, OUTPUT);

	Serial.begin(115200);
}

void loop() {
	digitalWrite(0, LOW);
	digitalWrite(2, LOW);

	WiFi.softAP("serverResetter", "123456789");

	// We avoid port 80 because some applications send pesky keep alives over HTTP
	WiFiServer initial_server{6554};
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
		Serial.println("Success receiving credentials.");
		client.write(SUCCESS_MESSAGE, SUCCESS_MESSAGE_LENGTH);
		credentials_initialized = true;

		client.stop();
	}

	initial_server.stop();
	WiFi.softAPdisconnect(true);

	if (!WiFi.begin(ssid, pass)) {
		Serial.println("Failed to connect to wifi, returning to idle states");
		WiFi.disconnect();
		return;
	}

	Serial.print("Trying to connect to network with SSID: ");
	Serial.print(ssid);
	Serial.print(" with pass: ");
	Serial.print(pass);
	Serial.print("\n");
	Serial.println("Waiting to finish connection...");

	unsigned int attempts = 0;
	constexpr unsigned int MAX_WIFI_CONNECT_ATTEMTPS = 60;
	while (WiFi.status() != WL_CONNECTED) {
		if (WiFi.status() == WL_CONNECT_FAILED || attempts >= MAX_WIFI_CONNECT_ATTEMTPS) {
			Serial.println("\nConnection failed, returning to idle state.");
			WiFi.disconnect();
			return;
		}
		Serial.print(".");
		attempts++;
		delay(500);
	}

	Serial.println("Successfully connected.");
    digitalWrite(2, LOW);
	Pinger pinger;

	uint8_t missed_responses = 0;
	constexpr uint8_t MAX_MISSED_RESPONSES = 32;

	pinger.OnReceive([&] (const PingerResponse& response) {
		if (!response.ReceivedResponse) {
			Serial.print("Could not receive response from host: ");
			Serial.println(host);
			Serial.print("Missed ping count at: ");
			Serial.println(missed_responses + 1);

			missed_responses += 1;
		} else {
			Serial.print("Received ICMP response from host: ");
			Serial.println(host);
			missed_responses = 0;
		}
		return true;
	});

	Serial.print("Beginning to ping host: ");
	Serial.println(host);
	while (true) {
		if (missed_responses > MAX_MISSED_RESPONSES) {
			Serial.print("Too many pings missed, resetting...");
			missed_responses = 0;
			digitalWrite(2, HIGH);
			delay(5000);
			digitalWrite(2, LOW);
			delay(1000 * 60 * 5);
		}

		Serial.print("Sending ping to host: ");
		Serial.println(host);
		pinger.Ping(host);
		delay(1000 * 15);
	}

}

