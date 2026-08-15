#include <Wire.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// SMART H2S MONITOR - LAYER 1 IoT
// Board : ESP32-S2 DevKitM-1
// Author: AFYUADRI PUTRA
//
// DATA FLOW:
//
// Potentiometer
//      ↓
// ESP32-S2
//      ↓
// Simulated H2S ppm
//      ↓
// Classification
//      ├── OLED
//      ├── LED
//      ├── Buzzer
//      └── MQTT
//            ↓
//      HiveMQ Broker
//            ↓
//      React Dashboard
//
// CATATAN:
// Potentiometer hanya digunakan sebagai SIMULATOR H2S.
// Nilai ppm pada Wokwi BUKAN hasil kalibrasi sensor nyata.
// ======================================================


// ======================================================
// PIN CONFIGURATION
// ======================================================

#define H2S_PIN 4

#define OLED_SDA 8
#define OLED_SCL 9

#define LED_GREEN  39
#define LED_YELLOW 40
#define LED_RED    41

#define BUZZER_PIN 42


// ======================================================
// OLED
// ======================================================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);


// ======================================================
// WIFI
// ======================================================

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";


// ======================================================
// MQTT
// ======================================================

const char* MQTT_BROKER = "broker.hivemq.com";

const int MQTT_PORT = 1883;

const char* MQTT_TOPIC =
  "afyuadri/h2s-demo/a7c91f/device-001/telemetry";


// Unique MQTT client ID
const char* MQTT_CLIENT_ID =
  "afyuadri-h2s-tpa-001-wokwi";


// Network client
WiFiClient wifiClient;

// MQTT client
MqttClient mqttClient(wifiClient);


// ======================================================
// SIMULASI SENSOR
// ======================================================

// ADC ESP32 = 0 - 4095
#define ADC_MAX 4095.0

// Potentiometer mensimulasikan 0 - 1000 ppm H2S
#define H2S_MAX_PPM 1000.0


// ======================================================
// FILTER
// ======================================================

const int FILTER_SIZE = 10;

int adcBuffer[FILTER_SIZE];

int bufferIndex = 0;


// ======================================================
// TIMING
// ======================================================

// OLED refresh
unsigned long lastDisplayUpdate = 0;

const unsigned long DISPLAY_INTERVAL = 250;


// Serial Monitor
unsigned long lastSerialUpdate = 0;

const unsigned long SERIAL_INTERVAL = 1000;


// MQTT publish
unsigned long lastMqttPublish = 0;

const unsigned long MQTT_PUBLISH_INTERVAL = 1000;


// Reconnect WiFi/MQTT
unsigned long lastNetworkCheck = 0;

const unsigned long NETWORK_CHECK_INTERVAL = 3000;


// ======================================================
// STRUCT STATUS H2S
// ======================================================

struct H2SStatus {

  int level;

  String title;

  String oledLine1;

  String oledLine2;

  String description;
};


// ======================================================
// FUNCTION PROTOTYPES
// ======================================================

float getFilteredADC(int newReading);

float adcToPPM(float adcValue);

H2SStatus classifyH2S(float ppm);

void updateLED(int level);

void updateAlarm(int level);

void beepPattern(
  unsigned long now,
  unsigned long interval,
  unsigned long beepDuration,
  int frequency
);

void updateOLED(
  float ppm,
  float adc,
  H2SStatus status
);

void printSerial(
  int rawADC,
  float filteredADC,
  float ppm,
  H2SStatus status
);

bool connectWiFi();

bool connectMQTT();

void maintainNetwork();

