/**
 * @file gsmlte.cpp
 * @brief Implementación optimizada del sistema de comunicación LTE/GSM para SIM7080G
 * @author Jorge Meza
 * @date 2025-10-23
 * @version 3.0
 * 
 * @details Implementación completa y optimizada para módem SIM7080G con:
 * - Conexiones TCP persistentes con keep-alive automático
 * - Sistema de reconexión robusta con límites de reintentos
 * - Configuración automatizada y rápida del módem
 * - Diagnóstico completo del estado del hardware
 * - Logging estructurado con control de niveles
 * - Código limpio sin dependencias innecesarias
 * 
 * @note Optimizado específicamente para ESP32-S3 + SIM7080G
 * @warning Requiere conexiones de hardware correctas y SIM con datos activos
 */

#include "gsmlte.h"
#include <TinyGsmClient.h>

TinyGsm modem(SerialAT);
ModemConfig modemConfig;

bool modemInitialized = false;
int consecutiveFailures = 0;

bool tcpConnected = false;
unsigned long lastTcpActivity = 0;
unsigned long tcpKeepAliveInterval = 30000;
int tcpReconnectAttempts = 0;
const int MAX_RECONNECT_ATTEMPTS = 3;

String iccidsim0 = "";
int signalsim0 = 0;

void initModemConfig() {
  modemConfig.serverIP = DB_SERVER_IP;
  modemConfig.serverPort = TCP_PORT;
  modemConfig.apn = APN;
  modemConfig.networkMode = MODEM_NETWORK_MODE;
  modemConfig.bandMode = CAT_M;
  modemConfig.maxRetries = SEND_RETRIES;
  modemConfig.baseTimeout = 5000;  
  modemConfig.enableDebug = true;
  
  logMessage(2, "🔧 Configuración del módem inicializada");
}

unsigned long getAdaptiveTimeout() {
  unsigned long baseTimeout = modemConfig.baseTimeout;
  
  if (signalsim0 > 15) {
    baseTimeout = 2000;
  } else if (signalsim0 < 5) {
    baseTimeout = 5000;
  } else {
    baseTimeout = 3000;
  }
  
  if (consecutiveFailures > 0) {
    baseTimeout += (consecutiveFailures * 500);
  }
  
  if (baseTimeout > 8000) baseTimeout = 8000;
  if (baseTimeout < 2000) baseTimeout = 2000;
  
  return baseTimeout;
}

void logMessage(int level, const String& message) {
  if (!modemConfig.enableDebug && level > 2) return;
  if (level > 1 && millis() < 30000) return;
  
  String timestamp = String(millis()) + "ms";
  String levelStr;

  switch (level) {
    case 0: levelStr = "ERROR"; break;
    case 1: levelStr = "WARN"; break;
    case 2: levelStr = "INFO"; break;
    case 3: levelStr = "DEBUG"; break;
    default: levelStr = "UNKN"; break;
  }

  Serial.println("[" + timestamp + "] " + levelStr + ": " + message);
}



/**
 * @brief Configura e inicializa el módem LTE/GSM completo
 * @details Ejecuta la secuencia completa de inicialización del módem:
 * 1. Configura parámetros del módem (APN, modo de red, etc.)
 * 2. Inicializa comunicación serial con el módem
 * 3. Ejecuta secuencia de encendido y configuración GSM
 * 4. Obtiene información de SIM y calidad de señal
 * 5. Establece conexión LTE/CAT-M
 * 6. Inicializa conexión TCP persistente
 * 
 * @note Esta función debe llamarse una vez durante setup()
 * @warning Asegurar que el hardware esté correctamente conectado
 */
void setupModem() {
  logMessage(2, "🚀 Iniciando configuración del módem LTE/GSM");

  initModemConfig();

  SerialMon.begin(115200);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  startGsm();
  getIccid();

  if (startLTE() == true) {
    logMessage(2, "✅ Conexión LTE establecida, iniciando TCP persistente");
    
    if (tcpInitPersistent()) {
      logMessage(2, "✅ Conexión TCP persistente establecida");
      consecutiveFailures = 0;
      
      logMessage(2, "🔗 Conexión TCP persistente mantenida para futuras operaciones");
    } else {
      logMessage(1, "⚠️  Fallo estableciendo TCP persistente");
      consecutiveFailures++;
    }
  } else {
    consecutiveFailures++;
    logMessage(1, "⚠️  Fallo en conexión LTE (intento " + String(consecutiveFailures) + ")");
  }

  modemInitialized = true;

  logMessage(2, "🏁 Configuración del módem completada");
}

