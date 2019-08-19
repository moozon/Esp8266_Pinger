/*
    Name:       Esp8266_Pinger.ino
    Created:	18.08.2019 23:47:40
    Author:     Michael
*/

// EEPROM
#include <EEPROM.h>

// ESP8266Ping
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>

// TimeNTP
#include <TimeLib.h>
#include <WiFiUdp.h>

// OneButton
#include "OneButton.h"

// Oled
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OneButton Defines
#define BUTTON_PIN D3
#define BUTTON_PIN2 D7
// Interrupt Button Define
//#define INTERRUPT_PIN D7

// Oled Defines
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// SCL=D1 , SDA=D2 - Default I2C pins on NodeMCU
#define OLED_RESET     2 // 2 pin on NodeMCU Reset pin # (or -1 if sharing Arduino reset pin)
//#define OLED_RESET     4 // Default pin

// User Define
#define PING_AVERAGE_SIZE 10


char ssid[] = "HTC Portable Hotspot";  //  your network SSID (name)
char pass[] = "1234567890";       // your network password

const char* remote_host = "ya.ru";
//const IPAddress remote_host(192, 168, 1, 24);

// OneButton Init
OneButton button(BUTTON_PIN, true);

// Oled Init
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//uint8_t textSize = 1;

// NTP Init

//static const char ntpServerName[] = "ntp1.stratum2.ru";
static const char ntpServerName[] = "ru.pool.ntp.org";
//static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time.windows.com";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = 5;     // UTC+5 Yekaterinburg Time zone
//const int timeZone = 3;     // UTC+3 Moscow Time zone
//const int timeZone = 1;     // Central European Time

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
time_t getNtpTime();
time_t prevDisplay = 0; // when the digital clock was displayed
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
void oledPrint(const char str[], uint8_t textSize = 1, uint8_t x = 0, uint8_t y = 0);

// User vars
uint16_t maxPing, minPing = 1000;
int counter, loss;
uint8_t screen = 0;
bool noNTP = false;
//bool x = false;
//uint8_t x = 0;
//uint8_t y = 0;
//pingAvgSize = 100;
int pingAvgArr[PING_AVERAGE_SIZE] = { 0 };
uint8_t pingAvgCount = PING_AVERAGE_SIZE;
int pingAvg = 0;
unsigned long currentTime = 0;
uint16_t pingDelay = 1000;

int _delay = 0;
uint16_t memoryDelay = 0;
//uint16_t pingDelay = 0;
//uint16_t pingDelay = 0;

//unsigned long currentTimeDelay = 0;

unsigned long timeFunc = 0;

bool rssiFlag = false;


// myDelay
unsigned long delayCurTimeArr[10] = { 0 };
uint8_t delayFlagArr[10] = { 0 };
callbackFunction delayFuncArr[10] = { NULL };
unsigned long delayDelayArr[10] = { 0 };
void myDelay(callbackFunction function, uint16_t delay = 1000, bool stopAnother = false);

// EEPROM
int addrEEPROM = 0;
byte valEEPROM = 0;
uint8_t sizeSSID_EEPROM = 20;
uint8_t sizePass_EEPROM = 10;
uint8_t sizeSSIDAdr_EEPROM = 50;
uint8_t sizePassAdr_EEPROM = 51;


// ----- Callback function types -----
extern "C" {
	typedef void(*callbackFunction)(void);
}
callbackFunction _myFunc = NULL;



