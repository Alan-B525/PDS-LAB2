#include <Arduino.h>
#include <FixedPoints.h>  // Incluye la librería de punto fijo de Pharap

// Pines ADC y DAC
const int adc0Pin = 34;  // Pin ADC para el ruido
const int adc1Pin = 35;  // Pin ADC para el ruido + señal
const int dacPin = 25;   // Pin DAC para la salida del filtro

// Variables del filtro adaptativo LMS
const int filterLength = 10; // Longitud del filtro
typedef SFixed<2, 29> MyFixedType;  // Definimos un tipo de variable con signo Q2.29
MyFixedType w[filterLength];       // Coeficientes del filtro
MyFixedType x[filterLength];       // Valores de entrada del filtro
MyFixedType mu = MyFixedType(0.0001); // Tasa de aprendizaje
MyFixedType y, e, sum;               // Salidas intermedias

// Setup
void setup() {
  analogReadResolution(12);     // Configura la resolución del ADC a 12 bits
  analogWriteResolution(8);     // Configura la resolución del DAC a 8 bits

  // Configura los pines ADC y DAC
  pinMode(adc0Pin, INPUT);
  pinMode(adc1Pin, INPUT);
  pinMode(dacPin, OUTPUT);

  // Inicializa los coeficientes y valores de entrada del filtro
  for (int i = 0; i < filterLength; i++) {
    w[i] = 0;
    x[i] = 0;
  }
}

// Loop principal
void loop() {
  // Lee los valores de los ADC
  int adc0Value = analogRead(adc0Pin);
  int adc1Value = analogRead(adc1Pin);

  // Normaliza las lecturas del ADC
  MyFixedType normalizedAdc0 = MyFixedType(adc0Value) / MyFixedType(4095.0) - MyFixedType(0.5);
  MyFixedType normalizedAdc1 = MyFixedType(adc1Value) / MyFixedType(4095.0) - MyFixedType(0.5);

  // Desplaza los valores del buffer x
  for (int i = 0; i < filterLength - 1; i++) {
    x[i] = x[i + 1];
  }
  x[filterLength - 1] = normalizedAdc0;

  // Aplica el filtro FIR
  sum = 0;
  for (int i = 0; i < filterLength; i++) {
    sum += w[i] * x[filterLength - 1 - i];
  }

  // Calcula la señal de error
  y = normalizedAdc1;
  e = y - sum;

  // Actualiza los coeficientes del filtro usando el algoritmo LMS
  for (int i = 0; i < filterLength; i++) {
    w[i] += 2 * mu * e * x[filterLength - 1 - i];
  }

  // Desnormaliza la señal de error para el DAC
  MyFixedType outputValue = e + MyFixedType(0.5);
  int dacOutput = int(outputValue * MyFixedType(255.0));

  // Escribe el valor al DAC
  dacWrite(dacPin, dacOutput);
}
