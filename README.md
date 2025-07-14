# IMD ESP32 Rain Gauge 🌧️

This project was built during my internship at the India Meteorological Department.

It uses an ESP32 to:
- Measure rainfall via a pulse-based sensor
- Monitor temperature (AHT10 and DS18B20)
- Display data on a web dashboard hosted on the ESP32
- Provide CSV download of live rainfall + sensor data

## Features
- Self-hosted HTML dashboard
- Real-time updates
- Wi-Fi + NTP synced time
- No external server required

## Run Instructions
1. Flash the `src/imd_rain_gauge.ino` file to your ESP32.
2. Connect to Wi-Fi (change `ssid` and `password` in code).
3. Open the IP shown in Serial Monitor to access dashboard.

## Dashboard Screenshot
<img width="522" height="638" alt="image" src="https://github.com/user-attachments/assets/e5406b4d-644f-4e35-97c5-d2e723c06105" />

## License
MIT
