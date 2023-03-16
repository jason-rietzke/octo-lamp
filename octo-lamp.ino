#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define PIXELPIN       D4
#define NUMPIXELS      98
#define BRIGHTNESS     0.4
#define RINGNUM        68
#define CATNUM         30
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  // setup neopixel
  pixels.begin();
  for(int i=0;i<NUMPIXELS;i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
  pixels.show();

  // setup WiFi
  WiFi.begin("WiFi", "Password");
  breathSetup();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("connected");
  Serial.print("Use this URL: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}


void loop() {
  server.handleClient();
  delay(100);
}