void setup()
{
	Serial.begin(115200);
	delay(10);
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	//pinMode(INTERRUPT_PIN, INPUT_PULLUP);

	// EEPROM Setup
	EEPROM.begin(512);
	
	EEPROM.put(addrEEPROM, ssid);
	EEPROM.put(addrEEPROM + sizeof(ssid), pass);

	Serial.println("write");

	uint8_t ssidEEPROM[16] = { 0 };
	uint8_t passEEPROM[16] = { 0 };

	//for (size_t i = 0; i < sizeSSID_EEPROM - 1; i++)
	//{
	//	//char dff[10] = { 0 };
	//	
	//	EEPROM.get(addrEEPROM, ssid);
	//	EEPROM.get(addrEEPROM + 16, pass);
	//	//ssidEEPROM[i] = EEPROM.read(addrEEPROM);
	//	
	//}
	
	EEPROM.get(addrEEPROM, ssid);
	Serial.println(ssid);
	EEPROM.get(sizeSSIDAdr_EEPROM, sizeSSID_EEPROM);
	EEPROM.get(sizePassAdr_EEPROM, sizePass_EEPROM);
	for (uint8_t i = sizeof(ssid) - 1; i > sizeSSID_EEPROM - 1; i--)
	{
		ssid[i] = 0;
	}

	EEPROM.get(addrEEPROM + sizeSSID_EEPROM, pass);
	Serial.println(pass);
	for (uint8_t i = sizeof(pass) - 1; i > sizePass_EEPROM - 1; i--)
	{
		pass[i] = 0;
	}

	Serial.println("read");
	Serial.println(ssid);
	Serial.println(pass);

	// Oled Setup
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
		Serial.println(F("SSD1306 allocation failed"));
		for (;;); // Don't proceed, loop forever
	}
	//display.setTextSize(textSize);             // Draw 2X-scale text	
	display.setTextColor(WHITE);
	
	// Change SSID Setup
	oledPrint("Press button if you want to connect to another SSID whithin 5 second.");
	Serial.println();
	Serial.println("Press button if you want to connect to another SSID whithin 5 second.");
	unsigned long now = millis();
	while (1) {
		wdt_reset();
		if (millis() - now < 5000) {
			if (digitalRead(BUTTON_PIN) == 0) {
				oledPrint("Look at the Serial Monitor please.");
				Serial.println("Want you set new SSID and Password? (Y/N)");
				while (!Serial.available()) {}
				Serial.println("Read");
				if (Serial.read() == 'y') {
					delay(10);
					Serial.read();
					Serial.read();
					Serial.println("Please enter new SSID");
					while (!Serial.available()) {}
					String newSSID = Serial.readString();
					Serial.print("New SSID is set: " + newSSID);
					Serial.read();
					//Serial.println("OK");
					Serial.println("Please enter new password");
					while (!Serial.available()) {}
					String newPass = Serial.readString();
					Serial.print("New password is set: " + newPass);
					Serial.read();
					//Serial.println("OK");
					newSSID.toCharArray((char*)ssid, (newSSID.length() - 1), 0);
					newPass.toCharArray((char*)pass, (newPass.length() - 1), 0);
					Serial.println("New SSID and Password were set successfully: ");
					Serial.println(ssid);
					Serial.println(pass);	
					sizeSSID_EEPROM = sizeof(ssid);
					sizePass_EEPROM = sizeof(pass);
					EEPROM.put(addrEEPROM, ssid);
					EEPROM.put(sizeSSIDAdr_EEPROM, sizeSSID_EEPROM);
					EEPROM.put(addrEEPROM + sizeSSID_EEPROM, pass);
					EEPROM.put(sizePassAdr_EEPROM, sizePass_EEPROM);
					//EEPROM.commit();
					//EEPROM.end();
					
					break;
				}
			}
		}
		else {
			break;
		}				
	}
	
	Serial.println("SSID no changed");	
	
	// OneButton Setup	
	//button.setPressTicks(100);	// Время регистрации длительного нажатия кнопки, По умолчанию 1 секунда
	//button.setDebounceTicks(80);	// Время задержки для устранения дребезга
	button.attachClick(pressed);	// Происходит после отпускания кнопки
	//button.attachLongPressStart(pressed);	// Происходит в момент нажатия кнопки
	//button.attachDuringLongPress(pressed); // Происходит, когда кнопки нажата
	//button.attachLongPressStop(pressStop);	// Происходит в момент отпускания кнопки


	

	// WiFi Setup
	Serial.println();
	Serial.println("Connecting to " + String(ssid));
	char strSSID[24] = "%s %s";
	sprintf(strSSID, "Connecting to ", ssid);
	oledPrint(strSSID);
	WiFi.begin(ssid, pass);
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		Serial.print(".");
		if (display.getCursorX() == 126 && display.getCursorY() == 56) {
			oledPrint("Connecting to WiFi");
		}
		display.print('.');
		display.display();
	}
	Serial.println();
	Serial.print("WiFi connected with ip ");
	oledPrint("WiFi is connected.");
	display.setCursor(0, 16);
	display.print("IP:  " + WiFi.localIP().toString());
	display.display();
	delay(5000);
	Serial.println(WiFi.localIP());
	Serial.println();

	// NTP Setup
	Udp.begin(localPort);
	Serial.print("Local port: ");
	Serial.println(Udp.localPort());
	Serial.println("waiting for sync");
	setSyncProvider(getNtpTime);
	setSyncInterval(300);


	Serial.println("Ping: " + String(remote_host));

	//attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), pressedINT, FALLING);
	currentTime = millis();
	//currentTimeDelay = millis();
}