/**
 * @brief Controla el pin de encendido del módem SIM7080G con secuencia optimizada
 * @details Ejecuta la secuencia específica de encendido para el SIM7080G:
 * 1. Asegura estado inicial LOW del pin PWRKEY
 * 2. Genera pulso HIGH de 2 segundos (requerido por SIM7080G)
 * 3. Retorna a estado LOW y espera estabilización
 * 4. Verifica respuesta básica del módem
 * 
 * @note Los tiempos están optimizados específicamente para SIM7080G
 * @warning El pin PWRKEY debe estar correctamente conectado
 */
void modemPwrKeyPulse() {
  logMessage(3, "🔌 Iniciando secuencia de encendido SIM7080G");
  
  digitalWrite(PWRKEY_PIN, LOW);
  delay(100);
  
  digitalWrite(PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(PWRKEY_PIN, LOW);
  
  logMessage(3, "⏳ Esperando estabilización del módem (3s)...");
  delay(3000);
  
  SerialAT.println("AT");
  delay(500);
  if (SerialAT.available()) {
    String response = SerialAT.readString();
    logMessage(3, "📡 Respuesta inicial: " + response);
  }
  
  logMessage(2, "✅ Secuencia PWRKEY completada");
}

/**
 * Diagnóstico completo del estado del módem SIM7080G
 */
void diagnosticoModem() {
  logMessage(2, "🔍 === DIAGNÓSTICO DEL MÓDEM SIM7080G ===");
  
  logMessage(2, "📡 Verificando comunicación AT...");
  if (modem.testAT(3000)) {
    logMessage(2, "✅ Comunicación AT: OK");
  } else {
    logMessage(0, "❌ Comunicación AT: FALLO");
    return;
  }
  
  String response = "";
  SerialAT.println("ATI");
  delay(300);
  if (SerialAT.available()) {
    response = SerialAT.readString();
    logMessage(2, "📋 Info módem: " + response.substring(0, 50));
  }
  
  if (sendATCommand("+CPIN?", "READY", 3000)) {
    logMessage(2, "✅ SIM Card: READY");
  } else {
    logMessage(0, "❌ SIM Card: NO READY");
  }
  
  SerialAT.println("AT+CFUN?");
  delay(300);
  if (SerialAT.available()) {
    response = SerialAT.readString();
    logMessage(2, "📡 Estado RF: " + response);
  }
  
  SerialAT.println("AT+CREG?");
  delay(300);
  if (SerialAT.available()) {
    response = SerialAT.readString();
    logMessage(2, "🌐 Registro red: " + response);
  }
  
  SerialAT.println("AT+CSQ");
  delay(300);
  if (SerialAT.available()) {
    response = SerialAT.readString();
    logMessage(2, "📶 Calidad señal: " + response);
  }
  
  logMessage(2, "🔍 === FIN DIAGNÓSTICO ===");
}

bool startLTE() {
  logMessage(2, "🌐 Iniciando conexión LTE");

  if (!sendATCommand("+CNMP=" + String(modemConfig.networkMode), "OK", getAdaptiveTimeout())) {
    logMessage(0, "❌ Fallo configurando modo de red");
    return false;
  }

  if (!sendATCommand("+CMNB=" + String(modemConfig.bandMode), "OK", getAdaptiveTimeout())) {
    logMessage(0, "❌ Fallo configurando modo de banda");
    return false;
  }

  if (!sendATCommand("+CBANDCFG=\"CAT-M\",2,4,5", "OK", getAdaptiveTimeout())) {
    logMessage(1, "⚠️  Fallo configurando bandas CAT-M");
  }

  if (!sendATCommand("+CBANDCFG=\"NB-IOT\"", "OK", getAdaptiveTimeout())) {
    logMessage(1, "⚠️  Fallo configurando bandas NB-IoT");
  }

  sendATCommand("+CBANDCFG?", "OK", 2000);
  delay(SHORT_DELAY);

  String pdpCommand = "+CGDCONT=1,\"IP\",\"" + modemConfig.apn + "\"";
  if (!sendATCommand(pdpCommand, "OK", 3000)) {
    logMessage(0, "❌ Fallo configurando contexto PDP");
    return false;
  }

  if (!sendATCommand("+CNACT=0,1", "OK", 3000)) {
    logMessage(0, "❌ Fallo activando contexto PDP");
    return false;
  }

  unsigned long t0 = millis();
  unsigned long maxWaitTime = 45000;  // Reducido de 60s a 45s

  while (millis() - t0 < maxWaitTime) {
    int signalQuality = modem.getSignalQuality();
    logMessage(3, "📶 Calidad de señal: " + String(signalQuality));

    sendATCommand("+CNACT?", "OK", 2000);

    if (modem.isNetworkConnected()) {
      logMessage(2, "✅ Conectado a la red LTE");
      sendATCommand("+CPSI?", "OK", 2000);
      flushPortSerial();
      return true;
    }

    delay(500);  // Reducido de 1000ms a 500ms
  }

  logMessage(0, "❌ Timeout: No se pudo conectar a la red LTE");
  return false;
}

/**
 * Abre conexión TCP con reintentos adaptativos
 * @return true si la conexión se abre exitosamente
 */


/**
 * Limpia todos los buffers de comunicación serial
 */
void flushPortSerial() {
  int bytesCleared = 0;
  while (SerialAT.available()) {
    char c = SerialAT.read();
    bytesCleared++;
  }

  if (bytesCleared > 0 && modemConfig.enableDebug) {
    logMessage(3, "🧹 Limpiados " + String(bytesCleared) + " bytes del buffer serial");
  }
}

/**
 * Lee respuesta del módem con timeout adaptativo
 * @param timeout - Timeout base en milisegundos
 * @return Respuesta del módem como String
 */
String readResponse(unsigned long timeout) {
  unsigned long start = millis();
  String response = "";
  unsigned long adaptiveTimeout = getAdaptiveTimeout();

  unsigned long finalTimeout = (timeout > adaptiveTimeout) ? timeout : adaptiveTimeout;

  flushPortSerial();

  while (millis() - start < finalTimeout) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
    }
  }

  if (modemConfig.enableDebug) {
    logMessage(3, "📥 Respuesta recibida (" + String(response.length()) + " bytes): " + response);
  }

  return response;
}

