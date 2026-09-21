#ifndef INTERRUPCION_H
#define INTERRUPCION_H

#include <stdint.h>

namespace Interrupcion {

// Configura Timer1 para producir una interrupción periódica.
void inicializar() noexcept;

// Indica si ocurrió una interrupción desde la última consulta.
//
// Si existe un evento pendiente:
//
//      devuelve true
//
// y simultáneamente consume dicho evento.
//
// La lectura y borrado de la bandera se realiza de forma
// atómica dentro de interrupcion.cpp.
bool consumirEvento() noexcept;

// Periodo de muestreo del controlador.
//
// constexpr significa que no existe una variable mutable:
// el compilador conoce directamente este valor.
//
// Esto permite que main.cpp conozca el periodo sin duplicar
// el número 100 en varios archivos.
constexpr uint16_t periodoMs() noexcept { return 100U; }

} // namespace Interrupcion

#endif
