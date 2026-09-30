#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <INA226.h>
#include <SHT31.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <vector>
#include "webpage.h"

// ============================================================================
// CONFIGURATION & PIN DEFINITIONS
// ============================================================================
#define FIRMWARE_VERSION "v1.7.0-RTOS"

#define I2C_SDA_PIN    8     // Default SDA
#define I2C_SCL_PIN    9     // Default SCL

// Shunt Resistor value on INA226 board (labeled "R010" = 0.010 Ohm = 10 mOhm)
#define SHUNT_RESISTANCE_OHMS  0.010f
#define MAX_EXPECTED_CURRENT_A 8.0f

// 24VDC Power Quality Thresholds
#define VOLTAGE_DIP_THRESHOLD_V   22.5f   // Below 22.5V is classified as a bus dip
#define VOLTAGE_SURGE_THRESHOLD_V 26.5f   // Above 26.5V is classified as a bus surge
#define VOLTAGE_VALID_MIN_V       5.0f    // Ignore 0V unpowered states for dip tracking

// Fallback SoftAP details
const char* AP_SSID = "PowerMonitor-Setup";
const byte DNS_PORT = 53;

// ============================================================================
// GLOBAL OBJECTS & STATE
// ============================================================================
INA226 ina(0x40);
SHT31 sht;
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool isApMode = false;
String wifi_ssid_str = "";
String wifi_pass_str = "";

// MQTT Settings (persisted in NVS)
String device_name = "pmon";
String mqtt_server = "";
uint16_t mqtt_port = 1883;
String mqtt_topic = "telescope/";
String mqtt_user = "";
String mqtt_pass = "";
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastMqttPublishTime = 0;

// FreeRTOS Mutex for thread-safe telemetry access across cores
SemaphoreHandle_t telemMutex = NULL;

struct Telemetry {
  float voltage = 0.0f;       // Bus Voltage (V)
  float shunt_mv = 0.0f;      // Shunt Drop (mV)
  float current = 0.0f;       // Current (A)
  float power = 0.0f;         // Power (W)
  float energy_wh = 0.0f;     // Total Accumulated Energy in NVS (Wh)
  float session_wh = 0.0f;    // Energy since boot (Wh)
  float session_ah = 0.0f;    // Charge since boot (Ah)

  // Advanced Power Quality & Load Analytics
  float v_ripple_mv = 0.0f;         // Peak-to-peak bus voltage ripple in last 1s (mV)
  float i_ripple_ma = 0.0f;         // Peak-to-peak current ripple in last 1s (mA)
  float avg_power_1m = 0.0f;        // 1-minute rolling average power (W)
  float avg_current_1m = 0.0f;      // 1-minute rolling average current (A)
  float projected_kwh_month = 0.0f; // Extrapolated monthly kWh
  float shunt_loss_mw = 0.0f;       // Shunt I^2*R power loss (mW)

  // Climate & Enclosure Safety
  float temperature = 0.0f;         // Enclosure Temp (°C)
  float humidity = 0.0f;            // Relative Humidity (%)
  float dew_point = 0.0f;           // Dew Point (°C)
  float condensation_margin_c = 0.0f; // Temp - Dew Point (°C)

  // Peak & Dip Transient Metrics
  float peak_voltage = 0.0f;
  float lowest_dip_voltage = 999.0f;
  uint32_t dip_count = 0;
  uint32_t surge_count = 0;
  float last_dip_depth_v = 0.0f;
  bool in_dip_state = false;
  bool in_surge_state = false;

  float peak_current = 0.0f;
  float peak_power = 0.0f;
  float max_temp = -99.0f;

  bool ina_ok = false;
  bool sht_ok = false;
  uint8_t sht_address = 0x00;

  std::vector<uint8_t> i2c_devices;
} telem;

bool isUpdating = false;

