// ---------------------------------------------------------------------
// SmallTV Firmware
// Version: 2026.03.07
// Features:
// - NTP clock with Italy timezone (CET/CEST)
// - OpenWeatherMap weather (temperature/humidity)
// - ANSA Top News RSS (JSON endpoint)
// - Web UI with JSON API + brightness control
// - OTA via ElegantOTA
// - WiFi/API secrets via optional local wifi_secrets.h + secrets.h (not hardcoded)
// - Faster boot flow with bounded WiFi/NTP wait and periodic retries
// - Optional FAST_BOOT mode to minimize startup delays/animations
// - Clock/news scene scheduler + marquee text in clock scene
// - Icons in PROGMEM (WiFi/sync/temp/humidity/weather/RSS)
// ---------------------------------------------------------------------

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <time.h>
#include <pgmspace.h>
#include <string.h>

#include "config.h"

// HTML (PROGMEM)
#include "index_html.h"

// WiFi icons (32x32, 1-bit, PROGMEM) generated from icons/wifi/*.png
#include "icons/wifi/icons_1bit.h"

// Sync icons (32x32, 1-bit, PROGMEM) generated from icons/sync/*.png
#include "icons/sync/icons_1bit.h"

// Weather icon (80x80, RGB565, PROGMEM) generated from icons/mm/mm.png
#include "icons/mm/mm_rgb565.h"

// Temperature icon (12x32, RGB565, PROGMEM) generated from icons/temp/temp.jpg
#include "icons/temp/temp_rgb565.h"

// Humidity icon (22x32, RGB565, PROGMEM) generated from icons/temp/humi.jpg
#include "icons/temp/humi_rgb565.h"

// RSS icon (32x32, RGB565, PROGMEM) generated from icons/rss/rss.jpg
#include "icons/rss/rss_rgb565.h"

// --- WiFi ---
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

// --- API secrets ---
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef OWM_API_KEY
#define OWM_API_KEY ""
#endif

// --- Display/server instances ---
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
ESP8266WebServer server(80);

struct UiText {
  const char* wifi;
  const char* connected;
  const char* ntpSync;
  const char* synced;
  const char* updating;
  const char* updated;
  const char* failed;
  const char* ipPrefix;
  const char* wifiMissing;
  const char* wifiOffline;
  const char* wifiTimeout;
};

constexpr UiText UI = {
  "WiFi...",
  "Connected!",
  "NTP sync...",
  "Synced!",
  "Updating...",
  "Updated!",
  "Failed!",
  "IP: ",
  "WiFi cfg missing",
  "WiFi offline",
  "WiFi timeout"
};

enum class BootState : uint8_t {
  BOOT_WIFI,
  BOOT_NTP,
  BOOT_READY,
  BOOT_DEGRADED
};

enum class WiFiConnectResult : uint8_t {
  Connected,
  MissingCredentials,
  Timeout
};

// --- State ---
unsigned long lastWeather = 0;
bool otaInProgress = false;
bool showNews = false;
int currentNewsIndex = 0;
bool ntpSynced = false;
unsigned long lastWiFiRetry = 0;
unsigned long lastNtpRetry = 0;
unsigned long bootMs = 0;
bool initialDataFetched = false;
unsigned long lastPrint = 0;
int16_t marqueeX = SCREEN_W;
char marqueeMessage[32] = "Ciao Come Stai?";
BootState bootState = BootState::BOOT_WIFI;
WiFiConnectResult lastWiFiResult = WiFiConnectResult::Timeout;
unsigned long lastWeatherSuccessMs = 0;
unsigned long lastNewsSuccessMs = 0;

// --- Brightness (0-255, user-facing; inverted before PWM) ---
int currentBrightness = 50;

// --- Weather data ---
float wtTemp = 0;
int wtHumidity = 0;
String wtDesc = "";

