#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "settings.h"
#include <ArduinoJson.h>
#include <time.h>

struct SolarEdgeOverview
{
    unsigned long RequestTime;
    time_t LastUpdateTime;
    unsigned int LastDayDataEnergy;
    unsigned int CurrentPower;
};

struct SolarEdgePower
{
    unsigned long RequestTime;
    unsigned int Values[60]{};
};

class network {
    private:
        WiFiMulti _WiFiMulti;
        SolarEdgeOverview _solarEdgeOverview;
        SolarEdgePower _solarEdgePower;

        // SolarEdge uses the DigiCert Global Root CA
        // This is the BASE64-exported X.509 version
        const char* DigiCertRootCertificate = \
            "-----BEGIN CERTIFICATE-----\n"\
            "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"\
            "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"\
            "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"\
            "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"\
            "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"\
            "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"\
            "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"\
            "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"\
            "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"\
            "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"\
            "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"\
            "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"\
            "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"\
            "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"\
            "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"\
            "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"\
            "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"\
            "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"\
            "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"\
            "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"\
            "-----END CERTIFICATE-----\n";

        // Not sure if WiFiClientSecure checks the validity date of the certificate. 
        // Setting clock just to be sure...
        void SetClock() {
            configTime(TIMEZONE_OFFSET, 0, "pool.ntp.org", "time.nist.gov");

            Serial.print(F("Waiting for NTP time sync: "));
            time_t nowSecs = time(nullptr);
            while (nowSecs < 8 * 3600 * 2) {
                delay(500);
                Serial.print(F("."));
                yield();
                nowSecs = time(nullptr);
            }

            Serial.println();
            struct tm timeinfo;
            gmtime_r(&nowSecs, &timeinfo);
            Serial.print(F("Current time: "));
            Serial.print(asctime(&timeinfo));
        }

        String GetData(String url)
        {
            String result = "";
            WiFiClientSecure *client = new WiFiClientSecure;
            if (client) 
            {
                client->setCACert(DigiCertRootCertificate);
                {
                    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
                    HTTPClient https;

                    Serial.print("[HTTPS] begin...\n");
                    Serial.println(url);
                    if (https.begin(*client, url))
                    {  // HTTPS
                        Serial.print("[HTTPS] GET...\n");
                        // start connection and send HTTP header
                        int httpCode = https.GET();

                        // httpCode will be negative on error
                        if (httpCode > 0)
                        {
                            // HTTP header has been send and Server response header has been handled
                            Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

                            // file found at server
                            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                            {
                                result = https.getString();
                                Serial.println(result);
                            }
                        }
                        else
                        {
                            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
                        }
                        https.end();
                    }
                    else
                    {
                        Serial.printf("[HTTPS] Unable to connect\n");
                    }
                }

                delete client;
            }
            else
            {
                Serial.println("Unable to create client");
            }
            return result;
        }

        struct tm Now(void)
        {
            struct tm timeinfo;
            if(getLocalTime(&timeinfo))
            {
                Serial.print(asctime(&timeinfo));
            }         
            else
            {
                Serial.println("Failed to obtain time");
            }
            return timeinfo;   
        }

        time_t ParseDateTime(const char* jsonData)
        {
            struct tm tm;
            memset(&tm, 0, sizeof(struct tm));
            strptime(jsonData, "%Y-%m-%d %H:%M:%S", &tm);
            
            Serial.print("ParseDateTime:");
            Serial.print(asctime(&tm));            
            return mktime(&tm);
        }

    public:
        void Init(void)
        {
            Serial.print("Attempting to connect to SSID: ");
            Serial.println(WIFI_SSID);
            WiFi.mode(WIFI_STA);
            _WiFiMulti.addAP(WIFI_SSID, WIFI_PWD);

            // attempt to connect to Wifi network:
            while ((_WiFiMulti.run() != WL_CONNECTED)) 
            {
                Serial.print(".");
                // wait 1 second for re-trying
                delay(50);
            }

            Serial.print("\nConnected to ");
            Serial.println(WIFI_SSID);
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP()); 