// ============================================================================
// I2C SCANNER & SENSOR SETUP
// ============================================================================
void scanI2CBus() {
  telem.i2c_devices.clear();
  Serial.println("\n--- [I2C Bus Scan] ---");
  byte count = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      telem.i2c_devices.push_back(addr);
      Serial.printf(" [I2C] Found device at 0x%02X", addr);
      if (addr == 0x40) Serial.print(" (INA226)");
      else if (addr == 0x44) Serial.print(" (SHT30 default)");
      else if (addr == 0x45) Serial.print(" (SHT30 alt/shield)");
      Serial.println();
      count++;
    }
  }
  if (count == 0) {
    Serial.println(" [I2C] WARNING: No I2C devices found! Check SDA/SCL wiring & pullups.");
  } else {
    Serial.printf(" [I2C] Total devices found: %d\n", count);
  }
  Serial.println("----------------------\n");
}

bool initSHT30() {
  // Try 0x44 first
  Wire.beginTransmission(0x44);
  if (Wire.endTransmission() == 0) {
    sht = SHT31(0x44);
    if (sht.begin()) {
      telem.sht_ok = true;
      telem.sht_address = 0x44;
      Serial.println("[SHT30] Successfully connected at address 0x44");
      return true;
    }
  }

  // Try 0x45 (common on Wemos D1 SHT30 shields)
  Wire.beginTransmission(0x45);
  if (Wire.endTransmission() == 0) {
    sht = SHT31(0x45);
    if (sht.begin()) {
      telem.sht_ok = true;
      telem.sht_address = 0x45;
      Serial.println("[SHT30] Successfully connected at address 0x45 (Wemos D1 Shield)");
      return true;
    }
  }

  telem.sht_ok = false;
  telem.sht_address = 0x00;
  return false;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
float calculateDewPoint(float tempC, float humPct) {
  if (humPct <= 0.0f) return 0.0f;
  const float a = 17.27f;
  const float b = 237.7f;
  float alpha = ((a * tempC) / (b + tempC)) + log(humPct / 100.0f);
  return (b * alpha) / (a - alpha);
}

String formatUptime() {
  unsigned long sec = millis() / 1000;
  unsigned long days = sec / 86400;
  sec %= 86400;
  unsigned long hours = sec / 3600;
  sec %= 3600;
  unsigned long minutes = sec / 60;
  sec %= 60;

  char buf[32];
  if (days > 0) {
    snprintf(buf, sizeof(buf), "%lud %luh %lum", days, hours, minutes);
  } else {
    snprintf(buf, sizeof(buf), "%luh %lum %lus", hours, minutes, sec);
  }
  return String(buf);
}

// ============================================================================
// FREERTOS TASK: SENSOR SAMPLING (CORE 1 - Real-Time Priority 3)
// ============================================================================
void TaskSensors(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(20); // 50 Hz (20ms cadence)
  uint32_t loopCounter = 0;
  unsigned long lastIntegrationTime = millis();
  unsigned long lastShtRetryTime = 0;

  // Rolling 1-second (50 samples) window for peak-to-peak ripple calculations
  float v_window_max = 0.0f;
  float v_window_min = 999.0f;
  float i_window_max = 0.0f;
  float i_window_min = 999.0f;

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xPeriod);

    if (isUpdating) {
      continue;
    }

    unsigned long now = millis();
    float dt = (now - lastIntegrationTime) / 1000.0f;
    lastIntegrationTime = now;

    // 1. Read INA226 ADC
    float v_raw = 0.0f, shunt_raw = 0.0f, i_raw = 0.0f, p_raw = 0.0f;
    if (telem.ina_ok) {
      v_raw = ina.getBusVoltage();
      shunt_raw = ina.getShuntVoltage_mV();
      i_raw = ina.getCurrent_mA() / 1000.0f;
      p_raw = ina.getPower_mW() / 1000.0f;
      if (abs(i_raw) < 0.003f) {
        i_raw = 0.0f;
        p_raw = 0.0f;
      }
    }

    // Voltage & Current ripple tracking across 50 samples
    if (v_raw > 1.0f) {
      if (v_raw > v_window_max) v_window_max = v_raw;
      if (v_raw < v_window_min) v_window_min = v_raw;
    }
    if (i_raw >= 0.0f) {
      if (i_raw > i_window_max) i_window_max = i_raw;
      if (i_raw < i_window_min) i_window_min = i_raw;
    }

    // 2. Read SHT30 and evaluate ripple every 1 second (50 ticks @ 50Hz)
    float temp_raw = telem.temperature;
    float hum_raw = telem.humidity;
    float dew_raw = telem.dew_point;
    bool sht_valid = telem.sht_ok;

    if (loopCounter % 50 == 0) {
      // Calculate peak-to-peak voltage ripple over previous 1s
      if (v_window_max >= v_window_min && v_window_min < 900.0f) {
        telem.v_ripple_mv = (v_window_max - v_window_min) * 1000.0f;
      }
      v_window_max = v_raw;
      v_window_min = (v_raw > 1.0f) ? v_raw : 999.0f;

      // Calculate peak-to-peak current ripple over previous 1s
      if (i_window_max >= i_window_min && i_window_min < 900.0f) {
        telem.i_ripple_ma = (i_window_max - i_window_min) * 1000.0f;
      }
      i_window_max = i_raw;
      i_window_min = (i_raw >= 0.0f) ? i_raw : 999.0f;

      if (telem.sht_ok) {
        if (sht.read()) {
          temp_raw = sht.getTemperature();
          hum_raw = sht.getHumidity();
          dew_raw = calculateDewPoint(temp_raw, hum_raw);
        } else {
          sht_valid = false;
        }
      } else {
        if (now - lastShtRetryTime > 5000) {
          lastShtRetryTime = now;
          sht_valid = initSHT30();
        }
      }
    }

    // 3. Thread-safe Telemetry Update under Mutex
    if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      telem.voltage = v_raw;
      telem.shunt_mv = shunt_raw;
      telem.current = i_raw;
      telem.power = p_raw;
      telem.temperature = temp_raw;
      telem.humidity = hum_raw;
      telem.dew_point = dew_raw;
      telem.condensation_margin_c = (sht_valid) ? (temp_raw - dew_raw) : 0.0f;
      telem.shunt_loss_mw = (i_raw * i_raw) * SHUNT_RESISTANCE_OHMS * 1000.0f;
      telem.sht_ok = sht_valid;

      // Peak & Dip Transient Analysis
      if (telem.voltage > VOLTAGE_VALID_MIN_V) {
        if (telem.voltage > telem.peak_voltage) {
          telem.peak_voltage = telem.voltage;
        }
        if (telem.lowest_dip_voltage > 900.0f || telem.voltage < telem.lowest_dip_voltage) {
          telem.lowest_dip_voltage = telem.voltage;
        }

        // Dip Event with 0.3V hysteresis
        if (telem.voltage < VOLTAGE_DIP_THRESHOLD_V && !telem.in_dip_state) {
          telem.in_dip_state = true;
          telem.dip_count++;
          telem.last_dip_depth_v = telem.voltage;
          Serial.printf("[RTOS Core 1] Voltage Dip #%u: %.2f V\n", telem.dip_count, telem.voltage);
        } else if (telem.in_dip_state) {
          if (telem.voltage < telem.last_dip_depth_v) {
            telem.last_dip_depth_v = telem.voltage;
          }
          if (telem.voltage > (VOLTAGE_DIP_THRESHOLD_V + 0.3f)) {
            telem.in_dip_state = false;
          }
        }

        // Surge Event with 0.3V hysteresis
        if (telem.voltage > VOLTAGE_SURGE_THRESHOLD_V && !telem.in_surge_state) {
          telem.in_surge_state = true;
          telem.surge_count++;
          Serial.printf("[RTOS Core 1] Voltage Surge #%u: %.2f V\n", telem.surge_count, telem.voltage);
        } else if (telem.in_surge_state && telem.voltage < (VOLTAGE_SURGE_THRESHOLD_V - 0.3f)) {
          telem.in_surge_state = false;
        }
      }

      if (telem.current > telem.peak_current) telem.peak_current = telem.current;
      if (telem.power > telem.peak_power) telem.peak_power = telem.power;
      if (telem.temperature > telem.max_temp) telem.max_temp = telem.temperature;

      // Accurate Integration (Ah, Wh, and Total NVS Wh)
      if (dt > 0.0f && dt < 1.0f) {
        if (telem.current > 0.0f) {
          telem.session_ah += (telem.current * dt) / 3600.0f;
        }
        if (telem.power > 0.0f) {
          float delta_wh = (telem.power * dt) / 3600.0f;
          telem.session_wh += delta_wh;
          telem.energy_wh += delta_wh;
        }

        // 1-minute rolling average power & current filters & monthly extrapolation
        telem.avg_power_1m = (telem.avg_power_1m * 0.999f) + (telem.power * 0.001f);
        telem.avg_current_1m = (telem.avg_current_1m * 0.999f) + (telem.current * 0.001f);
        telem.projected_kwh_month = (telem.avg_power_1m * 24.0f * 30.5f) / 1000.0f;
      }

      xSemaphoreGive(telemMutex);
    }

    // 4. Serial Telemetry Summary every 2 seconds (100 ticks)
    if (loopCounter % 100 == 0) {
      Serial.printf("[RTOS] %5.2fV (Vpp:%3.0fmV | Dip:%5.2fV) | %5.3fA (Ipp:%3.0fmA | %5.3fAh) | %5.1fW | Tot:%6.3fkWh | SHT:%4.1fC\n",
                    telem.voltage, telem.v_ripple_mv,
                    telem.lowest_dip_voltage < 900 ? telem.lowest_dip_voltage : 0.0f,
                    telem.current, telem.i_ripple_ma, telem.session_ah,
                    telem.power, telem.energy_wh / 1000.0f,
                    telem.temperature);
    }

    loopCounter++;
  }
}

