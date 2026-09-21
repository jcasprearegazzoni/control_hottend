// ============================================================
// CONTROL DE TEMPERATURA DE HOTEND
// Arduino Mega 2560 + RAMPS
//
// Termistor:
//      A13
//
// Calentador:
//      D10
//
// D10 utiliza TIMER2A para generar PWM.
//
// TIMER1 se utiliza exclusivamente para generar una
// interrupción periódica cada 100 ms.
//
// Por tanto:
//
//      TIMER1 ---> periodo de muestreo del controlador
//      TIMER2 ---> PWM del calentador
//
// ============================================================

#include <Arduino.h>

// Necesarias para trabajar directamente con
// los registros e interrupciones del AVR.
#include <avr/interrupt.h>
#include <avr/io.h>

// ============================================================
// PINES
// ============================================================

// Entrada analógica correspondiente al termistor.
#define TERMISTOR_PIN A13

// Salida PWM hacia el MOSFET de la RAMPS.
//
// IMPORTANTE:
//
// En Arduino Mega 2560:
//
//      D10 = OC2A es decir, utiliza TIMER2 para el PWM.
//
// Esto permite utilizar TIMER1 para la interrupción.
#define HEATER_PIN 10

// ============================================================
// PARÁMETROS DEL TERMISTOR
// ============================================================

// Resistencia fija del divisor resistivo.
#define RESISTENCIA_SERIE 4700.0

// Resistencia nominal del termistor a 25 °C.
#define RESISTENCIA_NOMINAL 100000.0

// Temperatura nominal del termistor.
#define TEMPERATURA_NOMINAL 25.0

// Constante Beta del termistor.
#define BETA 3950.0

// ============================================================
// CONTROLADOR
// ============================================================

// Temperatura deseada del hotend.
// Famoso SET POINT que utilizamos.
// En este caso se puede interpretar como un escalón de 80 grados
constexpr float CONSIGNA_C = 80.0f;

// Periodo del controlador:
//
//      Ts = 100 ms
//
// es decir:
//
//      fs = 10 Hz
constexpr uint32_t PERIODO_CONTROL_MS = 100;

// Coeficientes correspondientes a los errores anteriores.
constexpr float COEF_ERROR_ANTERIOR = 0.012022686684899f;
constexpr float COEF_ERROR_ANTEANTERIOR = -0.011993116567433f;

// ============================================================
// VARIABLES DEL CONTROLADOR
// ============================================================

// salida[k-1]
float salidaAnterior = 0.0f;

// salida[k-2]
float salidaAnteanterior = 0.0f;

// error[k-1]
float errorAnterior = 0.0f;

// error[k-2]
float errorAnteanterior = 0.0f;

// ============================================================
// VARIABLES COMPARTIDAS CON LA INTERRUPCIÓN
// ============================================================

// Esta variable es modificada dentro de una ISR.
//
// Por eso DEBE ser volatile.
//
// La palabra volatile indica al compilador que la variable
// puede cambiar en cualquier momento, incluso sin que el
// programa principal la modifique explícitamente.
volatile bool controlPendiente = false;

// ============================================================
// CONTADOR DE MUESTRAS
// ============================================================

// Cada ejecución del controlador representa exactamente
// una muestra de 100 ms.
//
// Por tanto:
//
//      muestra 1 -> 100 ms
//      muestra 2 -> 200 ms
//      muestra 3 -> 300 ms
//          ...
//
uint32_t numeroMuestra = 0;

// ============================================================
// INTERRUPCIÓN TIMER1
// ============================================================
//
// Esta rutina se ejecutará automáticamente cada 100 ms.
//
// MUY IMPORTANTE:
//
// La ISR debe ser extremadamente corta.
//
// NO hacemos:
//
//      analogRead()
//      log()
//      Serial.print()
//      cálculo del controlador
//
// dentro de esta función.
//
// Simplemente avisamos al programa principal de que llegó
// el momento de realizar una nueva muestra.
//
// ============================================================
ISR(TIMER1_COMPA_vect) { controlPendiente = true; }

// ============================================================
// CONFIGURACIÓN TIMER1
// ============================================================