void publishH2SData(
  int rawADC,
  float filteredADC,
  float ppm,
  H2SStatus status
);


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);


  // ====================================================
  // PIN
  // ====================================================

  pinMode(H2S_PIN, INPUT);

  pinMode(LED_GREEN, OUTPUT);

  pinMode(LED_YELLOW, OUTPUT);

  pinMode(LED_RED, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);


  digitalWrite(LED_GREEN, LOW);

  digitalWrite(LED_YELLOW, LOW);

  digitalWrite(LED_RED, LOW);

  noTone(BUZZER_PIN);


  // ====================================================
  // OLED
  // ====================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );


  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      0x3C
    )
  ) {

    Serial.println(
      "OLED SSD1306 tidak ditemukan!"
    );


    while (true) {

      delay(100);
    }
  }


  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);


  display.setCursor(15, 10);

  display.println(
    "SMART H2S"
  );


  display.setCursor(20, 25);

  display.println(
    "IoT MONITOR"
  );


  display.setCursor(12, 45);

  display.println(
    "Initializing..."
  );


  display.display();


  delay(1000);


  // ====================================================
  // FILTER BUFFER INITIALIZATION
  // ====================================================

  for (
    int i = 0;
    i < FILTER_SIZE;
    i++
  ) {

    adcBuffer[i] =
      analogRead(H2S_PIN);
  }


  // ====================================================
  // START INFO
  // ====================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    " SMART H2S IoT MONITOR"
  );

  Serial.println(
    " ESP32-S2 DevKitM-1"
  );

  Serial.println(
    " MQTT + REACT DASHBOARD"
  );

  Serial.println(
    "========================================"
  );

  Serial.println();


  // ====================================================
  // MQTT CONFIG
  // ====================================================

  mqttClient.setId(
    MQTT_CLIENT_ID
  );


  // ====================================================
  // WIFI
  // ====================================================

  connectWiFi();


  // ====================================================
  // MQTT
  // ====================================================

  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    connectMQTT();
  }
}


// ======================================================
// LOOP
// ======================================================

void loop() {

  // ====================================================
  // NETWORK
  // ====================================================

  maintainNetwork();


  // MQTT keep alive
  if (
    mqttClient.connected()
  ) {

    mqttClient.poll();
  }


  // ====================================================
  // SENSOR
  // ====================================================

  int rawADC =
    analogRead(H2S_PIN);


  float filteredADC =
    getFilteredADC(
      rawADC
    );


  float h2sPPM =
    adcToPPM(
      filteredADC
    );


  // ====================================================
  // CLASSIFICATION
  // ====================================================

  H2SStatus status =
    classifyH2S(
      h2sPPM
    );


  // ====================================================
  // LED
  // ====================================================

  updateLED(
    status.level
  );


  // ====================================================
  // BUZZER
  // ====================================================

  updateAlarm(
    status.level
  );


  // ====================================================
  // OLED
  // ====================================================

  if (
    millis() - lastDisplayUpdate
    >= DISPLAY_INTERVAL
  ) {

    lastDisplayUpdate =
      millis();


    updateOLED(
      h2sPPM,
      filteredADC,
      status
    );
  }


  // ====================================================
  // SERIAL
  // ====================================================

  if (
    millis() - lastSerialUpdate
    >= SERIAL_INTERVAL
  ) {

    lastSerialUpdate =
      millis();


    printSerial(
      rawADC,
      filteredADC,
      h2sPPM,
      status
    );
  }


  // ====================================================
  // MQTT PUBLISH
  // ====================================================

  if (
    millis() - lastMqttPublish
    >= MQTT_PUBLISH_INTERVAL
  ) {

    lastMqttPublish =
      millis();


    if (
      mqttClient.connected()
    ) {

      publishH2SData(
        rawADC,
        filteredADC,
        h2sPPM,
        status
      );
    }
  }


  delay(10);
}


// ======================================================
// WIFI CONNECTION
// ======================================================

bool connectWiFi() {

  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    return true;
  }


  Serial.println();

  Serial.println(
    "[WiFi] Connecting..."
  );


  WiFi.mode(
    WIFI_STA
  );


  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long startTime =
    millis();


  const unsigned long timeout =
    10000;


  while (
    WiFi.status()
      != WL_CONNECTED
    &&
    millis() - startTime
      < timeout
  ) {

    delay(250);

    Serial.print(".");
  }


  Serial.println();


  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    Serial.println(
      "[WiFi] CONNECTED"
    );


    Serial.print(
      "[WiFi] IP: "
    );

    Serial.println(
      WiFi.localIP()
    );


    return true;
  }


  Serial.println(
    "[WiFi] Connection timeout"
  );


  return false;
}


// ======================================================
// MQTT CONNECTION
// ======================================================

bool connectMQTT() {

  if (
    mqttClient.connected()
  ) {

    return true;
  }


  if (
    WiFi.status()
    != WL_CONNECTED
  ) {

    return false;
  }


  Serial.println();

  Serial.print(
    "[MQTT] Connecting to "
  );

  Serial.print(
    MQTT_BROKER
  );

  Serial.print(":");

  Serial.println(
    MQTT_PORT
  );


  if (
    !mqttClient.connect(
      MQTT_BROKER,
      MQTT_PORT
    )
  ) {

    Serial.print(
      "[MQTT] FAILED. Error code: "
    );

    Serial.println(
      mqttClient.connectError()
    );


    return false;
  }


  Serial.println(
    "[MQTT] CONNECTED"
  );


  Serial.print(
    "[MQTT] Topic: "
  );

  Serial.println(
    MQTT_TOPIC
  );


  return true;
}


