#ifndef WIFI_SSID
#define WIFI_SSID "Development"
#endif

#ifndef WIFI_PWD
#define WIFI_PWD "EnterPasswordHere1234"
#endif

#include <WiFiClientSecure.h>


class network {
    private:
        WiFiClientSecure client;
        const IPAddress server = IPAddress(88,214,28,7);  // Server IP address Regeling.com
        const int    port = 80; // server's port (8883 for MQTT)

        const char*  pskIdent = "Client_identity"; // PSK identity (sometimes called key hint)
        const char*  psKey = "1a2b3c4d"; // PSK Key (must be hex string without 0x)

    public:
        void Init(void)
        {
            Serial.print("Attempting to connect to SSID: ");
            Serial.println(WIFI_SSID);
            WiFi.begin(WIFI_SSID, WIFI_PWD);

            // attempt to connect to Wifi network:
            while (WiFi.status() != WL_CONNECTED) 
            {
                Serial.print(".");
                // wait 1 second for re-trying
                delay(1000);
            }

            Serial.print("Connected to ");
            Serial.println(WIFI_SSID);
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());            

            client.setPreSharedKey(pskIdent, psKey);

            Serial.println("\nStarting connection to server...");
            
            // TODO - get some data and handle TLS correctly;
            // - [E][ssl_client.cpp:33] _handle_error(): [start_ssl_client():199]: (-29184) SSL - An invalid SSL record was received
            // - [E][WiFiClientSecure.cpp:152] connect(): start_ssl_client: -29184
            // - Connection failed!

            if (!client.connect(server, port))
            {
                Serial.println("Connection failed!");
            }
            else
            {
                Serial.println("Connected to server!");
                // Make a HTTP request:
                client.println("GET / HTTP/1.0");
                client.print("Host: ");
                //client.println(server);
                client.println("Connection: close");
                client.println();

                while (client.connected()) 
                {
                    String line = client.readStringUntil('\n');
                    if (line == "\r") 
                    {
                        Serial.println("headers received");
                        break;
                    }
                }

                // if there are incoming bytes available
                // from the server, read them and print them:
                while (client.available()) {
                    char c = client.read();
                    Serial.write(c);
                }
            }

            client.stop();
            Serial.println("client stopped. End of wifi demo.");
        };
};
