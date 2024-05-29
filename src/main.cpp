#include <Arduino.h>
#include <FixedPoints.h> // del autor Pharap https://github.com/Pharap/FixedPointsArduino
#include <driver/adc.h>
#include <driver/timer.h>

int adc0Value = 0;     // Valor leído por el ADC0 (ruido)
int adc1Value = 0;     // Valor leído por el ADC1 (ruido+señal)
int adcValue = 0;
bool AdcReady = false; // Bandera que indica si el ADC ha terminado la conversión
typedef SFixed<2, 29> MyFixedType;  // Definimos un tipo de variable con signo Q2,29
MyFixedType w[10], x[10], sum, e, y;  // Define las variables globales para el filtro adaptativo
MyFixedType mu = 0.0001;             // Define el paso de aprendizaje
int in, out;

// Prototipo de la función de interrupción del temporizador
void IRAM_ATTR onTimer(void* arg);

void setupADC() {
  adc1_config_width(ADC_WIDTH_BIT_12); // Configura la resolución del ADC a 12 bits
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0); // Configura la atenuación para A0 (GPIO34)
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0); // Configura la atenuación para A1 (GPIO35)
}

void setupTimer() {
  timer_config_t config = {
    .alarm_en = TIMER_ALARM_EN,
    .counter_en = TIMER_PAUSE,
    .intr_type = TIMER_INTR_LEVEL,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = 80   // 1 us de incremento
  };
  timer_init(TIMER_GROUP_0, TIMER_0, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 1000); // 1ms
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_register(TIMER_GROUP_0, TIMER_0, onTimer, NULL, ESP_INTR_FLAG_IRAM, NULL);
  timer_start(TIMER_GROUP_0, TIMER_0);
}

void IRAM_ATTR onTimer(void* arg) {
  AdcReady = true;
  timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
  timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

void setup() {
  for (int i = 0; i < 10; i++) { // Setea un valor inicial a las vectores
    x[i] = 0; w[i] = 0;
  }

  pinMode(25, OUTPUT); // DAC1 (GPIO25)
  pinMode(26, OUTPUT); // DAC2 (GPIO26)

  analogWriteResolution(12); // Define la resolución del DAC en 12 bits

  setupADC(); // Llama a la función que configura el ADC
  setupTimer(); // Llama a la función que configura el temporizador
}

// Bucle principal del programa
void loop() {
  if (AdcReady) {
    adc0Value = adc1_get_raw(ADC1_CHANNEL_6); // Leer el valor del ADC0
    adc1Value = adc1_get_raw(ADC1_CHANNEL_7); // Leer el valor del ADC1

    for (int i = 0; i < 10 - 1; i++) { // Realiza un desplazamiento de las muestras de la señal
      x[i] = x[i + 1];
    }

    sum = 0;

    x[10 - 1] = (adc0Value * 0.0002442); // Normalizamos la lectura de ruido ADC0 1/4095  
    x[10 - 1] -= 0.5; // Quitamos el offset

    for (int i = 0; i < 10; i++) { // Aplicamos el filtro fir a la señal de ruido
      sum = sum + w[i] * x[10 - 1 - i];
    }

    y = (adc1Value * 0.0002442); // Normalizamos la lectura de ruido ADC1
    y -= 0.5; // Quitamos el offset
    e = y - sum; // Generamos la señal de error

    for (int i = 0; i < 10; i++) { // Aplicamos el algoritmo LMS
      w[i] = w[i] + 2 * mu * e * x[10 - 1 - i];
    }
    e += 0.5; // Agregamos el offset a la señal deseada
    y += 0.5;

    out = int(roundFixed(SFixed<13, 18>(e) * 4095)); // Casteamos a un valor Q13,18 para desnormalizar y lo convertimos lo redondeamos
    in = int(roundFixed(SFixed<13, 18>(y) * 4095));

    dacWrite(25, out); // Representamos la señal a través del DAC1 (GPIO25)
    dacWrite(26, in); // Representamos la señal a través del DAC2 (GPIO26)

    AdcReady = false;
  }
}
