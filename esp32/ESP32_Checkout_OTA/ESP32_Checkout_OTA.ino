#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "secrets.h"

// ===============================
// LCD
// ===============================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===============================
// Hardware Pins
// ===============================
#define BUZZER_PIN 25
#define GREEN_LED  26
#define RED_LED    27

// ===============================
// OTA
// ===============================
const char* otaHostname = "ESP32-Checkout-01";

// ===============================
// ThingsBoard
// ===============================
const char* mqttServer = "mqtt.eu.thingsboard.cloud";
const int mqttPort = 8883;

const char* telemetryTopic =
  "v1/devices/me/telemetry";

const char* rpcTopic =
  "v1/devices/me/rpc/request/+";

// ===============================
// TLS Certificate
// ===============================
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEMjCCAxqgAwIBAgIBATANBgkqhkiG9w0BAQUFADB7MQswCQYDVQQGEwJHQjEb
MBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQHDAdTYWxmb3JkMRow
GAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UEAwwYQUFBIENlcnRpZmlj
YXRlIFNlcnZpY2VzMB4XDTA0MDEwMTAwMDAwMFoXDTI4MTIzMTIzNTk1OVowezEL
MAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UE
BwwHU2FsZm9yZDEaMBgGA1UECgwRQ29tb2RvIENBIExpbWl0ZWQxITAfBgNVBAMM
GEFBQSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczCCASIwDQYJKoZIhvcNAQEBBQADggEP
ADCCAQoCggEBAL5AnfRu4ep2hxxNRUSOvkbIgwadwSr+GB+O5AL686tdUIoWMQua
BtDFcCLNSS1UY8y2bmhGC1Pqy0wkwLxyTurxFa70VJoSCsN6sjNg4tqJVfMiWPPe
3M/vg4aijJRPn2jymJBGhCfHdr/jzDUsi14HZGWCwEiwqJH5YZ92IFCokcdmtet4
YgNW8IoaE+oxox6gmf049vYnMlhvB/VruPsUK6+3qszWY19zjNoFmag4qMsXeDZR
rOme9Hg6jc8P2ULimAyrL58OAd7vn5lJ8S3frHRNG5i1R8XlKdH5kBjHYpy+g8cm
ez6KJcfA3Z3mNWgQIJ2P2N7Sw4ScDV7oL8kCAwEAAaOBwDCBvTAdBgNVHQ4EFgQU
oBEKIz6W8Qfs4q8p74Klf9AwpLQwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQF
MAMBAf8wewYDVR0fBHQwcjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5jb20v
QUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNqA0oDKGMGh0dHA6Ly9jcmwuY29t
b2RvLm5ldC9BQUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDANBgkqhkiG9w0BAQUF
AAOCAQEACFb8AvCb6P+k+tZ7xkSAzk/ExfYAWMymtrwUSWgEdujm7l3sAg9g1o1Q
GE8mTgHj5rCl7r+8dFRBv/38ErjHT1r0iWAFf2C3BUrz9vHCv8S5dIa2LX1rzNLz
Rt0vxuBqw8M0Ayx9lt1awg6nCpnBBYurDC/zXDrPbDdVCYfeU0BsWO/8tqtlbgT2
G9w84FoVxp7Z8VlIMCFlA2zs6SFz7JsDoeA3raAVGI/6ugLOpyypEBMs1OUIJqsi
l2D4kF501KKaU73yqWjgom7C12yxow+ev+to51byrvLjKzg6CYG1a4XXvi3tPxq3
smPi9WIsgtRqAEFQ8TmDn5XpNpaYbg==
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// ===============================
// Retry Timers
// ===============================
unsigned long lastMqttAttempt = 0;
unsigned long lastTelemetry = 0;
unsigned long lastTimeSyncAttempt = 0;

const unsigned long MQTT_RETRY_INTERVAL = 5000;
const unsigned long TELEMETRY_INTERVAL = 10000;
const unsigned long TIME_SYNC_RETRY_INTERVAL = 60000;

// ===============================
// SUCCESS
// ===============================
void showSuccess() {

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Product Found");

  lcd.setCursor(0, 1);
  lcd.print("SUCCESS");

  // صفارة قصيرة
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  digitalWrite(BUZZER_PIN, HIGH);
}

// ===============================
// REJECT
// ===============================
void showReject() {

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Unknown Product");

  lcd.setCursor(0, 1);
  lcd.print("REJECT");

  // صفارة أطول
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
  digitalWrite(BUZZER_PIN, HIGH);
}

