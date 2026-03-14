# 📺 SmallTV RSS - ESP8266 Weather Clock & News Feed

<p align="center">
  <img src="https://img.shields.io/badge/version-2026.03.11-blue.svg" alt="Version">
  <img src="https://img.shields.io/badge/platform-ESP8266-green.svg" alt="Platform">
  <img src="https://img.shields.io/badge/license-MIT-yellow.svg" alt="License">
</p>

Smart weather station + RSS news reader for **GeekMagic SmallTV** with NTP clock, ANSA news feed, and responsive web dashboard.

> Amateur project with AI-assisted code. Use at your own risk.

---

## ✨ Features

- 🌡️ **Live Weather**: OpenWeatherMap API + color-coded bars, auto-update 10min
- 🕐 **NTP Clock**: Italy timezone (CET/CEST DST), custom Bebas Neue font
- 📰 **RSS Feed**: ANSA news (3 headlines), HTTPS, auto-retry
- 🌐 **Web Dashboard**: Bootstrap 5, mobile-responsive, brightness control
- 🔄 **OTA Updates**: Wireless firmware updates via ElegantOTA
- 🎨 **Animations**: WiFi/NTP sync feedback, custom fonts (Bebas + Oswald)
- 🔒 **Security**: Input validation, response limits, credential separation

---

## 🛠️ Requirements

| Component | Details |
|-----------|---------|
| **Board** | ESP8266 (ESP-12E/12F) |
| **Display** | ST7789 240x240 TFT (GeekMagic SmallTV) |
| **Pin Config** | TFT: DC=GPIO0, RST=GPIO2, CS=GPIO15, BL=GPIO5 |
| **Power** | 5V USB (500mA min) |
| **WiFi** | 2.4GHz 802.11 b/g/n |

### Dependencies
```cpp
Adafruit_GFX, Adafruit_ST7789          // Graphics & display
ArduinoJson (6.x)                      // JSON parsing
ElegantOTA                              // OTA updates
ESP8266WiFi, ESP8266WebServer (built-in)
```

**Fonts**: Bebas Neue (clock), Oswald (UI/news) → pre-generated GFX headers included

---

## 🚀 Setup (5 steps)

**1. Clone & configure WiFi:**
```bash
git clone https://github.com/stefanopennaa/SmallTV_RSS.git && cd SmallTV_RSS
cp wifi_secrets.example.h wifi_secrets.h
# Edit: add SSID and password
```

**2. Configure API:**
```bash
cp secrets.example.h secrets.h
# Edit: add OpenWeatherMap API key (free at openweathermap.org/api)
```

**3. Set location (optional):**
Edit `config.h`:
```cpp
constexpr char OWM_LAT[] = "45.0703";   // Your latitude
constexpr char OWM_LON[] = "7.6869";    // Your longitude
```

**4. Upload:**
- Connect ESP8266 via USB
- Open `SmallTV_RSS.ino` in Arduino IDE
- Select board & port → Upload

**5. Access dashboard:**
- Check display for device IP
- Open `http://DEVICE_IP` in browser

---

## ⚙️ Configuration

Edit `config.h` for:
- **Weather update**: `OWM_INTERVAL_MS` (default: 600s)
- **News update**: `NEWS_INTERVAL_MS` (default: 600s)
- **Brightness**: `setBrightness(50)` in `setup()` or via web UI slider
- **Colors**: `BG_COLOR`, `TEMP_COLOR`, `HUM_COLOR` (RGB565 format)

---

## 🌐 Web API

| Endpoint | Purpose |
|----------|---------|
| `GET /` | Web dashboard |
| `GET /api` | Device status + sensor data (JSON) |
| `GET /news` | RSS feed with debug info (JSON) |
| `GET /brightness?value=N` | Control backlight (0-255) |
| `GET /update` | OTA firmware update interface |

**Example** `/api` response:
```json
{
  "temp": 23.5, "humidity": 65, "description": "Partly cloudy",
  "ip": "192.168.1.100", "brightness": 128,
  "wifi": true, "ntp": true, "boot_state": "ready"
}
```

---

