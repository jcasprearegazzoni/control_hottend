// Funciones del entorno Arduino: entradas analogicas, PWM, tiempo y puerto serie.
#include <Arduino.h>

// Entrada del divisor resistivo del termistor y salida hacia la etapa de potencia.
// HEATER_PIN controla el driver/MOSFET; no alimenta directamente el calentador.
#define TERMISTOR_PIN A13
#define HEATER_PIN 10

// Parametros del divisor y del modelo Beta del termistor NTC.
// Resistencias en ohmios, temperatura nominal en grados Celsius y Beta en kelvin.
#define RESISTENCIA_SERIE 4700.0
#define RESISTENCIA_NOMINAL 100000.0
#define TEMPERATURA_NOMINAL 25.0
#define BETA 3950.0

// Temperatura objetivo absoluta: 80 grados Celsius (no es un valor de PWM).
constexpr float CONSIGNA_C = 80.0f;
// Ejecutar una muestra de control cada 100 ms, aproximadamente 10 veces por segundo.
// Los coeficientes del controlador deben corresponder a este periodo de muestreo.
constexpr unsigned long PERIODO_CONTROL_MS = 100;
// Pesos de los errores pasados en la ecuacion implementada mas abajo.
// Atencion: el segundo coeficiente es negativo y la ecuacion lo RESTA;
// por tanto, su contribucion efectiva es +0.011993116567433 * e[k-2].
constexpr float COEF_ERROR_ANTERIOR = 0.012022686684899f;
constexpr float COEF_ERROR_ANTEANTERIOR = -0.011993116567433f;

// Instante en milisegundos de la ultima ejecucion del control.
unsigned long ahora = 0;
// Memoria del controlador: k es la muestra actual, k-1 la anterior y k-2
// la anteanterior. Se inicia en cero y se conserva entre llamadas a loop().
// Las salidas son float para no perder sus fracciones de PWM en el historial.
float salidaAnterior = 0.0f;     // salida[k-1], limitada a 0..255
float salidaAnteanterior = 0.0f; // salida[k-2], limitada a 0..255
float errorAnterior = 0.0f;      // e[k-1]
float errorAnteanterior = 0.0f;  // e[k-2]

void reiniciarControl()
{
  // Borrar toda la memoria: el proximo calculo parte otra vez desde cero.
  // Se usa al arrancar y cuando la lectura del sensor es invalida.
  salidaAnterior = 0.0f;
  salidaAnteanterior = 0.0f;
  errorAnterior = 0.0f;
  errorAnteanterior = 0.0f;
  analogWrite(HEATER_PIN, 0); // PWM cero: apagar la orden al calentador.
}

int calcularControl(float errorActual)
{
  // Ecuacion actual, con los signos efectivos de los coeficientes:
  // salida[k] = 1.987714034298315 * salida[k-1]
  //             - 0.987743604415780 * salida[k-2]
  //             + 0.012022686684899 * e[k-1]
  //             + 0.011993116567433 * e[k-2].
  // El error actual NO interviene directamente: se guarda para la proxima muestra.
  // Por eso, tras reiniciar, el primer calculo da cero aunque haya error actual.
  float salida = 1.987714034298315f * salidaAnterior - 0.987743604415780 * salidaAnteanterior + COEF_ERROR_ANTERIOR * errorAnterior - COEF_ERROR_ANTEANTERIOR * errorAnteanterior;
  // Saturacion de la orden PWM: 0 = apagado, 255 = encendido continuo.
  // El ternario siguiente deja 255 si se supera el maximo, 0 si es negativa
  // y conserva la salida cuando esta dentro del rango permitido.
  //salida = constrain(salida, 0.0f, 255.0f);
  (salida>255.0f) ? salida = 255.0f : (salida<0.0f) ? salida = 0.0f : salida = salida;

  // Desplazar el historial despues de calcular y limitar la salida.
  // Primero copiar el valor anterior al anteanterior para no perderlo.
  // Se realimenta la salida SATURADA, no la salida sin limitar ni el entero PWM.
  errorAnteanterior = errorAnterior;
  errorAnterior = errorActual;
  salidaAnteanterior = salidaAnterior;
  salidaAnterior = salida;

  // Conservar decimales en el historial; convertir solo al aplicar PWM.
  // static_cast<int> trunca: por ejemplo, 12.9 pasa a 12 y 0.9 pasa a 0.
  return static_cast<int>(salida);
}

