#pragma once

#include <Arduino.h>

// --- Display: GeekMagic SmallTV pinout ---
constexpr uint8_t TFT_DC = 0;        // GPIO0
constexpr uint8_t TFT_RST = 2;       // GPIO2
constexpr uint8_t TFT_CS = 15;       // GPIO15
constexpr uint8_t TFT_BACKLIGHT = 5; // GPIO5, PWM inverted

// --- Fast Boot ---
#define FAST_BOOT 1
constexpr bool kFastBoot = (FAST_BOOT == 1);

// --- OpenWeatherMap ---
constexpr char OWM_LAT[] = "45.0703";
constexpr char OWM_LON[] = "7.6869";
constexpr unsigned long OWM_INTERVAL_MS = 600000UL;

// --- ANSA RSS ---
constexpr char ANSA_RSS_URL[] = "https://www.ansa.it/sito/notizie/topnews/topnews_rss.xml";
constexpr int NEWS_MAX = 3;
constexpr unsigned long NEWS_INTERVAL_MS = 600000UL;
constexpr unsigned long DISPLAY_CLOCK_MS = 10000UL;
constexpr unsigned long DISPLAY_NEWS_MS = 5000UL;

// --- Colors (RGB565) ---
constexpr uint16_t BG_COLOR = ST77XX_BLACK;
constexpr uint16_t TEMP_COLOR = 0xFD20;
constexpr uint16_t HUM_COLOR = 0x07FF;
constexpr uint16_t DESC_COLOR = 0xC618;

// --- Layout ---
constexpr int16_t SCREEN_W = 240;
constexpr int16_t SCREEN_H = 240;
constexpr int16_t TOP_PANEL_H = 124;
constexpr int16_t WEATHER_Y = 125;
constexpr int16_t WEATHER_H = 115;

constexpr int16_t WIFI_ICON_X = 104;
constexpr int16_t WIFI_ICON_Y = 90;
constexpr int16_t SYNC_ICON_X = 104;
constexpr int16_t SYNC_ICON_Y = 90;
constexpr int16_t MM_ICON_X = 140;
constexpr int16_t MM_ICON_Y = 145;
constexpr int16_t TEMP_ICON_X = 25;
constexpr int16_t TEMP_ICON_Y = 150;
constexpr int16_t HUMI_ICON_X = 15;
constexpr int16_t HUMI_ICON_Y = 195;
constexpr int16_t RSS_ICON_X = 10;
constexpr int16_t RSS_ICON_Y = 10;
constexpr int16_t STATUS_TEXT_X = 10;
constexpr int16_t STATUS_TEXT_Y = 10;

// --- Marquee ---
constexpr unsigned long MARQUEE_INTERVAL_MS = 500;
constexpr int16_t MARQUEE_Y = 100;
constexpr int16_t MARQUEE_H = 16;
constexpr int16_t MARQUEE_STEP_PX = 5;

// --- Boot/runtime tuning ---
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 12000;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 7000;
constexpr unsigned long NTP_RETRY_INTERVAL_MS = 60000;
constexpr unsigned long INITIAL_DATA_FETCH_DELAY_MS = 1200;
constexpr unsigned long BOOT_INFO_DELAY_MS_NORMAL = 250;
constexpr unsigned long BOOT_INFO_DELAY_MS_FAST = 120;

// --- Network ---
constexpr uint16_t WEATHER_HTTP_TIMEOUT_MS = 2500;
constexpr uint16_t RSS_HTTP_TIMEOUT_MS = 3500;
constexpr uint8_t RSS_HTTP_ATTEMPTS = 2;

// --- Draw helpers ---
constexpr int16_t RGB565_LINE_MAX = 80;
