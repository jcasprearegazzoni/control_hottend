#include "controlador.h"

namespace {

// ============================================================
// COEFICIENTES DEL CONTROLADOR
// ============================================================
//
// Ecuación:
//
// u[k] =
//      A1 * u[k-1]
//    + A2 * u[k-2]
//    + B1 * e[k-1]
//    + B2 * e[k-2]
//
// ============================================================

constexpr float A1 = 1.987714034298315F;

constexpr float A2 = -0.987743604415780F;

constexpr float B1 = 0.012022686684899F;

constexpr float B2 = 0.011993116567433F;

// Límites físicos de la salida PWM.
constexpr float PWM_MINIMO = 0.0F;
constexpr float PWM_MAXIMO = 255.0F;

} // namespace

// ============================================================
// CONSTRUCTOR
// ============================================================

Controlador::Controlador() noexcept
    : salidaAnterior_(0.0F), salidaAnteanterior_(0.0F), errorAnterior_(0.0F),
      errorAnteanterior_(0.0F) {}

// ============================================================
// REINICIAR
// ============================================================

void Controlador::reiniciar() noexcept {
  salidaAnterior_ = 0.0F;
  salidaAnteanterior_ = 0.0F;

  errorAnterior_ = 0.0F;
  errorAnteanterior_ = 0.0F;
}

// ============================================================
// CALCULAR CONTROL
// ============================================================

uint8_t Controlador::calcular(const float errorActual) noexcept {
  // --------------------------------------------------------
  // Ecuación en diferencias:
  //
  // u[k] =
  //
  //      1.987714034298315 u[k-1]
  //
  //    - 0.987743604415780 u[k-2]
  //
  //    + 0.012022686684899 e[k-1]
  //
  //    + 0.011993116567433 e[k-2]
  //
  // El error actual e[k] se almacena para la próxima
  // iteración.
  // --------------------------------------------------------

  float salida = (A1 * salidaAnterior_) + (A2 * salidaAnteanterior_) +
                 (B1 * errorAnterior_) + (B2 * errorAnteanterior_);

  // --------------------------------------------------------
  // Saturación.
  // --------------------------------------------------------

  if (salida > PWM_MAXIMO) {
    salida = PWM_MAXIMO;
  } else if (salida < PWM_MINIMO) {
    salida = PWM_MINIMO;
  }

  // --------------------------------------------------------
  // Actualizar memoria de errores.
  // --------------------------------------------------------

  errorAnteanterior_ = errorAnterior_;

  errorAnterior_ = errorActual;

  // --------------------------------------------------------
  // Actualizar memoria de salidas.
  //
  // Se almacena la salida saturada.
  // --------------------------------------------------------

  salidaAnteanterior_ = salidaAnterior_;

  salidaAnterior_ = salida;

  // --------------------------------------------------------
  // Convertir únicamente al salir del controlador.
  //
  // Internamente seguimos trabajando en coma flotante.
  // --------------------------------------------------------

  return static_cast<uint8_t>(salida);
}
