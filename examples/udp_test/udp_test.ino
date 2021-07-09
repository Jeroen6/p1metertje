/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-set-custom-hostname-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP UDP;
WiFiUDP UDPtx;

// Replace with your network credentials (STATION)
const char* ssid = "Jeroen";
const char* password = "supergeheim wachtwoord";

String hostname = "ESP32 Node Temperature";

char packet[1024];
uint16_t p1port = 40721;
IPAddress p1broadcast(255,255,255,255);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname
  //wifi_station_set_hostname( hostname.c_str() );
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

byte command = 0;

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  UDP.beginMulticast(p1broadcast, p1port);
  //UDP.begin(p1port);
  Serial.print("Listening on UDP port ");
  Serial.println(p1port);
  UDPtx.beginPacket(p1broadcast,p1port);
  UDPtx.write(&command, 1);
  UDPtx.endPacket();
}

void loop() {
  // If packet received...
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packet, sizeof(packet));
    if (len > 0)
    {
      packet[len] = '\0';
    }
    Serial.println(packet);
    UDPtx.beginPacket(p1broadcast,p1port);
    UDPtx.write(&command, 1);
    UDPtx.endPacket();
  }
}
