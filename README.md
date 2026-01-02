# 🌡️ Sensor WiFi ESP8266 (CavaWiFi)

[Ver código fuente](Esp8266_WifiSensor.ino)

## 1. Descripción 📝

**CavaWiFi** es un sistema de monitoreo de temperatura basado en el microcontrolador **ESP8266**. Utiliza sensores **DS18B20** para leer la temperatura ambiente y enviarla periódicamente a un servidor remoto.

El dispositivo cuenta con una interfaz web moderna y responsiva integrada (alojada en la memoria flash) que permite visualizar el estado en tiempo real y realizar configuraciones sin necesidad de reflashear el código.

## 2. Características Principales ✨

*   **Conectividad WiFi Inteligente:** Utiliza **WiFiManager** para configurar la red WiFi sin hardcodear credenciales. Si no conecta, crea un punto de acceso (AP) para su configuración.
*   **Interfaz Web Integrada:** Dashboard con carrusel de tarjetas para visualizar temperatura, estado y configuración.
*   **Soporte HTTP/HTTPS:** Capacidad de enviar reportes tanto a servidores seguros como estándar.
*   **Intervalo Configurable:** Frecuencia de reporte ajustable desde 1 minuto hasta 24 horas (o manual).
*   **Persistencia de Datos:** Guarda la configuración (Host, Protocolo, Intervalo) en la memoria EEPROM.
*   **Modo Oscuro:** La interfaz web detecta automáticamente la preferencia de color del sistema.

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
```

## 5. Configuración y Uso ⚙️

1.  **Primer Inicio:** Al encender por primera vez, el dispositivo creará una red WiFi llamada `WifiSensor-XXXX`. Conéctate a ella con tu celular o PC.
2.  **Portal Captivo:** Se abrirá automáticamente una ventana para seleccionar tu red WiFi doméstica e ingresar la contraseña.
3.  **Dashboard:** Una vez conectado a tu red, accede a la IP asignada (puedes verla en el Monitor Serie o en tu router).
4.  **Ajustes:** En la pestaña "Configuración" de la web, define:
    *   **Host:** Servidor donde se enviarán los datos (ej: `miservidor.com`).
    *   **Protocolo:** HTTP o HTTPS.
    *   **Intervalo:** Cada cuánto tiempo enviar el reporte.

## 6. API de Reporte 📡

El dispositivo realiza una petición **GET** al servidor configurado con el siguiente formato:

```http
GET /cava/carga.php/?sn={MAC_ADDRESS}&s1={TEMPERATURA}&s2=0 HTTP/1.1
Host: {HOST_CONFIGURADO}
```

*   **sn:** Número de serie (Dirección MAC del ESP8266).
*   **s1:** Temperatura en grados Celsius.
*   **s2:** Reservado para segundo sensor (actualmente envía 0).

---
**Nota:** El sistema reintenta la conexión si falla y posee mecanismos de reinicio en caso de pérdida prolongada de conectividad.