void loop()
{
	myDelayMain();
	//myDelay(test1);
	//myDelay(test2, 5000);

	// debug print
	//Serial.println(noNTP);
	//Serial.println(x);
	delay(10);

	button.tick();

	// Ping
	if (screen == 0) {
		myDelay(pingDisplay, 1000, true);
		/*if ((millis() - currentTime) >= pingDelay) {
			pingDisplay();
			currentTime = millis();
		}*/
	}
	// DateTime
	else if (screen == 1) {
		if (noNTP) {
			myDelay(noNTPDisplay, 1000, true);
			//delay(1000);
			//return;
		}
		else {
			myDelay(timeDisplay, 0, true);
		}
	}

	// WiFi Signal(RSSI)
	else if (screen == 2) {
		myDelay(rssiDisplay, 1000, true);

		//rssiFlag = true;
		//myDelay(10000000, rssiDisplay);

		/*rssiDisplay();
		delay(1000);*/
	}
	else if (screen == 3) {
		myDelay(memoryDisplay, 1000, true);
		//oledPrint("Menory", 2);
		//myDelay(20000000, memoryDisplay);

		/*memoryDisplay();
		delay(1000);*/
	}
	else {
		screen = 0;
	}
}

// My funcs
void noNTPDisplay() {
	Serial.println("No NTP Response");
	oledPrint("No NTP Response");
}
void test1() {
	Serial.println("Test1");
	//delay(2000);
}
void test2() {
	Serial.println("Test2");
}

void myDelay(callbackFunction function, uint16_t delay, bool stopAnother) {
	for (int i = 0; i < sizeof(delayFlagArr); i++) {
		if (function == delayFuncArr[i]) {
			//Serial.println("Delay with that function name has been already created. Exit: ");
			return;
		}
	}

	for (int i = 0; i < sizeof(delayFlagArr); i++)
	{
		if (delayFlagArr[i] == 0) {
			if (stopAnother) {
				for (int i = 0; i < sizeof(delayFlagArr); i++) {
					delayFlagArr[i] = 0;
				}
			}
			delayFlagArr[i] = 1;
			delayCurTimeArr[i] = millis();
			delayFuncArr[i] = function;
			delayDelayArr[i] = delay;
			// Debug
			Serial.println("Create new delay: ");

			return;
		}
		else {
			Serial.println("Attention! You can no more create delay function!");
		}
	}


	//if (_delay == delay) {
	//	//_myFunc = myFunction;
	//	myFunction();
	//	Serial.println("Time of function: " + String(timeFunc));
	//	Serial.println("_delay: " + String(_delay));
	//	_delay = 0;
	//	return;
	//}
	////Serial.println(_delay);
	//_delay++;
}
void myDelayMain() {
	for (int i = 0; i < sizeof(delayFlagArr); i++)
	{
		if (delayFlagArr[i] != 0) {
			if (millis() - delayCurTimeArr[i] >= delayDelayArr[i]) {
				_myFunc = delayFuncArr[i];
				_myFunc();
				delayCurTimeArr[i] = millis();
			}
		}
	}
}


