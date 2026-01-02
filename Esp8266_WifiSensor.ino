
//CavaWiFi Version 0.9
//Author Juan Maioli
#include <DNSServer.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <OneWire.h> 
#include <WiFiClientSecure.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#define ONE_WIRE_BUS1 (D4) // Inicia Medicion Temp Ambiental 
//#define ONE_WIRE_BUS2 (D5) // Inicia Medicion Temp Ambiental 

OneWire oneWire1(ONE_WIRE_BUS1); 
//OneWire oneWire2(ONE_WIRE_BUS2); 

DallasTemperature sensors1(&oneWire1);
//DallasTemperature sensors2(&oneWire2);

const char* host = "pikapp.com.ar"; 
String serial_number;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  Serial.begin(115200);
  delay(10);
  sensors1.begin(); 
  //sensors2.begin(); 
  
  // Obtener MAC y configurar Hostname único
  serial_number = WiFi.macAddress();
  String chipID = serial_number;
  chipID.replace(":", "");
  String hostName = "WifiSensor-" + chipID.substring(chipID.length() - 4);
  
  WiFi.hostname(hostName);
  Serial.print("Hostname: ");
  Serial.println(hostName);
  Serial.print("MAC: ");
  Serial.println(serial_number);

  WiFiManager wifiManager;
//wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect(hostName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println("Connected OK");
}
 
void loop() {
  sensors1.requestTemperatures(); // Inicia Medicion Temp Freezer 
  //sensors2.requestTemperatures(); // Inicia Medicion Temp Ambiental 
  
  float celsius1 = sensors1.getTempCByIndex(0); 
  float celsius2 = 0; //sensors2.getTempCByIndex(0); 
 
  String tempSerial;
  tempSerial=  String(celsius1)+ " ºC Int";
  Serial.println(tempSerial);

  //tempSerial=  String(celsius2)+ " ºC Ext";
  //Serial.println(tempSerial);
 
  String txtUrl;
  txtUrl = "/cava/carga.php/?sn=" + String(serial_number) + "&s1=" + String(celsius1)+ "&s2=" + String(celsius2);

WiFiClient client;
// WiFiClientSecure client;

  Serial.print("connecting to ");
  Serial.println(host);
  // if (!client.connect(host, 443)) {
  if (!client.connect(host, 80)) {
    Serial.println("Conexion Fallida");
    return;
  }

  Serial.print("requesting URL: ");
  client.print(String("GET ") + txtUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.print("reply was:");
  Serial.println(line);
  Serial.println("closing connection");
  delay(59000);
 }