// ===============================
// MQTT RPC Callback
// ===============================
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println();
  Serial.println("RPC RECEIVED");

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  Serial.println(message);

  JsonDocument doc;

  DeserializationError error =
    deserializeJson(doc, message);

  if (error) {
    Serial.println("JSON error");
    return;
  }

  String method = doc["method"] | "";
  String command = doc["params"] | "";

  Serial.print("Method: ");
  Serial.println(method);

  Serial.print("Command: ");
  Serial.println(command);

  bool success = false;

  if (method == "setStatus") {

    if (command == "SUCCESS") {

      showSuccess();
      success = true;

    } else if (command == "REJECT") {

      showReject();
      success = true;
    }
  }

  // استخراج Request ID من الـ Topic
  String topicString = String(topic);

  String requestId =
    topicString.substring(
      topicString.lastIndexOf('/') + 1
    );

  String responseTopic =
    "v1/devices/me/rpc/response/" +
    requestId;

  String response;

  if (success) {

    response =
      "{\"ack\":true,\"status\":\"executed\"}";

  } else {

    response =
      "{\"ack\":false,\"status\":\"unknown_command\"}";
  }

  mqttClient.publish(
    responseTopic.c_str(),
    response.c_str()
  );

  Serial.print("ACK sent: ");
  Serial.println(response);
}

// ===============================
// Wi-Fi
// ===============================
void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP: ");
  Serial.println(
    WiFi.localIP()
  );

  // ===============================
  // Energy Management
  // ===============================
  // تفعيل Wi-Fi Modem Sleep لتقليل استهلاك الطاقة
  // مع إبقاء الجهاز متصلًا بالشبكة.
  WiFi.setSleep(true);

  Serial.println(
    "WiFi Power Saving: ENABLED"
  );
}

// ===============================
// Time Sync
// ===============================
bool syncTime() {

  Serial.println(
    "Synchronizing time..."
  );

  configTime(
    0,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );

  unsigned long start =
    millis();

  while (
    millis() - start < 15000
  ) {

    time_t now =
      time(nullptr);

    if (now >= 100000) {

      Serial.println(
        "Time synchronized!"
      );

      return true;
    }

    // إبقاء OTA فعالًا أثناء انتظار الوقت
    ArduinoOTA.handle();

    delay(250);
  }

  Serial.println(
    "Time sync timeout - will retry later."
  );

  return false;
}

// ===============================
// OTA
// ===============================
void setupOTA() {

  ArduinoOTA.setHostname(
    otaHostname
  );

  // كلمة المرور موجودة في secrets.h
  ArduinoOTA.setPassword(
    otaPassword
  );

  ArduinoOTA.onStart([]() {

    Serial.println();
    Serial.println(
      "OTA Update Started"
    );

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(
      "OTA Updating"
    );

    lcd.setCursor(0, 1);
    lcd.print(
      "Please wait..."
    );

    digitalWrite(
      GREEN_LED,
      LOW
    );

    digitalWrite(
      RED_LED,
      LOW
    );
  });

  ArduinoOTA.onEnd([]() {

    Serial.println();

    Serial.println(
      "OTA Update Finished"
    );

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(
      "OTA Complete"
    );

    lcd.setCursor(0, 1);
    lcd.print(
      "Restarting..."
    );
  });

  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {

      unsigned int percent =
        (total > 0)
          ? ((progress * 100U) / total)
          : 0;

      Serial.printf(
        "OTA Progress: %u%%\r",
        percent
      );

      lcd.setCursor(0, 1);

      lcd.print(
        "Progress: "
      );

      lcd.print(percent);

      lcd.print(
        "%   "
      );
    }
  );

  ArduinoOTA.onError(
    [](ota_error_t error) {

      Serial.printf(
        "OTA Error[%u]: ",
        error
      );

      if (
        error == OTA_AUTH_ERROR
      ) {

        Serial.println(
          "Auth Failed"
        );

      } else if (
        error == OTA_BEGIN_ERROR
      ) {

        Serial.println(
          "Begin Failed"
        );

      } else if (
        error == OTA_CONNECT_ERROR
      ) {

        Serial.println(
          "Connect Failed"
        );

      } else if (
        error == OTA_RECEIVE_ERROR
      ) {

        Serial.println(
          "Receive Failed"
        );

      } else if (
        error == OTA_END_ERROR
      ) {

        Serial.println(
          "End Failed"
        );
      }

      lcd.clear();

      lcd.setCursor(0, 0);

      lcd.print(
        "OTA Error"
      );
    }
  );

  ArduinoOTA.begin();

  Serial.println(
    "OTA Ready"
  );

  Serial.print(
    "OTA Hostname: "
  );

  Serial.println(
    otaHostname
  );

  Serial.print(
    "OTA IP: "
  );

  Serial.println(
    WiFi.localIP()
  );
}

