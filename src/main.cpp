#include <Arduino.h>
#include <math.h>
#include <stdint.h>

#include "controlador.h"
#include "interrupcion.h"
#include "termistor.h"

//namespace {

// ============================================================
// CONFIGURACIÓN GENERAL DE LA APLICACIÓN
// ============================================================
//
// Son constantes, no estado mutable global.
// ============================================================

// D10 = OC2A en Arduino Mega 2560.
// analogWrite() utiliza Timer2 para producir el PWM.
constexpr uint8_t HEATER_PIN = 10U;

// Temperatura objetivo.
float CONSIGNA_C = 0.0F;

//} // namespace

// ============================================================
// SETUP
// ============================================================

void setup() {
  // Puerto serie.
  Serial.begin(115200);

  // --------------------------------------------------------
  // Inicializar salida del calentador.
  // --------------------------------------------------------

  pinMode(HEATER_PIN, OUTPUT);

  // Arrancar obligatoriamente apagado.
  analogWrite(HEATER_PIN, 0);

  // --------------------------------------------------------
  // Inicializar termistor.
  // --------------------------------------------------------

  Termistor::inicializar();

  // --------------------------------------------------------
  // Inicializar Timer1.
  //
  // A partir de este momento se genera una interrupción
  // cada 100 ms.
  // --------------------------------------------------------

  Interrupcion::inicializar();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  // --------------------------------------------------------
  // Estado persistente del controlador.
  //
  // IMPORTANTE:
  //
  // Al declarar estas variables static DENTRO de loop()
  // conseguimos que:
  //
  // - conserven su valor entre llamadas,
  // - no sean variables globales,
  // - solamente sean visibles dentro de loop().
  //
  // --------------------------------------------------------

  //static uint8_t PID();

  static uint32_t numeroMuestra = 0UL;

  // --------------------------------------------------------
  // ¿Llegó el instante de ejecutar el controlador?
  //
  // Si no hay evento pendiente no hacemos absolutamente
  // nada.
  // --------------------------------------------------------

  while (Serial.available()>0)  {
    CONSIGNA_C=Serial.parseFloat();
}

//    
  
  if (!Interrupcion::consumirEvento()) {
    return;
  }

  // --------------------------------------------------------
  // Nueva muestra.
  // --------------------------------------------------------

  ++numeroMuestra;

  // Tiempo teórico correspondiente a esta muestra.
  const uint32_t tiempoMs =
      numeroMuestra * static_cast<uint32_t>(Interrupcion::periodoMs());

  // --------------------------------------------------------
  // MEDICIÓN
  // --------------------------------------------------------

  const float temperatura = Termistor::leerTemperatura();

  // --------------------------------------------------------
  // SEGURIDAD
  // --------------------------------------------------------

  if (isnan(temperatura)) {
    // Lectura inválida:
    //
    // - apagar calentador,
    // - borrar memoria del controlador.

    analogWrite(HEATER_PIN, 0);

    return;
  }

  // --------------------------------------------------------
  // ERROR
  // --------------------------------------------------------

  const float error = CONSIGNA_C - temperatura;

  // --------------------------------------------------------
  // CONTROLADOR
  // --------------------------------------------------------

  const uint8_t pwm = PID(error);

  // --------------------------------------------------------
  // ACTUADOR
  //
  // D10 utiliza Timer2.
  // Timer1 queda reservado para el periodo de muestreo.
  // --------------------------------------------------------

  analogWrite(HEATER_PIN, static_cast<int>(pwm));

  // --------------------------------------------------------
  // TELEMETRÍA CSV
  //
  // tiempo_ms,temperatura_C,pwm
  // --------------------------------------------------------

  Serial.print(tiempoMs);

  Serial.write(',');

  Serial.print(CONSIGNA_C);

  Serial.write(',');

  Serial.print(temperatura, 2);

  Serial.write(',');

  Serial.println(static_cast<unsigned int>(pwm));
}
