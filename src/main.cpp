#include <Arduino.h>

// Define los pines que vamos a utilizar
const int adcPin = 34;  // ADC1_6
const int dacPin = 25;  // DAC_1 / ADC1_8

void setup() {
  // Inicialización de los puertos
  //Serial.begin(115200);  // Inicializa el puerto serie
  //delay(100);

  // Configura el canal ADC
  analogReadResolution(8);  // Resolución de 12 bits
  analogSetWidth(8);        // Establece el ancho en 12 bits
  analogSetAttenuation(ADC_2_5db);  // Ajuste de atenuación para el rango de entrada

  // Configura el canal DAC
  dacWrite(25, 0);  // Inicializa el DAC con un valor de 0
}

void loop() {
  // Lee el valor del ADC
  int adcValue = analogRead(adcPin);

  //adcValue /= 2;
  //adcValue += 1024;

  adcValue = adcValue;

  // Escribe el valor leído al DAC
  dacWrite(dacPin, adcValue);

  // Espera un tiempo antes de la próxima lectura
  //delay(100);
}
