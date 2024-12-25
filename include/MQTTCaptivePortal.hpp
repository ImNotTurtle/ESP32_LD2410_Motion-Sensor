#ifndef MQTT_CAPTIVE_PORTAL_HPP
#define MQTT_CAPTIVE_PORTAL_HPP
#include <functional>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
using namespace std;


extern const char index_html[];


class MQTTCaptivePortal{
public:
    MQTTCaptivePortal(int webServerPort);


    void begin(String accessPointName, unsigned long timeout = 180);
    void onPortalSubmit(function<void(String serverIP, int port)> callback);

    void stop();

    void loop();
    void resetTimeout();

private:
    AsyncWebServer _webServer;
    DNSServer _dnsServer;
    bool _isServerStop;
    bool _hasServerStopped;
    unsigned long _timeoutDuration; // Thời gian timeout (milliseconds)
    unsigned long _startMillis;     // Thời điểm bắt đầu chạy server
    bool _isTimeoutEnabled;         // Cờ bật/tắt timeout
    function<void(String, int)> _portalSubmitCallback;

    void startWebServer();
};



#endif