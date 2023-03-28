#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>

#define PIXELPIN       D4
#define NUMPIXELS      98
#define RINGNUM        68
#define CATNUM         30
float BRIGHTNESS = 0.4;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);

const char *WIFI_SSID = "ssid";           // your network SSID (name)
const char *WIFI_PASSWORD = "paddword";   // your network key

String SERVER_URL = "http://server.url/event";  // your server url
String SERVER_SECRET = "server_secret";         // your server secret

WiFiClient wifi;
HttpClient client = HttpClient(wifi, SERVER_URL.c_str(), 80);
boolean fetching = true;
int lastFetch = 0;
int fetchInterval = 15000;

// all pixel states are stored in this 2d array
int pixels[NUMPIXELS][4] = {0};
boolean isOn = true;
void (*animation)(int d) = nullptr;
int animationTime = millis();
boolean isOverride = false;
float overrideDuration = 0;
void (*overrideAnimation) (int d) = nullptr;

void setup() {
  Serial.begin(115200);

  // setup neopixel
  strip.begin();
  for(int i=0;i<NUMPIXELS;i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();

  // setup WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int i = 0;
  boolean fill = true;
  while (WiFi.status() != WL_CONNECTED) {
    strip.setPixelColor(i, fill ? strip.Color(0,0,255) : strip.Color(0,0,0));
    strip.show();
    i++;
    if (i > NUMPIXELS) {
      i = 0;
      fill = !fill;
    }
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

  EEPROM.begin(1);
  int state = 0;
  EEPROM.get(0, state);
  if (state == 0) {
    isOn = false;
    animation = idle;
  } else if (state == 2) {
    isOn = true;
    animation = star;
  } else if (state == 3) {
    isOn = true;
    animation = commit;
  } else if (state == 4) {
    isOn = true;
    animation = alert;
  } else {
    isOn = true;
    animation = idle;
  }
  lastFetch = fetchInterval * -1;
}

void loop() {
  server.handleClient();
  fetchEvents();
  if (isOverride) {
    overrideHandler();
  }
  runAnimation();
  delay(10);
}

void runAnimation() {
  if (fetching) {
    return;
  }
  int now = millis();
  int delta = now - animationTime;
  animation(delta);
  if (!isOverride) {
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

int allIndex = 0;
boolean allFilling = true;
void fillAll(int d, int r, int g, int b, boolean clear = true) {
  int p[1][4] = {0};
  p[0][0] = clear ? (allFilling ? r : 0) : r;
  p[0][1] = clear ? (allFilling ? g : 0) : g;
  p[0][2] = clear ? (allFilling ? b : 0) : b;
  p[0][3] = 1;
  setPixels(allIndex, p, 1);
  allIndex++;
  if (allIndex >= NUMPIXELS) {
    fetching = true;
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
    fetching = true;
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
    fetching = true;
    ringFilling = !ringFilling;
    ringIndex = 0;
  }
}

int idleIndex = 0;
boolean idleFilling = true;
int startTimer = millis();
int idleR = 0;
int idleG = 0;
int idleB = 0;
void idle(int d) {
  int now = millis();
  int delta = now - startTimer;
  int redRange[2] = {64, 192};
  int greenRange[2] = {0, 128};
  int blueRange[2] = {64, 255};
  // rotate each color channel in the given range within 5 seconds gradually from one end to the other
  // bouncing back and forth
  // each channel starts at a different point in the cycle
  float cycle = 15000.0;
  int red = (int) (redRange[0] + (redRange[1] - redRange[0]) * (sin((delta) / cycle * 2 * PI + 0 * 2 * PI / 3) + 1) / 2);
  int green = (int) (greenRange[0] + (greenRange[1] - greenRange[0]) * (sin((delta) / cycle * 2 * PI + 1 * 2 * PI / 3) + 1) / 2);
  int blue = (int) (blueRange[0] + (blueRange[1] - blueRange[0]) * (sin((delta) / cycle * 2 * PI + 2 * 2 * PI / 3) + 1) / 2);
  idleR = red;
  idleG = green;
  idleB = blue;
  fillAll(d, red, green, blue, false);
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

int alertIndex = 0;
boolean alertFilling = true;
void alert(int d) {
  int r = 255;
  int g = 0;
  int b = 0;
  fillAll(d, r, g, b);
}

int prevAnimationIndex = 1;
void setupServer() {
  server.on("/", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    server.send(200, "text/html", "<h1>Octo Lamp</h1>" + animationOptions());
  });
  server.on("/toggle", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    isOn = !isOn;
    if (!isOn) {
      EEPROM.put(0, 0);
      EEPROM.commit();
    } else {
      EEPROM.put(0, prevAnimationIndex);
      EEPROM.commit();
    }
    String msg = isOn ? "ON" : "OFF";
    server.send(200, "text/html", "<h1>Octo Lamp is now " + msg + "</h1>" + animationOptions());
  });
  server.on("/idle", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    animation = idle;
    isOn = true;
    prevAnimationIndex = 1;
    EEPROM.put(0, 1);
    EEPROM.commit();
    server.send(200, "text/html", "<h1>Octo Lamp is now Idleing</h1>" + animationOptions());
  });
  server.on("/star", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    animation = star;
    isOn = true;
    prevAnimationIndex = 2;
    EEPROM.put(0, 2);
    EEPROM.commit();
    server.send(200, "text/html", "<h1>Octo Lamp is now a star</h1>" + animationOptions());
  });
  server.on("/commit", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    animation = commit;
    isOn = true;
    prevAnimationIndex = 3;
    EEPROM.put(1, 3);
    EEPROM.commit();
    server.send(200, "text/html", "<h1>Octo Lamp is now committing</h1>" + animationOptions());
  });
  server.on("/alert", []() {
    if (isOverride == true) {
      server.send(200, "text/html", "<h1>Octo Lamp is currently reacting to GitHub event</h1>");
      return;
    }
    animation = alert;
    isOn = true;
    prevAnimationIndex = 4;
    EEPROM.put(0, 4);
    EEPROM.commit();
    server.send(200, "text/html", "<h1>Octo Lamp is now alerting</h1>" + animationOptions());
  });
  server.begin();
}

