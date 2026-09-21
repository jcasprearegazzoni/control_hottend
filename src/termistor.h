#ifndef TERMISTOR_H
#define TERMISTOR_H

namespace Termistor {

// Configura el hardware necesario para leer el termistor.
void inicializar() noexcept;

// Lee el ADC y devuelve la temperatura en grados Celsius.
//
// Devuelve NAN si la lectura del ADC no permite calcular
// una temperatura válida.
float leerTemperatura() noexcept;

} // namespace Termistor

#endif
