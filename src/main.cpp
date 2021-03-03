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
    Serial.println("SolarEdgeDisplay->setup()");

    randomSeed(analogRead(3));

    _myDisplay.Init();

    _myNetwork.Init();

    _myNetwork.GetDataOverview();
    _myNetwork.GetDataPower();
}

void loop() 
{
    Serial.println("SolarEdgeDisplay->loop()");

    char chartTitle[32];

    struct SolarEdgeOverview overview = _myNetwork.GetDataOverview();
    if (_displayToggle)
        sprintf_P(chartTitle, PSTR("P=%d"), overview.CurrentPower);
    else
        sprintf_P(chartTitle, PSTR("E=%d"), overview.LastDayDataEnergy);
    _displayToggle = !_displayToggle;

    // demo-chart
    byte myValues[arraySize];
    for (int i = 0; i < arraySize; i++)
    {
        if (i < 10)       myValues[i] = (byte)random(0, 10);
        else if (i < 20)  myValues[i] = (byte)random(10, 20);
        else if (i < 30)  myValues[i] = (byte)random(20, 30);
        else if (i < 40)  myValues[i] = (byte)random(30, 40);
        else if (i < 50)  myValues[i] = (byte)random(10, 20);
        else              myValues[i] = (byte)random(0, 10);        
    }
    _myDisplay.DrawChart(chartTitle, myValues);
    delay(1000);
}
