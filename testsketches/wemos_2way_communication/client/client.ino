#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
const char *ssid = "WiFi_Robot";  //
const char *password = "";

char packetBuffer[255];
unsigned int localPort = 9999;
IPAddress ipServidor(192, 168, 4, 1);
IPAddress ipCliente(192, 168, 4, 10);
IPAddress Subnet(255, 255, 255, 0);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  Serial.println("\nStart Client");
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA); // para que solo sea STA y no genere la IP 192.168.4.1
  WiFi.config(ipCliente, ipServidor, Subnet);
  Udp.begin(localPort);
}

void loop() {
  unsigned long Tiempo_Envio = millis();
  //ENVIO
  Udp.beginPacket(ipServidor, 9999);
  Udp.write("Millis: ");
  char buf[20];   unsigned long testID = millis();    sprintf(buf, "%lu", testID);
  Udp.write(buf);
  Udp.write("\r\n");
  Udp.endPacket();
  Serial.print("enviando: "); Serial.println(buf);
  digitalWrite(BUILTIN_LED, LOW);
  delay(10); // para que le de tiempo a recibir la respuesta al envio anterior
  digitalWrite(BUILTIN_LED, HIGH);

  //RECEPCION
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len - 1] = 0;
    Serial.print("RECIBIDO(IP/Port/Size/Datos): ");
    Serial.print(Udp.remoteIP()); Serial.print(" / ");
    Serial.print(Udp.remotePort()); Serial.print(" / ");
    Serial.print(packetSize); Serial.print(" / ");
    Serial.println(packetBuffer);


  }
  Serial.println("");
  delay(500);
}
//- See more at: http://www.esp8266.com/viewtopic.php?f=29&t=4006#sthash.i5nGIIyP.dpuf
