# 🌡️ Sensor WiFi ESP8266 (WifiSensor)

[Ver código fuente](Esp8266_WifiSensor.ino)

## 1. Descripción 📝

**WifiSensor** es un sistema de monitoreo de temperatura basado en el microcontrolador **ESP8266**. Utiliza sensores **DS18B20** para leer la temperatura ambiente y enviarla periódicamente a un servidor remoto.

El dispositivo cuenta con una interfaz web moderna y responsiva integrada (alojada en la memoria flash) que permite visualizar el estado en tiempo real, consultar datos meteorológicos externos y realizar configuraciones sin necesidad de reflashear el código.

## 2. Características Principales ✨

*   **Estabilidad y Rendimiento:** Arquitectura modular con manejo de memoria optimizado (Chunked Responses) para evitar reinicios por fragmentación de RAM.
*   **Conectividad WiFi Inteligente:** Utiliza **WiFiManager** para configurar la red WiFi sin hardcodear credenciales.
*   **Interfaz Web Integrada:** Dashboard con carrusel de tarjetas para visualizar temperatura, clima externo, estado y configuración.
*   **Personalización Dinámica:** La interfaz muestra el nombre del lugar configurado (ej. "Temperatura Actual En Cocina") y adapta los iconos del clima según el reporte meteorológico.
*   **Lectura Desacoplada:** El sensor monitorea la temperatura cada 10 segundos para la UI local, independientemente del intervalo de reporte al servidor.
*   **Datos Meteorológicos:** Módulo integrado que consulta una API local para mostrar el estado del tiempo con iconos dinámicos.
*   **Soporte HTTP/HTTPS:** Capacidad de enviar reportes tanto a servidores seguros como estándar.
*   **Modo Oscuro:** Interfaz web con soporte nativo para temas claros y oscuros.

## 3. Hardware Requerido 🛠️

Para montar este proyecto necesitas:

*   Placa de desarrollo basada en **ESP8266** (Wemos D1 Mini, NodeMCU, etc.).
*   Sensor de temperatura **DS18B20**.
*   Resistencia de **4.7kΩ** (pull-up para la línea de datos del sensor).
*   Cables de conexión y fuente de alimentación USB.

### 📌 Pinout

| Componente | Pin ESP8266 | Descripción |
| :--- | :--- | :--- |
| **DS18B20 Data** | D4 (GPIO2) | Pin de datos del sensor |
| **VCC** | 3.3V | Alimentación |
| **GND** | G | Tierra |

## 4. Dependencias 📚

Asegúrate de tener instaladas las siguientes librerías en tu Arduino IDE:

```text
ESP8266WiFi
ESP8266WebServer
WiFiManager
OneWire
DallasTemperature
EEPROM
```

## 5. Configuración y Uso ⚙️

1.  **Primer Inicio:** Al encender por primera vez, el dispositivo creará una red WiFi llamada `WifiSensor-XXXX`.
2.  **Portal Captivo:** Conéctate a la red y configura tu WiFi doméstica.
3.  **Dashboard:** Accede a la IP del dispositivo.
4.  **Ajustes:** En la pestaña "Configuración":
    *   **Descripción:** Nombre amigable para identificar el sensor (ej: "Sótano").
    *   **Host:** Servidor de destino para los reportes.
    *   **Intervalo:** Frecuencia de envío de datos (1 min - 24 hs).

## 6. API de Reporte 📡

El dispositivo realiza una petición **GET** al servidor configurado:

```http
GET /wifisensor/carga.php/?sn={MAC_ADDRESS}&s1={TEMPERATURA}&s2=0 HTTP/1.1
Host: {HOST_CONFIGURADO}
```

---
**Nota:** El sistema reintenta la conexión si falla y posee mecanismos de reinicio en caso de pérdida prolongada de conectividad.