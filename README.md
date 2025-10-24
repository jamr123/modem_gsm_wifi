# 📡 ESP32-S3 SIM7080G LTE/GSM Módem

[![Version](https://img.shields.io/badge/version-3.0-blue.svg)](https://github.com/jamr123/modem_gsm_wifi)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Modem](https://img.shields.io/badge/modem-SIM7080G-orange.svg)](https://www.simcom.com/product/SIM7080G.html)
[![License](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)

Sistema minimalista y optimizado de comunicación LTE/GSM para ESP32-S3 utilizando módem SIM7080G con conexiones TCP persistentes.

## 🚀 Características Principales

- ✅ **Conexión TCP Persistente** con keep-alive automático
- ✅ **Configuración Rápida** del módem (40-50% más rápido)
- ✅ **Reconexión Automática** en caso de fallas de red
- ✅ **Sistema de Diagnóstico** completo del módem
- ✅ **Logging Estructurado** con control de niveles
- ✅ **Código Limpio** sin dependencias innecesarias
- ✅ **Soporte CAT-M/NB-IoT** optimizado para SIM7080G

## 📋 Tabla de Contenidos

- [🔧 Instalación](#-instalación)
- [🔌 Conexiones de Hardware](#-conexiones-de-hardware)
- [⚙️ Configuración](#️-configuración)
- [🚀 Uso Rápido](#-uso-rápido)
- [📡 Comandos Serie](#-comandos-serie)
- [📁 Estructura del Proyecto](#-estructura-del-proyecto)
- [🛠️ API Reference](#️-api-reference)
- [🐛 Troubleshooting](#-troubleshooting)
- [📜 Changelog](#-changelog)

## 🔧 Instalación

### Prerequisitos

- **Arduino IDE** 2.0+ o **PlatformIO**
- **ESP32 Arduino Core** 2.0.0+
- **Librería TinyGSM** 0.11.5+

### Dependencias

```bash
# Arduino IDE - Instalar via Library Manager
TinyGSM by Volodymyr Shymanskyy
```

### Clonar Repositorio

```bash
git clone https://github.com/jamr123/modem_gsm_wifi.git
cd modem_gsm_wifi
```

## 🔌 Conexiones de Hardware

### Diagrama de Conexiones

```
ESP32-S3          SIM7080G
┌─────────┐      ┌──────────┐
│  GPIO10 │─────▶│ RX       │
│  GPIO11 │◀─────│ TX       │
│  GPIO9  │─────▶│ PWRKEY   │
│  GPIO12 │      │          │ (LED Status)
│  3.3V   │─────▶│ VCC      │
│  GND    │─────▶│ GND      │
└─────────┘      └──────────┘
```

### Lista de Conexiones

| ESP32-S3 Pin | SIM7080G Pin | Función | Notas |
|-------------|-------------|---------|--------|
| GPIO 10 | RX | UART TX | Datos hacia módem |
| GPIO 11 | TX | UART RX | Datos desde módem |
| GPIO 9 | PWRKEY | Control | Pin de encendido |
| GPIO 12 | - | LED | Indicador de estado |
| 3.3V | VCC | Alimentación | 3.3-4.2V |
| GND | GND | Tierra | Común |

### ⚠️ Notas Importantes

- Asegurar alimentación estable para el SIM7080G (mínimo 2A)
- Conectar antena LTE antes de energizar
- Verificar que la SIM tenga plan de datos activo

## ⚙️ Configuración

### Parámetros Configurables

```cpp
// gsmlte.h - Configuración principal
#define DB_SERVER_IP "dp01.lolaberries.com.mx"  // Servidor TCP
#define TCP_PORT "12607"                         // Puerto TCP
#define APN "\"em\""                            // APN de la operadora

// Configuración UART
#define UART_BAUD 115200
#define PIN_TX 10
#define PIN_RX 11
#define PWRKEY_PIN 9

// Timeouts optimizados
#define SHORT_DELAY 300
#define LONG_DELAY 1000
```

### Configuración de Red

El sistema está configurado para:
- **Modo de Red**: CAT-M (modo 38)
- **Bandas**: 2, 4, 5 (CAT-M)
- **APN**: "em" (Telcel México)
- **Contexto PDP**: Automático

## 🚀 Uso Rápido

### Código Básico

```cpp
#include "gsmlte.h"

void setup() {
  Serial.begin(115200);
  
  // Configurar keep-alive cada 30 segundos
  tcpConfigurePersistent(30000);
  
  // Inicializar módem
  setupModem();
  
  Serial.println("Sistema listo!");
}

void loop() {
  // Mantener conexión TCP activa
  tcpMaintainPersistent();
  
  // Enviar datos si está conectado
  if (tcpIsPersistentActive()) {
    String datos = "Hola desde ESP32: " + String(millis());
    tcpSendPersistent(datos, 5000);
  }
  
  delay(60000); // Enviar cada minuto
}
```

### Compilar y Subir

1. Abrir `modem_gsm_wifi.ino` en Arduino IDE
2. Seleccionar placa: **ESP32S3 Dev Module**
3. Configurar puerto serie correspondiente
4. Compilar y subir código
5. Abrir Monitor Serie a 115200 baud

## 📡 Comandos Serie

Una vez cargado el sketch, puedes usar estos comandos en el Monitor Serie:

| Comando | Descripción | Ejemplo de Salida |
|---------|-------------|------------------|
| `status` | Estado de conexiones | `TCP: OK, Señal: 18, ICCID: 89521...` |
| `send` | Forzar envío inmediato | `Enviando datos...` |
| `test` | Mensaje de prueba | `Enviando TEST_MESSAGE` |
| `diag` | Diagnóstico completo | `=== DIAGNÓSTICO DEL MÓDEM ===` |
| `restart` | Reiniciar módem | `=== REINICIANDO MÓDEM ===` |
| `fast` | Modo configuración rápida | `=== MODO RÁPIDO ACTIVADO ===` |

### Ejemplo de Sesión

```
=== ESP32-S3 Módem LTE/GSM ===
Iniciando módem...
[2345ms] INFO: 🚀 Iniciando configuración del módem LTE/GSM
[5678ms] INFO: ✅ Comunicación AT establecida con SIM7080G
[8901ms] INFO: ✅ Conexión LTE establecida, iniciando TCP persistente
[9234ms] INFO: ✅ Conexión TCP persistente establecida
Sistema iniciado

> status
TCP: OK
Señal: 18
ICCID: 8952102012345678901

> test
Enviando TEST_MESSAGE por TCP persistente
[12345ms] INFO: ✅ Datos enviados exitosamente por TCP persistente
```

## 📁 Estructura del Proyecto

```
modem_gsm_wifi/
├── README.md                 # Este archivo
├── gsmlte.h                  # Header principal con declaraciones
├── gsmlte.cpp                # Implementación principal
└── modem_gsm_wifi.ino        # Sketch de demostración
```

### Descripción de Archivos

- **`gsmlte.h`**: Declaraciones de funciones, constantes y configuración
- **`gsmlte.cpp`**: Implementación completa de todas las funciones
- **`modem_gsm_wifi.ino`**: Sketch ejemplo con comandos interactivos

## 🛠️ API Reference

### Funciones Principales

#### `void setupModem()`
Configura e inicializa el módem completo.
```cpp
setupModem(); // Llamar una vez en setup()
```

#### `bool tcpInitPersistent()`
Inicializa conexión TCP persistente.
```cpp
if (tcpInitPersistent()) {
  Serial.println("TCP conectado!");
}
```

#### `bool tcpSendPersistent(String datos, uint32_t timeout)`
Envía datos por TCP persistente.
```cpp
tcpSendPersistent("Hola Mundo", 5000);
```

#### `void tcpMaintainPersistent()`
Mantiene conexión TCP activa (llamar en loop).
```cpp
void loop() {
  tcpMaintainPersistent();
  // ... resto del código
}
```

#### `bool tcpIsPersistentActive()`
Verifica si TCP está conectado.
```cpp
if (tcpIsPersistentActive()) {
  // Enviar datos
}
```

### Variables Globales

```cpp
extern bool tcpConnected;              // Estado de conexión TCP
extern String iccidsim0;              // ICCID de la SIM
extern int signalsim0;                // Calidad de señal (0-31)
extern bool modemInitialized;         // Estado del módem
```

## 🐛 Troubleshooting

### Problemas Comunes

#### ❌ Módem no enciende
```
Síntoma: "Sin respuesta AT del SIM7080G"
Solución:
- Verificar conexión PWRKEY (GPIO 9)
- Asegurar alimentación suficiente (2A+)
- Verificar conexiones UART (TX/RX)
```

#### ❌ No conecta a la red
```
Síntoma: "Fallo en conexión LTE"
Solución:
- Verificar SIM con plan de datos activo
- Conectar antena LTE correctamente
- Verificar cobertura de red en ubicación
```

#### ❌ TCP no se conecta
```
Síntoma: "Falló inicialización de conexión TCP"
Solución:
- Verificar configuración del servidor
- Comprobar firewall/puerto 12607
- Verificar APN correcto para operadora
```

### Comandos de Diagnóstico

```cpp
// Usar en Monitor Serie
diag        // Diagnóstico completo
status      // Estado actual
restart     // Reiniciar módem
```

### Logs Detallados

Habilitar debug completo modificando:
```cpp
modemConfig.enableDebug = true;
```

## 📜 Changelog

### v3.0 (2025-10-23) - Versión Optimizada
- ✅ **Optimización mayor**: 40-50% más rápido
- ✅ **Limpieza de código**: Eliminados comentarios innecesarios
- ✅ **Documentación completa**: Doxygen actualizado
- ✅ **Timeouts optimizados**: Configuración más rápida
- ✅ **Sistema de diagnóstico**: Función `diagnosticoModem()`

### v2.0 (2025-10-23) - TCP Persistente
- ✅ Implementación de TCP persistente
- ✅ Keep-alive automático
- ✅ Sistema de reconexión robusta
- ✅ Eliminación de funciones GPS/sensores

### v1.0 (2025-10-23) - Versión Inicial
- ✅ Configuración básica SIM7080G
- ✅ Comunicación AT básica
- ✅ Conexión LTE/CAT-M

---

## 👨‍💻 Autor

**Jorge Meza**
- GitHub: [@jamr123](https://github.com/jamr123)
- Proyecto: [modem_gsm_wifi](https://github.com/jamr123/modem_gsm_wifi)

## 📄 Licencia

Este proyecto está bajo la Licencia MIT - ver el archivo [LICENSE](LICENSE) para detalles.

---

### ⭐ Si este proyecto te ayudó, ¡dale una estrella en GitHub!

```
🌟 Star este repo: https://github.com/jamr123/modem_gsm_wifi
```