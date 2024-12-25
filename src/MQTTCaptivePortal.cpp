#include <MQTTCaptivePortal.hpp>

class CaptivePortalHandler;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta name='format-detection' content='telephone=no'>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>
  <title>Captive Portal</title>
</head>
<body>
  <h1>Config MQTT settings</h1>
  <p>Enter MQTT settings and click Save.</p>
  <form action="/save" method="post">
    <label for="mqttServer">MQTT server IP:</label><br>
    <input type="text" id="mqttServer" name="mqttServer"><br><br>
    <label for="mqttPort">MQTT port 2:</label><br>
    <input type="text" id="mqttPort" name="mqttPort"><br><br>
    <input type="submit" value="Save">
  </form>
</body>
</html>
)rawliteral";

MQTTCaptivePortal::MQTTCaptivePortal(int webServerPort) : _webServer(webServerPort), _isServerStop(true), _hasServerStopped(false) // assume the portal will not be used
{
    _dnsServer = DNSServer();
}

void MQTTCaptivePortal::begin(String accessPointName, unsigned long timeout) {
    _timeoutDuration = timeout;
    _startMillis = millis();
    _isTimeoutEnabled = true;

    _isServerStop = false;
    _hasServerStopped = false;

    if (!WiFi.softAP(accessPointName)) {
        Serial.println("Failed to start AP");
        while (true) delay(100); // Dừng hệ thống nếu không khởi tạo được
    }

    if (!_dnsServer.start(53, "*", WiFi.softAPIP())) {
        Serial.println("Failed to start DNS server");
        while (true) delay(100); // Dừng hệ thống nếu không khởi tạo được
    }

    startWebServer();

    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.setTTL(300);

    _webServer.begin();

    Serial.println("Started Captive Portal...");
}

// void MQTTCaptivePortal::begin(String accessPointName)
// {
//     _isServerStop = false;
//     _hasServerStopped = false;


//     if (!WiFi.softAP(accessPointName))
//     {
//         Serial.println("Failed to start AP");
//         while (true)
//             delay(100); // Halt system
//     }

//     if (!_dnsServer.start(53, "*", WiFi.softAPIP()))
//     {
//         Serial.println("Failed to start DNS server");
//         while (true)
//             delay(100); // Halt system
//     }

//     startWebServer();

//     _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
//     _dnsServer.setTTL(300);

//     _webServer.begin();

//     Serial.println("Started Captive Portal...");
// }

void MQTTCaptivePortal::onPortalSubmit(function<void(String serverIP, int port)> callback)
{
    _portalSubmitCallback = callback;
}

void MQTTCaptivePortal::stop()
{
    _webServer.end();
    _dnsServer.stop();
    _isServerStop = true;
    _hasServerStopped = true;
}

void MQTTCaptivePortal::loop() {
    if (!_isServerStop) {
        _dnsServer.processNextRequest();

        // Kiểm tra timeout
        if (_isTimeoutEnabled && (millis() - _startMillis >= _timeoutDuration)) {
            Serial.println("Timeout reached. Stopping Captive Portal...");
            stop();
        }
    } else {
        if (!_hasServerStopped) {
            stop();
        }
    }
}

void MQTTCaptivePortal::resetTimeout() {
    _startMillis = millis();
    Serial.println("Timeout reset!");
}

// void MQTTCaptivePortal::loop()
// {
//     //the logic could be weird, but it works fine
//     if (!_isServerStop)
//     {
//         _dnsServer.processNextRequest();
//     }
//     else
//     {
//         if(!_hasServerStopped){// stop one time only
//             this->stop();
//         }
//     }
// }

// void MQTTCaptivePortal::startWebServer()
// {
//     static CaptivePortalHandler captivePortalHandler;
//     _webServer.addHandler(&captivePortalHandler).setFilter(ON_AP_FILTER);

//     // Handle form submission
//     _webServer.on("/save", HTTP_POST, [&](AsyncWebServerRequest *request)
//                   {
//     String mqttServer = "";
//     String mqttPort = "";

//     // Extract values from the POST request
//     if (request->hasParam("mqttServer", true)) {
//       mqttServer = request->getParam("mqttServer", true)->value();
//     }
//     if (request->hasParam("mqttPort", true)) {
//       mqttPort = request->getParam("mqttPort", true)->value();
//     }

//     if(_portalSubmitCallback != NULL){
//         int port = mqttPort.toInt();
//         if (port == 0 && mqttPort != "0") //invalid port number
//         {
//             port = 1883;
//         }
//         _portalSubmitCallback(mqttServer, port);
//     }

//     // Send a response back to the client
//     request->send(200, "text/plain", 
//         "Data saved successfully! MQTT server: " + mqttServer + ", MQTT port: " + mqttPort + 
//         ". Feel free to close this window");
//         this->stop();
//     });

//     _webServer.onNotFound([](AsyncWebServerRequest *request)
//                           { request->send(200, "text/html", index_html); });
// }

void MQTTCaptivePortal::startWebServer() {
    static CaptivePortalHandler captivePortalHandler;
    _webServer.addHandler(&captivePortalHandler).setFilter(ON_AP_FILTER);

    // Đặt lại timeout khi truy cập Captive Portal
    _webServer.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        resetTimeout();
        request->send(200, "text/html", index_html);
    });

    // Xử lý form submission
    _webServer.on("/save", HTTP_POST, [&](AsyncWebServerRequest *request) {
        resetTimeout(); // Đặt lại timeout khi có POST

        String mqttServer = "";
        String mqttPort = "";

        if (request->hasParam("mqttServer", true)) {
            mqttServer = request->getParam("mqttServer", true)->value();
        }
        if (request->hasParam("mqttPort", true)) {
            mqttPort = request->getParam("mqttPort", true)->value();
        }

        if (_portalSubmitCallback != NULL) {
            int port = mqttPort.toInt();
            if (port == 0 && mqttPort != "0") port = 1883; // Cổng mặc định
            _portalSubmitCallback(mqttServer, port);
        }

        request->send(200, "text/plain",
                      "Data saved successfully! MQTT server: " + mqttServer + ", MQTT port: " + mqttPort +
                          ". Feel free to close this window");
        this->stop(); // Dừng server sau khi lưu cấu hình
    });

    _webServer.onNotFound([&](AsyncWebServerRequest *request) {
        resetTimeout();
        request->send(200, "text/html", index_html);
    });
}


class CaptivePortalHandler : public AsyncWebHandler
{
public:
    CaptivePortalHandler() {}
    virtual ~CaptivePortalHandler() {}

    bool canHandle(AsyncWebServerRequest *request) override
    {
        return request->url() == "/";
    }

    void handleRequest(AsyncWebServerRequest *request) override
    {
        request->send(200, "text/html", index_html);
    }
};