/**
 * Envía comando AT y espera respuesta específica
 * @param command - Comando AT a enviar
 * @param expectedResponse - Respuesta esperada
 * @param timeout - Timeout en milisegundos
 * @return true si se recibe la respuesta esperada
 */
bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout) {
  logMessage(3, "📤 Enviando comando AT: " + command);

  String response = "";
  unsigned long start = millis();
  unsigned long adaptiveTimeout = getAdaptiveTimeout();
  unsigned long finalTimeout = (timeout > adaptiveTimeout) ? timeout : adaptiveTimeout;

  flushPortSerial();

  modem.sendAT(command);

  while (millis() - start < finalTimeout) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
      if (modemConfig.enableDebug) {
        Serial.print(c);
      }
    }
  }

  if (response.indexOf(expectedResponse) != -1) {
    logMessage(3, "✅ Comando AT exitoso: " + command);
    return true;
  }

  logMessage(1, "⚠️  Comando AT falló: " + command + " (esperaba: " + expectedResponse + ")");
  return false;
}

/**
 * Busca texto específico en una cadena principal
 * @param textoPrincipal - Texto donde buscar
 * @param textoBuscado - Texto a buscar
 * @return true si se encuentra el texto
 */




/**
 * Obtiene información de la tarjeta SIM y calidad de señal
 */
void getIccid() {
  logMessage(2, "📱 Obteniendo información de la tarjeta SIM");

  flushPortSerial();

  for (int i = 0; i < 3; i++) {
    iccidsim0 = modem.getSimCCID();
    signalsim0 = modem.getSignalQuality();
    delay(300);  
  }

  logMessage(2, "📱 ICCID: " + iccidsim0);
  logMessage(2, "📶 Calidad de señal: " + String(signalsim0));

  if (signalsim0 >= 20) {
    logMessage(2, "✅ Señal excelente");
  } else if (signalsim0 >= 15) {
    logMessage(2, "✅ Señal buena");
  } else if (signalsim0 >= 10) {
    logMessage(1, "⚠️  Señal regular");
  } else {
    logMessage(0, "❌ Señal débil - problemas de conectividad esperados");
  }
}