// ======================================================
// NETWORK MAINTENANCE
// ======================================================

void maintainNetwork() {

  if (
    millis() - lastNetworkCheck
    < NETWORK_CHECK_INTERVAL
  ) {

    return;
  }


  lastNetworkCheck =
    millis();


  // ====================================================
  // CHECK WIFI
  // ====================================================

  if (
    WiFi.status()
    != WL_CONNECTED
  ) {

    Serial.println(
      "[NETWORK] WiFi disconnected"
    );


    connectWiFi();

    return;
  }


  // ====================================================
  // CHECK MQTT
  // ====================================================

  if (
    !mqttClient.connected()
  ) {

    Serial.println(
      "[NETWORK] MQTT disconnected"
    );


    connectMQTT();
  }
}


// ======================================================
// MQTT PUBLISH
// ======================================================

void publishH2SData(
  int rawADC,
  float filteredADC,
  float ppm,
  H2SStatus status
) {

  if (
    !mqttClient.connected()
  ) {

    return;
  }


  // ====================================================
  // CREATE JSON PAYLOAD
  // ====================================================

  String payload = "{";


  payload +=
    "\"device_id\":\"H2S-TPA-001\",";


  payload +=
    "\"ppm\":";

  payload +=
    String(ppm, 2);

  payload += ",";


  payload +=
    "\"adc\":";

  payload +=
    String(rawADC);

  payload += ",";


  payload +=
    "\"filtered_adc\":";

  payload +=
    String(filteredADC, 2);

  payload += ",";


  payload +=
    "\"level\":";

  payload +=
    String(status.level);

  payload += ",";


  payload +=
    "\"status\":\"";

  payload +=
    status.title;

  payload += "\",";


  payload +=
    "\"effect\":\"";

  payload +=
    status.description;

  payload += "\",";


  payload +=
    "\"uptime_ms\":";

  payload +=
    String(millis());

  payload += ",";


  payload +=
    "\"simulated\":true";


  payload += "}";


  // ====================================================
  // SEND MQTT
  // ====================================================

  mqttClient.beginMessage(
    MQTT_TOPIC
  );


  mqttClient.print(
    payload
  );


  int result =
    mqttClient.endMessage();


  // ====================================================
  // DEBUG
  // ====================================================

  if (result == 1) {

    Serial.println();

    Serial.println(
      "[MQTT] PUBLISH SUCCESS"
    );


    Serial.print(
      "[MQTT] "
    );

    Serial.println(
      payload
    );

  }

  else {

    Serial.println(
      "[MQTT] PUBLISH FAILED"
    );
  }
}


// ======================================================
// MOVING AVERAGE FILTER
// ======================================================

float getFilteredADC(
  int newReading
) {

  adcBuffer[
    bufferIndex
  ] = newReading;


  bufferIndex++;


  if (
    bufferIndex
    >= FILTER_SIZE
  ) {

    bufferIndex = 0;
  }


  long total = 0;


  for (
    int i = 0;
    i < FILTER_SIZE;
    i++
  ) {

    total +=
      adcBuffer[i];
  }


  return
    (float) total
    /
    FILTER_SIZE;
}


// ======================================================
// ADC → SIMULATED H2S PPM
// ======================================================

float adcToPPM(
  float adcValue
) {

  float ppm =
    (
      adcValue
      /
      ADC_MAX
    )
    *
    H2S_MAX_PPM;


  if (
    ppm < 0
  ) {

    ppm = 0;
  }


  if (
    ppm > H2S_MAX_PPM
  ) {

    ppm =
      H2S_MAX_PPM;
  }


  return ppm;
}


// ======================================================
// PEMETAAN KONDISI H2S
// ======================================================