// --- RSS data (titles + links) ---
String newsTitles[NEWS_MAX];
String newsLinks[NEWS_MAX];
unsigned long lastNews = 0;
// Timestamp used by scene scheduler (clock/news switching).
unsigned long lastDisplay = 0;
int lastNewsHttpCode = 0;
String lastNewsError = "";
int lastNewsCount = 0;

// ---------------------------------------------------------------------
// showStatus()
// Shows a full-screen status message (used during boot/sync/OTA).
// ---------------------------------------------------------------------
void showStatus(const String& msg, uint16_t color = ST77XX_WHITE, int16_t x = STATUS_TEXT_X, int16_t y = STATUS_TEXT_Y) {
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print(msg);
}

// ---------------------------------------------------------------------
// drawRGB565_P()
// Draws an RGB565 icon stored in PROGMEM using a single-line buffer.
// Note: icons wider than RGB565_LINE_MAX are clipped.
// ---------------------------------------------------------------------
void drawRGB565_P(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* icon) {
  if (w > RGB565_LINE_MAX) w = RGB565_LINE_MAX;  // prevent buffer overflow
  uint16_t line[RGB565_LINE_MAX];
  for (int16_t row = 0; row < h; row++) {
    for (int16_t col = 0; col < w; col++) {
      line[col] = pgm_read_word(&icon[row * w + col]);
    }
    tft.drawRGBBitmap(x, y + row, line, w, 1);
  }
}

// ---------------------------------------------------------------------
// extractTag()
// Extracts the first <tag>...</tag> content from a string.
// Handles <![CDATA[...]]> if present.
// ---------------------------------------------------------------------
String extractTag(const String& src, const char* tag) {
  String openTag = String("<") + tag + ">";
  String closeTag = String("</") + tag + ">";
  int start = src.indexOf(openTag);
  if (start < 0) return "";
  start += openTag.length();
  int end = src.indexOf(closeTag, start);
  if (end < 0) return "";
  String out = src.substring(start, end);
  out.replace("<![CDATA[", "");
  out.replace("]]>", "");
  out.trim();
  return out;
}

// ---------------------------------------------------------------------
// jsonEscape()
// Escapes a string for JSON output (quotes, backslashes, control chars).
// ---------------------------------------------------------------------
String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\"': out += "\\\""; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
          out += ' ';
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

const char* bootStateToString(BootState state) {
  switch (state) {
    case BootState::BOOT_WIFI: return "boot_wifi";
    case BootState::BOOT_NTP: return "boot_ntp";
    case BootState::BOOT_READY: return "ready";
    case BootState::BOOT_DEGRADED: return "degraded";
    default: return "unknown";
  }
}

const char* wifiResultToString(WiFiConnectResult result) {
  switch (result) {
    case WiFiConnectResult::Connected: return "connected";
    case WiFiConnectResult::MissingCredentials: return "missing_credentials";
    case WiFiConnectResult::Timeout: return "timeout";
    default: return "unknown";
  }
}

// ---------------------------------------------------------------------
// setBrightness()
// Applies brightness (PWM is inverted on this panel).
// ---------------------------------------------------------------------
void setBrightness(int value) {
  currentBrightness = constrain(value, 0, 255);
  analogWrite(TFT_BACKLIGHT, 255 - currentBrightness);
}

