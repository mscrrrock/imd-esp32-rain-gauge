#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Adafruit_AHTX0.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi credentials
const char* ssid = "Nikitas Iphone";
const char* password = "niki1234";

// Rainfall sensor setup
const byte flowSensorPin = 27;
volatile unsigned long pulseCount = 0;
const float calibrationConstant = 162 / 0.5; // 214 pulses for 500mL

// Sensor setup
#define ONE_WIRE_BUS 4 // DS18B20 data pin (change if needed)
Adafruit_AHTX0 aht;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Global sensor variables
float aht10Temp = NAN, aht10Hum = NAN;
float ds18b20Temp = NAN;

// Web server
WebServer server(80);

// Rainfall variables
float currentMinuteRain = 0;
float current30MinRain = 0;
float currentHourRain = 0;
float currentDayRain = 0;
float currentWeekRain = 0;
float currentMonthRain = 0;
float currentYearRain = 0;

float lastMinuteRain = 0;
float last30MinRain = 0;
float lastHourRain = 0;
float lastDayRain = 0;
float lastWeekRain = 0;
float lastMonthRain = 0;
float lastYearRain = 0;

// Time tracking variables
int lastMinute = -1;
int last30Min = -1;
int lastHour = -1;
int lastDay = -1;
int lastMonth = -1;
int lastYear = -1;

// Rainfall area calculation
const float funnelRadiusCM = 8.45; // 8mm diameter
const float funnelAreaCM2 = PI * funnelRadiusCM * funnelRadiusCM;

// CSS for webpage
const char style_css[] PROGMEM = R"rawliteral(
body {
  font-family: Arial, sans-serif;
  background-color: #f0f0f0;
  margin: 0;
  padding: 20px;
}
.data-box {
  background: white;
  padding: 30px;
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  max-width: 600px;
  margin: 20px auto;
}
table {
  width: 100%;
  margin-top: 20px;
  border-collapse: collapse;
}
th, td {
  padding: 12px;
  text-align: left;
  border-bottom: 1px solid #ddd;
}
)rawliteral";

// Interrupt for rainfall pulse
void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(flowSensorPin, INPUT_PULLUP);
  delay(2000);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), countPulse, FALLING);

  // Sensor initialization
  if (!aht.begin()) {
    Serial.println("Could not find AHT10/AHT20? Check wiring!");
    while (1) delay(10);
  }
  sensors.begin();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Time
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  // Web server endpoints
  server.on("/", handleRoot);
  server.on("/data.html", handleData);
  server.on("/style.css", handleStyle);
  server.on("/rain.csv", handleCSV);
  server.begin();
}

void loop() {
  server.handleClient();
  static unsigned long lastMillis = 0;

  // Sensor reading every 2 seconds
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead > 2000) {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    aht10Temp = temp.temperature;
    aht10Hum = humidity.relative_humidity;
    sensors.requestTemperatures();
    ds18b20Temp = sensors.getTempCByIndex(0);
    lastSensorRead = millis();
  }

  // Rainfall calculation
  time_t now = time(nullptr);
  struct tm *ti = localtime(&now);

  if (ti->tm_year + 1900 >= 2023) {
    if (millis() - lastMillis >= 1000) {
      float ml = (pulseCount / calibrationConstant) * 1000.0;
      pulseCount = 0;
      float mm = ml / funnelAreaCM2;

      currentMinuteRain += mm;
      current30MinRain += mm;
      currentHourRain += mm;
      currentDayRain += mm;
      currentWeekRain += mm;
      currentMonthRain += mm;
      currentYearRain += mm;

      lastMillis = millis();
    }
    updateTimePeriods(ti);
  }
}