void configurarTimer1() {
  // --------------------------------------------------------
  // Deshabilitar temporalmente las interrupciones globales.
  cli();

  // limpiar la configuración anterior del Timer1.
  TCCR1A = 0;
  TCCR1B = 0;

  // Inicializar el contador.
  TCNT1 = 0;

  // Queremos una interrupción cada:
  //
  //      100 ms
  //
  // Arduino Mega:
  //
  //      F_CPU = 16 MHz
  //
  // Prescaler elegido:
  //
  //      64
  //
  // Frecuencia del Timer1:
  //
  //      16 MHz ÷ 64 = 250 kHz
  //
  // Periodo de cada incremento:
  //
  //      1 ÷ 250000 = 4 µs
  //
  // Para 100 ms necesitamos:
  //
  //      100000 µs ÷ 4 µs = 25000 cuentas
  //
  // Como contamos desde 0:
  //
  //      OCR1A = 25000 - 1 = 24999
  OCR1A = 24999;

  // MODO CTC
  //
  // CTC = Clear Timer on Compare Match
  //
  // Timer1 cuenta:
  //
  //      0
  //      1
  //      ...
  //      24999
  //
  // Cuando:
  //
  //      TCNT1 == OCR1A
  //
  // ocurre un Compare Match. Automáticamente el contador vuelve a cero.
  // Para seleccionar CTC:
  //
  //      WGM12 = 1
  TCCR1B |= (1 << WGM12);

  // Habilitar interrupción por Compare Match A.
  //
  // Cuando TCNT1 alcance OCR1A se ejecutará:
  //      ISR(TIMER1_COMPA_vect)
  TIMSK1 |= (1 << OCIE1A);

  // Seleccionar prescaler = 64.
  //
  // Para Timer1:
  //
  //      CS12 CS11 CS10
  //
  //       0    1    1   -> división por 64
  TCCR1B |= (1 << CS11) | (1 << CS10);

  // Volver a habilitar las interrupciones globales.
  sei();
}

// ============================================================
// REINICIAR CONTROLADOR
// ============================================================

void reiniciarControl() {
  // LA DESHABILITAMOS POR UN TIEMPO PARA TEST
  // SI NO LEE QUE SIGA COMO ANTES
  // QUE NO LO APAGUE NI EMPIECE DE NUEVO
  // ESTO ES POR SEGURIDAD, PERO FEO
  // DESPUÉS IMPLEMENTAMOS OTRO TIPO

  // Borrar memoria del controlador.

  // salidaAnterior = 0.0f;
  // salidaAnteanterior = 0.0f;

  // errorAnterior = 0.0f;
  // errorAnteanterior = 0.0f;

  // Apagar inmediatamente el calentador.

  // analogWrite(HEATER_PIN, 0);
}

// ============================================================
// CONTROLADOR DIGITAL
// ============================================================

int calcularControl(float errorActual) {
  // --------------------------------------------------------
  // Ecuación en diferencias original:
  //
  //
  // salida[k] =
  //      1.987714034298315 * salida[k-1]
  //    - 0.987743604415780 * salida[k-2]
  //    + 0.012022686684899 * error[k-1]
  //    + 0.011993116567433 * error[k-2]
  //
  float salida = 1.987714034298315f * salidaAnterior -
                 0.987743604415780f * salidaAnteanterior +
                 COEF_ERROR_ANTERIOR * errorAnterior -
                 COEF_ERROR_ANTEANTERIOR * errorAnteanterior;

  // --------------------------------------------------------
  // Saturación PWM.
  //
  // El rango permitido por analogWrite() es:
  //
  //      0 ... 255
  //
  // --------------------------------------------------------

  if (salida > 255.0f) {
    salida = 255.0f;
  } else if (salida < 0.0f) {
    salida = 0.0f;
  }

  // --------------------------------------------------------
  // Desplazar el historial de errores.
  // error[k-2] <- error[k-1]
  // error[k-1] <- error[k]
  errorAnteanterior = errorAnterior;
  errorAnterior = errorActual;

  // --------------------------------------------------------
  // Desplazar el historial de salidas.
  // salida[k-2] <- salida[k-1]
  // salida[k-1] <- salida[k]
  salidaAnteanterior = salidaAnterior;
  salidaAnterior = salida;

  return static_cast<int>(salida);
}

