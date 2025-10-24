/**
 * @file gsmlte.h
 * @brief Sistema minimalista de comunicación LTE/GSM para ESP32-S3 con módem SIM7080G
 * @author Jorge Meza
 * @date 2025-10-23
 * @version 3.0
 * 
 * @details Este módulo proporciona funcionalidad optimizada para comunicación 
 * LTE/GSM utilizando el módem SIM7080G con TCP persistente. Incluye soporte 
 * para CAT-M/NB-IoT, configuración automatizada y sistema de reconexión robusta.
 * 
 * Características principales:
 * - Conexión TCP persistente con keep-alive automático
 * - Configuración rápida y optimizada del módem
 * - Sistema de reconexión automática en caso de fallas
 * - Logging estructurado con niveles de debug
 * - Diagnóstico completo del estado del módem
 * - Código limpio sin dependencias innecesarias
 * 
 * @note Requiere librería TinyGSM para comunicación con el módem
 * @warning Este código está optimizado para SIM7080G específicamente
 * 
 * @example
 * @code
 * #include "gsmlte.h"
 * 
 * void setup() {
 *   Serial.begin(115200);
 *   tcpConfigurePersistent(30000);  // Keep-alive cada 30s
 *   setupModem();                   // Configurar módem
 * }
 * 
 * void loop() {
 *   tcpMaintainPersistent();        // Mantener conexión
 *   if (tcpIsPersistentActive()) {
 *     tcpSendPersistent("datos", 5000);
 *   }
 *   delay(5000);
 * }
 * @endcode
 */

#ifndef GSMLTE_H
#define GSMLTE_H

#include <stdint.h>
#include "Arduino.h"

#define UART_BAUD 115200
#define PIN_TX 10
#define PIN_RX 11
#define PWRKEY_PIN 9
#define LED_PIN 12

#define SEND_RETRIES 6
#define SHORT_DELAY 300
#define LONG_DELAY 1000
#define MODEM_PWRKEY_DELAY 2000
#define MODEM_STABILIZE_DELAY 2000

#define DB_SERVER_IP "dp01.lolaberries.com.mx"
#define TCP_PORT "12607"

#define MODEM_NETWORK_MODE 38
#define CAT_M 1
#define NB_IOT 2
#define CAT_M_NB_IOT 3

#define TINY_GSM_MODEM_SIM7080
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_YIELD_MS 10
#define SerialAT Serial1
#define SerialMon Serial
#define PDP_CONTEXT 1
#define APN "\"em\""



/**
 * @struct ModemConfig
 * @brief Estructura de configuración dinámica del módem
 */
struct ModemConfig {
  String serverIP;
  String serverPort;
  String apn;
  int networkMode;
  int bandMode;
  int maxRetries;
  unsigned long baseTimeout;
  bool enableDebug;
};

extern String iccidsim0;
extern int signalsim0;
extern bool modemInitialized;

extern bool tcpConnected;
extern unsigned long lastTcpActivity;
extern unsigned long tcpKeepAliveInterval;
extern int tcpReconnectAttempts;



/**
 * @brief Sistema de logging estructurado con niveles
 * @param level Nivel de log (0=Error, 1=Warning, 2=Info, 3=Debug)
 * @param message Mensaje a loguear
 */
void logMessage(int level, const String& message);

/**
 * @brief Inicializa la configuración del módem
 */
void initModemConfig();

/**
 * @brief Obtiene timeout adaptativo según calidad de señal
 * @return Timeout en milisegundos
 */
unsigned long getAdaptiveTimeout();

/**
 * @brief Limpia el buffer serial del módem
 */
void flushPortSerial();

/**
 * @brief Envía comando AT y espera respuesta
 * @param command Comando AT a enviar
 * @param expectedResponse Respuesta esperada
 * @param timeout Timeout en milisegundos
 * @return true si se recibe la respuesta esperada
 */
bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout);

/**
 * @brief Inicia comunicación GSM/LTE con el módem
 */
void startGsm();

/**
 * @brief Diagnóstico completo del estado del módem
 */
void diagnosticoModem();

/**
 * @brief Controla el pin de encendido del módem
 */
void modemPwrKeyPulse();

/**
 * @brief Configura e inicializa el módem LTE/GSM
 */
void setupModem();

/**
 * @brief Establece conexión LTE
 * @return true si la conexión es exitosa
 */
bool startLTE();



/**
 * @brief Envía datos TCP con gestión robusta de errores
 * @param datos Datos a enviar
 * @param timeout_ms Timeout en milisegundos
 * @return true si el envío es exitoso
 */
bool tcpSendData(const String& datos, uint32_t timeout_ms);



/**
 * @brief Inicializa la conexión TCP persistente al servidor configurado
 * @details Establece una conexión TCP que se mantiene activa para futuras 
 * comunicaciones. Utiliza comandos AT del SIM7080G para crear la conexión.
 * @return true si la conexión se establece exitosamente, false en caso contrario
 * @note La conexión utiliza los parámetros configurados en ModemConfig
 * @see tcpConfigurePersistent(), tcpMaintainPersistent()
 */
bool tcpInitPersistent();

/**
 * @brief Verifica si la conexión TCP persistente está activa
 * @details Consulta el estado de la conexión TCP mediante comandos AT y 
 * actualiza el estado interno de la conexión.
 * @return true si la conexión está activa y funcional, false si está desconectada
 * @note Esta función actualiza lastTcpActivity si la conexión está activa
 */
bool tcpIsPersistentActive();

/**
 * @brief Mantiene la conexión TCP persistente enviando keep-alive
 * @details Envía paquetes de keep-alive periódicos según el intervalo configurado
 * para evitar que el servidor cierre la conexión por inactividad.
 * @return true si la conexión sigue activa, false si se perdió
 * @note Solo envía keep-alive si ha pasado el tiempo del intervalo configurado
 * @see tcpConfigurePersistent()
 */
bool tcpKeepAlivePersistent();

/**
 * @brief Reconecta la conexión TCP persistente si se perdió
 * @details Intenta reestablecer la conexión TCP si se detectó una desconexión.
 * Incluye lógica de reintentos con límite máximo.
 * @return true si se reconectó exitosamente, false si falló la reconexión
 * @note Incrementa el contador de intentos de reconexión
 */
bool tcpReconnectPersistent();

/**
 * @brief Envía datos usando la conexión TCP persistente
 * @param datos Datos a enviar
 * @param timeout_ms Timeout en milisegundos
 * @return true si el envío es exitoso
 */
bool tcpSendPersistent(const String& datos, uint32_t timeout_ms);

/**
 * @brief Cierra la conexión TCP persistente
 */
void tcpClosePersistent();

/**
 * @brief Gestiona el mantenimiento de la conexión TCP persistente
 * @brief Debe llamarse periódicamente desde el loop principal
 */
void tcpMaintainPersistent();

/**
 * @brief Configura parámetros de la conexión TCP persistente
 * @param keepAliveIntervalMs Intervalo de keep-alive en milisegundos
 */
void tcpConfigurePersistent(unsigned long keepAliveIntervalMs);

/**
 * @brief Obtiene información de la tarjeta SIM y calidad de señal
 */
void getIccid();







#endif
