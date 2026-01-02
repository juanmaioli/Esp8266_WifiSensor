
//CavaWiFi Version 0.9
//Author Juan Maioli
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS1 (D4) // Inicia Medicion Temp Ambiental 
//#define ONE_WIRE_BUS2 (D5) // Inicia Medicion Temp Ambiental 

OneWire oneWire1(ONE_WIRE_BUS1); 
//OneWire oneWire2(ONE_WIRE_BUS2); 

DallasTemperature sensors1(&oneWire1);
//DallasTemperature sensors2(&oneWire2);

String serial_number;
unsigned long last_report_time = 0;
unsigned long last_sensor_read = 0;
const long sensor_read_interval = 5000; // Leer sensor cada 5s
float globalTempC = DEVICE_DISCONNECTED_C;

struct Config {
  char host[64];
  bool use_https;
  int interval_minutes;
  char magic[4]; // Para verificar si la EEPROM está inicializada
} settings;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

ESP8266WebServer server(80);

// --- Recursos Web ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensor WiFi</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <a class="prev" onclick="changeSlide(-1)">&#10094;</a>
        <a class="next" onclick="changeSlide(1)">&#10095;</a>
        <div class="carousel-container">
            <!-- Slide 1: Estado del Dispositivo -->
            <div class="carousel-slide fade">
                <h2>Estado del Dispositivo</h2>
                <div class="emoji-container"><span class="emoji">📟</span></div>
                <h3>
                    <strong>🖥️ Hostname:</strong> %HOSTNAME%<br>
                    <strong>🏠 IP:</strong> %IP%<br>
                    <strong>📶 Señal:</strong> %RSSI% dBm<br>
                    <strong>🆔 MAC:</strong> %MAC%<br>
                    <strong>🧠 Heap Libre:</strong> %FREE_HEAP% KB<br>
                    <strong>⚡ Activo:</strong> %UPTIME%</h3>
            </div>
            <!-- Slide 2: Temperatura -->
            <div class="carousel-slide fade">
                <h2>Temperatura Actual</h2>
                <div class="emoji-container"><span class="emoji">🌡️</span></div>
                <div style="text-align:center; margin-top: 20px;">
                    <span style="font-size: 4em; font-weight: bold; color: #4CAF50;">%TEMP1% ºC</span>
                    <p>Sensor Interior</p>
                </div>
            </div>
            <!-- Slide 3: Configuración -->
            <div class="carousel-slide fade">
                <h2>Configuración</h2>
                <div class="emoji-container"><span class="emoji">⚙️</span></div>
                <form action="/save" method="POST" style="padding: 0 10px;">
                    <label>Servidor (Host):</label>
                    <input type="text" name="host" value="%CONF_HOST%">
                    <label>Protocolo:</label>
                    <select name="protocol">
                        <option value="0" %CONF_HTTP%>HTTP</option>
                        <option value="1" %CONF_HTTPS%>HTTPS</option>
                    </select>
                    <label>Intervalo (1m - 1440m):</label>
                    <input type="number" name="interval" value="%CONF_INTERVAL%" min="1" max="1440">
                    <div style="text-align:center; margin-top:15px;">
                        <button type="submit" class="button">Guardar</button>
                    </div>
                </form>
            </div>
        </div>
        <div class="dots">
            <span class="dot" onclick="currentSlide(1)"></span>
            <span class="dot" onclick="currentSlide(2)"></span>
            <span class="dot" onclick="currentSlide(3)"></span>
        </div>
    </div>
    <script src="script.js"></script>
</body>
</html>
)rawliteral";