/**
 * Inicia la comunicación GSM con secuencia robusta para SIM7080G
 */
void startGsm() {
  logMessage(2, "📱 Iniciando comunicación GSM con SIM7080G");
  
  pinMode(PWRKEY_PIN, OUTPUT);
  digitalWrite(PWRKEY_PIN, LOW);
  
  logMessage(2, "🔌 Ejecutando secuencia de encendido inicial");
  modemPwrKeyPulse();
  
  int retry = 0;
  const int maxRetries = 5;
  
  while (!modem.testAT(2000)) {
    flushPortSerial();
    logMessage(3, "🔄 Esperando respuesta AT del SIM7080G... (intento " + String(retry + 1) + ")");
    
    if (retry++ >= maxRetries) {
      logMessage(1, "⚠️  Sin respuesta AT, ejecutando nuevo ciclo de encendido");
      
      digitalWrite(PWRKEY_PIN, HIGH);
      delay(1500);
      digitalWrite(PWRKEY_PIN, LOW);
      delay(1000);
      
      modemPwrKeyPulse();
      retry = 0;
    } else {
      delay(500);
    }
  }
  
  logMessage(2, "✅ Comunicación AT establecida con SIM7080G");
  
  logMessage(2, "🔍 Verificando estado del módem");
  sendATCommand("", "OK", 500);
  
  if (sendATCommand("+CPIN?", "READY", 5000)) {
    logMessage(2, "✅ SIM card lista y desbloqueada");
  } else {
    logMessage(1, "⚠️  Problema con SIM card, continuando...");
  }
  
  logMessage(2, "📡 Activando RF del SIM7080G");
  
  if (sendATCommand("+CFUN=1", "OK", 8000)) {
    logMessage(2, "✅ RF del módem activada correctamente");
  } else {
    logMessage(1, "⚠️  Error al activar RF, forzando reinicio...");
    
    if (sendATCommand("+CFUN=1,1", "OK", 12000)) {
      logMessage(2, "✅ RF activada con reinicio del módem");
      delay(2000);
    } else {
      logMessage(0, "❌ Fallo crítico al activar RF del módem");
    }
  }
  
  delay(1000);
  
  if (sendATCommand("+CFUN?", "+CFUN: 1", 3000)) {
    logMessage(2, "✅ SIM7080G completamente funcional y listo");
  } else {
    logMessage(1, "⚠️  Advertencia: No se pudo verificar estado final de RF");
  }
}

/**
 * Espera un token específico en el stream con timeout
 * @param s - Stream a monitorear
 * @param token - Token a buscar
 * @param timeout_ms - Timeout en milisegundos
 * @return true si se encuentra el token
 */
static bool waitForToken(Stream& s, const String& token, uint32_t timeout_ms) {
  uint32_t start = millis();
  String buf;
  buf.reserve(256);

  while (millis() - start < timeout_ms) {
    while (s.available()) {
      char c = s.read();
      buf += c;

      if (buf.length() > 512) buf.remove(0, buf.length() - 256);

      if (buf.indexOf(token) >= 0) return true;
    }
    delay(1);
  }

  return false;
}

/**
 * Espera cualquiera de varios tokens con timeout
 * @param s - Stream a monitorear
 * @param okTokens - Array de tokens de éxito
 * @param okCount - Cantidad de tokens de éxito
 * @param errTokens - Array de tokens de error
 * @param errCount - Cantidad de tokens de error
 * @param timeout_ms - Timeout en milisegundos
 * @return 1=OK, -1=Error, 0=Timeout
 */