// ---------------------------------------------------------------------
// connectWiFi()
// Blocks until WiFi is connected, with a simple animation.
// ---------------------------------------------------------------------
WiFiConnectResult ICACHE_FLASH_ATTR connectWiFi() {
  if (strlen(WIFI_SSID) == 0) {
    showStatus(UI.wifiMissing, ST77XX_RED, 15, 110);
    delay(kFastBoot ? BOOT_INFO_DELAY_MS_FAST : BOOT_INFO_DELAY_MS_NORMAL);
    lastWiFiResult = WiFiConnectResult::MissingCredentials;
    return lastWiFiResult;
  }

  showStatus(UI.wifi, ST77XX_WHITE, 78, 135);
  WiFi.persistent(false);   // Avoid flash writes on reconnect loops.
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Initial static icon
  tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, WIFI, WIFI_W, WIFI_H, ST77XX_WHITE, BG_COLOR);
  unsigned long start = millis();
  unsigned long lastStep = millis();
  uint8_t frame = 0;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    if (kFastBoot) {
      delay(10);  // feed watchdog without adding noticeable boot latency
      continue;
    }

    if (millis() - lastStep < 250) {
      delay(1);
      continue;
    }

    lastStep = millis();
    switch (frame) {
      case 0:
        tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, WIFI_1, WIFI_1_W, WIFI_1_H, ST77XX_WHITE, BG_COLOR);
        break;
      case 1:
        tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, WIFI_2, WIFI_2_W, WIFI_2_H, ST77XX_WHITE, BG_COLOR);
        break;
      case 2:
        tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, WIFI_3, WIFI_3_W, WIFI_3_H, ST77XX_WHITE, BG_COLOR);
        break;
      default:
        tft.drawBitmap(WIFI_ICON_X, WIFI_ICON_Y, WIFI, WIFI_W, WIFI_H, ST77XX_WHITE, BG_COLOR);
        break;
    }
    frame = (frame + 1) & 0x03;
  }

  const bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected) {
    showStatus(String(UI.ipPrefix) + WiFi.localIP().toString(), ST77XX_WHITE, 18, 110);
    delay(kFastBoot ? BOOT_INFO_DELAY_MS_FAST : BOOT_INFO_DELAY_MS_NORMAL);
    lastWiFiResult = WiFiConnectResult::Connected;
  } else {
    showStatus(UI.wifiTimeout, ST77XX_YELLOW, 40, 110);
    delay(kFastBoot ? BOOT_INFO_DELAY_MS_FAST : BOOT_INFO_DELAY_MS_NORMAL);
    lastWiFiResult = WiFiConnectResult::Timeout;
  }

  return lastWiFiResult;
}

// ---------------------------------------------------------------------
// syncNTP()
// Italy timezone: DST handled via POSIX TZ string.
// ---------------------------------------------------------------------
bool ICACHE_FLASH_ATTR syncNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    ntpSynced = false;
    return false;
  }

  showStatus(UI.ntpSync, ST77XX_WHITE, 54, 135);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  const unsigned long MIN_SYNC_MS = kFastBoot ? 0 : 250;
  unsigned long syncStart = millis();
  tft.drawBitmap(SYNC_ICON_X, SYNC_ICON_Y, SYNC_1, SYNC_1_W, SYNC_1_H, ST77XX_WHITE, BG_COLOR);
  unsigned long lastStep = millis();
  uint8_t frame = 0;
  while ((time(nullptr) < 100000 || (millis() - syncStart) < MIN_SYNC_MS) &&
         (millis() - syncStart) < NTP_SYNC_TIMEOUT_MS) {
    if (kFastBoot) {
      delay(10);
      continue;
    }

    if (millis() - lastStep < 250) {
      delay(1);
      continue;
    }

    lastStep = millis();
    switch (frame) {
      case 0:
        tft.drawBitmap(SYNC_ICON_X, SYNC_ICON_Y, SYNC_2, SYNC_2_W, SYNC_2_H, ST77XX_WHITE, BG_COLOR);
        break;
      case 1:
        tft.drawBitmap(SYNC_ICON_X, SYNC_ICON_Y, SYNC_3, SYNC_3_W, SYNC_3_H, ST77XX_WHITE, BG_COLOR);
        break;
      case 2:
        tft.drawBitmap(SYNC_ICON_X, SYNC_ICON_Y, SYNC_4, SYNC_4_W, SYNC_4_H, ST77XX_WHITE, BG_COLOR);
        break;
      default:
        tft.drawBitmap(SYNC_ICON_X, SYNC_ICON_Y, SYNC_1, SYNC_1_W, SYNC_1_H, ST77XX_WHITE, BG_COLOR);
        break;
    }
    frame = (frame + 1) & 0x03;
  }

  ntpSynced = (time(nullptr) >= 100000);
  if (!kFastBoot) {
    showStatus(ntpSynced ? UI.synced : UI.failed, ntpSynced ? ST77XX_GREEN : ST77XX_RED, 78, 110);
    delay(BOOT_INFO_DELAY_MS_NORMAL);
  }
  tft.fillScreen(BG_COLOR);
  return ntpSynced;
}

