
#include <stdlib.h>
#include <string.h>

#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <AsyncUDP.h>
#include <ESPAsyncDNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>

#include <Adafruit_NeoPixel.h>

AsyncDNSServer dns;
IPAddress apIP(192, 168, 4, 1);
AsyncWebServer server(80); //Server on port 80

#define NEO_PIN       13 // On Trinket or Gemma, suggest changing this to 1

#define n_leds 256
Adafruit_NeoPixel pixels(n_leds, NEO_PIN, NEO_GRB + NEO_KHZ800);

int currentLED = 0;

void delayA(uint32_t ms) {
  uint32_t start_tick = millis();
  while((millis() - start_tick) < ms);
}

void loadWebFile(AsyncWebServerRequest *request, String filename) {
  if(!SPIFFS.exists(filename)) {
    Serial.print("URL Not Found: ");
    Serial.println(filename);
    request->send(404, "text/html", "<h1>Content Not Found</h1>");
  } else {
    request->send(request->beginResponse(SPIFFS, filename));
  }
}

void refreshLED() {
  pixels.clear();
  pixels.setPixelColor(currentLED, pixels.Color(0, 255, 0));
  pixels.show();
}

void setCurrentLED(int LEDIndex) {
  currentLED = LEDIndex;
  refreshLED();
}

void setup() {
  Serial.println("\nSetting up LEDs");
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  if(SPIFFS.begin(true)){
    Serial.println("SPIFFS initialized");
  } else {
    Serial.println("An Error has occurred while initializing SPIFFS");
    while(1);
  }
  for(int i=0; i<n_leds; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.

  Serial.println("\nStarting up WiFi Access Point");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("NeoPixelRanger");  //Start HOTspot removing password will disable security
  dns.start(53, "*", apIP);       //Start Captive Portal service
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(SPIFFS.exists("/index.html")) {
      refreshLED();
      loadWebFile(request, "/index.html");
    } else
      request->send(404, "text/html", "File /index.html Not Found. Please upload index.html file first!");
  });      //Which routine to handle at root location

  server.on("/get_current_led", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", String(currentLED));
  });

  server.on("/inc_current_led", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentLED++;
    currentLED = (currentLED % n_led);
    refreshLED();
    request->send(200, "text/html", String(currentLED));
  });

  server.on("/dec_current_led", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentLED--;
    if(currentLED < 0)
      currentLED = 0;
    refreshLED();
    request->send(200, "text/html", String(currentLED));
  });
  
  server.onNotFound([](AsyncWebServerRequest *request) {
    if(request->host() == apIP.toString()) {
      loadWebFile(request, request->url());
    } else {
      String redirURL = "http://";
      redirURL.concat(apIP.toString());
      redirURL.concat("/");
      request->redirect(redirURL);
    }
  });

  server.begin();
}

void loop() {
}
