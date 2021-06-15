#include "esp_log.h"
#include "settings.h"
#include <Arduino.h>
#include "display.cpp"
#include "network.cpp"

display _myDisplay = display();
network _myNetwork = network();

const int arraySize = 60;
bool _displayToggle = false;

void setup() {
    Serial.begin(115200);

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_DEBUG);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client
    esp_log_level_set("network", ESP_LOG_VERBOSE); 
    esp_log_level_set("display", ESP_LOG_INFO); 

    log_d("SolarEdgeDisplay setup");

    _myDisplay.Init();

    _myNetwork.Init();
}

void loop() 
{
    char chartTitle[32];

    struct SolarEdgeOverview overview = _myNetwork.GetDataOverview();
    struct SolarEdgePower power = _myNetwork.GetDataPower();

    if (_displayToggle)
        sprintf_P(chartTitle, PSTR("P=%d"), overview.CurrentPower);
    else
        sprintf_P(chartTitle, PSTR("E=%d"), overview.LastDayDataEnergy);
    _displayToggle = !_displayToggle;

    _myDisplay.DrawChartAutoScaled(chartTitle, power.Values);

    delay(1000);
}