String getMqttBaseTopic() {
  String t = mqtt_topic;
  t.trim();
  if (t.length() == 0) t = "telescope/";
  if (!t.endsWith("/")) t += "/";
  return t;
}

void publishMqttTelemetry() {
  if (!mqttClient.connected()) return;

  Telemetry snap;
  if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    snap = telem;
    xSemaphoreGive(telemMutex);
  } else {
    snap = telem;
  }

  JsonDocument doc;
  doc["device"] = device_name;
  doc["voltage"] = snap.voltage;
  doc["shunt_mv"] = snap.shunt_mv;
  doc["current"] = snap.current;
  doc["power"] = snap.power;
  doc["energy_wh"] = snap.energy_wh;
  doc["total_kwh"] = snap.energy_wh / 1000.0f;
  doc["session_wh"] = snap.session_wh;
  doc["session_ah"] = snap.session_ah;

  doc["v_ripple_mv"] = snap.v_ripple_mv;
  doc["i_ripple_ma"] = snap.i_ripple_ma;
  doc["avg_power_1m"] = snap.avg_power_1m;
  doc["avg_current_1m"] = snap.avg_current_1m;
  doc["projected_kwh_month"] = snap.projected_kwh_month;
  doc["shunt_loss_mw"] = snap.shunt_loss_mw;

  doc["temperature"] = snap.temperature;
  doc["humidity"] = snap.humidity;
  doc["dew_point"] = snap.dew_point;
  doc["condensation_margin_c"] = snap.condensation_margin_c;

  doc["peak_voltage"] = snap.peak_voltage;
  doc["lowest_dip_v"] = (snap.lowest_dip_voltage < 900.0f) ? snap.lowest_dip_voltage : snap.voltage;
  doc["dip_count"] = snap.dip_count;
  doc["surge_count"] = snap.surge_count;
  doc["last_dip_depth_v"] = snap.last_dip_depth_v;

  doc["peak_current"] = snap.peak_current;
  doc["peak_power"] = snap.peak_power;
  doc["max_temp"] = snap.max_temp;

  doc["uptime_sec"] = millis() / 1000;
  doc["wifi_rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  doc["free_heap"] = ESP.getFreeHeap();

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  String dataTopic = getMqttBaseTopic() + device_name;
  mqttClient.publish(dataTopic.c_str(), jsonPayload.c_str(), false);
}