static int8_t waitForAnyToken(Stream& s,
                              const char* okTokens[], size_t okCount,
                              const char* errTokens[], size_t errCount,
                              uint32_t timeout_ms) {
  uint32_t start = millis();
  String buf;
  buf.reserve(512);

  while (millis() - start < timeout_ms) {
    while (s.available()) {
      char c = s.read();
      buf += c;

      if (buf.length() > 1024) buf.remove(0, buf.length() - 512);

      for (size_t i = 0; i < errCount; ++i) {
        if (buf.indexOf(errTokens[i]) >= 0) return -1;
      }

      for (size_t i = 0; i < okCount; ++i) {
        if (buf.indexOf(okTokens[i]) >= 0) return 1;
      }
    }
    delay(1);
  }

  return 0;
}

/**
 * Envía datos TCP con gestión robusta de errores
 * @param datos - Datos a enviar
 * @param timeout_ms - Timeout en milisegundos
 * @return true si el envío es exitoso
 */
bool tcpSendData(const String& datos, uint32_t timeout_ms) {
  logMessage(3, "📤 Enviando " + String(datos.length()) + " bytes por TCP");

  flushPortSerial();
  while (SerialAT.available()) SerialAT.read();

  const size_t len = datos.length() + 2;

  modem.sendAT(String("+CASEND=0,") + String(len));
  if (!waitForToken(SerialAT, ">", timeout_ms)) {
    logMessage(0, "❌ Timeout esperando prompt '>' para envío");
    return false;
  }
  modem.sendAT(datos);
  modem.sendAT("\r\n");

  const char* okTokens[] = { "CADATAIND: 0", "SEND OK", "OK" };
  const char* errTokens[] = { "SEND FAIL", "ERROR", "+CME ERROR", "+CMS ERROR" };

  int8_t result = waitForAnyToken(SerialAT,
                                  okTokens, sizeof(okTokens) / sizeof(okTokens[0]),
                                  errTokens, sizeof(errTokens) / sizeof(errTokens[0]),
                                  timeout_ms);

  if (result == 1) {
    logMessage(3, "✅ Datos TCP enviados exitosamente");
    return true;
  }

  if (result == -1) {
    logMessage(0, "❌ Error en envío TCP");
    return false;
  }

  logMessage(0, "❌ Timeout en envío TCP");
  return false;
}



/**
 * Inicializa la conexión TCP persistente
 * @return true si la conexión se establece exitosamente
 */
bool tcpInitPersistent() {
  logMessage(2, "🔌 Inicializando conexión TCP persistente");
  
  tcpConnected = false;
  tcpReconnectAttempts = 0;
  
  String openCommand = "+CAOPEN=0,0,\"TCP\",\"" + modemConfig.serverIP + "\"," + modemConfig.serverPort;
  if (sendATCommand(openCommand, "+CAOPEN: 0,0", getAdaptiveTimeout())) {
    tcpConnected = true;
    lastTcpActivity = millis();
    logMessage(2, "✅ Conexión TCP persistente establecida");
    return true;
  }
  
  logMessage(0, "❌ Falló inicialización de conexión TCP persistente");
  return false;
}

/**
 * Verifica si la conexión TCP persistente está activa
 * @return true si la conexión está activa
 */
bool tcpIsPersistentActive() {
  if (!tcpConnected) {
    return false;
  }
  
  if (sendATCommand("+CASTATE?", "+CASTATE: 0,1", 5000)) {
    lastTcpActivity = millis();
    return true;
  }
  
  logMessage(1, "⚠️  Conexión TCP persistente perdida - marcando como desconectada");
  tcpConnected = false;
  return false;
}

/**
 * Mantiene la conexión TCP persistente enviando keep-alive
 * @return true si la conexión sigue activa
 */
bool tcpKeepAlivePersistent() {
  unsigned long currentTime = millis();
  
  if (tcpConnected && (currentTime - lastTcpActivity > tcpKeepAliveInterval)) {
    logMessage(3, "💓 Enviando keep-alive TCP persistente");
    
    if (sendATCommand("+CASTATE?", "+CASTATE: 0,1", 5000)) {
      lastTcpActivity = currentTime;
      logMessage(3, "✅ Keep-alive TCP exitoso");
      return true;
    } else {
      logMessage(1, "⚠️  Keep-alive TCP falló - conexión perdida");
      tcpConnected = false;
      return false;
    }
  }
  
  return tcpConnected;
}

