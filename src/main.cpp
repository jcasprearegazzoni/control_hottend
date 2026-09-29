#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#define TERMISTOR_PIN A13
#define HEATER_PIN 10
#define RESISTENCIA_SERIE 4700.0
#define RESISTENCIA_NOMINAL 100000.0
#define TEMPERATURA_NOMINAL 25.0
#define BETA 3950.0
constexpr float CONSIGNA_C = 80.0f;
constexpr uint32_t PERIODO_CONTROL_MS = 100;
constexpr float COEF_ERROR_ANTERIOR = 0.012022686684899f;
constexpr float COEF_ERROR_ANTEANTERIOR = -0.011993116567433f;
float salidaAnterior = 0.0f;
float salidaAnteanterior = 0.0f;
float errorAnterior = 0.0f;
float errorAnteanterior = 0.0f;
volatile bool controlPendiente = false;
uint32_t numeroMuestra = 0;
ISR(TIMER1_COMPA_vect) { controlPendiente = true; }
void configurarTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 24999;
  TCCR1B |= (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A);
  TCCR1B |= (1 << CS11) | (1 << CS10);
  sei();
}
void reiniciarControl() {}
int calcularControl(float errorActual) {
  float salida = 1.987714034298315f * salidaAnterior -
                 0.987743604415780f * salidaAnteanterior +
                 COEF_ERROR_ANTERIOR * errorAnterior -
                 COEF_ERROR_ANTEANTERIOR * errorAnteanterior;
  if (salida > 255.0f) {
    salida = 255.0f;
  } else if (salida < 0.0f) {
    salida = 0.0f;
  }
  errorAnteanterior = errorAnterior;
  errorAnterior = errorActual;
  salidaAnteanterior = salidaAnterior;
  salidaAnterior = salida;
  return static_cast<int>(salida);
}
float leerTemperatura() {
  int lectura = analogRead(TERMISTOR_PIN);
  if (lectura <= 0 || lectura >= 1023) {
    return NAN;
  }
  float resistencia = RESISTENCIA_SERIE * lectura / (1023.0f - lectura);
  float temperatura = resistencia / RESISTENCIA_NOMINAL;
  temperatura = log(temperatura);
  temperatura /= BETA;
  temperatura += 1.0f / (TEMPERATURA_NOMINAL + 273.15f);
  temperatura = 1.0f / temperatura;
  temperatura -= 273.15f;
  return temperatura;
}
void setup() {
  Serial.begin(115200);
  pinMode(HEATER_PIN, OUTPUT);
  reiniciarControl();
  configurarTimer1();
}
void loop() {
  if (controlPendiente) {
    controlPendiente = false;
    numeroMuestra++;
    uint32_t tiempo_ms = numeroMuestra * PERIODO_CONTROL_MS;
    float temperatura = leerTemperatura();
    if (isnan(temperatura)) {
      reiniciarControl();
      return;
    }
    float errorActual = CONSIGNA_C - temperatura;
    int pwm = calcularControl(errorActual);
    analogWrite(HEATER_PIN, pwm);
    Serial.print(tiempo_ms);
    Serial.write(',');
    Serial.print(temperatura, 2);
    Serial.write(',');
    Serial.println(pwm);
  }
}
