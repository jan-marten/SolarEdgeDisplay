#include "esp_log.h"
#include <Arduino.h>
#include <WiFi.h>
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

            log_d("Waiting for NTP time sync");
            time_t nowSecs = time(nullptr);
            while (nowSecs < 8 * 3600 * 2) {
                delay(1000);
                log_v("Waiting for NTP sync");
                yield();
                nowSecs = time(nullptr);
            }

            struct tm timeinfo;
            gmtime_r(&nowSecs, &timeinfo);
            log_i("Result: %s", asctime(&timeinfo));
        }

        String GetData(String url)
        {
            String result = "";

            try
            {
                if (WiFi.status() != WL_CONNECTED)
                {
                    log_i("Not connected, going to Init now");
                    Init();
                }
                else
                {
                    WiFiClientSecure *client = new WiFiClientSecure;
                    if (client) 
                    {
                        client->setCACert(DigiCertRootCertificate);
                        {
                            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
                            HTTPClient https;

                            log_d("Begin: %s", url.c_str());
                            //log_d(url);
                            if (https.begin(*client, url))
                            {  // HTTPS
                                log_d("GET...");
                                // start connection and send HTTP header
                                int httpCode = https.GET();

                                // httpCode will be negative on error
                                if (httpCode > 0)
                                {
                                    // HTTP header has been send and Server response header has been handled
                                    log_d("httpCode: %d", httpCode);

                                    // file found at server
                                    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                                    {
                                        result = https.getString();
                                        log_d("Result: %s", result.c_str());
                                    }
                                    else 
                                    {
                                        log_w("Invalid result: %s", https.getString().c_str());
                                    }
                                }
                                else
                                {
                                    log_w("Failed, error: %s", https.errorToString(httpCode).c_str());
                                }
                                https.end();
                            }
                            else
                            {
                                log_w("Unable to connect");
                            }
                        }

                        delete client;
                    }
                    else
                    {
                        log_w("Unable to create client");
                    }
                    
                }
            }
            catch(const std::exception& e)
            {
                log_e("Exception: %s", e.what());
            }
            return result;
        }

        struct tm Now(void)
        {
            struct tm timeinfo;
            if(!getLocalTime(&timeinfo))
            {
                log_w("Failed to obtain time");
            }
            return timeinfo;   
        }

        time_t ParseDateTime(const char* jsonData)
        {
            struct tm tm;
            memset(&tm, 0, sizeof(struct tm));
            strptime(jsonData, "%Y-%m-%d %H:%M:%S", &tm);
            
            return mktime(&tm);
        }

        static void WiFi_Connected(WiFiEvent_t event, WiFiEventInfo_t info)
        {
            log_i("Connected to AP successfully!");
        }

        static void WiFi_GotIP(WiFiEvent_t event, WiFiEventInfo_t info)
        {
            log_i("Connected with IP: %s", WiFi.localIP().toString().c_str());
        }

        static void WiFi_Disconnected(WiFiEvent_t event, WiFiEventInfo_t info)
        {
            log_w("Disconnected from WiFi access point, reason:");
            switch(info.disconnected.reason)
            {
                case WIFI_REASON_UNSPECIFIED : log_w("WIFI_REASON_UNSPECIFIED "); break; // = 1,
                case WIFI_REASON_AUTH_EXPIRE : log_w("WIFI_REASON_AUTH_EXPIRE "); break; // = 2,
                case WIFI_REASON_AUTH_LEAVE : log_w("WIFI_REASON_AUTH_LEAVE "); break; // = 3,
                case WIFI_REASON_ASSOC_EXPIRE : log_w("WIFI_REASON_ASSOC_EXPIRE "); break; // = 4,
                case WIFI_REASON_ASSOC_TOOMANY : log_w("WIFI_REASON_ASSOC_TOOMANY "); break; // = 5,
                case WIFI_REASON_NOT_AUTHED : log_w("WIFI_REASON_NOT_AUTHED "); break; // = 6,
                case WIFI_REASON_NOT_ASSOCED : log_w("WIFI_REASON_NOT_ASSOCED "); break; // = 7,
                case WIFI_REASON_ASSOC_LEAVE : log_w("WIFI_REASON_ASSOC_LEAVE "); break; // = 8,
                case WIFI_REASON_ASSOC_NOT_AUTHED : log_w("WIFI_REASON_ASSOC_NOT_AUTHED "); break; // = 9,
                case WIFI_REASON_DISASSOC_PWRCAP_BAD : log_w("WIFI_REASON_DISASSOC_PWRCAP_BAD "); break; // = 10,
                case WIFI_REASON_DISASSOC_SUPCHAN_BAD : log_w("WIFI_REASON_DISASSOC_SUPCHAN_BAD "); break; // = 11,
                case WIFI_REASON_IE_INVALID : log_w("WIFI_REASON_IE_INVALID "); break; // = 13,
                case WIFI_REASON_MIC_FAILURE : log_w("WIFI_REASON_MIC_FAILURE "); break; // = 14,
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT : log_w("WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT "); break; // = 15,
                case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT : log_w("WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT "); break; // = 16,
                case WIFI_REASON_IE_IN_4WAY_DIFFERS : log_w("WIFI_REASON_IE_IN_4WAY_DIFFERS "); break; // = 17,
                case WIFI_REASON_GROUP_CIPHER_INVALID : log_w("WIFI_REASON_GROUP_CIPHER_INVALID "); break; // = 18,
                case WIFI_REASON_PAIRWISE_CIPHER_INVALID : log_w("WIFI_REASON_PAIRWISE_CIPHER_INVALID "); break; // = 19,
                case WIFI_REASON_AKMP_INVALID : log_w("WIFI_REASON_AKMP_INVALID "); break; // = 20,
                case WIFI_REASON_UNSUPP_RSN_IE_VERSION : log_w("WIFI_REASON_UNSUPP_RSN_IE_VERSION "); break; // = 21,
                case WIFI_REASON_INVALID_RSN_IE_CAP : log_w("WIFI_REASON_INVALID_RSN_IE_CAP "); break; // = 22,
                case WIFI_REASON_802_1X_AUTH_FAILED : log_w("WIFI_REASON_802_1X_AUTH_FAILED "); break; // = 23,
                case WIFI_REASON_CIPHER_SUITE_REJECTED : log_w("WIFI_REASON_CIPHER_SUITE_REJECTED "); break; // = 24,
                case WIFI_REASON_BEACON_TIMEOUT : log_w("WIFI_REASON_BEACON_TIMEOUT "); break; // = 200,
                case WIFI_REASON_NO_AP_FOUND : log_w("WIFI_REASON_NO_AP_FOUND "); break; // = 201,
                case WIFI_REASON_AUTH_FAIL : log_w("WIFI_REASON_AUTH_FAIL "); break; // = 202,
                case WIFI_REASON_ASSOC_FAIL : log_w("WIFI_REASON_ASSOC_FAIL "); break; // = 203,
                case WIFI_REASON_HANDSHAKE_TIMEOUT : log_w("WIFI_REASON_HANDSHAKE_TIMEOUT "); break; // = 204,
                case WIFI_REASON_CONNECTION_FAIL : log_w("WIFI_REASON_CONNECTION_FAIL "); break; // = 205,
                case WIFI_REASON_AP_TSF_RESET : log_w("WIFI_REASON_AP_TSF_RESET "); break; // = 206,
                default: log_w("WIFI - unknown disconnected reason "); break;
            }            
        }

    public:
        network()
        {
            WiFi.onEvent(WiFi_Connected, SYSTEM_EVENT_STA_CONNECTED);
            WiFi.onEvent(WiFi_GotIP, SYSTEM_EVENT_STA_GOT_IP);
            WiFi.onEvent(WiFi_Disconnected, SYSTEM_EVENT_STA_DISCONNECTED);
        }

        void Init(void)
        {
            log_i("Init begin...");
            unsigned long startConnectTime;

            WiFi.disconnect(true);
            WiFi.mode(WIFI_STA);

            startConnectTime = millis();
            WiFi.begin(WIFI_SSID, WIFI_PWD);

            wl_status_t wifiStatus = WiFi.status();
            while (wifiStatus != WL_CONNECTED) 
            {
                log_v("Status:%d", wifiStatus);

                delay(100);
                wifiStatus = WiFi.status();

                if (wifiStatus != WL_CONNECTED && startConnectTime + 15000 < millis())
                {
                    log_w("Retry WiFi connection");
                    startConnectTime = millis();
                    WiFi.begin(WIFI_SSID, WIFI_PWD);                    
                    wifiStatus = WiFi.status();
                }
            }

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
       
            String result = GetData(url);
            if (result.length() > 0)
            {
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, result);

                _solarEdgeOverview.LastUpdateTime = ParseDateTime(doc["overview"]["lastUpdateTime"]);
                _solarEdgeOverview.CurrentPower = doc["overview"]["currentPower"]["power"];
                _solarEdgeOverview.LastDayDataEnergy = doc["overview"]["lastDayData"]["energy"];
            }
            else 
            {
                log_w("Invalid JSON data, using buffered result");
            }

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
            char currentDate[11]; // last char is NULL terminator; "YYYY-MM-DD\0"
            strftime(currentDate, 11, "%Y-%m-%d", &now);

            String url = "https://monitoringapi.solaredge.com/site/";
            url.concat(SOLAREDGE_SITEID);
            url.concat("/power?startTime=");
            url.concat(currentDate);
            url.concat("%2006:00:00&endTime=");
            url.concat(currentDate);
            url.concat("%2021:00:00&api_key=");
            url.concat(SOLAREDGE_APIKEY);
            
            String result = GetData(url);
            if (result.length() > 0)
            {            
                DynamicJsonDocument doc(7000);
                deserializeJson(doc, result);

                byte index = 0;
                JsonArray arr = doc["power"]["values"].as<JsonArray>();
                for (JsonVariant value : arr)
                {
                    _solarEdgePower.Values[index] = (unsigned int)(value["value"].as<float>());
                    index++;
                }
            }
            else 
            {
                log_w("Invalid JSON data, using buffered result");
            }
            return _solarEdgePower;
        }
};