            SetClock();  

            _solarEdgeOverview.RequestTime = 0;
            _solarEdgeOverview.LastUpdateTime = 0; // epoch
            _solarEdgeOverview.CurrentPower = 0;
            _solarEdgeOverview.LastDayDataEnergy = 0;
            
            _solarEdgePower.RequestTime = 0;
        };

        SolarEdgeOverview GetDataOverview(void)
        {
            //https://monitoringapi.solaredge.com/site/{{SITE_ID}}/overview?api_key={{API_KEY}}
            // {
            //     "overview": {
            //         "lastUpdateTime": "2021-03-03 19:44:54",
            //         "lifeTimeData": {
            //             "energy": 2105148.0,
            //             "revenue": 435.4053
            //         },
            //         "lastYearData": {
            //             "energy": 359081.0
            //         },
            //         "lastMonthData": {
            //             "energy": 30520.0
            //         },
            //         "lastDayData": {
            //             "energy": 2380.0
            //         },
            //         "currentPower": {
            //             "power": 0.0
            //         },
            //         "measuredBy": "INVERTER"
            //     }
            // }

            // Throttling is required, max 300 requests per day per SiteID
            if (_solarEdgeOverview.RequestTime != 0 &&
                _solarEdgeOverview.RequestTime + (15 * 60 * 1000) > millis()) 
            {
                // return buffered result for 15 minutes.
                return _solarEdgeOverview;
            }
            _solarEdgeOverview.RequestTime = millis();

            String url = "https://monitoringapi.solaredge.com/site/";
            url.concat(SOLAREDGE_SITEID);
            url.concat("/overview?api_key=");
            url.concat(SOLAREDGE_APIKEY);
       
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, GetData(url));

            _solarEdgeOverview.LastUpdateTime = ParseDateTime(doc["overview"]["lastUpdateTime"]);
            _solarEdgeOverview.CurrentPower = doc["overview"]["currentPower"]["power"];
            _solarEdgeOverview.LastDayDataEnergy = doc["overview"]["lastDayData"]["energy"];

            return _solarEdgeOverview;
        }

        SolarEdgePower GetDataPower(void)
        {
            // https://monitoringapi.solaredge.com/site/{{SITE_ID}}/power?startTime={{STATS_CURRENTDAY}} 06:00:00&endTime={{STATS_CURRENTDAY}} 20:45:00&api_key={{API_KEY}}
            // {
            //     "power": {
            //         "timeUnit": "QUARTER_OF_AN_HOUR",
            //         "unit": "W",
            //         "measuredBy": "INVERTER",
            //         "values": [ 
            //             {
            //                 "date": "2021-03-04 06:00:00",
            //                 "value": null
            //             }, --> 06:00:00 - 20:45:00 = 60 datapoints


            // Throttling is required, max 300 requests per day per SiteID
            if (_solarEdgePower.RequestTime != 0 &&
                _solarEdgePower.RequestTime + (15 * 60 * 1000) > millis()) 
            {
                // return buffered result for 15 sec.
                return _solarEdgePower;
            }
            _solarEdgePower.RequestTime = millis();

            struct tm now = Now();
            char currentDate[11]; // last char is NULL terminator
            strftime(currentDate, 11, "%Y-%m-%d", &now);

            String url = "https://monitoringapi.solaredge.com/site/";
            url.concat(SOLAREDGE_SITEID);
            url.concat("/power?startTime=");
            url.concat(currentDate);
            url.concat("%2006:00:00&endTime=");
            url.concat(currentDate);
            url.concat("%2021:00:00&api_key=");
            url.concat(SOLAREDGE_APIKEY);
            
            DynamicJsonDocument doc(7000);
            deserializeJson(doc, GetData(url));

            byte index = 0;
            JsonArray arr = doc["power"]["values"].as<JsonArray>();
            for (JsonVariant value : arr)
            {
                _solarEdgePower.Values[index] = (unsigned int)(value["value"].as<float>());
                index++;
            }
            return _solarEdgePower;
        }
};