/**
 * Reconecta la conexión TCP persistente si se perdió
 * @return true si se reconectó exitosamente
 */
bool tcpReconnectPersistent() {
  if (tcpConnected) {
    return true;
  }
  
  if (tcpReconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
    logMessage(0, "❌ Máximo número de reconexiones TCP alcanzado");
    return false;
  }
  
  tcpReconnectAttempts++;
  logMessage(2, "🔄 Intentando reconexión TCP persistente (intento " + String(tcpReconnectAttempts) + "/" + String(MAX_RECONNECT_ATTEMPTS) + ")");
  
  sendATCommand("+CACLOSE=0", "OK", 3000);
  delay(1000);
  
  String openCommand = "+CAOPEN=0,0,\"TCP\",\"" + modemConfig.serverIP + "\"," + modemConfig.serverPort;
  if (sendATCommand(openCommand, "+CAOPEN: 0,0", getAdaptiveTimeout())) {
    tcpConnected = true;
    lastTcpActivity = millis();
    tcpReconnectAttempts = 0;
    logMessage(2, "✅ Reconexión TCP persistente exitosa");
    return true;
  }
  
  logMessage(1, "⚠️  Falló reconexión TCP persistente");
  return false;
}

/**
 * Envía datos usando la conexión TCP persistente
 * @param datos Datos a enviar
 * @param timeout_ms Timeout en milisegundos
 * @return true si el envío es exitoso
 */
bool tcpSendPersistent(const String& datos, uint32_t timeout_ms) {
  if (!tcpIsPersistentActive()) {
    if (!tcpReconnectPersistent()) {
      logMessage(0, "❌ No se pudo establecer conexión TCP persistente para envío");
      return false;
    }
  }
  
  logMessage(3, "📤 Enviando " + String(datos.length()) + " bytes por TCP persistente");
  
  if (tcpSendData(datos, timeout_ms)) {
    lastTcpActivity = millis();
    logMessage(3, "✅ Datos enviados exitosamente por TCP persistente");
    return true;
  } else {
    logMessage(1, "⚠️  Fallo en envío TCP - intentando reconectar");
    tcpConnected = false;
    
    if (tcpReconnectPersistent()) {
      if (tcpSendData(datos, timeout_ms)) {
        lastTcpActivity = millis();
        logMessage(2, "✅ Datos enviados tras reconexión TCP");
        return true;
      }
    }
  }
  
  return false;
}

/**
 * Cierra la conexión TCP persistente
 */
void tcpClosePersistent() {
  if (tcpConnected) {
    logMessage(2, "🔌 Cerrando conexión TCP persistente");
    sendATCommand("+CACLOSE=0", "OK", getAdaptiveTimeout());
    tcpConnected = false;
    tcpReconnectAttempts = 0;
    logMessage(2, "✅ Conexión TCP persistente cerrada");
  }
}

/**
 * Gestiona el mantenimiento de la conexión TCP persistente
 * Debe llamarse periódicamente desde el loop principal
 */
void tcpMaintainPersistent() {
  if (!modemInitialized) {
    return;
  }
  
  if (!tcpKeepAlivePersistent()) {
    if (!tcpReconnectPersistent()) {
      logMessage(1, "⚠️  No se pudo mantener conexión TCP persistente");
      
      if (tcpReconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        logMessage(0, "🔄 Reiniciando módem por fallas TCP persistentes");
        tcpClosePersistent();
        
        if (startLTE()) {
          tcpInitPersistent();
        }
      }
    }
  }
}

/**
 * Configura parámetros de la conexión TCP persistente
 * @param keepAliveIntervalMs Intervalo de keep-alive en milisegundos
 */
void tcpConfigurePersistent(unsigned long keepAliveIntervalMs) {
  tcpKeepAliveInterval = keepAliveIntervalMs;
  logMessage(2, "🔧 TCP persistente configurado: keep-alive cada " + String(keepAliveIntervalMs) + "ms");
}


/**
 * Inicializa el sistema básico para ESP32-S3
 */
void initESP32S3System() {
  logMessage(2, "🚀 Inicializando sistema ESP32-S3");
  
  pinMode(PWRKEY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(PWRKEY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  logMessage(2, "✅ Sistema ESP32-S3 inicializado");
}