bool reconnectMQTT() {
  if (mqtt_server.length() == 0 || WiFi.status() != WL_CONNECTED) return false;

  mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
  mqttClient.setBufferSize(1024);

  String clientId = device_name + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  String lwtTopic = "tele/" + device_name + "/LWT";

  Serial.printf("[MQTT] Connecting to %s:%d as '%s' (LWT Topic: '%s')...\n",
                mqtt_server.c_str(), mqtt_port, clientId.c_str(), lwtTopic.c_str());

  bool connected = false;
  if (mqtt_user.length() > 0) {
    connected = mqttClient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str(), lwtTopic.c_str(), 1, true, "Offline");
  } else {
    connected = mqttClient.connect(clientId.c_str(), lwtTopic.c_str(), 1, true, "Offline");
  }

  if (connected) {
    Serial.printf("[MQTT] Connected successfully! Retaining LWT 'Online' on '%s'\n", lwtTopic.c_str());
    mqttClient.publish(lwtTopic.c_str(), "Online", true);
    publishMqttTelemetry();
    return true;
  } else {
    Serial.printf("[MQTT] Connection failed (rc=%d). Retrying in 10s...\n", mqttClient.state());
    return false;
  }
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================
void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleApiData() {
  Telemetry snap;

  // Snapshot telemetry under mutex
  if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    snap = telem;
    xSemaphoreGive(telemMutex);
  } else {
    snap = telem;
  }

  JsonDocument doc;

  doc["device_name"] = device_name;
  doc["voltage"] = snap.voltage;
  doc["shunt_mv"] = snap.shunt_mv;
  doc["current"] = snap.current;
  doc["power"] = snap.power;
  doc["energy_wh"] = snap.energy_wh;
  doc["total_kwh"] = snap.energy_wh / 1000.0f;
  doc["session_wh"] = snap.session_wh;
  doc["session_ah"] = snap.session_ah;

  doc["v_ripple_mv"] = snap.v_ripple_mv;
  doc["i_ripple_ma"] = snap.i_ripple_ma;
  doc["avg_power_1m"] = snap.avg_power_1m;
  doc["avg_current_1m"] = snap.avg_current_1m;
  doc["projected_kwh_month"] = snap.projected_kwh_month;
  doc["shunt_loss_mw"] = snap.shunt_loss_mw;

  doc["temperature"] = snap.temperature;
  doc["humidity"] = snap.humidity;
  doc["dew_point"] = snap.dew_point;
  doc["condensation_margin_c"] = snap.condensation_margin_c;

  doc["peak_voltage"] = snap.peak_voltage;
  doc["lowest_dip_v"] = (snap.lowest_dip_voltage < 900.0f) ? snap.lowest_dip_voltage : snap.voltage;
  doc["dip_count"] = snap.dip_count;
  doc["surge_count"] = snap.surge_count;
  doc["last_dip_depth_v"] = snap.last_dip_depth_v;

  doc["peak_current"] = snap.peak_current;
  doc["peak_power"] = snap.peak_power;
  doc["max_temp"] = snap.max_temp;

  doc["ina_status"] = snap.ina_ok;
  doc["sht_status"] = snap.sht_ok;
  doc["sht_addr"] = snap.sht_address;
  doc["uptime_str"] = formatUptime();
  doc["uptime_sec"] = millis() / 1000;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(AP_SSID);
  doc["wifi_rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  doc["mqtt_connected"] = mqttClient.connected();
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_topic"] = getMqttBaseTopic() + device_name;
  doc["mqtt_lwt_topic"] = "tele/" + device_name + "/LWT";
  doc["version"] = FIRMWARE_VERSION;
  doc["build_date"] = __DATE__;
  doc["build_time"] = __TIME__;
  doc["free_heap"] = ESP.getFreeHeap();

  JsonArray devArray = doc["i2c_devices"].to<JsonArray>();
  for (uint8_t d : snap.i2c_devices) {
    devArray.add(d);
  }

  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void handleResetEnergy() {
  if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    telem.energy_wh = 0.0f;
    telem.session_wh = 0.0f;
    telem.session_ah = 0.0f;
    prefs.putFloat("energy_wh", 0.0f);
    xSemaphoreGive(telemMutex);
  }
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Energy counters reset\"}");
}