void updateTimePeriods(struct tm *ti) {
  // Minute transition
  if (ti->tm_min != lastMinute) {
    lastMinuteRain = currentMinuteRain;
    currentMinuteRain = 0;
    lastMinute = ti->tm_min;
  }

  // 30-Minute transition
  int curr30Min = ti->tm_min / 30;
  if (curr30Min != last30Min) {
    last30MinRain = current30MinRain;
    current30MinRain = 0;
    last30Min = curr30Min;
  }

  // Hour transition
  if (ti->tm_hour != lastHour) {
    lastHourRain = currentHourRain;
    currentHourRain = 0;
    lastHour = ti->tm_hour;
  }

  // Day transition
  if (ti->tm_mday != lastDay) {
    lastDayRain = currentDayRain;
    currentDayRain = 0;
    lastDay = ti->tm_mday;

    // Weekly reset (every Sunday)
    if (ti->tm_wday == 0) {
      lastWeekRain = currentWeekRain;
      currentWeekRain = 0;
    }

    // Monthly reset (first day of month)
    if (ti->tm_mday == 1) {
      lastMonthRain = currentMonthRain;
      currentMonthRain = 0;
    }

    // Yearly reset (January 1st)
    if (ti->tm_mon == 0 && ti->tm_mday == 1) {
      lastYearRain = currentYearRain;
      currentYearRain = 0;
    }
  }
}

void handleRoot() {
  server.send(200, "text/html",
    "<html><body style='text-align:center'>"
    "<h2>Rainfall Monitoring System</h2>"
    "<p>Visit <a href='/data.html'>/data.html</a> for detailed data</p>"
    "<p>Download as <a href='/rain.csv'>CSV</a></p>"
    "</body></html>");
}

void handleData() {
  time_t now = time(nullptr);
  struct tm *ti = localtime(&now);
  String dateStr = String(ti->tm_mday) + "-"
                 + String(ti->tm_mon + 1) + "-"
                 + String(ti->tm_year + 1900);

  String html = "<!DOCTYPE html><html><head>"
                "<title>Rainfall & Weather Data</title>"
                "<link rel='stylesheet' href='style.css'>"
                "<meta http-equiv='refresh' content='10'>"
                "</head><body>"
                "<div class='data-box'>"
                "<h2>Rainfall & Weather Data (" + dateStr + ")</h2>"
                "<table>"
                "<tr><th>Period</th><th>Rainfall (mm)</th></tr>"
                "<tr><td>Last Minute</td><td>" + String(lastMinuteRain, 2) + "</td></tr>"
                "<tr><td>Last 30 Minutes</td><td>" + String(last30MinRain, 2) + "</td></tr>"
                "<tr><td>Last Hour</td><td>" + String(lastHourRain, 2) + "</td></tr>"
                "<tr><td>Last Day</td><td>" + String(lastDayRain, 2) + "</td></tr>"
                "<tr><td>Last Week</td><td>" + String(lastWeekRain, 2) + "</td></tr>"
                "<tr><td>Last Month</td><td>" + String(lastMonthRain, 2) + "</td></tr>"
                "<tr><td>Last Year</td><td>" + String(lastYearRain, 2) + "</td></tr>"
                "</table>"
                "<h3>Sensor Data</h3>"
                "<table>"
                "<tr><th>Sensor</th><th>Value</th></tr>"
                "<tr><td>AHT10 Temperature (°C)</td><td>" + (isnan(aht10Temp) ? "N/A" : String(aht10Temp, 2)) + "</td></tr>"
                "<tr><td>AHT10 Humidity (%)</td><td>" + (isnan(aht10Hum) ? "N/A" : String(aht10Hum, 2)) + "</td></tr>"
                "<tr><td>DS18B20 Temperature (°C)</td><td>" + (isnan(ds18b20Temp) ? "N/A" : String(ds18b20Temp, 2)) + "</td></tr>"
                "</table>"
                "<br><a href='/rain.csv'>Download as CSV</a>"
                "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleCSV() {
  String csv = "Period,Rainfall (mm),AHT10 Temp (C),AHT10 Hum (%),DS18B20 Temp (C)\n";
  csv += "Last Minute," + String(lastMinuteRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last 30 Minutes," + String(last30MinRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last Hour," + String(lastHourRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last Day," + String(lastDayRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last Week," + String(lastWeekRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last Month," + String(lastMonthRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";
  csv += "Last Year," + String(lastYearRain, 2) + "," + String(aht10Temp, 2) + "," + String(aht10Hum, 2) + "," + String(ds18b20Temp, 2) + "\n";

  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=rain.csv");
  server.send(200, "text/csv", csv);
}

void handleStyle() {
  server.send_P(200, "text/css", style_css);
}