#include <Arduino.h>
#include "display.cpp"
#include "network.cpp"

display _myDisplay = display();
network _myNetwork = network();

const int arraySize = 60;

void setup() {
    Serial.begin(115200);
    Serial.println("SolarEdgeDisplay->setup()");

    randomSeed(analogRead(3));

    _myDisplay.Init();

    _myNetwork.Init();
}

void loop() {
    Serial.println("SolarEdgeDisplay->loop()");

    char chartTitle[32];
    sprintf_P(chartTitle, PSTR("T=%lu"), millis());

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
    delay(250);
}