// ---------------------------------------------------------------------
// fetchWeather()
// Calls OpenWeatherMap and updates global variables.
// ---------------------------------------------------------------------
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (strlen(OWM_API_KEY) == 0) return;

  WiFiClient client;
  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/weather?lat=";
  url += OWM_LAT;
  url += "&lon=";
  url += OWM_LON;
  url += "&appid=";
  url += OWM_API_KEY;
  url += "&units=metric&lang=it";

  http.begin(client, url);
  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, http.getString());

    if (!err) {
      wtTemp = doc["main"]["temp"].as<float>();
      wtHumidity = doc["main"]["humidity"].as<int>();
      wtDesc = doc["weather"][0]["description"].as<String>();
      if (wtDesc.length() > 0) wtDesc[0] = toupper(wtDesc[0]);
      lastWeatherSuccessMs = millis();
    }
  }

  http.end();
}

int parseRssItems(const String& xml, String* outTitles, String* outLinks, int maxItems, String& parseError) {
  int pos = 0;
  int found = 0;
  parseError = "";

  while (found < maxItems) {
    int itemStart = xml.indexOf("<item>", pos);
    if (itemStart < 0) break;
    int itemEnd = xml.indexOf("</item>", itemStart);
    if (itemEnd < 0) {
      parseError = "Malformed XML: missing </item>";
      break;
    }

    String item = xml.substring(itemStart, itemEnd);
    String title = extractTag(item, "title");
    String link = extractTag(item, "link");

    if (title.length() > 0) {
      outTitles[found] = title;
      outLinks[found] = link;
      found++;
    }

    pos = itemEnd + 7;
  }

  if (found == 0 && parseError.length() == 0) {
    parseError = "No <item> parsed";
  }
  return found;
}

