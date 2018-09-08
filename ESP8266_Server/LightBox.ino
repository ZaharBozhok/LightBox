#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ShiftRegister74HC595.h>

#define AP_SSID "lightbox"
#define AP_PWD ""

#define SSID_EEPROM_ADDR 0
#define PWD_EEPROM_ADDR 34

#define MULTIPLIER 8
#define REGISTERS 8
#define SHIFT_REGISTERS REGISTERS*MULTIPLIER
uint8_t ledData[REGISTERS] = { 0 };

ESP8266WebServer server(80);
ShiftRegister74HC595 shift_resiters(
  REGISTERS,  //Shift registers amount
  13,         //Serial data pin
  14,         //Clock pin
  12);        //Latch pin

void handleRoot();
String loadStringFromEEPROM(int addr);
void saveStringToEEPROM(const String &str, int addr);
void enterConfiguringMode();
bool connectToAP();
void bootUpMDNS();
String getMacWithoutDots(void);
void handleConfigs();
void configWebServer();
void handleControl();


void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  if(connectToAP() == false)
    enterConfiguringMode();
  bootUpMDNS();
  configWebServer();
    
}

void loop() {
  server.handleClient();
  if(WiFi.status() ==  WL_CONNECTION_LOST)
    ESP.restart();
}

void configWebServer()
{
    server.on("/", handleRoot);
    server.on("/configs", handleConfigs);
    server.on("/control", handleControl);
    server.begin();
    Serial.println("HTTP server started");
  }

void bootUpMDNS()
{
  String mdnsName = ((String)AP_SSID)+ getMacWithoutDots();
  Serial.println(mdnsName);
  if (!MDNS.begin(mdnsName.c_str())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  }

bool connectToAP()
{
  String savedAPssid = loadStringFromEEPROM(SSID_EEPROM_ADDR);
  if(savedAPssid.length() > 0)
  {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    String pwd = loadStringFromEEPROM(PWD_EEPROM_ADDR);
    Serial.print("Password : ");
    Serial.println(pwd);
    WiFi.begin(savedAPssid.c_str(), pwd.c_str());
    Serial.print("Connecting to ");
    Serial.print(savedAPssid);
    Serial.println("...");
    for(int i=0; i<10; i++)
    {
      Serial.print("Try #");
      Serial.print(i+1);
      Serial.println("...");
      if(WiFi.status() == WL_CONNECTED)
      {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
        }
      delay(2000);
    }
  }
  return false;
  }

void enterConfiguringMode()
{
  Serial.print("Configuring access point...");
      WiFi.softAP((((String)AP_SSID)+ getMacWithoutDots()).c_str(), AP_PWD);
    
      IPAddress myIP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(myIP);
  }

void handleRoot()
{
  
  String response = 
  R"(<html><head></head><body> <form action="/configs" method="post"> <label>SSID: </label><input maxlength="32" type="text" name="accessPoint" value=")"
  + loadStringFromEEPROM(SSID_EEPROM_ADDR)
  + R"("><br><label>PWD: </label><input type="text" maxlength="64" name="password" value=")"
  + loadStringFromEEPROM(PWD_EEPROM_ADDR)
  + R"("><br><label>Reboot </label><input type="checkbox" name="reboot" value="true" checked/><br><input type="submit" value="Save"></form></body></html>)";
  server.send(200,"text/html",response.c_str());
  }
  
void handleConfigs()
{
  if(server.method() == HTTP_GET)
  {
    String response = String(R"({"accessPoint":")")
    +loadStringFromEEPROM(SSID_EEPROM_ADDR)
    +R"(","password":")"
    +loadStringFromEEPROM(PWD_EEPROM_ADDR)
    +R"(","mac":")"
    +WiFi.macAddress()
    +R"(","url":")"
    +"http://" + AP_SSID + getMacWithoutDots() + ".local/"
    +"\"}";
    server.send(200,"application/json",response.c_str());
  }
  else if(server.method() == HTTP_POST)
  {
    String ssid = server.arg("accessPoint");
    String password = server.arg("password");
    saveStringToEEPROM(ssid,SSID_EEPROM_ADDR);
    saveStringToEEPROM(password,PWD_EEPROM_ADDR);
    String reboot = server.arg("reboot");
    if(reboot == "true")
      ESP.restart();
    }
    server.send(200);
  }

void handleControl()
{
  String message = server.arg("ledsData");
  Serial.println("Bits : ");
  Serial.println(message);
  if(message.length() < SHIFT_REGISTERS)
  {
    server.send(400, "text/plain", "Wrong amount of bits");
    return;
  }
  shift_resiters.setAllLow();
  int it = 0;
  Serial.println("Convertation started...");
  for (int regs = 0; regs<REGISTERS; regs++)
  {
    ledData[regs] = 0;
    for (int shifts = 0; shifts<MULTIPLIER; shifts++, it++)
    {
      ledData[regs] |= ((message[it] == '1') << (shifts));
    }
    Serial.print((int)ledData[regs]);
    Serial.print(' ');
  }
  Serial.println();
  Serial.println("Convertation complete");
  shift_resiters.setAll(ledData);
  server.send(200);
  }
  
void saveStringToEEPROM(const String &str, int addr){
  Serial.print("Size : ");
  Serial.println(str.length());
  Serial.print("Value : ");
  Serial.println(str);
  EEPROM.write(addr++,str.length());
  Serial.println(str.length());
  for(int i=0; i<str.length(); i++,addr++)
  {
    EEPROM.write(addr,str[i]);
  }
  EEPROM.commit();
}

String loadStringFromEEPROM(int addr)
{
  int length = EEPROM.read(addr++);
  Serial.println(length);
  if(length == 255)
    return "";
  char *arr = new char[length+1];
  arr[length] = 0;
  for(int i=0; i<length; i++,addr++)
  {
    arr[i] = EEPROM.read(addr);
  }
  Serial.println(arr);
  String str(arr);
  delete[]arr;
  return str;
}

String getMacWithoutDots(void) {
    uint8_t mac[6];
    char macStr[18] = { 0 };
    wifi_get_macaddr(STATION_IF, mac);

    sprintf(macStr, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}