// ===============================
// ThingsBoard Connection
// ===============================
bool connectMQTT() {

  if (
    mqttClient.connected()
  ) {
    return true;
  }

  Serial.println(
    "Connecting to ThingsBoard..."
  );

  if (
    mqttClient.connect(
      "ESP32-Checkout-01",
      accessToken,
      nullptr
    )
  ) {

    Serial.println(
      "ThingsBoard Connected!"
    );

    mqttClient.subscribe(
      rpcTopic
    );

    Serial.println(
      "Subscribed to RPC commands!"
    );

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(
      "Cloud Connected"
    );

    lcd.setCursor(0, 1);
    lcd.print(
      "ESP32 Online"
    );

    return true;

  } else {

    Serial.print(
      "MQTT failed, state = "
    );

    Serial.println(
      mqttClient.state()
    );

    return false;
  }
}

// ===============================
// Telemetry
// ===============================
void sendTelemetry() {

  if (
    !mqttClient.connected()
  ) {
    return;
  }

  String payload = "{";

  payload +=
    "\"esp32_status\":\"online\",";

  payload +=
    "\"wifi_rssi\":" +
    String(WiFi.RSSI());

  payload += "}";

  mqttClient.publish(
    telemetryTopic,
    payload.c_str()
  );

  Serial.print(
    "Telemetry: "
  );

  Serial.println(
    payload
  );
}

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(115200);

  // I2C
  // SDA = GPIO21
  // SCL = GPIO22
  Wire.begin(
    21,
    22
  );

  // LCD
  lcd.init();
  lcd.backlight();

  // المخارج
  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  pinMode(
    RED_LED,
    OUTPUT
  );

  // البزر Active LOW
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    RED_LED,
    LOW
  );

  // ===============================
  // Wi-Fi
  // ===============================
  connectWiFi();

  // ===============================
  // OTA
  // ===============================
  setupOTA();

  Serial.println(
    "Firmware Version: OTA-V2"
  );

  // ===============================
  // NTP Time
  // ===============================
  // مزامنة الوقت مطلوبة
  // للتحقق من شهادة TLS
  syncTime();

  // ===============================
  // TLS
  // ===============================
  secureClient.setCACert(
    root_ca
  );

  // ===============================
  // MQTT
  // ===============================
  mqttClient.setServer(
    mqttServer,
    mqttPort
  );

  mqttClient.setCallback(
    mqttCallback
  );

  connectMQTT();

  // إرسال أول Telemetry
  sendTelemetry();
}

// ===============================
// LOOP
// ===============================
void loop() {

  // ===============================
  // OTA
  // ===============================
  // ضروري لاستقبال التحديثات اللاسلكية
  ArduinoOTA.handle();

  // ===============================
  // Wi-Fi Reconnect
  // ===============================
  if (
    WiFi.status()
      != WL_CONNECTED
  ) {

    connectWiFi();

    // بعد رجوع Wi-Fi
    // نعيد تشغيل خدمة OTA
    setupOTA();
  }

  // ===============================
  // MQTT Reconnect
  // ===============================
  // محاولة اتصال كل 5 ثوانٍ
  // بدون Blocking Loop
  if (
    !mqttClient.connected()
  ) {

    if (
      millis() - lastMqttAttempt
        >= MQTT_RETRY_INTERVAL
    ) {

      lastMqttAttempt =
        millis();

      connectMQTT();
    }

  } else {

    mqttClient.loop();
  }

  // ===============================
  // Time Sync Retry
  // ===============================
  time_t now =
    time(nullptr);

  if (
    now < 100000 &&
    millis() - lastTimeSyncAttempt
      >= TIME_SYNC_RETRY_INTERVAL
  ) {

    lastTimeSyncAttempt =
      millis();

    syncTime();
  }

  // ===============================
  // Telemetry
  // ===============================
  // إرسال حالة ESP32 وقوة Wi-Fi
  // كل 10 ثوانٍ
  if (
    mqttClient.connected() &&
    millis() - lastTelemetry
      >= TELEMETRY_INTERVAL
  ) {

    lastTelemetry =
      millis();

    sendTelemetry();
  }
}