// ---------------------------------------------------------------------
// fetchAnsaRSS()
// Fetches ANSA RSS feed and updates newsTitles/newsLinks arrays.
// Keeps previous data if the fetch fails and updates debug fields.
// ---------------------------------------------------------------------
void fetchAnsaRSS(const char* feedUrl) {
  if (WiFi.status() != WL_CONNECTED) {
    lastNewsHttpCode = -1;
    lastNewsError = UI.wifiOffline;
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  int code = -1;
  String xml;

  for (uint8_t attempt = 0; attempt < RSS_HTTP_ATTEMPTS; attempt++) {
    HTTPClient http;
    http.begin(client, feedUrl);
    http.setTimeout(RSS_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "Mozilla/5.0");
    code = http.GET();
    if (code == HTTP_CODE_OK) {
      xml = http.getString();
      http.end();
      break;
    }
    http.end();
    delay(30);
  }

  lastNewsHttpCode = code;
  lastNewsError = "";
  lastNewsCount = 0;

  if (code == HTTP_CODE_OK) {
    lastNewsCount = parseRssItems(xml, newsTitles, newsLinks, NEWS_MAX, lastNewsError);
    if (lastNewsCount > 0) {
      lastNewsSuccessMs = millis();
    }
  } else {
    lastNewsError = "HTTP error or timeout";
  }
}

// ---------------------------------------------------------------------
// drawWeather()
// Draws the lower panel: temperature/humidity bars and side icons.
// ---------------------------------------------------------------------
void drawWeather() {
  tft.fillRect(0, 125, 240, 115, BG_COLOR);
  tft.drawFastHLine(20, 125, 200, 0x4208);
  tft.setTextSize(2);
  tft.setTextColor(TEMP_COLOR);
  tft.setCursor(45, 150);
  tft.drawRoundRect(45, 170, 80, 9, 50, ST77XX_WHITE);
  tft.fillCircle(48, 174, 2, TEMP_COLOR);
  float t = constrain(wtTemp, 0.0f, 40.0f);
  int tempWidth = (int)((78.0f * t) / 40.0f);
  tft.fillRect(48, 171, tempWidth, 7, TEMP_COLOR);
  drawRGB565_P(TEMP_ICON_X, TEMP_ICON_Y, TEMP_ICON_W, TEMP_ICON_H, TEMP_ICON_RGB565);
  char tempText[12];
  snprintf(tempText, sizeof(tempText), "%.1fC", wtTemp);
  tft.print(tempText);
  tft.setTextSize(2);
  tft.setTextColor(HUM_COLOR);
  tft.setCursor(45, 195);
  tft.drawRoundRect(45, 215, 80, 9, 50, ST77XX_WHITE);
  tft.fillCircle(48, 219, 2, HUM_COLOR);
  int h = constrain(wtHumidity, 0, 100);
  int humiWidth = (int)((78.0f * h) / 100.0f);
  tft.fillRect(48, 216, humiWidth, 7, HUM_COLOR);
  drawRGB565_P(HUMI_ICON_X, HUMI_ICON_Y, HUMI_ICON_W, HUMI_ICON_H, HUMI_ICON_RGB565);
  char humidityText[8];
  snprintf(humidityText, sizeof(humidityText), "%d%%", wtHumidity);
  tft.print(humidityText);
  drawRGB565_P(MM_ICON_X, MM_ICON_Y, MM_RGB565_W, MM_RGB565_H, MM_RGB565);
  tft.setTextColor(ST77XX_WHITE);
}

// ---------------------------------------------------------------------
// drawNews()
// Draws one news item with RSS icon and top-right progress indicator.
// ---------------------------------------------------------------------
void drawNews(int index) {
  const int16_t textX = 10;
  const int16_t textY = 50;
  const int16_t maxW = 220;  // 240 - margins
  const int16_t maxH = 185;  // from textY down to bottom margin
  const int16_t charW = 12;  // default font at text size 2
  const int16_t lineH = 16;  // default font at text size 2
  const int16_t maxChars = maxW / charW;

  tft.fillScreen(BG_COLOR);

  // RSS Icon
  drawRGB565_P(RSS_ICON_X, RSS_ICON_Y, RSS_ICON_W, RSS_ICON_H, RSS_ICON_RGB565);

  // RSS News
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(false);
  tft.setTextSize(2);

  tft.setCursor(textX, textY);
  if (index < 0 || index >= NEWS_MAX || newsTitles[index].length() == 0) {
    tft.print("No news available");
  } else {
    // Current item index (1-based)
    tft.setCursor(185, 10);
    tft.print(String(index + 1) + "/" + String(NEWS_MAX));

    // Normalize text before wrapping (single spaces, no newlines).
    String text = newsTitles[index];
    text.replace("\n", " ");
    text.trim();

    // Word-wrap renderer: wraps on spaces, avoids splitting words.
    String line = "";
    int y = textY;
    int pos = 0;

    while (pos < text.length() && (y - textY) <= maxH) {
      while (pos < text.length() && text[pos] == ' ') pos++;
      int start = pos;
      while (pos < text.length() && text[pos] != ' ') pos++;
      String word = text.substring(start, pos);
      if (word.length() == 0) continue;

      String candidate = (line.length() == 0) ? word : (line + " " + word);
      if (candidate.length() <= maxChars) {
        line = candidate;
        continue;
      }

      if (line.length() > 0) {
        tft.setCursor(textX, y);
        tft.print(line);
        y += lineH;
      }

      if (word.length() <= maxChars) {
        line = word;
      } else {
        // Fallback for single words longer than one full line.
        int wp = 0;
        while (wp < word.length() && (y - textY) <= maxH) {
          String chunk = word.substring(wp, wp + maxChars);
          tft.setCursor(textX, y);
          tft.print(chunk);
          y += lineH;
          wp += maxChars;
        }
        line = "";
      }
    }

    if (line.length() > 0 && (y - textY) <= maxH) {
      tft.setCursor(textX, y);
      tft.print(line);
    }
  }

  // Fonte
  tft.setCursor(10, 210);
  tft.print("Fonte: ANSA");
}

// ---------------------------------------------------------------------
// drawClock()
// Draws clock in the upper panel.
// Called by the scene scheduler when the clock scene is active.
// ---------------------------------------------------------------------
void drawClock() {
  time_t now = time(nullptr);
  char timeStr[6];
  if (now < 100000) {
    strcpy(timeStr, "--:--");
  } else {
    struct tm* timeinfo = localtime(&now);
    if (timeinfo == nullptr) {
      strcpy(timeStr, "--:--");
    } else {
      strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
    }
  }

  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(0, 0, 240, 124, BG_COLOR);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  int cx = ((240 - w) / 2) - 5;
  int cy = 48 + h;
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(cx, cy);
  tft.print(timeStr);
  tft.setFont(NULL);
}

// ---------------------------------------------------------------------
// OTA callbacks — avoid SPI traffic during update
// ---------------------------------------------------------------------
void onOTAStart() {
  otaInProgress = true;
  showStatus(UI.updating, ST77XX_YELLOW);
}

void onOTAProgress(size_t current, size_t total) {
  // Intentionally empty
}

void onOTAEnd(bool success) {
  if (success) {
    showStatus(UI.updated, ST77XX_GREEN);
  } else {
    showStatus(UI.failed, ST77XX_RED);
  }
}

// INDEX_HTML lives in index_html.h (PROGMEM)

void tickWiFiRetry(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) return;
  if (now - lastWiFiRetry < WIFI_RETRY_INTERVAL_MS) return;

  lastWiFiRetry = now;
  WiFiConnectResult result = connectWiFi();
  if (result == WiFiConnectResult::Connected) {
    bootState = ntpSynced ? BootState::BOOT_READY : BootState::BOOT_NTP;
  } else {
    bootState = BootState::BOOT_DEGRADED;
    ntpSynced = false;
  }
}

