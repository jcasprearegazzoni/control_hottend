#include "termistor.h"

#include <Arduino.h>
#include <math.h>

namespace {

// ============================================================
// CONFIGURACIÓN DEL TERMISTOR
// ============================================================
//
// Estas constantes solamente son necesarias dentro de este
// módulo.
//
// Al estar dentro de un namespace anónimo tienen linkage
// interno: ningún otro archivo del proyecto puede acceder
// a ellas.
//
// No son variables globales mutables.
// Son constantes conocidas en tiempo de compilación.
// ============================================================

constexpr uint8_t TERMISTOR_PIN = A13;

// Resistencia fija del divisor resistivo.
constexpr float RESISTENCIA_SERIE = 4700.0F;

// Resistencia nominal del NTC a 25 °C.
constexpr float RESISTENCIA_NOMINAL = 100000.0F;

// Temperatura nominal del NTC.
constexpr float TEMPERATURA_NOMINAL = 25.0F;

// Constante Beta.
constexpr float BETA = 3950.0F;

// Conversión Celsius -> Kelvin.
constexpr float CERO_ABSOLUTO = 273.15F;

// Valor máximo de un ADC de 10 bits.
constexpr float ADC_MAXIMO = 1023.0F;

} // namespace

namespace Termistor {

void inicializar() noexcept {
  // El pin se utiliza exclusivamente como entrada analógica.
  pinMode(TERMISTOR_PIN, INPUT);
}

float leerTemperatura() noexcept {
  // --------------------------------------------------------
  // Leer convertidor ADC.
  //
  // El ADC del ATmega2560 tiene 10 bits:
  //
  //      0 ... 1023
  //
  // --------------------------------------------------------

  const int lectura = analogRead(TERMISTOR_PIN);

  // --------------------------------------------------------
  // Las lecturas extremas no permiten realizar correctamente
  // la conversión resistencia-temperatura.
  //
  // También pueden indicar:
  //
  // - termistor desconectado,
  // - cortocircuito,
  // - fallo de cableado.
  // --------------------------------------------------------

  if ((lectura <= 0) || (lectura >= 1023)) {
    return NAN;
  }

  // --------------------------------------------------------
  // Calcular resistencia del NTC.
  //
  // Circuito supuesto:
  //
  //        Vcc
  //         |
  //       Rserie
  //         |
  //         +------ ADC
  //         |
  //        NTC
  //         |
  //        GND
  //
  //              lectura
  // Rntc = Rs * ---------------
  //              1023-lectura
  //
  // --------------------------------------------------------

  const float lecturaFloat = static_cast<float>(lectura);

  const float resistencia =
      RESISTENCIA_SERIE * lecturaFloat / (ADC_MAXIMO - lecturaFloat);

  // --------------------------------------------------------
  // Ecuación Beta:
  //
  //  1       1      1       R
  // --- = -------- + - ln( ----- )
  //  T       T0     B       R0
  //
  // --------------------------------------------------------

  const float inversaTemperatura =
      (1.0F / (TEMPERATURA_NOMINAL + CERO_ABSOLUTO)) +
      (logf(resistencia / RESISTENCIA_NOMINAL) / BETA);

  // Temperatura absoluta en Kelvin.
  const float temperaturaKelvin = 1.0F / inversaTemperatura;

  // Convertir Kelvin -> Celsius.
  return temperaturaKelvin - CERO_ABSOLUTO;
}

} // namespace Termistor
