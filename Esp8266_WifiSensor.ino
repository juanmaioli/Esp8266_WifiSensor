
//WifiSensor Version 1.0.0
//Author Juan Maioli
#define FIRMWARE_VERSION "1.0.0"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

#define ONE_WIRE_BUS1 (D4) // Inicia Medicion Temp Ambiental 
//#define ONE_WIRE_BUS2 (D5) // Inicia Medicion Temp Ambiental 

OneWire oneWire1(ONE_WIRE_BUS1); 
//OneWire oneWire2(ONE_WIRE_BUS2); 

DallasTemperature sensors1(&oneWire1);
//DallasTemperature sensors2(&oneWire2);

String serial_number;
unsigned long last_report_time = 0;
unsigned long last_sensor_read = 0;
unsigned long last_success_temp_millis = 0;
float globalTempC = DEVICE_DISCONNECTED_C;

struct Config {
  char host[64];
  bool use_https;
  int interval_minutes;
  char description[51]; // Nueva variable
  char ota_password[21]; // Password OTA
  char magic[5]; // Reservar espacio para terminador nulo
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
    <link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🌡️</text></svg>">
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="container">
        <a class="prev" onclick="changeSlide(-1)">&#10094;</a>
        <a class="next" onclick="changeSlide(1)">&#10095;</a>
        <div class="carousel-container">
             <!-- Slide 1: Temperatura -->
            <div class="carousel-slide fade">
                <h2>Temperatura Actual En %DESC%</h2>
                <div class="emoji-container"><span class="emoji">🌡️</span></div>
                <div style="text-align:center; margin-top: 20px;">
                    <span style="font-size: 4em; font-weight: bold; color: %TEMP_COLOR%;">%TEMP1% ºC</span>
                    <p>Sensor Interior</p>
                    <p style="font-size: 0.8em; color: var(--text-secondary); margin-top: 10px;">Actualizado hace %TEMP_TIME%</p>
                </div>
            </div>
            <!-- Slide 2: Datos del Tiempo -->
            <div class="carousel-slide fade">
                <h2>Datos del Tiempo</h2>
                <div class="emoji-container"><span id="weather-main-icon" class="emoji">☁️</span></div>
                <div id="weather-data" style="text-align: center; margin-top: 20px;">
                    <p>Cargando datos...</p>
                </div>
                <button onclick="fetchWeather()" class="button" style="margin-top:10px; width:auto; padding: 5px 10px; font-size: 0.8em;">Actualizar</button>
            </div>
            <!-- Slide 3: Estado del Dispositivo -->
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
            <!-- Slide 4: Configuración -->
            <div class="carousel-slide fade">
                <h2>Configuración</h2>
                <div class="emoji-container"><span class="emoji">⚙️</span></div>
                <form action="/save" method="POST" style="padding: 0 10px;">
                    <label>Descripción:</label>
                    <input type="text" name="desc" value="%CONF_DESC%" maxlength="50" placeholder="Ej: Casa, Oficina">
                    <label>Servidor (Host):</label>
                    <input type="text" name="host" value="%CONF_HOST%">
                    <label>Protocolo:</label>
                    <select name="protocol">
                        <option value="0" %CONF_HTTP%>HTTP</option>
                        <option value="1" %CONF_HTTPS%>HTTPS</option>
                    </select>
                    <label>Intervalo de Reporte:</label>
                    <select name="interval_opt" id="interval_opt" onchange="toggleManual()">
                        <option value="1" %INT_1%>1 Minuto</option>
                        <option value="15" %INT_15%>15 Minutos</option>
                        <option value="30" %INT_30%>30 Minutos</option>
                        <option value="60" %INT_60%>1 Hora</option>
                        <option value="360" %INT_360%>6 Horas</option>
                        <option value="720" %INT_720%>12 Horas</option>
                        <option value="1440" %INT_1440%>24 Horas</option>
                        <option value="manual" %INT_MANUAL%>Ingreso Manual</option>
                    </select>
                    <div style="display: flex; gap: 10px; margin-top: 15px; align-items: center;">
                        <div id="manual_div" style="display: %MAN_DISP%; flex: 1;">
                            <input type="number" name="interval_val" value="%CONF_INTERVAL%" min="1" max="1440" placeholder="Minutos" style="margin: 0;">
                        </div>
                        <button type="submit" class="button" style="flex: 1; margin: 0;">Guardar</button>
                    </div>
                </form>
            </div>
        </div>
        <div class="dots">
            <span class="dot" onclick="currentSlide(1)"></span>
            <span class="dot" onclick="currentSlide(2)"></span>
            <span class="dot" onclick="currentSlide(3)"></span>
            <span class="dot" onclick="currentSlide(4)"></span>
        </div>
    </div>
    <script src="script.js"></script>
</body>
</html>
)rawliteral";

