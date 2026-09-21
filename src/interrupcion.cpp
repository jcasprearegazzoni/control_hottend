#include "interrupcion.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

namespace {

// ============================================================
// ESTADO PRIVADO DEL MÓDULO
// ============================================================
//
// Esta es prácticamente la única variable de almacenamiento
// global que necesitamos en todo el sistema.
//
// Es necesaria porque debe existir independientemente de
// loop() y ser accesible desde la ISR.
//
// Pero:
//
// 1. Está dentro de namespace anónimo.
// 2. Ningún otro módulo puede acceder directamente.
// 3. Solamente interrupcion.cpp conoce su existencia.
// 4. Es volatile porque es modificada por una ISR.
//
// ============================================================

volatile bool eventoPendiente = false;

// ============================================================
// CONFIGURACIÓN TIMER1
// ============================================================

constexpr uint32_t PRESCALER_TIMER1 = 64UL;

// Frecuencia resultante:
//
//          F_CPU
// fTimer = ------
//            64
//
// Para:
//      F_CPU = 16 MHz
//
// obtenemos:
//      250 kHz
//
constexpr uint32_t FRECUENCIA_TIMER1 =
    static_cast<uint32_t>(F_CPU) / PRESCALER_TIMER1;

// Número de cuentas por milisegundo.
//
// 250000 / 1000 = 250 cuentas/ms.
//
constexpr uint32_t CUENTAS_POR_MS = FRECUENCIA_TIMER1 / 1000UL;

// Número de cuentas para nuestro periodo.
//
// 250 cuentas/ms * 100 ms
//
// = 25000 cuentas
//
constexpr uint32_t CUENTAS_TIMER1 =
    CUENTAS_POR_MS * static_cast<uint32_t>(Interrupcion::periodoMs());

// ============================================================
// COMPROBACIONES EN TIEMPO DE COMPILACIÓN
// ============================================================
//
// Si alguna modificación futura hace imposible generar
// exactamente el periodo solicitado, queremos descubrirlo
// durante la compilación y no durante una práctica de
// laboratorio.
// ============================================================

static_assert((FRECUENCIA_TIMER1 % 1000UL) == 0UL,
              "Timer1 no produce un numero entero de cuentas por ms");

static_assert(CUENTAS_TIMER1 > 0UL, "El periodo de Timer1 no puede ser cero");

static_assert(CUENTAS_TIMER1 <= 65536UL,
              "El periodo solicitado no cabe en Timer1");

} // namespace

// ============================================================
// INTERRUPCIÓN TIMER1
// ============================================================
//
// Esta función debe quedar fuera del namespace Interrupcion
// porque la macro ISR genera el vector de interrupción esperado
// por avr-libc.
//
// Debe ser deliberadamente muy corta.
//
// NO:
//
// analogRead()
// log()
// controlador
// Serial.print()
//
// ============================================================

ISR(TIMER1_COMPA_vect) { eventoPendiente = true; }

// ============================================================
// IMPLEMENTACIÓN PÚBLICA DEL MÓDULO
// ============================================================

namespace Interrupcion {

void inicializar() noexcept {
  // --------------------------------------------------------
  // Guardar el estado actual de las interrupciones globales.
  //
  // Esto es mejor que hacer simplemente:
  //
  //      cli();
  //      ...
  //      sei();
  //
  // porque respetamos el estado existente antes de entrar
  // en esta función.
  // --------------------------------------------------------

  const uint8_t estadoSreg = SREG;

  // Deshabilitar temporalmente interrupciones.
  cli();

  // --------------------------------------------------------
  // Eliminar cualquier evento pendiente anterior.
  // --------------------------------------------------------

  eventoPendiente = false;

  // --------------------------------------------------------
  // Detener y limpiar Timer1.
  // --------------------------------------------------------

  TCCR1A = 0U;
  TCCR1B = 0U;

  TCNT1 = 0U;

  // --------------------------------------------------------
  // Configurar Compare Match A.
  //
  // Necesitamos 25000 cuentas.
  //
  // El contador comienza en:
  //
  //      0
  //
  // por tanto:
  //
  //      OCR1A = 25000 - 1
  //
  //             24999
  //
  // --------------------------------------------------------

  OCR1A = static_cast<uint16_t>(CUENTAS_TIMER1 - 1UL);

  // --------------------------------------------------------
  // Limpiar un posible flag de Compare Match pendiente.
  //
  // En AVR los flags de TIFR se limpian escribiendo un 1.
  // --------------------------------------------------------

  TIFR1 = static_cast<uint8_t>(_BV(OCF1A));

  // --------------------------------------------------------
  // Habilitar interrupción Compare Match A.
  // --------------------------------------------------------

  TIMSK1 = static_cast<uint8_t>(_BV(OCIE1A));

  // --------------------------------------------------------
  // Configurar Timer1:
  //
  // WGM12 = 1
  //
  //      modo CTC
  //
  // CS11 = 1
  // CS10 = 1
  //
  //      prescaler = 64
  //
  // Al escribir TCCR1B comenzará a funcionar Timer1.
  // --------------------------------------------------------

  TCCR1B = static_cast<uint8_t>(_BV(WGM12) | _BV(CS11) | _BV(CS10));

  // --------------------------------------------------------
  // Restaurar exactamente el estado anterior de SREG.
  // --------------------------------------------------------

  SREG = estadoSreg;
}

bool consumirEvento() noexcept {
  // --------------------------------------------------------
  // Tenemos una pequeña región crítica.
  //
  // Queremos realizar conjuntamente:
  //
  //      leer eventoPendiente
  //      eventoPendiente = false
  //
  // Podría suceder:
  //
  // main                         ISR
  // ----                         ---
  //
  // lee true
  //
  //                    <--- interrupción
  //                         evento=true
  //
  // escribe false
  //
  // En ese caso perderíamos la nueva interrupción.
  //
  // Por eso deshabilitamos interrupciones durante unas pocas
  // instrucciones.
  // --------------------------------------------------------

  const uint8_t estadoSreg = SREG;

  cli();

  const bool resultado = eventoPendiente;

  eventoPendiente = false;

  // Restaurar estado previo de interrupciones.
  SREG = estadoSreg;

  return resultado;
}

} // namespace Interrupcion
