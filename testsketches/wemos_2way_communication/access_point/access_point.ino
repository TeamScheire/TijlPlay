#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
char packetBuffer[255];
unsigned int localPort = 9999;
const char *ssid = "WiFi Robot";
const char *password = "";

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(BUILTIN_LED, HIGH); // doof led
  Serial.begin(115200);
  Serial.println("\nStart");
  WiFi.softAP(ssid, password);
  Udp.begin(localPort);
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len - 1] = 0;
    Serial.print("Recibido(IP/Size/Data): ");
    Serial.print(Udp.remoteIP()); Serial.print(" / ");
    Serial.print(packetSize); Serial.print(" / ");
    Serial.println(packetBuffer);

    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write("received: ");
    Udp.write(packetBuffer);
    Udp.write("\r\n");
    Udp.endPacket();
    digitalWrite(BUILTIN_LED, LOW); // knipper led kort bij ontvangst
    delay(3);
    digitalWrite(BUILTIN_LED, HIGH);

  }
}

// - See more at: http://www.esp8266.com/viewtopic.php?f=29&t=4006#sthash.i5nGIIyP.dpuf
