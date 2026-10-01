
#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Variable compartida
char acumulador[20][4];
volatile int j = 0;

// Handles
TaskHandle_t tareaMonitorHandle;

// --------------------------------------------------
// Tarea 1
// --------------------------------------------------
void tarea1(void *parameter) {

  for (int i = 1; i <= 10; i++) {

    snprintf(acumulador[j], sizeof(acumulador[j]), "%da", i);

    Serial.print(acumulador[j]);

    j++;
  }

  vTaskDelete(NULL);
}

// --------------------------------------------------
// Tarea 2
// --------------------------------------------------
void tarea2(void *parameter) {

  for (int i = 1; i <= 10; i++) {

    snprintf(acumulador[j], sizeof(acumulador[j]), "%db", i);

    Serial.print(acumulador[j]);

    j++;
  }


  vTaskDelete(NULL);
}



void setup() {

  Serial.begin(115200);

  // Crear tarea 1
  xTaskCreatePinnedToCore(
    tarea1,
    "Tarea 1",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );

  // Crear tarea 2
  xTaskCreatePinnedToCore(
    tarea2,
    "Tarea 2",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );
}

// --------------------------------------------------
// Loop
// --------------------------------------------------
void loop() {

}