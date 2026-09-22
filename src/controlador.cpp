#include "Arduino.h"
#include "controlador.h"

// ============================================================
// COEFICIENTES DEL CONTROLADOR
// ============================================================
//
// Ecuación:
//
// u[k] =
//      A1 * u[k-1]
//    + A2 * u[k-2]
//    + B0 * e[k]
//    + B1 * e[k-1]
//    + B2 * e[k-2]
//
// ============================================================

float Coef[6] = {0.606530659712, 1.606530659712633, 1 , 594.703904600827, 1183.783138555601, 589.093974386158};
float Estados[6] = {0,0,0,0,0,0};

float salkm0 = Estados[2];
float salkm1 = Estados[1];
float salkm2 = Estados[0];

float entkm0 = Estados[3];
float entkm1 = Estados[4];
float entkm2 = Estados[5];

float cskm0 = Coef[2];
float cskm1 = Coef[1];
float cskm2 = Coef[0];

float cekm0 = Coef[3];
float cekm1 = Coef[4];
float cekm2 = Coef[5];


// ============================================================
// CONSTRUCTOR
// ============================================================


// ============================================================
// CALCULAR CONTROL
// ============================================================

uint8_t PID(const float entkm0) {
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

  float salkm0 = (cskm1 * salkm1) - (cskm2 * salkm2) + (cekm0 * entkm0) -
                 (cekm1 * entkm1) + (cekm2 * entkm2);

  // --------------------------------------------------------
  // Saturación.
  // --------------------------------------------------------

  if (salkm0 > 255) {
    salkm0 = 255;
  } else if (salkm0 < 0) {
    salkm0 = 0;
  }

  // --------------------------------------------------------
  // Actualizar memoria de errores.
  // --------------------------------------------------------

  entkm2 = entkm1;

  entkm1 = entkm0;

  // --------------------------------------------------------
  // Actualizar memoria de salidas.
  //
  // Se almacena la salida saturada.
  // --------------------------------------------------------

  salkm2 = salkm1;

  salkm1 = salkm0;

  // --------------------------------------------------------
  // Convertir únicamente al salir del controlador.
  //
  // Internamente seguimos trabajando en coma flotante.
  // --------------------------------------------------------

  return static_cast<uint8_t>(salkm0);
}
