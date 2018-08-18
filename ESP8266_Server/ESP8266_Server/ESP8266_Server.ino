#include <ShiftRegister74HC595.h>

#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WiFi.h>

const char* ssid = "DearLab";
const char* password = "dearlab1608";

#define MULTIPLIER 8
#define REGISTERS 2
#define SHIFT_REGISTERS REGISTERS*MULTIPLIER
uint8_t ledData[REGISTERS] = { 0 };

//ShiftRegister74HC595(int numberOfShiftRegisters, int serialDataPin, int clockPin, int latchPin);
ShiftRegister74HC595 shift_resiters(REGISTERS, D0, D1, D2);
ESP8266WebServer server(80);


void processPostData() {
	String message = server.arg("ledsData");
	int it = 0;
	for (int regs = 0; regs<REGISTERS; regs++)
	{
		ledData[regs] = 0;
		for (int shifts = 0; shifts<MULTIPLIER; shifts++, it++)
		{
			ledData[regs] += ((message[it] == '1') << (shifts));
		}
		Serial.print((int)ledData[regs]);
		Serial.println(' ');
	}
	shift_resiters.setAll(ledData);
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.send(200, "text/plain", message);
}
void setup() {
	Serial.begin(115200);
	delay(10);

	// Connect to WiFi network
	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");

	// Start the server
	server.begin();
	Serial.println("Server started");

	// Print the IP address
	Serial.println(WiFi.localIP());

	server.on("/raw/", processPostData);
}

void loop() {
	server.handleClient();
}
