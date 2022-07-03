#include <ESP8266NetBIOS.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

ESP8266WebServer server(80);

AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;
// char *URL;
bool audioFlag = false;
String nameAP;
String playingnow;

bool testWifi(void);
void launchWeb(void);
void checkWhetherConnected(String ssid, String password, bool fromMemory = false);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512); 

  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }

  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.println(esid);
  Serial.println(epass);

  WiFi.begin(esid.c_str(), epass.c_str());
  nameAP = esid.c_str();
  checkWhetherConnected(esid.c_str(), epass.c_str(), true);
  // if (!NBNS.begin("ztu")) { // ztu.local
  //   Serial.println("Error setting up MDNS responder!");
  // }
  server.onNotFound(handleShowApList); 
  server.on("/apList", handleShowApList);
  server.on("/connect", handleConnect);
  server.on("/url", handleURL);
  server.on("/apname", [](){server.send(200, "text/html", nameAP);});
  server.on("/playingnow", [](){server.send(200, "text/html", playingnow);});

  server.begin();
}
void checkWhetherConnected(String ssid, String password, bool fromMemory){
  if (testWifi())
    {
      // server.on("/apList", handleShowApList);
      WiFi.softAPdisconnect(true);

      Serial.println("inside");
      if(fromMemory)
        return;

      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { //clearing eeprom
        EEPROM.write(i, 0);
      }

      for (int i = 0; i < ssid.length(); ++i)
      {
        EEPROM.write(i, ssid[i]);
        Serial.print("Wrote: ");
        Serial.println(ssid[i]);
      }
      Serial.println("writing eeprom pass:");
      for (int i = 0; i < password.length(); ++i)
      {
        EEPROM.write(32 + i, password[i]);
        Serial.print("Wrote: ");
        Serial.println(password[i]);
      }
      EEPROM.commit();
    }
    else
    {
      Serial.println("outside");
      launchWeb();
    }
}
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    Serial.println(WiFi.status());
    Serial.println(c);
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(1000);
    Serial.print("*");
    c++;
  }
  return false;
}
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() != WL_CONNECTED)
    WiFi.softAP("ZTU","password");
  else 
    Serial.println("WiFi connected");

  // server.on("/connect", handleConnect);
}

void handleShowApList() {;
  int ssidNum = WiFi.scanNetworks();
  if( ssidNum > 0 ){  
  String resultJson = "{ \"Points\": [";

  for (int i = 0; i < ssidNum; i++) {
    resultJson += "\"" + WiFi.SSID(i) + "\"";
    if (i != ssidNum - 1)
      resultJson += ",";
  };
  resultJson += "] }";

  server.send(200, "application/json", resultJson);
  }
  else
    server.send(200, "application/json", "{\"No available points\" } ");
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  WiFi.begin(ssid, password);

  checkWhetherConnected(ssid, password);
}

void handleURL() {
  audioFlag = false;
  char charBuf[50];
  String stringUrl = server.arg("url");
  Serial.printf(stringUrl.c_str());
  stringUrl.toCharArray(charBuf, 50);
  
  const char *URL = charBuf;

  file = new AudioFileSourceHTTPStream(URL);
  buff = new AudioFileSourceBuffer(file, 2048);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->begin(buff, out);

  audioFlag = true;
  playingnow = stringUrl.c_str();

  server.send(200, "text/html", nameAP);
}

void loop() {
  server.handleClient();

  static int lastms = 0;
  if(audioFlag){
  if (mp3->isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
    }
    if (!mp3->loop()) mp3->stop();
    } else {
      audioFlag = false;
    }
  }
}
