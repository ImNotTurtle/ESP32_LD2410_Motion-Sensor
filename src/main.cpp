#include <Arduino.h>
#include <SerialHandle.hpp>
#include <MQTT.hpp>
#include <WiFiManager.h>
#include <MQTTCaptivePortal.hpp>

static void ld2410_AckReceiveHandle(COMMAND_TYPE type, uint8_t *buffer, uint16_t length);
void setupWifiCredentials();

int sendCount = 0;
WiFiManager wifiManager;
const char *mqttTopicConfig = "ld2410/config";
Preferences preferences;
String mqttServer;
int mqttPort;
MQTTCaptivePortal mqttPortal(80);
MQTT mqttClient;
bool useMqtt = false;

// Hàm callback khi nhận dữ liệu
void MQTTMessageReceiveHandle(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Nhận dữ liệu từ topic ");
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.printf("%.2x ", payload[i]);
  }
  Serial.println();

  // handle the MQTT message
  if (strcmp(topic, mqttTopicConfig) == 0)
  {
    SerialHandle_HandleConfig(payload, length);
  }
}

void sendDataTask(void *pvParameters)
{
  vTaskDelay(pdMS_TO_TICKS(5000));

  int retryCount = 0;
  const int maxRetries = 5;

  while (!mqttClient.connected() && retryCount < maxRetries)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    retryCount++;
  }

  if (!mqttClient.connected())
  {
    Serial.println("Failed to connect MQTT. Exiting task...");
    vTaskDelete(NULL);
    return;
  }

  while (sendCount < 5)
  {
    sendCount++;
    String message = "Dữ liệu từ ESP32 lần " + String(sendCount);
    if (!mqttClient.publish(mqttTopicConfig, message.c_str()))
    {
      sendCount--;
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }

  Serial.println("Kết thúc task");
  vTaskDelete(NULL);
}

void saveMQTTSettings(String server, int port)
{
  mqttServer = server;
  mqttPort = port;
  preferences.begin("mqtt", false); // Open preferences in RW mode
  preferences.putString("server", server);
  preferences.putInt("port", port);
  preferences.end();
  Serial.println("Saved MQTT settings:");
  Serial.println("\tServer: " + mqttServer);
  Serial.println("\tPort: " + String(mqttPort));
}

void resetMQTTSettings()
{
  saveMQTTSettings("", 0);
}

void loadMQTTSettings()
{
  preferences.begin("mqtt", true); // Open preferences in read-only mode
  mqttServer = preferences.getString("server", "");
  mqttPort = preferences.getInt("port", 1883);
  preferences.end();
  Serial.println("Loaded MQTT settings:");
  Serial.println("\tServer: " + mqttServer);
  Serial.println("\tPort: " + String(mqttPort));
}

void onMQTTPortalSubmit(String server, int port)
{
  if (server.isEmpty() || port <= 0 || port > 65535)
  {
    Serial.println("Invalid MQTT settings. Ignoring...");
    return;
  }
  saveMQTTSettings(server, port);
}

void resetSetting()
{
  // 1 second when device is powered on is reset time
  // if the IO2 is high after 1 second, then some saved setting will be erased
  unsigned long start = millis();
  bool credentialsReset = false;
  Serial.println("Wait 1s for reset signal...");
  while (millis() - start <= 1000)
    ;
  if (digitalRead(2) == HIGH)
  {
    Serial.println("Reset signal confirm");
    credentialsReset = true;
  }
  else
  {
    Serial.println("No reset");
  }

  if (credentialsReset)
  {
    wifiManager.resetSettings();
    // reset mqtt server
    resetMQTTSettings();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(2, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLUP);

  resetSetting();

  setupWifiCredentials();

  loadMQTTSettings();

  // if (mqttServer.isEmpty()) // no saved mqtt settings
  // {
  //   mqttPortal.onPortalSubmit(onMQTTPortalSubmit);
  //   mqttPortal.begin("LD2410-MQTTConfig", 180);
  // }

  if (mqttServer.isEmpty())
  {
    mqttPortal.onPortalSubmit(onMQTTPortalSubmit);
    mqttPortal.begin("LD2410-MQTTConfig", 180);

    Serial.println("Waiting for MQTT configuration...");
    // block execution until MQTT setting finishes
    while (mqttServer.isEmpty())
    {
      mqttPortal.loop();
      delay(100);
    }
  }

  SerialHandle_Init(ld2410_AckReceiveHandle);

  // Kết nối tới MQTT
  Serial.println("Setup MQTT");
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(MQTTMessageReceiveHandle);

  if (mqttClient.connect("LD2410"))
  {
    useMqtt = true;
    mqttClient.subscribe(mqttTopicConfig);
    Serial.println("Connected to MQTT server");
  }
  else
  {
    useMqtt = false;
    Serial.println("Can't connect to MQTT server");
  }

  // Tạo task gửi dữ liệu mỗi 5 giây
  // xTaskCreate(sendDataTask, "SendDataTask", 2048, NULL, 1, NULL);

  delay(2000); // Đợi một chút để MQTT kết nối hoàn tất
}
void loop()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  if (useMqtt)
  {
    if (mqttClient.connected() == false)
      mqttClient.reconnect();
    else
      mqttClient.loop();
  }

  mqttPortal.loop();
  delay(1);
}

static void ld2410_AckReceiveHandle(COMMAND_TYPE type, uint8_t *buffer, uint16_t length)
{
  switch (type)
  {
  case COMMAND_TYPE::DISTANCE_DURATION:
  {
    Serial.println("ACK DISTANCE RECEIVED");
    break;
  }
  case COMMAND_TYPE::READ_PARAMS:
  {
    Serial.println("ACK READ PARAM RECEIVED");
    Serial.printf("distance: %.2x\n", buffer[11]);
    Serial.printf("duration: %.2x\n", buffer[32]);
    Serial.printf("sensitivity: %.2x\n", buffer[23]);
    break;
  }
  case COMMAND_TYPE::SENSITIVITY:
  {
    Serial.println("ACK SENSITIVITY RECEIVED");
    break;
  }
  default:
    break;
  }
}

void setupWifiCredentials()
{
  /*
  - try to connect to pre-configured wifi credentials
  - if the wifi is not exists or the connection can not be established, the user can configure wifi via PC / Mobile
    by connect to ESP32-AP wifi -> Config wifi -> Select SSID and enter password
  */
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(180); // 3 minutes
  wifiManager.setEnableConfigPortal(true); // execute portal config when wifi connection is failed
  wifiManager.setConfigPortalTimeoutCallback(
      []
      {
        // deep sleep
        Serial.println("Deep sleep");
        esp_deep_sleep_start();
      });

  // connect to saved wifi
  wifiManager.autoConnect("LD2410-WifiManager"); // generate Access Point to change wifi config

  Serial.println("Connected to wifi: ");
  Serial.printf("\tSSID: %s\n", wifiManager.getWiFiSSID());
  Serial.printf("\tPASS: %s\n", wifiManager.getWiFiPass());
}