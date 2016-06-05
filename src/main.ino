#include <Arduino.h>
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

typedef enum {
  MESSAGE_OPEN,
  MESSAGE_CLOSE,
  MESSAGE_STOP
} Message;

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
  Serial.print("digitalPulse ");
  Serial.println(port);

  digitalWrite(BUILTIN_LED, 1);
  digitalWrite(port, 1);
  delay(250);
  digitalWrite(BUILTIN_LED, 0);
  digitalWrite(port, 0);
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
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.println("       ");

  Serial.print("Connecting");
  while (WiFi.begin("***", "***") != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(BUILTIN_LED, 1);
    delay(90);
    digitalWrite(BUILTIN_LED, 0);
    delay(10);
  }
  Serial.println();

  Serial.println("Connected");

  Serial.print("localIP: ");
  Serial.println(WiFi.localIP());

  // Setup OTA
  ArduinoOTA.setHostname("curtain");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    digitalWrite(BUILTIN_LED, progress % 3);
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
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);

  // Initialize pins
  digitalWrite(BUILTIN_LED, 0);
  digitalWrite(D1, 0);
  digitalWrite(D2, 0);
  digitalWrite(D3, 0);

  Serial.println("Ready");
}

void loop() {
  ArduinoOTA.handle();

  Message message;
  if (popMessage(&message)) {
    switch(message) {
      case MESSAGE_OPEN:
        digitalPulse(D1);
        break;
      case MESSAGE_CLOSE:
        digitalPulse(D2);
        break;
      case MESSAGE_STOP:
        digitalPulse(D3);
        break;
    }
  }
}
