#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

void setup() {
  Serial.begin(115200);
  Serial.println("       ");

  Serial.print("Connecting");
  while (WiFi.begin("***", "***") != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  Serial.println("Connected");

  Serial.print("localIP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("curtain");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");

  pinMode(BUILTIN_LED, OUTPUT);  // initialize onboard LED as output
}

void loop() {
  ArduinoOTA.handle();
}