float leerTemperatura()
{
  // ADC de 10 bits: lectura entre 0 y 1023.
  int lectura = analogRead(TERMISTOR_PIN);

  // Los extremos no permiten una conversion valida con estas formulas.
  // NAN representa una medicion invalida; loop() la detecta y apaga la salida.
  if (lectura <= 0 || lectura >= 1023)
  {
    return NAN;
  }

  // Despejar la resistencia del termistor a partir del divisor de tension.
  // Esta formula supone: alimentacion -> resistencia fija -> A13 -> NTC -> GND,
  // con alimentacion del divisor igual a la referencia de conversion del ADC.
  float resistencia = RESISTENCIA_SERIE * lectura / (1023.0 - lectura);

  // Modelo Beta: 1/T = 1/Tnominal + ln(R/Rnominal)/Beta, con T en kelvin.
  // Las operaciones siguientes construyen esa expresion paso a paso.
  float temperatura = resistencia / RESISTENCIA_NOMINAL;
  temperatura = log(temperatura); // Logaritmo natural de R/Rnominal.
  temperatura /= BETA; // Termino ln(R/Rnominal)/Beta.
  temperatura += 1.0 / (TEMPERATURA_NOMINAL + 273.15); // Obtener 1/T.
  temperatura = 1.0 / temperatura; // Invertir para obtener kelvin.
  temperatura -= 273.15; // Convertir kelvin a grados Celsius.

  return temperatura;
}

void setup()
{
  // Se ejecuta una sola vez al encender o reiniciar la placa.
  // El monitor serie debe usar la misma velocidad: 115200 baudios.
  Serial.begin(115200);
  pinMode(HEATER_PIN, OUTPUT);
  reiniciarControl();

  ahora = millis(); // La primera muestra se ejecutara 100 ms despues.
}

void loop()
{
  // loop() se repite continuamente. millis() permite esperar sin usar delay().
  const unsigned long instante = millis();
  // La resta sin signo permite manejar el desbordamiento normal de millis().
  // Solo se ejecuta una muestra al cumplirse el periodo; no recupera muestras
  // perdidas si otra operacion demora mas de 100 ms.
  if (instante - ahora >= PERIODO_CONTROL_MS)
  {
    ahora = instante; // Tomar esta ejecucion como referencia para la siguiente.
    float temperatura = leerTemperatura();

    if (isnan(temperatura))
    {
      // Ante una lectura invalida, apagar y borrar el historial.
      // Omitir esta fila serie y volver a intentar en el siguiente periodo.
      reiniciarControl();
      return;
    }

    // Error en grados Celsius: positivo si falta calentar; negativo si se
    // supera la consigna. El controlador lo guarda en su historial.
    float errorActual = CONSIGNA_C - temperatura;
    int pwm = calcularControl(errorActual);
    // El hardware mantiene el PWM entre muestras; 100 ms es el periodo del
    // calculo del control, no el periodo de los pulsos PWM.
    analogWrite(HEATER_PIN, pwm);

    // CSV: tiempo_ms,temperatura_C,pwm. Omitir muestras invalidas.
    // Una fila por muestra valida, sin etiquetas; temperatura con 2 decimales.
    // write(',') envia un separador y println() termina la fila.
    Serial.print(ahora);
    Serial.write(',');
    Serial.print(temperatura, 2);
    Serial.write(',');
    Serial.println(pwm);
  }
}