void tickNtpRetry(unsigned long now) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (ntpSynced) return;
  if (now - lastNtpRetry < NTP_RETRY_INTERVAL_MS) return;

  lastNtpRetry = now;
  bootState = BootState::BOOT_NTP;
  ntpSynced = syncNTP();
  bootState = ntpSynced ? BootState::BOOT_READY : BootState::BOOT_DEGRADED;
}

void tickInitialDataFetch(unsigned long now) {
  if (initialDataFetched) return;
  if (now - bootMs < INITIAL_DATA_FETCH_DELAY_MS) return;

  fetchWeather();
  fetchAnsaRSS(ANSA_RSS_URL);
  lastWeather = now;
  lastNews = now;
  initialDataFetched = true;
  if (!showNews) drawWeather();
}

void tickWeather(unsigned long now) {
  if (now - lastWeather < OWM_INTERVAL_MS) return;
  lastWeather = now;
  fetchWeather();
  if (!showNews) drawWeather();
}

void tickNews(unsigned long now) {
  if (now - lastNews < NEWS_INTERVAL_MS) return;
  lastNews = now;
  fetchAnsaRSS(ANSA_RSS_URL);
}

void tickMarquee(unsigned long now) {
  if (showNews) return;
  if (now - lastPrint < MARQUEE_INTERVAL_MS) return;

  lastPrint = now;
  tft.fillRect(0, MARQUEE_Y, SCREEN_W, MARQUEE_H, BG_COLOR);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(marqueeX, MARQUEE_Y);
  tft.print(marqueeMessage);

  int16_t bx, by;
  uint16_t bw, bh;
  tft.getTextBounds(marqueeMessage, 0, 0, &bx, &by, &bw, &bh);
  marqueeX -= MARQUEE_STEP_PX;
  if (marqueeX < -(int16_t)bw) {
    marqueeX = SCREEN_W;
  }
}

