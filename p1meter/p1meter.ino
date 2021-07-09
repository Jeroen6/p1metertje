#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ArduinoJson.h"

const char * ssid = "Jeroen";
const char * password = "supergeheim wachtwoord";

AsyncUDP udp;

int port = 40721;

#define DAC1 25
#define DAC2 26

volatile float net_actual;
volatile float net_daily;

void processPacket(uint8_t *data, size_t len){
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, data);
        
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    uint32_t time = doc["TIMESTAMP_UTC"];
    float exkw = doc["PRODUCTION_KW"];
    float imkw = doc["CONSUMPTION_KW"];
    float net = imkw - exkw;

    net_actual = net;

    net_daily

    Serial.print("Time: ");
    Serial.println(time);
    Serial.print("Sensor exp: ");
    Serial.println(exkw);
    Serial.print("Sensor imp: ");
    Serial.println(imkw);
    Serial.print("Net: ");
    Serial.println(net);
}

/*
{
 "API_STATUS": "production",
 "API_VERSION": 6,
 "CONSUMPTION_GAS_M3": 974.324,
 "CONSUMPTION_KW": 0.172,
 "CONSUMPTION_KWH_HIGH": 1076.242,
 "CONSUMPTION_KWH_LOW": 1416.185,
 "P1_SOFTWARE_VERSION": "202004-0.9.17(Maxine)",
 "P1_SYSTEM_ID": "E6FD-79DA-36A3-AE6C-4975",
 "PRODUCTION_KW": 0.0,
 "PRODUCTION_KWH_HIGH": 54.125,
 "PRODUCTION_KWH_LOW": 26.234,
 "ROOM_TEMPERATURE_IN": 0.0,
 "ROOM_TEMPERATURE_OUT": 0.0,
 "TARIFCODE": "HIGH",
 "TIMESTAMP_UTC": 1621449893,
 "TIMESTAMP_lOCAL": "2021-05-19 20:44:53",
 "WATERMETER_CONSUMPTION_TOTAL_M3": 0.0
}
*/

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA); // Make this the client (the server is WIFI_AP)
    WiFi.begin(ssid, password);
    esp_wifi_set_ps(WIFI_PS_NONE);
    delay(100);

    Serial.print("Connecting...");
    // Display a period every 0.5 s to show the user something is happening.
    while (WiFi.waitForConnectResult() != WL_CONNECTED) { 
      Serial.print(".");    
      delay(500);
    }
    Serial.println("connected");

    if(udp.listen(port)) {
        udp.onPacket([](AsyncUDPPacket packet) {
            processPacket(packet.data(), packet.length());
        });
    }
}

uint8_t energy_now(){
    const float min = -2.5;
    const float max = 2.5;
    float dac = 0;
    float net = net_actual;
    if(net > 0){
        dac = net / max;
    }else{
        dac = -net / min;
    }
    dac = (dac + 1.0) / 2;
    dac *= 255;
    uint8_t udac = dac;
}

uint8_t energy_daily(){
    const float min = -2.5;
    const float max = 2.5;
    float dac = 0;
    float net = net_actual;
    if(net > 0){
        dac = net / max;
    }else{
        dac = -net / min;
    }
    dac = (dac + 1.0) / 2;
    dac *= 255;
    uint8_t udac = dac;
}


void loop(){
    uint8_t udac = energy_now();
    dacWrite(DAC1, udac);
    dacWrite(DAC2, udac);
    
    delay(1);
}