void rssiDisplay() {
	long rssi = WiFi.RSSI();
	Serial.println("RSSI:" + String(rssi));

	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(10, 0);
	display.print("WiFi RSSI");
	display.setTextSize(3);
	display.setCursor(0, 30);
	display.print(rssi + String(" dB"));
	//display.print("RSSI: " + String(rssi));
	display.display();
	//const char s[] = String.(rssi);

	//oledPrint(String(rssi), 3, 0, 20);

}
void memoryDisplay() {
	oledPrint("Memory:", 2);
	Serial.print("ESP.getBootMode(); ");
	Serial.println(ESP.getBootMode());
	Serial.print("ESP.getSdkVersion(); ");
	Serial.println(ESP.getSdkVersion());
	Serial.print("ESP.getBootVersion(); ");
	Serial.println(ESP.getBootVersion());
	Serial.print("ESP.getChipId(); ");
	Serial.println(ESP.getChipId());
	Serial.print("ESP.getFlashChipSize(); ");
	Serial.println(ESP.getFlashChipSize());
	Serial.print("ESP.getFlashChipRealSize(); ");
	Serial.println(ESP.getFlashChipRealSize());
	Serial.print("ESP.getFlashChipSizeByChipId(); ");
	Serial.println(ESP.getFlashChipSizeByChipId());
	Serial.print("ESP.getFlashChipId(); ");
	Serial.println(ESP.getFlashChipId());
	Serial.print("ESP.getFreeHeap(); ");
	Serial.println(ESP.getFreeHeap());
	Serial.print("ESP.checkFlashConfig(); ");
	Serial.println(ESP.checkFlashConfig());
	Serial.print("ESP.getFlashChipSpeed(); ");
	Serial.println(ESP.getFlashChipSpeed());
	Serial.print("ESP.getFreeContStack(); ");
	Serial.println(ESP.getFreeContStack());

	Serial.print("ESP.getCpuFreqMHz(); ");
	Serial.println(ESP.getCpuFreqMHz());
	Serial.print("ESP.getCycleCount(); ");
	Serial.println(ESP.getCycleCount());
	Serial.print("ESP.getFreeSketchSpace(); ");
	Serial.println(ESP.getFreeSketchSpace());
	Serial.print("ESP.getFullVersion(); ");
	Serial.println(ESP.getFullVersion());

	Serial.print("ESP.getHeapFragmentation(); ");
	Serial.println(ESP.getHeapFragmentation());
	//Serial.print("ESP.getHeapStats(); ");
	//Serial.println(ESP.getHeapStats());
	Serial.print("ESP.getMaxFreeBlockSize(); ");
	Serial.println(ESP.getMaxFreeBlockSize());

	Serial.print("ESP.getResetInfo(); ");
	Serial.println(ESP.getResetInfo());

	Serial.print("ESP.getResetReason(); ");
	Serial.println(ESP.getResetReason());
	Serial.print("ESP.getVcc(); ");
	Serial.println(ESP.getVcc());


	//ESP.restart();


	//heap = ESP.getFreeHeap();
}