H2SStatus classifyH2S(
  float ppm
) {

  H2SStatus result;


  // ====================================================
  // < 0.13 ppm
  // ====================================================

  if (
    ppm < 0.13
  ) {

    result.level = 0;

    result.title =
      "NORMAL";

    result.oledLine1 =
      "Belum tercium";

    result.oledLine2 =
      "";

    result.description =
      "Konsentrasi di bawah 0.13 ppm.";
  }


  // ====================================================
  // 0.13 - <4.6 ppm
  // ====================================================

  else if (
    ppm < 4.6
  ) {

    result.level = 1;

    result.title =
      "BAU TERDETEKSI";

    result.oledLine1 =
      "Bau mulai";

    result.oledLine2 =
      "dirasakan";

    result.description =
      "Bau H2S mulai dapat dirasakan.";
  }


  // ====================================================
  // 4.6 - <10 ppm
  // ====================================================

  else if (
    ppm < 10.0
  ) {

    result.level = 2;

    result.title =
      "WASPADA";

    result.oledLine1 =
      "Bau mudah";

    result.oledLine2 =
      "terdeteksi";

    result.description =
      "Bau H2S mudah terdeteksi.";
  }


  // ====================================================
  // 10 - <27 ppm
  // ====================================================

  else if (
    ppm < 27.0
  ) {

    result.level = 3;

    result.title =
      "IRITASI";

    result.oledLine1 =
      "Awal iritasi";

    result.oledLine2 =
      "mata / berair";

    result.description =
      "Awal iritasi mata dan mata berair.";
  }


  // ====================================================
  // 27 - <100 ppm
  // ====================================================

  else if (
    ppm < 100.0
  ) {

    result.level = 4;

    result.title =
      "DANGER";

    result.oledLine1 =
      "Bau sangat";

    result.oledLine2 =
      "tak tertoleransi";

    result.description =
      "Bau sangat tidak dapat ditoleransi.";
  }


  // ====================================================
  // 100 - <200 ppm
  // ====================================================

  else if (
    ppm < 200.0
  ) {

    result.level = 5;

    result.title =
      "BAHAYA";

    result.oledLine1 =
      "Batuk / iritasi";

    result.oledLine2 =
      "gangguan bau";

    result.description =
      "Batuk, iritasi mata, dan gangguan penciuman.";
  }


  // ====================================================
  // 200 - 300 ppm
  // ====================================================

  else if (
    ppm <= 300.0
  ) {

    result.level = 6;

    result.title =
      "BAHAYA BERAT";

    result.oledLine1 =
      "Iritasi berat";

    result.oledLine2 =
      "mata/tenggorok";

    result.description =
      "Iritasi berat mata dan tenggorokan.";
  }


  // ====================================================
  // >300 - <500 ppm
  // ====================================================

  else if (
    ppm < 500.0
  ) {

    result.level = 7;

    result.title =
      "BAHAYA TINGGI";

    result.oledLine1 =
      "300-500 ppm";

    result.oledLine2 =
      "Data blm spesifik";

    result.description =
      "Rentang 300-500 ppm tidak didefinisikan secara spesifik pada tabel acuan.";
  }


  // ====================================================
  // 500 - 700 ppm
  // ====================================================

  else if (
    ppm <= 700.0
  ) {

    result.level = 8;

    result.title =
      "SANGAT BAHAYA";

    result.oledLine1 =
      "Hilang sadar";

    result.oledLine2 =
      "Dapat fatal";

    result.description =
      "Kehilangan kesadaran dan dapat berakibat fatal.";
  }


  // ====================================================
  // >700 ppm
  // ====================================================

  else {

    result.level = 9;

    result.title =
      "EMERGENCY";

    result.oledLine1 =
      "HILANG SADAR";

    result.oledLine2 =
      "RISIKO KEMATIAN";

    result.description =
      "Kehilangan kesadaran cepat dan dapat berlanjut ke kematian.";
  }


  return result;
}


// ======================================================
// LED CONTROL
// ======================================================

void updateLED(
  int level
) {

  digitalWrite(
    LED_GREEN,
    LOW
  );

  digitalWrite(
    LED_YELLOW,
    LOW
  );

  digitalWrite(
    LED_RED,
    LOW
  );


  // ====================================================
  // LEVEL 0 - 1
  // ====================================================

  if (
    level <= 1
  ) {

    digitalWrite(
      LED_GREEN,
      HIGH
    );
  }


  // ====================================================
  // LEVEL 2 - 3
  // ====================================================

  else if (
    level <= 3
  ) {

    digitalWrite(
      LED_YELLOW,
      HIGH
    );
  }


  // ====================================================
  // LEVEL 4+
  // ====================================================

  else {

    if (
      level >= 6
    ) {

      bool blinkState =
        (
          millis()
          /
          300
        )
        %
        2;


      digitalWrite(
        LED_RED,
        blinkState
          ? HIGH
          : LOW
      );

    }

    else {

      digitalWrite(
        LED_RED,
        HIGH
      );
    }
  }
}


