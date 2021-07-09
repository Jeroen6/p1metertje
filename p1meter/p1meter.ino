#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

// Settings */
const int p1monitorBroadcastPort = 40721;
#define RESETWIFI 13
#define DAC1 25
#define DAC2 26

/* Broadcaster obtained values */
volatile float net_actual = 0;
volatile float net_daily = 0;
volatile float energynow = 0;
IPAddress p1ip;
volatile bool runApiRequestFlag = false;
/* Api obtained values */
float daily_import_dal = 0;
float daily_import_piek = 0;
float daily_export_dal = 0;
float daily_export_piek = 0;
float daily_net = 0;
float daily_net_meter = 0;

/* Operating variables */
AsyncUDP udp;

/* Private functions */
void processBroadcastPacketFromP1Monitor(uint8_t *data, size_t len, IPAddress ip);
void p1runApiRequest();
float energy_now_for_meter(const float meter,const float min, const float max);
void p1runApiRequest();

void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    Serial.begin(115200);
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;
    //reset settings if pin high during reset
    if(digitalRead(RESETWIFI)){
        wm.resetSettings();    
    }
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result
    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP","password"); // remove password for unsecured ap
    res = wm.autoConnect("P1 Metertje");
    if(!res) {
        Serial.println("Failed to connect");
    }else{    
        Serial.println("connected...yeey :)");
    }
    // Disable power saving to receive UDP broadcasts
    esp_wifi_set_ps(WIFI_PS_NONE);
    if(udp.listen(p1monitorBroadcastPort)) {
        udp.onPacket([](AsyncUDPPacket packet) {
            processBroadcastPacketFromP1Monitor(packet.data(), packet.length(), packet.remoteIP());
        });
    }
}

float dither = 0;

void loop(){
    if(runApiRequestFlag){
      runApiRequestFlag = false;
      p1runApiRequest();
    }

    int udac = (float)daily_net_meter * 255.0f;
    udac += random(-2,2);
    if(udac > 255)
      udac = 255;
     if(udac < 0)
      udac = 0;
    dacWrite(DAC1, udac);
    dacWrite(DAC2, udac);
    delay(0.1);
}

float energy_now_for_meter(const float net,const float min = -2.5, const float max = 2.5){
    float dac = 0;
    if(net > 0){
        dac = net / max;
    }else{
        dac = -net / min;
    }
    dac = (dac + 1.0) / 2;
    return dac;
}

/* Broadcsat packet
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
void processBroadcastPacketFromP1Monitor(uint8_t *data, size_t len, IPAddress ip){
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    p1ip = ip;
    runApiRequestFlag = true;
    uint32_t time = doc["TIMESTAMP_UTC"];
    float exkw = doc["PRODUCTION_KW"];
    float imkw = doc["CONSUMPTION_KW"];
    float net = imkw - exkw;
    net_actual = net;
    energynow = energy_now_for_meter(net_actual);
    Serial.println("Received broadcast from P1 Monitor");
    Serial.print("IP: ");
    Serial.println(ip);
    Serial.print("Time: ");
    Serial.println(time);
    Serial.print("exp: ");
    Serial.println(exkw);
    Serial.print("imp: ");
    Serial.println(imkw);
    Serial.print("Net: ");
    Serial.println(net); 
}

// 8 "Huidige dag KWh verbruik dal/nacht dag (1.8.1)",
// 9 "Huidige dag KWh verbruik piek/dag (1.8.2)",
// 10 "Huidige dag KWh geleverd dal/nacht dag (2.8.1)",
// 11 "Huidige dag KWh geleverd piek/dag (2.8.2)",
// Lees index 7 voor waarde 8
void p1runApiRequest(){
    String url = "http://";
    String apiUrl = "/api/v1/status";
    url = url + p1ip.toString() + apiUrl;
    Serial.println(url);
    HTTPClient http;
    http.begin(url.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        // Parse response
        DynamicJsonDocument doc(10000);
        deserializeJson(doc, http.getStream());
        // Read values
        JsonVariant variant;
        variant = doc[7][1].as<JsonVariant>();
        daily_import_dal = variant.as<float>();
        variant = doc[8][1].as<JsonVariant>();
        daily_import_piek = variant.as<float>();
        variant = doc[9][1].as<JsonVariant>();
        daily_export_dal = variant.as<float>();
        variant = doc[10][1].as<JsonVariant>();
        daily_export_piek = variant.as<float>();
        daily_net = daily_import_dal + daily_import_piek + -daily_export_dal + -daily_export_piek;
        daily_net_meter = energy_now_for_meter(daily_net,-1.5,1.5);
        Serial.print("daily_import_dal: ");
        Serial.println(daily_import_dal);
        Serial.print("daily_import_piek: ");
        Serial.println(daily_import_piek);
        Serial.print("daily_export_dal: ");
        Serial.println(daily_export_dal);
        Serial.print("daily_export_piek: ");
        Serial.println(daily_export_piek);
        Serial.print("daily_netto: ");
        Serial.println(daily_net, 4);
        Serial.print("meter_value: ");
        Serial.println(daily_net_meter, 4);
    }else{
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
}