void tickSceneScheduler(unsigned long now) {
  unsigned long interval = showNews ? DISPLAY_NEWS_MS : DISPLAY_CLOCK_MS;
  if (now - lastDisplay < interval) return;

  lastDisplay = now;
  if (!showNews) {
    showNews = true;
    currentNewsIndex = 0;
    drawNews(currentNewsIndex);
    return;
  }

  currentNewsIndex++;
  if (currentNewsIndex >= NEWS_MAX) {
    showNews = false;
    tft.fillScreen(BG_COLOR);
    marqueeX = SCREEN_W;
    drawClock();
    drawWeather();
  } else {
    drawNews(currentNewsIndex);
  }
}

// ---------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------
void setup() {
  bootMs = millis();
  setBrightness(50);
  tft.init(SCREEN_W, SCREEN_H, SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(BG_COLOR);

  WiFiConnectResult wifiResult = connectWiFi();
  if (wifiResult == WiFiConnectResult::Connected) {
    bootState = BootState::BOOT_NTP;
    ntpSynced = syncNTP();
    bootState = ntpSynced ? BootState::BOOT_READY : BootState::BOOT_DEGRADED;
  } else {
    bootState = BootState::BOOT_DEGRADED;
    ntpSynced = false;
  }

  lastWeather = millis();
  lastNews = millis();
  lastDisplay = millis();

  server.on("/", []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api", []() {
    String escapedDesc = jsonEscape(wtDesc);
    char tempText[12];
    snprintf(tempText, sizeof(tempText), "%.1f", wtTemp);
    unsigned long now = millis();
    long weatherAge = (lastWeatherSuccessMs == 0) ? -1L : (long)(now - lastWeatherSuccessMs);
    long newsAge = (lastNewsSuccessMs == 0) ? -1L : (long)(now - lastNewsSuccessMs);
    char json[512];
    snprintf(
      json,
      sizeof(json),
      "{\"temp\":%s,\"humidity\":%d,\"description\":\"%s\",\"ip\":\"%s\",\"brightness\":%d,"
      "\"wifi\":%s,\"wifi_result\":\"%s\",\"ntp\":%s,\"boot_state\":\"%s\","
      "\"last_weather_ms\":%ld,\"last_news_ms\":%ld}",
      tempText,
      wtHumidity,
      escapedDesc.c_str(),
      WiFi.localIP().toString().c_str(),
      currentBrightness,
      (WiFi.status() == WL_CONNECTED) ? "true" : "false",
      wifiResultToString(lastWiFiResult),
      ntpSynced ? "true" : "false",
      bootStateToString(bootState),
      weatherAge,
      newsAge
    );
    server.send(200, "application/json", json);
  });

  server.on("/news", []() {
    String json = "{";
    json += "\"source\":\"ANSA\",";
    json += "\"items\":[";
    for (int i = 0; i < NEWS_MAX; i++) {
      if (i > 0) json += ",";
      json += "{";
      json += "\"title\":\"" + jsonEscape(newsTitles[i]) + "\",";
      json += "\"link\":\"" + jsonEscape(newsLinks[i]) + "\"";
      json += "}";
    }
    json += "],";
    json += "\"debug\":{";
    json += "\"http\":" + String(lastNewsHttpCode) + ",";
    json += "\"count\":" + String(lastNewsCount) + ",";
    json += "\"error\":\"" + jsonEscape(lastNewsError) + "\",";
    json += "\"age_ms\":" + String(millis() - lastNews);
    json += "}}";
    server.send(200, "application/json", json);
  });

  server.on("/brightness", []() {
    if (server.hasArg("value")) {
      setBrightness(server.arg("value").toInt());
    }
    server.send(200, "text/plain", "ok");
  });

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();

  drawClock();
  drawWeather();
}

// ---------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------
void loop() {
  server.handleClient();
  ElegantOTA.loop();

  if (otaInProgress) return;

  unsigned long now = millis();
  tickWiFiRetry(now);
  tickNtpRetry(now);
  tickInitialDataFetch(now);
  tickWeather(now);
  tickNews(now);
  tickMarquee(now);
  tickSceneScheduler(now);
}
