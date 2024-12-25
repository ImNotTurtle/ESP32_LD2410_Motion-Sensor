#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <vector>

class MQTT {
  public:
    // Constructor để khởi tạo đối tượng MQTT
    MQTT();

    void setServer(String domain, uint16_t port);

    // Hàm kết nối tới MQTT broker
    bool connect(const char* clientID);
    bool connected();
    void reconnect();

    // Hàm gửi dữ liệu tới một topic
    bool publish(const char* topic, const char* message);
    void publishBuffer(const char* topic, const uint8_t* payload, uint16_t length);

    void subscribe(const char* topic);

    
    // Hàm nhận dữ liệu và xử lý callback
    void setCallback(void (*callback)(char*, byte*, unsigned int));
    
    // Hàm vòng lặp để duy trì kết nối MQTT
    void loop();

  private:
    // const char* _mqtt_server;
    // int _mqtt_port;
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    String _mqttClientId;
    String _mqttServer;
    uint16_t _mqttPort;
    unsigned int _connectTryCount;

    std::vector<String> subscribedTopics;
};

#endif
