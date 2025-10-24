/**
 * @file modem_gsm_wifi.ino
 * @brief Sketch de demostración para módem SIM7080G con TCP persistente
 * @author Jorge Meza
 * @date 2025-10-23
 * @version 3.0
 * 
 * @details Sketch completo de demostración que implementa:
 * - Configuración automática del módem SIM7080G
 * - Conexión TCP persistente con keep-alive
 * - Envío periódico de datos de prueba
 * - Comandos seriales interactivos para control y diagnóstico
 * - Sistema de logging con información detallada
 * 
 * @section hardware Hardware Requerido
 * - ESP32-S3 (cualquier variante)
 * - Módem SIM7080G con antena LTE
 * - Tarjeta SIM con plan de datos activo
 * - Fuente de alimentación 3.3-4.2V para el módem
 * 
 * @section connections Conexiones de Hardware
 * | ESP32-S3 | SIM7080G | Función |
 * |----------|-----------|---------|
 * | GPIO 10  | RX        | TX Datos |
 * | GPIO 11  | TX        | RX Datos |
 * | GPIO 9   | PWRKEY    | Control Encendido |
 * | GPIO 12  | -         | LED Status |
 * | 3.3V     | VCC       | Alimentación |
 * | GND      | GND       | Tierra |
 * 
 * @section commands Comandos Serie Disponibles
 * - `status` - Mostrar estado de conexiones
 * - `send`   - Forzar envío inmediato de datos
 * - `test`   - Enviar mensaje de prueba
 * - `diag`   - Ejecutar diagnóstico completo del módem
 * - `restart`- Reiniciar configuración del módem
 * - `fast`   - Activar modo configuración rápida
 * 
 * @section config Configuración
 * - Velocidad serie: 115200 baud
 * - Keep-alive TCP: 30 segundos
 * - Envío automático: cada 60 segundos
 * - Servidor: dp01.lolaberries.com.mx:12607
 * 
 * @note Asegurar que la SIM tenga plan de datos activo antes de usar
 * @warning Verificar todas las conexiones antes de energizar
 */

#include "gsmlte.h"

unsigned long lastDataSend = 0;
const unsigned long DATA_SEND_INTERVAL = 60000;
String testData = "TEST_DATA_FROM_ESP32";

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== ESP32-S3 Módem LTE/GSM ===");
  
  tcpConfigurePersistent(30000);
  
  Serial.println("Iniciando módem...");
  setupModem();
  
  Serial.println("Sistema iniciado");
}

void loop() {
  tcpMaintainPersistent();
  
  if (millis() - lastDataSend >= DATA_SEND_INTERVAL) {
    Serial.println("Enviando datos...");
    
    String datos = "ESP32_" + String(millis()) + "_" + String(signalsim0);
    
    if (tcpIsPersistentActive()) {
      if (tcpSendPersistent(datos, 10000)) {
        Serial.println("Datos enviados OK: " + datos);
      } else {
        Serial.println("Error enviando datos");
      }
    } else {
      Serial.println("TCP no conectado");
    }
    
    lastDataSend = millis();
  }
  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "status") {
      Serial.println("TCP: " + String(tcpIsPersistentActive() ? "OK" : "Fail"));
      Serial.println("Señal: " + String(signalsim0));
      Serial.println("ICCID: " + iccidsim0);
    } else if (cmd == "send") {
      lastDataSend = 0;
    } else if (cmd == "test") {
      if (tcpIsPersistentActive()) {
        tcpSendPersistent("TEST_MESSAGE", 5000);
      }
    } else if (cmd == "diag") {
      Serial.println("=== EJECUTANDO DIAGNÓSTICO ===");
      diagnosticoModem();
    } else if (cmd == "restart") {
      Serial.println("=== REINICIANDO MÓDEM ===");
      setupModem();
    } else if (cmd == "fast") {
      Serial.println("=== MODO CONFIGURACIÓN RÁPIDA ===");
    }
  }
  
  delay(5000);
}

