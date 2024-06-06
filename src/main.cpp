#include <Arduino.h>

// Pines ADC y DAC
const int adc0Pin = 34;  // Pin ADC para el ruido
const int adc1Pin = 35;  // Pin ADC para el ruido + señal
const int dacPin = 25;   // Pin DAC para la salida del filtro
const int flagPin = 13;  // Pin de salida como flag

// Variables del filtro adaptativo LMS
const int filterLength = 10; // Longitud del filtro
float w[filterLength] = {0}; // Coeficientes del filtro
float x[filterLength] = {0}; // Valores de entrada del filtro
float mu = 0.01;           // Tasa de aprendizaje
float y, e, sum;             // Salidas intermedias

// Configuración de la tasa de muestreo
const int sampleRate = 6000; // Tasa de muestreo en Hz
unsigned long lastSampleTime = 0; // Tiempo de la última muestra

// Setup
void setup() {
  Serial.begin(115200); // Inicializa la comunicación serial para la salida de datos
  analogReadResolution(12);  // Configura la resolución del ADC a 12 bits
  analogWriteResolution(8);  // Configura la resolución del DAC a 8 bits

  // Configura los pines ADC y DAC
  pinMode(adc0Pin, INPUT);
  pinMode(adc1Pin, INPUT);
  pinMode(dacPin, OUTPUT);
  pinMode(flagPin, OUTPUT); // Configura el pin flag como salida
}

// Loop principal
void loop() {
  // Asegura una tasa de muestreo constante
  if (millis() - lastSampleTime >= 1000 / sampleRate) {
    lastSampleTime = millis();

    // Lee los valores de los ADC
    int adc0Value = analogRead(adc0Pin);
    int adc1Value = analogRead(adc1Pin);

    // Normaliza las lecturas del ADC
    float normalizedAdc0 = (adc0Value / 4095.0) - 0.5;
    float normalizedAdc1 = (adc1Value / 4095.0) - 0.5;

    // Establece el pin flag a HIGH al iniciar el cálculo
    digitalWrite(flagPin, HIGH);

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

    // Establece el pin flag a LOW al finalizar el cálculo
    digitalWrite(flagPin, LOW);

    // Desnormaliza la señal de error para el DAC
    float outputValue = e + 0.5;
    int dacOutput = int(outputValue * 255.0);

    // Escribe el valor al DAC
    dacWrite(dacPin, dacOutput);
  }
}