// ============================================================
// LECTURA DEL TERMISTOR
// ============================================================

float leerTemperatura() {
  // Convertidor ADC del Mega2560:
  //
  //      10 bits
  //
  // rango:
  //
  //      0 ... 1023
  int lectura = analogRead(TERMISTOR_PIN);

  // Evitar divisiones problemáticas.
  //
  // Una lectura 0 o 1023 puede indicar:
  //
  //      termistor desconectado
  //      cortocircuito
  //      problema de cableado
  if (lectura <= 0 || lectura >= 1023) {
    return NAN;
  }

  // Calcular resistencia del termistor.
  //
  // Circuito supuesto:
  //
  //             Vcc
  //              │
  //             Rserie
  //              │
  //              ├──── A13
  //              │
  //             NTC
  //              │
  //             GND
  //
  float resistencia = RESISTENCIA_SERIE * lectura / (1023.0f - lectura);

  // Modelo Beta del termistor:
  //
  //     1       1       1       R
  //    --- = ------- + ---- ln(---)
  //     T       T0      B       R0
  //
  // T en Kelvin.
  float temperatura = resistencia / RESISTENCIA_NOMINAL;

  // ln(R/R0)
  temperatura = log(temperatura);

  // ln(R/R0) / Beta
  temperatura /= BETA;

  // 1/T0 + ln(R/R0)/Beta
  temperatura += 1.0f / (TEMPERATURA_NOMINAL + 273.15f);

  // Obtener temperatura absoluta.
  temperatura = 1.0f / temperatura;

  // Kelvin -> Celsius
  temperatura -= 273.15f;

  return temperatura;
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  pinMode(HEATER_PIN, OUTPUT);
  reiniciarControl();

  // --------------------------------------------------------
  // Configurar Timer1.
  //
  // A partir de aquí comenzará a producirse una
  // interrupción cada 100 ms.
  // --------------------------------------------------------
  configurarTimer1();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  // --------------------------------------------------------
  // Espera a que Timer1 indique mediante una interrupción
  // que llegó el instante de realizar una muestra.
  // --------------------------------------------------------

  if (controlPendiente) {
    // Borrar primero la bandera.
    //
    // Si ocurre otra interrupción mientras estamos
    // procesando esta muestra, Timer1 podrá volver a
    // colocarla en true.
    controlPendiente = false;

    // Incrementar número de muestra.
    numeroMuestra++;

    // Construir un tiempo para mostrar por Serial.
    //
    // Ya NO usamos millis().
    //
    // Ejemplo:
    //
    //      muestra 1 -> 100 ms
    //      muestra 2 -> 200 ms
    uint32_t tiempo_ms = numeroMuestra * PERIODO_CONTROL_MS;

    // Medir temperatura.
    float temperatura = leerTemperatura();

    // Verificación de seguridad.
    if (isnan(temperatura)) {
      // Sensor inválido.
      //
      // Apagar calentador y borrar memoria
      // del controlador.

      reiniciarControl();

      return;
    }

    // Calcular error.
    //
    //              referencia - salida
    //
    // error positivo:
    //
    //      necesitamos calentar.
    //
    // error negativo:
    //
    //      temperatura superior a la consigna.
    // ----------------------------------------------------

    float errorActual = CONSIGNA_C - temperatura;

    // Ejecutar controlador digital.
    int pwm = calcularControl(errorActual);

    // Aplicar salida al calentador.
    //
    // D10 utiliza TIMER2A.
    //
    // El Timer2 seguirá generando el PWM automáticamente
    // hasta que cambiemos nuevamente OCR2A mediante
    // analogWrite().
    // ----------------------------------------------------
    analogWrite(HEATER_PIN, pwm);

    // Enviar datos en CSV:
    //
    //      tiempo_ms,temperatura,pwm
    //
    // Ejemplo:
    //
    //      100,24.53,0
    //      200,24.61,1
    //      300,24.78,2
    //
    // ----------------------------------------------------

    Serial.print(tiempo_ms);

    Serial.write(',');

    Serial.print(temperatura, 2);

    Serial.write(',');

    Serial.println(pwm);
  }
}
