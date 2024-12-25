#include "MQTT.hpp"


// Constructor để khởi tạo các thông tin MQTT
MQTT::MQTT() 
  : _mqttClient(_wifiClient), _mqttPort(0) , _mqttServer(""), _connectTryCount(0) {}


void MQTT::setServer(String domain, uint16_t port){
  _mqttServer = domain;
  _mqttPort = port;
}
// Hàm kết nối tới MQTT broker
bool MQTT::connect(const char* clientID) {
  _mqttClientId = clientID;
  _mqttClient.setKeepAlive(30); // Thiết lập keep-alive 60 giây
  if(_mqttServer.length() == 0){
    Serial.println("MQTT Server is not set");
  }
  if(_mqttPort == 0){
    Serial.println("MQTT Port is not set");
  }
  _mqttClient.setServer(_mqttServer.c_str(), _mqttPort);
  
  String id = clientID + String(WiFi.macAddress());

  while (!_mqttClient.connected()) {
    // Serial.print("Đang kết nối tới MQTT...");
    if (_mqttClient.connect(id.c_str())) {
      for(int i = 0; i < subscribedTopics.size(); i++){
        _mqttClient.subscribe(subscribedTopics[i].c_str());
      }
      // Serial.println("Thành công!");
    } else {
      Serial.print("Kết nối thất bại, mã lỗi: ");
      Serial.println(_mqttClient.state());
      _connectTryCount++;
      if(_connectTryCount >= 30){
        return false; // can't connect to MQTT server
      }
      delay(5000);
    }
  }
  // Serial.println("Đã kết nối MQTT");
  return true;
}

bool MQTT::connected(){
  return _mqttClient.connected();
}

// Hàm gửi dữ liệu tới topic
bool MQTT::publish(const char* topic, const char* message) {
  if (_mqttClient.connected()) {
    _mqttClient.publish(topic, message);
    Serial.printf("Đã gửi dữ liệu %s\n", message);
    return true;
  } else {
    Serial.println("Không thể gửi dữ liệu. MQTT chưa kết nối.");
    return false;
  }
}

void MQTT::publishBuffer(const char* topic, const uint8_t* payload, uint16_t length) {
    if (_mqttClient.connected()) {
        // Gửi dữ liệu dưới dạng mảng byte
        _mqttClient.publish(topic, payload, length);
        Serial.print("Đã gửi dữ liệu: ");
        for(int i = 0; i < length; i++){
          Serial.printf("%u ", payload[i]);
        }
        Serial.println("");
        // Serial.println("Data sent to MQTT broker");
    } else {
    Serial.println("Không thể gửi dữ liệu. MQTT chưa kết nối.");
    }
}

void MQTT::subscribe(const char* topic) {
  if (_mqttClient.connected()) {
    if (_mqttClient.subscribe(topic)) {
      subscribedTopics.push_back(topic);
      Serial.print("Đã đăng ký lắng nghe topic: ");
      Serial.println(topic);
    } else {
      Serial.print("Không thể đăng ký topic: ");
      Serial.println(topic);
    }
  } else {
    subscribedTopics.push_back(topic); // pending
    // Serial.println("MQTT chưa kết nối. Không thể đăng ký topic.");
  }
}

// Hàm set callback để xử lý dữ liệu nhận được
void MQTT::setCallback(void (*callback)(char*, byte*, unsigned int)) {
  _mqttClient.setCallback(callback);
}

// Hàm vòng lặp để duy trì kết nối MQTT
void MQTT::loop() {
  _mqttClient.loop();
}


void MQTT::reconnect() {
  if (!_mqttClient.connected()) {
    if (_mqttClientId == "") {
      // Serial.println("ClientID không hợp lệ. Không thể kết nối lại.");
      return;
    }

    // Serial.println("Đang kết nối lại tới MQTT...");
    connect(_mqttClientId.c_str()); // Sử dụng clientID đã lưu trữ
  }
}