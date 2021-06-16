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