## 🏗️ Architecture

**Boot Sequence:**
```
Init hardware → WiFi connect → NTP sync → Web server + OTA → Fetch data → Render
```

**Main Loop:**
- Handle HTTP requests, OTA updates
- WiFi/NTP reconnection (if needed)
- Weather refresh (10min interval)
- News refresh (10min interval)
- Scene management (clock ↔ news)

**States:** `BOOT_WIFI` → `BOOT_NTP` → `BOOT_READY` → `BOOT_DEGRADED` (offline fallback)

---

## 🐛 Troubleshooting

| Issue | Solutions |
|-------|-----------|
| No WiFi | Check SSID/password in `wifi_secrets.h` · Verify 2.4GHz network · Move closer to router |
| No weather | Confirm API key in `secrets.h` · Check OpenWeatherMap quota · Verify coordinates (decimal) |
| Clock shows "--:--" | Requires internet for NTP · Check UDP 123 is open · Wait 60s for sync, retries every 60s |
| Display too bright/dim | Use web slider or `http://DEVICE_IP/brightness?value=128` (0-255) |
| RSS errors | Check internet · Feed requires HTTPS/TLS · Check `/news` endpoint for details |
| OTA fails | Stable power required · Don't interrupt update · Verify ESP8266 firmware · Wait for "Updated!" |

---

## 📊 Performance

| Metric | Value |
|--------|-------|
| Boot time | ~15-20s (WiFi + NTP) |
| Memory (RAM) | ~35KB / 80KB |
| Flash usage | ~400KB / 4MB |
| Power | ~150-200mA @ 5V (50% brightness) |
| WiFi reconnect | <12s (timeout protected) |
| HTTP timeout | 2.5s (weather), 3.5s (news) |
| Display refresh | ~60ms (full screen) |

---

## 🔐 Security

**✅ Implemented:**
- Input validation on all HTTP endpoints
- Response size limits (32KB RSS, 4KB weather)
- JSON structure validation, buffer overflow protection
- Credential separation (secrets not in repo)
- HTTPS support for RSS feed
- Web UI: XSS protection, URL whitelist (http/https), 5s timeout, exponential backoff

**⚠️ Limitations:**
- No web authentication (home network only)
- WiFi credentials in plaintext on device
- OTA updates accept without additional auth

**Recommendation**: Use only on trusted private networks.

---

## 🤝 Contributing

1. Fork the repo
2. Create a feature branch (`git checkout -b feature/X`)
3. Commit changes (`git commit -m 'Add X'`)
4. Push to branch (`git push origin feature/X`)
5. Open a Pull Request

**Guidelines:** Follow Arduino C++ conventions, use descriptive names, test on hardware, update README if needed.

---

## 📝 Changelog

### v2026.03.11 - Typography Cleanup
- Added Bebas Neue for clock display, Oswald for UI/news text
- Increased clock font to 42pt for better TFT legibility
- Reworked RSS wrapping for proportional font width
- Removed legacy marquee system
- Organized fonts into `fonts/Bebas_Neue/` and `fonts/Oswald/`

### v2026.03.07 - Security & Code Quality
- Input validation + response limits + JSON validation
- Web UI: XSS protection, URL validation, 5s timeout, exponential backoff
- Eliminated magic numbers, improved naming, reduced duplication
- WCAG 2.1 AA accessibility compliance

### v2026.03.07 - Initial Release
- Core features: WiFi, NTP, weather, RSS, web dashboard, OTA

---

## 📄 License

MIT License - see [LICENSE](LICENSE) file

---

## 🙏 Credits

**Libraries:** Adafruit GFX, Adafruit ST7789, ArduinoJson, ElegantOTA, Bootstrap 5  
**Data:** OpenWeatherMap, ANSA news, pool.ntp.org  
**Hardware:** GeekMagic SmallTV

---

## 📧 Issues & Support

- **GitHub Issues**: [GitHub Issues](https://github.com/stefanopennaa/SmallTV_RSS/issues)
- **Email**: stefano@stefanopenna.it

<p align="center">Made with ❤️ for the ESP8266 community</p>
