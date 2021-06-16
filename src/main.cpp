#include "esp_log.h"
#include "settings.h"
#include <Arduino.h>
#include "display.cpp"
#include "network.cpp"

TaskHandle_t _taskNetwork;
TaskHandle_t _taskUI;

display _myDisplay = display();
network _myNetwork = network();

const int arraySize = 60;
bool _displayToggle = false;

void TaskUI(void *pvParameters)  // This is a task.
{
    (void) pvParameters;
    log_i("TaskUI starting");

    _myDisplay.Init();

    for (;;)
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
}

void TaskNetwork(void *pvParameters)  // This is a task.
{
    (void) pvParameters;
    log_i("TaskNetwork starting");
    for (;;)
    {
        _myNetwork.Init();
        _myNetwork.RetrieveDataOverview();
        _myNetwork.RetrieveDataPower();

        delay(1000);
    }    
}

void setup() 
{
    Serial.begin(115200);

    log_d("SolarEdgeDisplay setup");

    // Network on core 0
    xTaskCreatePinnedToCore(
        TaskNetwork
        ,  "TaskNetwork"   // A name just for humans
        ,  1024 * 10  // This stack size can be checked & adjusted by reading the Stack Highwater
        ,  NULL
        ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,  &_taskNetwork 
        ,  0);

    // Task2, ui on core 1
    xTaskCreatePinnedToCore(
        TaskUI
        ,  "TaskUI"   // A name just for humans
        ,  1024 * 10  // This stack size can be checked & adjusted by reading the Stack Highwater
        ,  NULL
        ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,  &_taskUI 
        ,  1);
}

void loop() 
{
    // everything is done in tasks now
}