void pressed() {
	if (screen == 3)
		screen = 0;
	else
		screen++;

	/*Serial.println("Pressed");
	display.clearDisplay();
	display.setCursor(0, 20);
	display.print("Pressed");
	display.display();
	delay(100);*/
}
void pressedINT() {
	if (digitalRead(D7) == LOW) {
		if (screen == 2)
			screen = 0;
		else
			screen++;

	}
	//Serial.println("pressedINT");

	//delay(20);
	//display.clearDisplay();
	//display.setCursor(0, 20);
	//display.print("INT");
	//display.display();
}
void pressStart() {
	Serial.println("Mode is changed");
	//mode = !mode;
	display.clearDisplay();
	display.setCursor(0, 20);
	display.print("Pressed");
	display.display();
}
void oledPrint(const char str[], uint8_t textSize, uint8_t x, uint8_t y) {
	display.clearDisplay();
	display.setCursor(x, y);
	display.setTextSize(textSize);
	display.print(str);
	display.display();
}
void timeDisplay() {
	if (timeStatus() != timeNotSet) {
		if (now() != prevDisplay) { //update the display only if time has changed
			prevDisplay = now();
			digitalClockDisplay();
		}
	}
}
void pingDisplay() {
	//Serial.println("Screen0");
	display.clearDisplay();
	display.setCursor(0, 0);

	if (Ping.ping(remote_host, 1)) { //Send 1 packages. Def 5
		int _averageTime = Ping.averageTime();
		if (pingAvgCount == 0) {
			int avg = 0;
			for (uint8_t i = 0; i < PING_AVERAGE_SIZE; i++) {
				avg += pingAvgArr[i];
			}
			avg = avg / PING_AVERAGE_SIZE;
			pingAvg = avg;
			pingAvgCount = (PING_AVERAGE_SIZE - 1);
		}
		else {
			pingAvgCount--;
		}
		pingAvgArr[pingAvgCount] = _averageTime;
		if (_averageTime > maxPing) maxPing = _averageTime;
		if (_averageTime < minPing) minPing = _averageTime;
		//t = millis() - t;
		Serial.println(String("Time: ") +
			String(Ping.averageTime()) +
			String(" ms    Max: ") +
			String(maxPing) +
			String(" ms    Min: ") +
			String(minPing) +
			String("    Count: ") +
			String(counter) +
			String("    Loss: ") +
			String(loss) +
			String("    Avg: ") +
			String(pingAvg));
		display.setTextSize(1);
		display.print("Ping: " + String(remote_host));
		/*if (typeof(remote_host) == 0)
			display.print("Ping: " + String(remote_host));
		else
			display.print("Ping: " + remote_host.toString());*/

		display.setCursor(0, 16);
		display.setTextSize(2);
		//Serial.println(Ping.averageTime() + String(" ms"));
		//display.print("Cur: " + String(Ping.averageTime()));
		display.print(_averageTime);
		display.setCursor(0, 37);
		display.setTextSize(1);
		display.print("Avg: " + String(pingAvg));
		//display.print("Avg(" + String(PING_AVERAGE_SIZE) + String("): ") + String(pingAvg));
		//display.print("Max: " + String(maxPing) + String("   Min: ") + String(minPing));
		display.setCursor(0, 47);
		display.print("Max: " + String(maxPing));
		display.setCursor(70, 47);
		display.print("Min: " + String(minPing));
		display.setCursor(0, 57);
		display.print("Cnt: " + String(counter));
		display.setCursor(70, 57);
		display.print("Err: " + String(loss));
		display.display();
		counter++;
	}
	else {
		Serial.println("Loss!!!");

		display.clearDisplay();
		display.setCursor(0, 0);
		display.setTextSize(1);
		display.print("Ping: " + String(remote_host));
		/*if (typeof(remote_host) == 0)
			display.print("Ping: " + String(remote_host));
		else
			display.print("Ping: " + remote_host.toString());*/
		display.setCursor(0, 18);
		display.setTextSize(2);
		//Serial.println(Ping.averageTime() + String(" ms"));
		display.print("Loss!!!");
		display.setCursor(0, 57);
		display.setTextSize(1);
		display.print("Loss: " + String(loss));

		display.display();
		loss++;
	}
}

/*-------- NTP code ----------*/
void digitalClockDisplay()
{
	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(10, 0);
	display.print("Time/Date ");

	// digital clock display of the time
	display.setCursor(20, 25);
	Serial.print(hour());
	display.print(hour());
	printDigits(minute());
	printDigits(second());
	display.setCursor(5, 50);
	Serial.print(" ");
	Serial.print(day());
	display.print(day());
	Serial.print(".");
	display.print(".");
	Serial.print(month());
	display.print(month());
	Serial.print(".");
	display.print(".");
	Serial.print(year());
	display.print(year());
	Serial.println();

	// Display Oled buffer
	display.display();

}
void printDigits(int digits)
{
	// utility for digital clock display: prints preceding colon and leading 0
	Serial.print(":");
	display.print(":");
	if (digits < 10) {
		Serial.print('0');
		display.print('0');
	}
	Serial.print(digits);
	display.print(digits);
}
time_t getNtpTime()
{
	IPAddress ntpServerIP; // NTP server's ip address

	while (Udp.parsePacket() > 0); // discard any previously received packets
	Serial.println("Transmit NTP Request");
	// get a random server from the pool
	WiFi.hostByName(ntpServerName, ntpServerIP);
	Serial.print(ntpServerName);
	Serial.print(": ");
	Serial.println(ntpServerIP);
	sendNTPpacket(ntpServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			Serial.println("Receive NTP Response");
			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			noNTP = false;
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
	}
	Serial.println("No NTP Response :-(");
	oledPrint("No NTP Response");
	noNTP = true;
	//delay(3000);
	return 0; // return 0 if unable to get the time
}
void sendNTPpacket(IPAddress &address)	// send an NTP request to the time server at the given address
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}