String animationOptions() {
  String html = "";
  String isOnStr = isOn ? "off" : "on";
  html += "<a href='/'>Home</a><br>";
  html += "<a href='/toggle'>Turn " + isOnStr + "</a><br>";
  html += "<a href='/idle'>Idle</a><br>";
  html += "<a href='/star'>Star</a><br>";
  html += "<a href='/commit'>Commit</a><br>";
  html += "<a href='/alert'>Alert</a><br>";
  return html;
}

String eventId = "";
void fetchEvents() {
  int now = millis();
  if (now - lastFetch < fetchInterval || fetching == false || isOverride == true) {
    fetching = false;
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    client.beginRequest();
    client.get("/event");
    client.sendHeader("Authorization", SERVER_SECRET.c_str());
    client.endRequest();

    int statusCode = client.responseStatusCode();
    if (statusCode > 0) {
      String payload = client.responseBody();
      JSONVar buffer = JSON.parse(payload);
      if (JSON.typeof(buffer) != "undefined") {
        JSONVar keys = buffer.keys();
        String id = buffer["id"];
        if (eventId == "") {
          eventId = id;
        }
        if (id != eventId) {
          eventId = id;
          for (int i = 0; i < keys.length(); i++) {
            String key = keys[i];
            String value = buffer[key];
            if (key == "type") {
              if (value == "star") {
                activateOverride(120000, star);
              } else if (value == "commit") {
                int amount = buffer["amount"];
                activateOverride(amount * 5000, commit);
              } else if (value == "alert") {
                activateOverride(30000, alert);
              }
            }
          }
        }
      }
    }
  }
  lastFetch = millis();
  fetching = false;
}

int overrideTime = 0;
void (*prevAnimation)(int d) = nullptr;
float prevBrightness = 0;
void activateOverride(float duration, void (*anim)(int d)) {
  if (isOverride) {
    return;
  }
  isOverride = true;
  overrideDuration = duration;
  overrideAnimation = anim;
  overrideTime = millis();
  prevAnimation = animation;
  prevBrightness = BRIGHTNESS;
  animation = overrideAnimation;
  BRIGHTNESS = 0.4;
}
void overrideHandler() {
  if (overrideTime + overrideDuration < millis()) {
    isOverride = false;
    animation = prevAnimation;
    BRIGHTNESS = prevBrightness;
  }
}
