#include <Arduino.h>
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char *wifi_ssid = "***";
const char *wifi_password = "***";
const char *hostName = "curtain";

AsyncWebServer server(80);

typedef enum {
  MESSAGE_OPEN,
  MESSAGE_CLOSE,
  MESSAGE_STOP
} Message;

const int PIN_OPEN = D6;
const int PIN_CLOSE = D5;
const int PIN_STOP = D7;
const int PIN_STATUS_LED = BUILTIN_LED;

int readMessageIndex = 0;
int writeMessageIndex = 0;
const int maxMessageCount = 8;
Message messageBuffer[maxMessageCount];

bool popMessage(Message *message) {
  noInterrupts();
  bool hasMessages = readMessageIndex != writeMessageIndex;
  if (hasMessages) {
    *message = messageBuffer[readMessageIndex];
    readMessageIndex = (readMessageIndex + 1) % maxMessageCount;
  }
  interrupts();
  return hasMessages;
}

bool pushMessage(Message message) {
  noInterrupts();
  int nextMessageIndex = (writeMessageIndex + 1) % maxMessageCount;
  bool canWrite = nextMessageIndex != readMessageIndex;
  if (canWrite) {
    messageBuffer[writeMessageIndex] = message;
    writeMessageIndex = nextMessageIndex;
  }
  interrupts();
  return canWrite;
}

void digitalPulse(int port) {
  digitalWrite(PIN_STATUS_LED, 1);
  digitalWrite(port, 0);
  delay(250);
  digitalWrite(PIN_STATUS_LED, 0);
  digitalWrite(port, 1);
  delay(250);
}

void handleMessageRequest(AsyncWebServerRequest *request, Message message) {
  if (pushMessage(message)) {
    request->send(200, "text/plain", "ok");
  } else {
    request->send(429, "text/plain", "too many requests");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_STATUS_LED, OUTPUT);

  Serial.println("       ");

  Serial.print("Connecting");
  WiFi.hostname(hostName);
  WiFi.setAutoReconnect(true);
  while (WiFi.begin(wifi_ssid, wifi_password) != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(PIN_STATUS_LED, 1);
    delay(90);
    digitalWrite(PIN_STATUS_LED, 0);
    delay(10);
  }
  Serial.println();

  Serial.println("Connected");

  Serial.print("localIP: ");
  Serial.println(WiFi.localIP());

  // Setup OTA
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    digitalWrite(PIN_STATUS_LED, progress % 3);
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

  // Setup HTTP
  server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request){
    handleMessageRequest(request, MESSAGE_OPEN);
  });

  server.on("/close", HTTP_GET, [](AsyncWebServerRequest *request){
    handleMessageRequest(request, MESSAGE_CLOSE);
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    handleMessageRequest(request, MESSAGE_STOP);
  });

  server.begin();

  // Setup pins
  pinMode(PIN_OPEN, OUTPUT);
  pinMode(PIN_CLOSE, OUTPUT);
  pinMode(PIN_STOP, OUTPUT);

  // Initialize pins
  digitalWrite(PIN_STATUS_LED, 0);
  digitalWrite(PIN_OPEN, 1);
  digitalWrite(PIN_CLOSE, 1);
  digitalWrite(PIN_STOP, 1);

  MDNS.addService("http", "tcp", 80);

  Serial.println("Ready");
}

void loop() {
  ArduinoOTA.handle();

  Message message;
  if (popMessage(&message)) {
    switch(message) {
      case MESSAGE_OPEN:
        Serial.println("Open");
        digitalPulse(PIN_OPEN);
        break;
      case MESSAGE_CLOSE:
        Serial.println("Close");
        digitalPulse(PIN_CLOSE);
        break;
      case MESSAGE_STOP:
        Serial.println("Stop");
        digitalPulse(PIN_STOP);
        break;
    }
  }
}
