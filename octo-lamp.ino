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

  setupServer();
}

void loop() {
  server.handleClient();
  runAnimation();
  delay(10);
}

void (*animation)(int d) = idle;
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

int idleIndex = 0;
boolean idleFilling = true;
int startTimer = millis();
int idleR = 0;
int idleG = 0;
int idleB = 0;
void idle(int d) {
  int now = millis();
  int redRange[2] = {64, 192};
  int greenRange[2] = {0, 128};
  int blueRange[2] = {64, 255};
  // rotate each color channel in the given range within 5 seconds gradually from one end to the other
  // bouncing back and forth
  // each channel starts at a different point in the cycle
  float cycle = 15000.0;
  int red = (int) (redRange[0] + (redRange[1] - redRange[0]) * (sin((now - startTimer) / cycle * 2 * PI + 0 * 2 * PI / 3) + 1) / 2);
  int green = (int) (greenRange[0] + (greenRange[1] - greenRange[0]) * (sin((now - startTimer) / cycle * 2 * PI + 1 * 2 * PI / 3) + 1) / 2);
  int blue = (int) (blueRange[0] + (blueRange[1] - blueRange[0]) * (sin((now - startTimer) / cycle * 2 * PI + 2 * 2 * PI / 3) + 1) / 2);
  idleR = red;
  idleG = green;
  idleB = blue;
  int p[1][4] = {0};
  p[0][0] = red;
  p[0][2] = blue;
  p[0][1] = green;
  p[0][3] = 1;
  setPixels(idleIndex, p, 1);
  if (idleFilling && idleIndex >= NUMPIXELS) {
    idleFilling = !idleFilling;
  } else if (!idleFilling && idleIndex <= 0) {
    idleFilling = !idleFilling;
  }
  if (idleFilling) {
    idleIndex++;
  } else {
    idleIndex--;
  }
}

void noIdle(int d) {
  int p[NUMPIXELS][4] = {0};
  setPixels(0, p, NUMPIXELS);
}

int allIndex = 0;
boolean allFilling = true;
void fillAll(int d, int r, int g, int b) {
  int p[1][4] = {0};
  p[0][0] = allFilling ? r : 0;
  p[0][1] = allFilling ? g : 0;
  p[0][2] = allFilling ? b : 0;
  p[0][3] = 1;
  setPixels(allIndex, p, 1);
  allIndex++;
  if (allIndex >= NUMPIXELS) {
    allFilling = !allFilling;
    allIndex = 0;
  }
}

int catIndex = 0;
boolean catFilling = true;
void fillCat(int d, int r, int g, int b) {
  int p[1][4] = {0};
  p[0][0] = catFilling ? r : 0;
  p[0][1] = catFilling ? g : 0;
  p[0][2] = catFilling ? b : 0;
  p[0][3] = 1;
  setPixels(RINGNUM + catIndex, p, 1);
  catIndex++;
  if (catIndex >= CATNUM) {
    catFilling = !catFilling;
    catIndex = 0;
  }
}

int ringIndex = 0;
boolean ringFilling = true;
void fillRing(int d, int r, int g, int b) {
  int p[1][4] = {0};
  p[0][0] = ringFilling ? r : 0;
  p[0][1] = ringFilling ? g : 0;
  p[0][2] = ringFilling ? b : 0;
  p[0][3] = 1;
  setPixels(ringIndex, p, 1);
  ringIndex++;
  if (ringIndex >= RINGNUM) {
    ringFilling = !ringFilling;
    ringIndex = 0;
  }
}

void star(int d) {
  int r = 255;
  int g = 165;
  int b = 0;
  fillCat(d, r, g, b);
  fillRing(d, r, g, b);
}

void commit(int d) {
  int r = 100;
  int g = 255;
  int b = 100;
  fillAll(d, r, g, b);
}

void setupServer() {
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Octo Lamp</h1>" + animationOptions());
  });
  server.on("/toggle", []() {
    isOn = !isOn;
    String msg = isOn ? "ON" : "OFF";
    server.send(200, "text/html", "<h1>Octo Lamp is now " + msg + "</h1>" + animationOptions());
  });
  server.on("/idle", []() {
    animation = idle;
    server.send(200, "text/html", "<h1>Octo Lamp is now Idleing</h1>" + animationOptions());
  });
  server.on("/no-idle", []() {
    animation = noIdle;
    server.send(200, "text/html", "<h1>Octo Lamp is now not Idleing</h1>" + animationOptions());
  });
  server.on("/star", []() {
    animation = star;
    server.send(200, "text/html", "<h1>Octo Lamp is now a star</h1>" + animationOptions());
  });
  server.on("/commit", []() {
    animation = commit;
    server.send(200, "text/html", "<h1>Octo Lamp is now committing</h1>" + animationOptions());
  });
  server.begin();
}

String animationOptions() {
  String html = "";
  String isOnStr = isOn ? "ON" : "OFF";
  html += "<a href='/toggle'>Toggle (now: " + isOnStr + ")</a><br>";
  html += "<a href='/idle'>Idle</a><br>";
  html += "<a href='/no-idle'>No Idle</a><br>";
  html += "<a href='/star'>Star</a><br>";
  html += "<a href='/commit'>Commit</a><br>";
  return html;
}