const char STYLE_CSS[] PROGMEM = R"rawliteral(
:root { --bg-color: #f0f2f5; --container-bg: #ffffff; --text-primary: #1c1e21; --text-secondary: #4b4f56; --dot-color: #bbb; --dot-active-color: #717171; --input-bg: #ffffff; --input-border: #ccc; --input-text: #1c1e21; }
@media (prefers-color-scheme: dark) { :root { --bg-color: #121212; --container-bg: #1e1e1e; --text-primary: #e0e0e0; --text-secondary: #b0b3b8; --dot-color: #555; --dot-active-color: #ccc; --input-bg: #2d2d2d; --input-border: #444; --input-text: #e0e0e0; } }
body { background-color: var(--bg-color); color: var(--text-secondary); font-family: sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }
.container { background-color: var(--container-bg); padding: 2rem; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 350px; height: 500px; position: relative; display: flex; flex-direction: column; }
.carousel-container { position: relative; flex-grow: 1; overflow: hidden; }
.carousel-slide { display: none; height: 100%; text-align: left; overflow-y: auto; }
.fade { animation: fade 0.5s; }
@keyframes fade { from {opacity: .4} to {opacity: 1} }
.prev, .next { cursor: pointer; position: absolute; top: 50%; width: auto; padding: 10px; color: var(--text-primary); font-weight: bold; font-size: 20px; z-index: 10; text-decoration: none; }
.prev { left: 10px; } .next { right: 10px; }
.dots { text-align: center; padding-top: 10px; }
.dot { cursor: pointer; height: 12px; width: 12px; margin: 0 2px; background-color: var(--dot-color); border-radius: 50%; display: inline-block; }
.active { background-color: var(--dot-active-color); }
.emoji-container { text-align: center; font-size: 3em; margin: 10px 0; }
h2 { text-align: center; color: var(--text-primary); }
h3 { font-size: 0.9em; line-height: 1.6; color: var(--text-primary); }
input, select { width: 100%; padding: 8px; margin: 5px 0 15px; display: inline-block; border: 1px solid var(--input-border); border-radius: 4px; box-sizing: border-box; background-color: var(--input-bg); color: var(--input-text); }
label { font-weight: bold; font-size: 0.9em; color: var(--text-primary); }
.button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
.button:hover { background-color: #45a049; }
)rawliteral";

const char SCRIPT_JS[] PROGMEM = R"rawliteral(
let slideIndex = 1;
document.addEventListener("DOMContentLoaded", () => {
    showSlide(slideIndex);
    fetchWeather();
    document.addEventListener("keydown", (e) => {
        if (e.key === "ArrowLeft") changeSlide(-1);
        if (e.key === "ArrowRight") changeSlide(1);
    });
});
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
function toggleManual() {
    let opt = document.getElementById('interval_opt').value;
    document.getElementById('manual_div').style.display = (opt === 'manual') ? 'block' : 'none';
}
function fetchWeather() {
    const container = document.getElementById('weather-data');
    container.innerHTML = '<p>Actualizando...</p>';
    
    fetch('http://172.21.5.3/ApiWheather/json/')
        .then(response => response.json())
        .then(data => {
            // Función para procesar y encontrar objetos con la estructura deseada
            const process = (item) => {
                if (typeof item === 'object' && item !== null) {
                    if (item.icon && item.etiqueta && item.dato) {
                        // Detectar el ítem especial de Estado/Icono
                        if (item.dato === 'Icono') {
                            const mainIcon = document.getElementById('weather-main-icon');
                            if (mainIcon) mainIcon.textContent = item.icon;
                            return ''; // No lo mostramos en la lista
                        }
                        
                        // Formato: Icono Etiqueta Dato en una sola línea
                        return `<div style="padding: 5px 0; font-size: 0.9em; text-align: left;">${item.icon} ${item.etiqueta} ${item.dato}</div>`;
                    }
                    // Si es un array o un objeto con otros datos, seguimos buscando
                    return Object.values(item).map(val => process(val)).join('');
                }
                return '';
            };
            
            const html = process(data);
            container.innerHTML = html || '<p>No hay datos disponibles</p>';
        })
        .catch(err => {
            console.error(err);
            container.innerHTML = '<p style="color: #dc3545;">⚠️ Error al obtener datos</p>';
        });
}
)rawliteral";

void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  if (String(settings.magic) != "CFG3") {
    // Valores por defecto
    Serial.println("EEPROM vacía o versión antigua, cargando defaults");
    memset(settings.host, 0, sizeof(settings.host));
    strcpy(settings.host, "pikapp.com.ar");
    settings.use_https = false;
    settings.interval_minutes = 1;
    memset(settings.description, 0, sizeof(settings.description));
    strcpy(settings.description, "Casa");
    memset(settings.ota_password, 0, sizeof(settings.ota_password));
    strcpy(settings.ota_password, "ArduinoOTA");
    strcpy(settings.magic, "CFG3");
    EEPROM.put(0, settings);
    EEPROM.commit();
  }
  Serial.print("Config Host: "); Serial.println(settings.host);
  Serial.print("Config Desc: "); Serial.println(settings.description);
  Serial.print("Config HTTPS: "); Serial.println(settings.use_https);
  Serial.print("Config OTA Pass: "); Serial.println(settings.ota_password);
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
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    // 1. Cabecera y Estilos
    String chunk = F("<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>");
    chunk += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    chunk += F("<title>Sensor WiFi</title>");
    chunk += F("<link rel='icon' href='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🌡️</text></svg>'>");
    chunk += F("<link rel='stylesheet' href='style.css'></head><body><div class='container'>");
    chunk += F("<a class='prev' onclick='changeSlide(-1)'>&#10094;</a><a class='next' onclick='changeSlide(1)'>&#10095;</a>");
    chunk += F("<div class='carousel-container'>");
    server.sendContent(chunk);

    // 2. Slide 1: Temperatura
    unsigned long diff = (last_success_temp_millis > 0) ? (millis() - last_success_temp_millis) / 1000 : 0;
    String tempVal = (globalTempC == DEVICE_DISCONNECTED_C) ? "--" : String(globalTempC, 1);
    
    String tempColor = "#343A40"; 
    if (globalTempC != DEVICE_DISCONNECTED_C) {
        if (globalTempC >= 30) tempColor = "#DC3545";
        else if (globalTempC >= 25) tempColor = "#FFC107";
        else if (globalTempC > 10) tempColor = "#28A745";
        else if (globalTempC > 0) tempColor = "#007BFF";
    }

    chunk = F("<div class='carousel-slide fade'><h2>Temperatura Actual En ");
    chunk += String(settings.description) + F("</h2>");
    chunk += F("<div class='emoji-container'><span class='emoji'>🌡️</span></div>");
    chunk += F("<div style='text-align:center; margin-top: 20px;'>");
    chunk += F("<span style='font-size: 4em; font-weight: bold; color: ") + tempColor + F(";'>") + tempVal + F(" ºC</span>");
    chunk += F("<p>Sensor Interior</p>");
    chunk += F("<p style='font-size: 0.8em; color: var(--text-secondary); margin-top: 10px;'>Actualizado hace ") + String(diff) + F("s</p></div></div>");
    server.sendContent(chunk);

    // 3. Slide 2: Datos del Tiempo
    chunk = F("<div class='carousel-slide fade'><h2>Datos del Tiempo</h2>");
    chunk += F("<div class='emoji-container'><span id='weather-main-icon' class='emoji'>☁️</span></div>");
    chunk += F("<div id='weather-data' style='text-align: center; margin-top: 20px;'><p>Cargando datos...</p></div>");
    chunk += F("<button onclick='fetchWeather()' class='button' style='margin-top:10px; width:auto; padding: 5px 10px; font-size: 0.8em;'>Actualizar</button></div>");
    server.sendContent(chunk);

    // 4. Slide 3: Estado del Dispositivo
    chunk = F("<div class='carousel-slide fade'><h2>Estado del Dispositivo</h2>");
    chunk += F("<div class='emoji-container'><span class='emoji'>📟</span></div><h3>");
    chunk += F("<strong>🖥️ Hostname:</strong> ") + WiFi.hostname() + F("<br>");
    chunk += F("<strong>💾 Firmware:</strong> ") + String(FIRMWARE_VERSION) + F("<br>");
    chunk += F("<strong>🏠 IP:</strong> ") + WiFi.localIP().toString() + F("<br>");
    chunk += F("<strong>📶 Señal:</strong> ") + String(WiFi.RSSI()) + F(" dBm<br>");
    chunk += F("<strong>🆔 MAC:</strong> ") + WiFi.macAddress() + F("<br>");
    chunk += F("<strong>🧠 Heap Libre:</strong> ") + String(ESP.getFreeHeap() / 1024) + F(" KB<br>");
    chunk += F("<strong>⚡ Activo:</strong> ") + getUptime() + F("</h3></div>");
    server.sendContent(chunk);

    // 5. Slide 4: Configuración
    int iv = settings.interval_minutes;
    bool is_manual = (iv!=1 && iv!=15 && iv!=30 && iv!=60 && iv!=360 && iv!=720 && iv!=1440);

    chunk = F("<div class='carousel-slide fade'><h2>Configuración</h2><div class='emoji-container'><span class='emoji'>⚙️</span></div>");
    chunk += F("<form action='/save' method='POST' style='padding: 0 10px;'>");
    
    // Grupo: Desc y Host en la misma línea
    chunk += F("<div style='display: flex; gap: 10px;'>");
    chunk += F("<div style='flex: 1;'><label>Descripción:</label><input type='text' name='desc' value='") + String(settings.description) + F("' maxlength='50' placeholder='Ej: Casa'></div>");
    chunk += F("<div style='flex: 1;'><label>Servidor (Host):</label><input type='text' name='host' value='") + String(settings.host) + F("'></div>");
    chunk += F("</div>");

    chunk += F("<div style='display: flex; gap: 10px;'>");
    chunk += F("<div style='flex: 1;'><label>Contrase&ntilde;a OTA:</label><input type='text' name='ota_pass' value='") + String(settings.ota_password) + F("' maxlength='20'></div>");
    chunk += F("<div style='flex: 1;'><label>Protocolo:</label><select name='protocol'>");
    chunk += F("<option value='0' ") + String(settings.use_https ? "" : "selected") + F(">HTTP</option>");
    chunk += F("<option value='1' ") + String(settings.use_https ? "selected" : "") + F(">HTTPS</option></select></div>");
    chunk += F("</div>");
    
    chunk += F("<label>Intervalo de Reporte:</label><select name='interval_opt' id='interval_opt' onchange='toggleManual()'>");
    chunk += String(F("<option value='1' ")) + (iv==1?"selected":"") + F(">1 Minuto</option>");
    chunk += String(F("<option value='15' ")) + (iv==15?"selected":"") + F(">15 Minutos</option>");
    chunk += String(F("<option value='30' ")) + (iv==30?"selected":"") + F(">30 Minutos</option>");
    chunk += String(F("<option value='60' ")) + (iv==60?"selected":"") + F(">1 Hora</option>");
    chunk += String(F("<option value='360' ")) + (iv==360?"selected":"") + F(">6 Horas</option>");
    chunk += String(F("<option value='720' ")) + (iv==720?"selected":"") + F(">12 Horas</option>");
    chunk += String(F("<option value='1440' ")) + (iv==1440?"selected":"") + F(">24 Horas</option>");
    chunk += String(F("<option value='manual' ")) + (is_manual?"selected":"") + F(">Ingreso Manual</option></select>");
    
    chunk += F("<div style='display: flex; gap: 10px; margin-top: 15px; align-items: center;'>");
    chunk += F("<div id='manual_div' style='display: ") + String(is_manual ? "block" : "none") + F("; flex: 1;'>");
    chunk += F("<input type='number' name='interval_val' value='") + String(iv) + F("' min='1' max='1440' placeholder='Minutos' style='margin: 0;'></div>");
    chunk += F("<button type='submit' class='button' style='flex: 1; margin: 0;'>Guardar</button></div></form></div>");
    server.sendContent(chunk);

    // 6. Cierre y Scripts
    chunk = F("</div><div class='dots'><span class='dot' onclick='currentSlide(1)'></span><span class='dot' onclick='currentSlide(2)'></span>");
    chunk += F("<span class='dot' onclick='currentSlide(3)'></span><span class='dot' onclick='currentSlide(4)'></span></div></div>");
    chunk += F("<script src='script.js'></script></body></html>");
    server.sendContent(chunk);
}

void handleSave() {
  if (server.hasArg("desc")) strncpy(settings.description, server.arg("desc").c_str(), 50);
  if (server.hasArg("host")) strncpy(settings.host, server.arg("host").c_str(), 63);
  if (server.hasArg("ota_pass")) strncpy(settings.ota_password, server.arg("ota_pass").c_str(), 20);
  if (server.hasArg("protocol")) settings.use_https = (server.arg("protocol").toInt() == 1);
  
  if (server.hasArg("interval_opt")) {
    String opt = server.arg("interval_opt");
    if (opt == "manual") {
      if (server.hasArg("interval_val")) settings.interval_minutes = constrain(server.arg("interval_val").toInt(), 1, 1440);
    } else {
      settings.interval_minutes = opt.toInt();
    }
  }
  
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

  // OTA Setup
  ArduinoOTA.setHostname(hostName.c_str());
  ArduinoOTA.setPassword(settings.ota_password);
  ArduinoOTA.begin();

  // Configurar Servidor Web
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/style.css", []() { server.send(200, "text/css", STYLE_CSS); });
  server.on("/script.js", []() { server.send(200, "application/javascript", SCRIPT_JS); });
  server.begin();
  Serial.println("HTTP server started");
}
 
// --- Tareas Modulares ---

void updateSensors() {
    sensors1.requestTemperatures();
    float t = sensors1.getTempCByIndex(0);
    
    if (t != DEVICE_DISCONNECTED_C) {
        globalTempC = t;
        last_success_temp_millis = millis();
    } else {
         Serial.println("⚠️ Sensor: Lectura fallida (Dispositivo desconectado o error)");
    }
}

void sendReport() {
    // Solo reportamos si tenemos una lectura válida reciente o valor en memoria
    if (globalTempC == DEVICE_DISCONNECTED_C) {
      Serial.println("❌ Omitiendo reporte: No hay temperatura válida.");
      return;
    }

    float celsius1 = globalTempC;
    float celsius2 = 0; 
    
    Serial.print("🌡️ Reportando Temp: "); Serial.println(celsius1);
   
    // Construcción eficiente de URL
    String txtUrl = "/wifisensor/carga.php/?sn=" + String(serial_number) + 
                    "&s1=" + String(celsius1, 1) + 
                    "&s2=" + String(celsius2, 1);
  
    Serial.print("Requesting URL: "); Serial.println(txtUrl);

    WiFiClient client;
    WiFiClientSecure clientSecure;
    
    if (settings.use_https) {
        clientSecure.setInsecure();
    }
  
    // Serial.print("Connecting to: "); Serial.println(settings.host);
    
    bool connected = false;
    Stream* stream;

    if (settings.use_https) {
        connected = clientSecure.connect(settings.host, 443);
        stream = &clientSecure;
    } else {
        connected = client.connect(settings.host, 80);
        stream = &client;
    }
  
    if (!connected) {
      Serial.println("❌ Conexión Fallida");
      return;
    }
    
    // Envio de Petición
    stream->print(String("GET ") + txtUrl + " HTTP/1.1\r\n" +
                 "Host: " + settings.host + "\r\n" +
                 "User-Agent: ESP8266WifiSensor/" + String(FIRMWARE_VERSION) + "\r\n" +
                 "Connection: close\r\n\r\n");
  
    Serial.println("✅ Request sent");
    
    // Timeout para respuesta
    unsigned long timeout = millis();
    while (settings.use_https ? clientSecure.connected() : client.connected()) {
      if (millis() - timeout > 5000) {
        Serial.println("⚠️ Timeout esperando respuesta");
        break; 
      }
      if (stream->available()) {
          String line = stream->readStringUntil('\n');
          if (line == "\r") break; // Fin de headers
      }
    }
    
    // Leer respuesta (opcional)
    if (stream->available()) {
      String line = stream->readStringUntil('\n');
      Serial.print("Reply: "); Serial.println(line);
    }
}
 
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  unsigned long currentMillis = millis();

  // Tarea 1: Lectura de Sensores (Cada 10 segundos)
  if (currentMillis - last_sensor_read >= 10000 || last_sensor_read == 0) {
    last_sensor_read = currentMillis;
    updateSensors();
  }
  
  // Tarea 2: Reporte al Servidor (Intervalo Configurable)
  unsigned long interval_ms = (unsigned long)settings.interval_minutes * 60000;
  
  if (currentMillis - last_report_time >= interval_ms) {
    last_report_time = currentMillis;
    sendReport();
  }
}
