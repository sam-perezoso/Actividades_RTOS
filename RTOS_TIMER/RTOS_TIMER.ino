#include <Arduino.h>

// Configuración del LED
#define LED_PIN 2

// Identificador del temporizador
TimerHandle_t temporizadorLED;

// Función callback del temporizador
void callbackLED(TimerHandle_t xTimer) {

    // Cambiar el estado actual del LED
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void setup() {

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Crear temporizador:
    // Nombre, periodo, auto-recarga, identificador, callback
    temporizadorLED = xTimerCreate(
        "TemporizadorLED",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        callbackLED
    );

    // Iniciar el temporizador
    if (temporizadorLED != NULL) {
        xTimerStart(temporizadorLED, 0);
    }
}

void loop() {
    // El temporizador trabaja independientemente del loop()
}