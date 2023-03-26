#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define PIXELPIN       D4
#define NUMPIXELS      98
#define RINGNUM        68
#define CATNUM         30
float BRIGHTNESS = 0.4;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);

// all pixel states are stored in this 2d array
int pixels[NUMPIXELS][4] = {0};
boolean isOn = true;

void setup() {
  Serial.begin(115200);

  // setup neopixel
  strip.begin();
  for(int i=0;i<NUMPIXELS;i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();

  // setup WiFi
  WiFi.begin("WiFi", "Password");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    strip.setPixelColor(i, strip.Color(0,0,255));
    strip.show();
    if (i >= NUMPIXELS) {
      i = 0;
    }
    i++;
    delay(100);
  }
  for(int i=0;i<NUMPIXELS;i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
  Serial.print("Use this URL: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}

void loop() {
  server.handleClient();
  runAnimation();
  delay(10);
}

void (*animation)(int d) = idel;
int animationTime = millis();
void runAnimation() {
  int now = millis();
  int delta = now - animationTime;
  if (BRIGHTNESS > 0) {
    animation(delta);
  }
  if (isOn) {
    if (BRIGHTNESS < 0.4) {
      BRIGHTNESS += 0.4 / 500 * delta;
    } else {
      BRIGHTNESS = 0.4;
    }
  } else {
    if (BRIGHTNESS > 0) {
      BRIGHTNESS -= 0.4 / 500 * delta;
    } else {
      BRIGHTNESS = 0;
      for(int i=0;i<NUMPIXELS;i++) {
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
      strip.show();
    }
  }
  animationTime = now;
}

void setPixels(int s, int p[][4], int n) {
  for (int i = s; i < s + n; i++) {
    pixels[i][0] = p[i - s][0];
    pixels[i][1] = p[i - s][1];
    pixels[i][2] = p[i - s][2];
    pixels[i][3] = p[i - s][3];
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    int r = pixels[i][0] * pixels[i][3] * BRIGHTNESS;
    int g = pixels[i][1] * pixels[i][3] * BRIGHTNESS;
    int b = pixels[i][2] * pixels[i][3] * BRIGHTNESS;
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