const char STYLE_CSS[] PROGMEM = R"rawliteral(
:root { --bg-color: #f0f2f5; --container-bg: #ffffff; --text-primary: #1c1e21; --text-secondary: #4b4f56; --dot-color: #bbb; --dot-active-color: #717171; }
@media (prefers-color-scheme: dark) { :root { --bg-color: #121212; --container-bg: #1e1e1e; --text-primary: #e0e0e0; --text-secondary: #b0b3b8; --dot-color: #555; --dot-active-color: #ccc; } }
body { background-color: var(--bg-color); color: var(--text-secondary); font-family: sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
.container { background-color: var(--container-bg); padding: 2rem; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 350px; height: 450px; position: relative; display: flex; flex-direction: column; }
.carousel-container { position: relative; flex-grow: 1; overflow: hidden; }
.carousel-slide { display: none; height: 100%; text-align: left; }
.fade { animation: fade 0.5s; }
@keyframes fade { from {opacity: .4} to {opacity: 1} }
.prev, .next { cursor: pointer; position: absolute; top: 50%; width: auto; padding: 10px; color: var(--text-primary); font-weight: bold; font-size: 20px; z-index: 10; text-decoration: none; }
.prev { left: 10px; } .next { right: 10px; }
.dots { text-align: center; padding-top: 10px; }
.dot { cursor: pointer; height: 12px; width: 12px; margin: 0 2px; background-color: var(--dot-color); border-radius: 50%; display: inline-block; }
.active { background-color: var(--dot-active-color); }
.emoji-container { text-align: center; font-size: 3em; margin: 10px 0; }
h2 { text-align: center; color: var(--text-primary); }
h3 { font-size: 0.9em; line-height: 1.6; }
input, select { width: 100%; padding: 8px; margin: 5px 0 15px; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
label { font-weight: bold; font-size: 0.9em; }
.button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
.button:hover { background-color: #45a049; }
)rawliteral";

const char SCRIPT_JS[] PROGMEM = R"rawliteral(
let slideIndex = 1;
document.addEventListener("DOMContentLoaded", () => showSlide(slideIndex));
function changeSlide(n) { showSlide(slideIndex += n); }
function currentSlide(n) { showSlide(slideIndex = n); }
function showSlide(n) {
    let slides = document.getElementsByClassName('carousel-slide');
    let dots = document.getElementsByClassName('dot');
    if (n > slides.length) slideIndex = 1;
    if (n < 1) slideIndex = slides.length;
    for (let i = 0; i < slides.length; i++) slides[i].style.display = 'none';
    for (let i = 0; i < dots.length; i++) dots[i].className = dots[i].className.replace(' active', '');
    slides[slideIndex - 1].style.display = 'block';
    dots[slideIndex - 1].className += ' active';
}
setInterval(() => changeSlide(1), 10000);
)rawliteral";

void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  if (strcmp(settings.magic, "CFG1") != 0) {
    // Valores por defecto
    Serial.println("EEPROM vacía, cargando defaults");
    strcpy(settings.host, "pikapp.com.ar");
    settings.use_https = false;
    settings.interval_minutes = 1;
    strcpy(settings.magic, "CFG1");
    EEPROM.put(0, settings);
    EEPROM.commit();
  }
  Serial.print("Config Host: "); Serial.println(settings.host);
  Serial.print("Config HTTPS: "); Serial.println(settings.use_https);
}

void saveConfig() {
  EEPROM.put(0, settings);
  EEPROM.commit();
  Serial.println("Configuración guardada");
}

// --- Handlers del Servidor ---
String getUptime() {
    unsigned long s = millis() / 1000;
    int d = s / 86400; s %= 86400;
    int h = s / 3600; s %= 3600;
    int m = s / 60; s %= 60;
    return String(d) + "d " + String(h) + "h " + String(m) + "m " + String(s) + "s";
}

void handleRoot() {
    String html = INDEX_HTML;
    
    html.replace("%HOSTNAME%", WiFi.hostname());
    html.replace("%IP%", WiFi.localIP().toString());
    html.replace("%RSSI%", String(WiFi.RSSI()));
    html.replace("%MAC%", WiFi.macAddress());
    html.replace("%FREE_HEAP%", String(ESP.getFreeHeap() / 1024));
    html.replace("%UPTIME%", getUptime());
    html.replace("%TEMP1%", (globalTempC == DEVICE_DISCONNECTED_C) ? "--" : String(globalTempC, 1));

    // Configuración
    html.replace("%CONF_HOST%", String(settings.host));
    html.replace("%CONF_HTTP%", settings.use_https ? "" : "selected");
    html.replace("%CONF_HTTPS%", settings.use_https ? "selected" : "");
    html.replace("%CONF_INTERVAL%", String(settings.interval_minutes));
    
    server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("host")) strncpy(settings.host, server.arg("host").c_str(), 63);
  if (server.hasArg("protocol")) settings.use_https = (server.arg("protocol").toInt() == 1);
  if (server.hasArg("interval")) settings.interval_minutes = constrain(server.arg("interval").toInt(), 1, 1440);
  
  saveConfig();
  
  String html = "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='3;url=/'></head><body><h2>Configuraci&oacute;n Guardada!</h2><p>Redirigiendo...</p></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  sensors1.begin(); 
  //sensors2.begin(); 
  
  // Cargar configuración persistente
  loadConfig();

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
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect(hostName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println("Connected OK");

  // Configurar Servidor Web
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/style.css", []() { server.send(200, "text/css", STYLE_CSS); });
  server.on("/script.js", []() { server.send(200, "application/javascript", SCRIPT_JS); });
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop() {
  server.handleClient();

  // --- 1. Lectura Frecuente del Sensor (Cada 5s) ---
  if (millis() - last_sensor_read > sensor_read_interval || last_sensor_read == 0) {
    last_sensor_read = millis();
    sensors1.requestTemperatures();
    float t = sensors1.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) {
      globalTempC = t;
    } else {
       Serial.println("⚠️ Advertencia: Lectura de sensor fallida (Web)");
    }
  }
  
    // --- 2. Reporte al Servidor (Dinámico) ---
    unsigned long interval_ms = (unsigned long)settings.interval_minutes * 60000;
    if (millis() - last_report_time < interval_ms && last_report_time != 0) {
      return;
    }
    last_report_time = millis();
  
    // Usamos el valor global actualizado
    float celsius1 = globalTempC; 
    
    // Validar si tenemos una temperatura válida para reportar
    if (celsius1 == DEVICE_DISCONNECTED_C) {
      Serial.println("❌ Error: No se pudo leer la temperatura para el reporte.");
      Serial.println("⏳ Reintentando en 30 segundos...");
      last_report_time = millis() - 30000; 
      return;
    }
  
    float celsius2 = 0; 
    String tempSerial = String(celsius1)+ " ºC Int";
    Serial.println(tempSerial);
   
    String txtUrl = "/cava/carga.php/?sn=" + String(serial_number) + "&s1=" + String(celsius1)+ "&s2=" + String(celsius2);
  
    WiFiClient client;
    WiFiClientSecure clientSecure;
    
    if (settings.use_https) {
        clientSecure.setInsecure(); // No validamos certificado para simplicidad
    }
  
    Serial.print("connecting to ");
    Serial.println(settings.host);
    
    bool connected = false;
    if (settings.use_https) {
        connected = clientSecure.connect(settings.host, 443);
    } else {
        connected = client.connect(settings.host, 80);
    }
  
    if (!connected) {
      Serial.println("Conexion Fallida");
      return;
    }
    
    // Referencia genérica al stream conectado para enviar datos
    Stream* stream = settings.use_https ? (Stream*)&clientSecure : (Stream*)&client;
  
    Serial.print("requesting URL: ");
    stream->print(String("GET ") + txtUrl + " HTTP/1.1\r\n" +
                 "Host: " + settings.host + "\r\n" +
                 "User-Agent: ESP8266WifiSensor\r\n" +
                 "Connection: close\r\n\r\n");
  
    Serial.println("request sent");
    unsigned long timeout = millis();
    while (settings.use_https ? clientSecure.connected() : client.connected()) {
      if (millis() - timeout > 5000) break; // Timeout simple
      if (stream->available()) {
          String line = stream->readStringUntil('\n');
          if (line == "\r") {
              Serial.println("headers received");
              break;
          }
      }
    }
    
    if (stream->available()) {
      String line = stream->readStringUntil('\n');
      Serial.print("reply was:");
      Serial.println(line);
    }
    Serial.println("closing connection");
   }
