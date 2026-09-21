#ifndef CONTROLADOR_H
#define CONTROLADOR_H

#include <stdint.h>

class Controlador {
public:
  // Construye un controlador con toda su memoria en cero.
  Controlador() noexcept;

  // Borra la memoria interna del controlador.
  void reiniciar() noexcept;

  // Calcula una nueva salida de control.
  //
  // Entrada:
  //      errorActual -> error de temperatura en grados Celsius
  //
  // Salida:
  //      valor PWM entre 0 y 255
  //
  uint8_t calcular(float errorActual) noexcept;

private:
  // ========================================================
  // MEMORIA DEL CONTROLADOR
  // ========================================================
  //
  // Estas variables antes eran globales.
  //
  // Ahora solamente existen dentro de una instancia de
  // Controlador y no pueden ser modificadas desde main.cpp.
  // ========================================================

  float salidaAnterior_;
  float salidaAnteanterior_;

  float errorAnterior_;
  float errorAnteanterior_;
};

#endif