void handleResetStats() {
  if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    telem.peak_voltage = telem.voltage;
    telem.lowest_dip_voltage = (telem.voltage > VOLTAGE_VALID_MIN_V) ? telem.voltage : 999.0f;
    telem.dip_count = 0;
    telem.surge_count = 0;
    telem.last_dip_depth_v = 0.0f;
    telem.peak_current = telem.current;
    telem.peak_power = telem.power;
    telem.avg_current_1m = telem.current;
    telem.avg_power_1m = telem.power;
    telem.i_ripple_ma = 0.0f;
    telem.v_ripple_mv = 0.0f;
    xSemaphoreGive(telemMutex);
  }
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Transient & current stats reset\"}");
}

// Wi-Fi & MQTT Setup Handlers
void handleSetup() {
  int n = WiFi.scanNetworks();
  String options = "";
  if (n <= 0) {
    options = "<option value=\"\">No networks found (refresh to retry)</option>";
  } else {
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      int rssi = WiFi.RSSI(i);
      String enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      options += "<option value=\"" + ssid + "\">" + ssid + " (" + String(rssi) + " dBm, " + enc + ")</option>\n";
    }
  }
  String html = String(SETUP_HTML);
  html.replace("{{NETWORKS}}", options);
  html.replace("{{WIFI_SSID}}", wifi_ssid_str);
  html.replace("{{DEVICE_NAME}}", device_name);
  html.replace("{{MQTT_SERVER}}", mqtt_server);
  html.replace("{{MQTT_PORT}}", String(mqtt_port));
  html.replace("{{MQTT_TOPIC}}", mqtt_topic);
  html.replace("{{MQTT_USER}}", mqtt_user);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", html);
}