// ======================================================
// BUZZER
// ======================================================

void updateAlarm(
  int level
) {

  unsigned long now =
    millis();


  // ====================================================
  // LEVEL 0 - 2
  // ====================================================

  if (
    level <= 2
  ) {

    noTone(
      BUZZER_PIN
    );

    return;
  }


  // ====================================================
  // LEVEL 3
  // ====================================================

  if (
    level == 3
  ) {

    beepPattern(
      now,
      2000,
      200,
      1200
    );

    return;
  }


  // ====================================================
  // LEVEL 4
  // ====================================================

  if (
    level == 4
  ) {

    beepPattern(
      now,
      1200,
      250,
      1500
    );

    return;
  }


  // ====================================================
  // LEVEL 5
  // ====================================================

  if (
    level == 5
  ) {

    beepPattern(
      now,
      700,
      300,
      1800
    );

    return;
  }


  // ====================================================
  // LEVEL 6 - 7
  // ====================================================

  if (
    level == 6
    ||
    level == 7
  ) {

    beepPattern(
      now,
      500,
      300,
      2200
    );

    return;
  }


  // ====================================================
  // LEVEL 8
  // ====================================================

  if (
    level == 8
  ) {

    beepPattern(
      now,
      300,
      200,
      2600
    );

    return;
  }


  // ====================================================
  // LEVEL 9
  // ====================================================

  tone(
    BUZZER_PIN,
    3000
  );
}


// ======================================================
// NON BLOCKING BUZZER PATTERN
// ======================================================

void beepPattern(
  unsigned long now,
  unsigned long interval,
  unsigned long beepDuration,
  int frequency
) {

  unsigned long cycle =
    now
    %
    interval;


  if (
    cycle < beepDuration
  ) {

    tone(
      BUZZER_PIN,
      frequency
    );

  }

  else {

    noTone(
      BUZZER_PIN
    );
  }
}


// ======================================================
// OLED
// ======================================================

void updateOLED(
  float ppm,
  float adc,
  H2SStatus status
) {

  display.clearDisplay();


  // ====================================================
  // HEADER
  // ====================================================

  display.setTextSize(1);


  display.setCursor(
    0,
    0
  );


  display.println(
    "SMART H2S MONITOR"
  );


  display.drawLine(
    0,
    9,
    127,
    9,
    SSD1306_WHITE
  );


  // ====================================================
  // PPM
  // ====================================================

  display.setTextSize(2);


  display.setCursor(
    0,
    14
  );


  display.print(
    ppm,
    1
  );


  display.setTextSize(1);


  display.print(
    " ppm"
  );


  // ====================================================
  // STATUS
  // ====================================================

  display.setCursor(
    0,
    35
  );


  display.print(
    "STATUS: "
  );


  display.println(
    status.title
  );


  // ====================================================
  // DESCRIPTION
  // ====================================================

  display.setCursor(
    0,
    47
  );


  display.println(
    status.oledLine1
  );


  display.setCursor(
    0,
    56
  );


  display.println(
    status.oledLine2
  );


  display.display();
}


// ======================================================
// SERIAL MONITOR
// ======================================================

void printSerial(
  int rawADC,
  float filteredADC,
  float ppm,
  H2SStatus status
) {

  Serial.println();


  Serial.println(
    "----------------------------------------"
  );


  Serial.print(
    "Raw ADC       : "
  );

  Serial.println(
    rawADC
  );


  Serial.print(
    "Filtered ADC  : "
  );

  Serial.println(
    filteredADC,
    1
  );


  Serial.print(
    "H2S           : "
  );

  Serial.print(
    ppm,
    2
  );

  Serial.println(
    " ppm"
  );


  Serial.print(
    "Level         : "
  );

  Serial.println(
    status.level
  );


  Serial.print(
    "Status        : "
  );

  Serial.println(
    status.title
  );


  Serial.print(
    "Efek          : "
  );

  Serial.println(
    status.description
  );


  Serial.print(
    "WiFi          : "
  );

  Serial.println(
    WiFi.status()
      == WL_CONNECTED
      ? "ONLINE"
      : "OFFLINE"
  );


  Serial.print(
    "MQTT          : "
  );

  Serial.println(
    mqttClient.connected()
      ? "ONLINE"
      : "OFFLINE"
  );


  Serial.println(
    "----------------------------------------"
  );
}