void handleSaveWifi() {
  if (server.hasArg("ssid") && server.arg("ssid").length() > 0) {
    String new_ssid = server.arg("ssid");
    new_ssid.trim();
    prefs.putString("wifi_ssid", new_ssid);
    wifi_ssid_str = new_ssid;
  }
  if (server.hasArg("password") && server.arg("password").length() > 0) {
    String new_pass = server.arg("password");
    new_pass.trim();
    prefs.putString("wifi_pass", new_pass);
    wifi_pass_str = new_pass;
  }

  if (server.hasArg("device_name")) {
    device_name = server.arg("device_name");
    device_name.trim();
    if (device_name.length() == 0) device_name = "pmon";
    prefs.putString("device_name", device_name);
  }
  if (server.hasArg("mqtt_server")) {
    mqtt_server = server.arg("mqtt_server");
    mqtt_server.trim();
    prefs.putString("mqtt_server", mqtt_server);
  }
  if (server.hasArg("mqtt_port")) {
    uint16_t p = server.arg("mqtt_port").toInt();
    if (p > 0) {
      mqtt_port = p;
      prefs.putUShort("mqtt_port", mqtt_port);
    }
  }
  if (server.hasArg("mqtt_topic")) {
    mqtt_topic = server.arg("mqtt_topic");
    mqtt_topic.trim();
    if (mqtt_topic.length() == 0) mqtt_topic = "telescope/";
    prefs.putString("mqtt_topic", mqtt_topic);
  }
  if (server.hasArg("mqtt_user")) {
    mqtt_user = server.arg("mqtt_user");
    mqtt_user.trim();
    prefs.putString("mqtt_user", mqtt_user);
  }
  if (server.hasArg("mqtt_pass") && server.arg("mqtt_pass").length() > 0) {
    mqtt_pass = server.arg("mqtt_pass");
    mqtt_pass.trim();
    prefs.putString("mqtt_pass", mqtt_pass);
  }

  Serial.printf("[Config] Saved Wi-Fi SSID='%s', Device='%s' & MQTT Server='%s:%d' (LWT: tele/%s/LWT)\n",
                wifi_ssid_str.c_str(), device_name.c_str(), mqtt_server.c_str(), mqtt_port, device_name.c_str());

  String resp = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><meta http-equiv='refresh' content='10;url=/'><style>body{background:#0b0f19;color:#f1f5f9;font-family:sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;text-align:center;}.card{background:rgba(18,26,44,0.9);padding:2rem;border-radius:16px;border:1px solid rgba(255,255,255,0.1);max-width:400px;box-shadow:0 10px 30px rgba(0,0,0,0.5);}h2{color:#00d4ff;margin-bottom:1rem;}p{color:#94a3b8;line-height:1.5;}</style></head><body><div class='card'><h2>Configuration Saved!</h2><p>Connecting to <b>" + wifi_ssid_str + "</b>...</p><p>Device Name: <b>" + device_name + "</b> (LWT: <code>tele/" + device_name + "/LWT</code>)</p><p>The monitor is rebooting now.</p></div></body></html>";

  server.send(200, "text/html", resp);
  delay(1500);
  ESP.restart();
}

void handleCaptiveRedirect() {
  server.sendHeader("Location", "http://192.168.4.1/setup", true);
  server.send(302, "text/plain", "");
}

void handleNotFound() {
  if (isApMode) {
    server.sendHeader("Location", "http://192.168.4.1/setup", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "404: Not Found");
}

// Web OTA Handlers
void handleUpdateDone() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "UPDATE FAILED" : "UPDATE SUCCESSFUL! Rebooting...");
  delay(1000);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    isUpdating = true;
    Serial.printf("[Web OTA] Update start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    isUpdating = false;
    if (Update.end(true)) {
      Serial.printf("[Web OTA] Update Success: %u bytes\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// ============================================================================
// FREERTOS TASK: WEB SERVER, MQTT & NETWORK (CORE 0 - Priority 1)
// ============================================================================
void TaskWeb(void *pvParameters) {
  unsigned long lastNvsSave = millis();

  for (;;) {
    if (isApMode) {
      dnsServer.processNextRequest();
    }
    server.handleClient();

    if (!isApMode && WiFi.status() == WL_CONNECTED) {
      ArduinoOTA.handle();

      // MQTT Management
      if (mqtt_server.length() > 0) {
        mqttClient.loop();

        unsigned long now = millis();
        if (!mqttClient.connected()) {
          if (now - lastMqttReconnectAttempt > 10000) {
            lastMqttReconnectAttempt = now;
            reconnectMQTT();
          }
        } else {
          if (now - lastMqttPublishTime >= 2000) {
            lastMqttPublishTime = now;
            publishMqttTelemetry();
          }
        }
      }
    }

    // Periodic NVS save of energy counter (every 5 minutes)
    unsigned long now = millis();
    if (now - lastNvsSave > 300000) {
      lastNvsSave = now;
      if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        prefs.putFloat("energy_wh", telem.energy_wh);
        xSemaphoreGive(telemMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(2)); // Yield to network stack on Core 0
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("  24VDC Power Monitor (FreeRTOS Dual-Core)");
  Serial.println("==========================================");

  // Create FreeRTOS Mutex
  telemMutex = xSemaphoreCreateMutex();

  // Initialize NVS storage
  prefs.begin("powermon", false);
  telem.energy_wh = prefs.getFloat("energy_wh", 0.0f);
  Serial.printf("[NVS] Restored Accumulated Energy: %.2f Wh\n", telem.energy_wh);

  wifi_ssid_str = prefs.getString("wifi_ssid", "");
  wifi_pass_str = prefs.getString("wifi_pass", "");

  device_name = prefs.getString("device_name", "pmon");
  if (device_name.length() == 0) device_name = "pmon";

  mqtt_server = prefs.getString("mqtt_server", "");
  mqtt_port = prefs.getUShort("mqtt_port", 1883);
  mqtt_topic = prefs.getString("mqtt_topic", "telescope/");
  mqtt_user = prefs.getString("mqtt_user", "");
  mqtt_pass = prefs.getString("mqtt_pass", "");
  if (mqtt_port == 0) mqtt_port = 1883;
  if (mqtt_topic.length() == 0) mqtt_topic = "telescope/";

  if (mqtt_server.length() > 0) {
    Serial.printf("[NVS] Restored Device: '%s', MQTT Broker: %s:%d (Topic: %s%s, LWT: tele/%s/LWT)\n",
                  device_name.c_str(), mqtt_server.c_str(), mqtt_port, mqtt_topic.c_str(), device_name.c_str(), device_name.c_str());
  }

  // Initialize I2C Bus at 100kHz standard speed
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000);
  Serial.printf("[I2C] Initialized on SDA=GPIO%d, SCL=GPIO%d (100kHz)\n", I2C_SDA_PIN, I2C_SCL_PIN);

  // Scan I2C bus
  scanI2CBus();

  // Scan & Init INA226 (0x40)
  if (ina.begin()) {
    telem.ina_ok = true;
    ina.setMaxCurrentShunt(MAX_EXPECTED_CURRENT_A, SHUNT_RESISTANCE_OHMS);
    Serial.println("[INA226] Current & Voltage Sensor initialized at 0x40");
  } else {
    Serial.println("[INA226] ERROR: Sensor not found at 0x40.");
  }

  // Scan & Init SHT30 (Auto-probe 0x44 and 0x45)
  if (!initSHT30()) {
    Serial.println("[SHT30] WARNING: SHT30 sensor not responding on 0x44 or 0x45. Task will retry.");
  }

  // Connect to WiFi
  WiFi.mode(WIFI_AP_STA);
  if (wifi_ssid_str.length() > 0) {
    Serial.printf("[WiFi] Attempting connection to '%s'...\n", wifi_ssid_str.c_str());
    WiFi.begin(wifi_ssid_str.c_str(), wifi_pass_str.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 8000) {
      delay(250);
      Serial.print(".");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    isApMode = false;
    Serial.println("\n[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    isApMode = true;
    Serial.println("\n[WiFi] Connection failed or not configured. Launching Fallback Captive Portal...");
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, ""); // Open SoftAP for easy captive configuration
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.printf("[Captive Portal] SoftAP SSID: '%s' (Open)\n", AP_SSID);
    Serial.println("[Captive Portal] Access Wi-Fi Setup at http://192.168.4.1/setup");
  }

  // Setup mDNS responder
  if (MDNS.begin("powermonitor")) {
    Serial.println("[mDNS] Web interface accessible at http://powermonitor.local");
  }

  // Setup ArduinoOTA for network upload
  ArduinoOTA.setHostname("powermonitor");
  ArduinoOTA.onStart([]() {
    isUpdating = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("[ArduinoOTA] Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    isUpdating = false;
    Serial.println("\n[ArduinoOTA] Update complete. Rebooting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[ArduinoOTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    isUpdating = false;
    Serial.printf("[ArduinoOTA] Error[%u]: ", error);
  });
  ArduinoOTA.begin();

  // Start HTTP Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setup", HTTP_GET, handleSetup);
  server.on("/save-wifi", HTTP_POST, handleSaveWifi);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/reset-energy", HTTP_POST, handleResetEnergy);
  server.on("/api/reset-stats", HTTP_POST, handleResetStats);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);

  // Captive Portal Detection Endpoints
  server.on("/generate_204", HTTP_GET, handleCaptiveRedirect);       // Android
  server.on("/gen_204", HTTP_GET, handleCaptiveRedirect);            // Android
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect);// Apple iOS/macOS
  server.on("/library/test/success.html", HTTP_GET, handleCaptiveRedirect); // Apple
  server.on("/ncsi.txt", HTTP_GET, handleCaptiveRedirect);           // Windows
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);    // Windows
  server.on("/redirect", HTTP_GET, handleCaptiveRedirect);           // Microsoft

  server.onNotFound(handleNotFound);

  server.enableCORS(true);
  server.begin();
  Serial.println("[HTTP] Web Server started on port 80");

  // Launch Dual-Core FreeRTOS Tasks
  Serial.println("[RTOS] Starting Dual-Core FreeRTOS Tasks...");
  
  // Task 1: Real-time sensor sampling pinned to Core 1 (High Priority 3)
  xTaskCreatePinnedToCore(
    TaskSensors,
    "TaskSensors",
    4096,
    NULL,
    3,
    NULL,
    1 // Core 1
  );

  // Task 2: Web Server, MQTT & Network pinned to Core 0 (Priority 1)
  xTaskCreatePinnedToCore(
    TaskWeb,
    "TaskWeb",
    8192,
    NULL,
    1,
    NULL,
    0 // Core 0
  );

  Serial.println("[RTOS] Core 1 -> 50Hz Sensor Sampling & Transient Capture");
  Serial.println("[RTOS] Core 0 -> Web Server, MQTT, JSON API, WiFi & OTA");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}