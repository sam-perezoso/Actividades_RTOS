
#include <Arduino.h>
#include "BluetoothSerial.h"

// ==================================================
// CONFIGURACION DE PINES
// ==================================================

#define PIN_PWM         LED_BUILTIN

#define PIN_ENTRADA1    16
#define PIN_ENTRADA2    17

#define PIN_LED1        22
#define PIN_LED2        23

// ==================================================
// CONFIGURACION PWM
// ==================================================

#define PWM_FREQUENCY   1000
#define PWM_RESOLUTION  8

// ==================================================
// CONFIGURACION TIMER
// ==================================================

#define TIMER_PERIOD_MS 500

// ==================================================
// OBJETOS Y VARIABLES COMPARTIDAS
// ==================================================

BluetoothSerial SerialBT;

SemaphoreHandle_t mutexPWM;

TimerHandle_t timerParpadeo;

TaskHandle_t handleEntradas = NULL;

uint8_t pwmCompartido = 0;

// Notificaciones de la tarea de entradas
#define NOTIF_GPIO   (1UL << 0)
#define NOTIF_TIMER  (1UL << 1)

// ==================================================
// INTERRUPCIONES GPIO
// ==================================================

void ARDUINO_ISR_ATTR ISR_Entrada1() {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handleEntradas != NULL) {
        xTaskNotifyFromISR(
            handleEntradas,
            NOTIF_GPIO,
            eSetBits,
            &xHigherPriorityTaskWoken
        );
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void ARDUINO_ISR_ATTR ISR_Entrada2() {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handleEntradas != NULL) {
        xTaskNotifyFromISR(
            handleEntradas,
            NOTIF_GPIO,
            eSetBits,
            &xHigherPriorityTaskWoken
        );
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// ==================================================
// TIMER FREERTOS
// ==================================================

void CallbackTimer(TimerHandle_t xTimer) {

    // El callback se ejecuta en la tarea de timers.
    // Solamente notifica a la tarea de Core 1.

    if (handleEntradas != NULL) {
        xTaskNotify(handleEntradas, NOTIF_TIMER, eSetBits);
    }
}

// ==================================================
// CORE 0: COMUNICACION BLUETOOTH
// ==================================================

void Instructor(void *pvParameters) {

    char comando[6];
    uint8_t indice = 0;

    for (;;) {

        while (SerialBT.available()) {

            char dato = SerialBT.read();

            // Ignorar terminadores
            if (dato == '\r' || dato == '\n') {
                continue;
            }

            // Sincronizar el inicio del comando
            if (dato == 'L') {
                comando[0] = dato;
                indice = 1;
                continue;
            }

            if (indice > 0) {

                comando[indice++] = dato;

                // Comando completo: LEDxxx
                if (indice == 6) {

                    indice = 0;

                    // Validar encabezado
                    if (comando[0] == 'L' &&
                        comando[1] == 'E' &&
                        comando[2] == 'D' &&
                        comando[3] >= '0' && comando[3] <= '9' &&
                        comando[4] >= '0' && comando[4] <= '9' &&
                        comando[5] >= '0' && comando[5] <= '9') {

                        int porcentaje =
                            (comando[3] - '0') * 100 +
                            (comando[4] - '0') * 10 +
                            (comando[5] - '0');

                        if (porcentaje <= 100) {

                            uint8_t pwm = map(
                                porcentaje,
                                0,
                                100,
                                0,
                                255
                            );

                            // Actualizar variable compartida
                            if (xSemaphoreTake(
                                mutexPWM,
                                portMAX_DELAY
                            )) {

                                pwmCompartido = pwm;

                                xSemaphoreGive(mutexPWM);
                            }

                            // ECO de confirmacion
                            SerialBT.print("ECO:");
                            SerialBT.println(comando);
                        }
                        else {
                            SerialBT.println("ERR:RANGO");
                        }
                    }
                    else {
                        SerialBT.println("ERR:FORMATO");
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ==================================================
// CORE 1: CONTROL PWM
// ==================================================

void ControlPWM(void *pvParameters) {

    uint8_t pwmLocal = 0;
    uint8_t pwmAnterior = 255;

    for (;;) {

        if (xSemaphoreTake(mutexPWM, portMAX_DELAY)) {

            pwmLocal = pwmCompartido;

            xSemaphoreGive(mutexPWM);
        }

        // Actualizar solamente cuando exista un cambio
        if (pwmLocal != pwmAnterior) {

            ledcWrite(PIN_PWM, pwmLocal);

            pwmAnterior = pwmLocal;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ==================================================
// CORE 1: LECTURA DE ENTRADAS Y LEDS
// ==================================================

void ControlEntradas(void *pvParameters) {

    bool entrada1 = false;
    bool entrada2 = false;

    bool estadoLED1 = false;
    bool estadoLED2 = false;

    uint32_t notificaciones;

    for (;;) {

        // Esperar una interrupcion o notificacion del timer
        xTaskNotifyWait(
            0,
            0xFFFFFFFFUL,
            &notificaciones,
            portMAX_DELAY
        );

        // ------------------------------------------
        // ACTUALIZACION DE ENTRADAS
        // ------------------------------------------

        if (notificaciones & NOTIF_GPIO) {

            // INPUT_PULLUP: LOW significa activo
            bool nuevoEstado1 = (digitalRead(PIN_ENTRADA1) == LOW);
            bool nuevoEstado2 = (digitalRead(PIN_ENTRADA2) == LOW);

            if (nuevoEstado1 != entrada1) {

                entrada1 = nuevoEstado1;

                estadoLED1 = entrada1;

                digitalWrite(PIN_LED1, estadoLED1);
            }

            if (nuevoEstado2 != entrada2) {

                entrada2 = nuevoEstado2;

                estadoLED2 = entrada2;

                digitalWrite(PIN_LED2, estadoLED2);
            }
        }

        // ------------------------------------------
        // PARPADEO DE LEDS
        // ------------------------------------------

        if (notificaciones & NOTIF_TIMER) {

            if (entrada1) {
                estadoLED1 = !estadoLED1;
            }
            else {
                estadoLED1 = false;
            }

            if (entrada2) {
                estadoLED2 = !estadoLED2;
            }
            else {
                estadoLED2 = false;
            }

            digitalWrite(PIN_LED1, estadoLED1);
            digitalWrite(PIN_LED2, estadoLED2);
        }
    }
}

// ==================================================
// SETUP
// ==================================================

void setup() {

    Serial.begin(115200);

    // ----------------------------------------------
    // CONFIGURAR GPIO
    // ----------------------------------------------

    pinMode(PIN_ENTRADA1, INPUT_PULLUP);
    pinMode(PIN_ENTRADA2, INPUT_PULLUP);

    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);

    digitalWrite(PIN_LED1, LOW);
    digitalWrite(PIN_LED2, LOW);

    // ----------------------------------------------
    // CONFIGURAR LEDC
    // ----------------------------------------------



        ledcAttach(PIN_PWM, PWM_FREQUENCY, PWM_RESOLUTION);
        ledcWrite(PIN_PWM, 0);

    // ----------------------------------------------
    // INICIALIZAR BLUETOOTH
    // ----------------------------------------------

    if (!SerialBT.begin("ESP_2core")) {
        Serial.println("ERROR: Bluetooth");
    }
    else {
        Serial.println("Bluetooth iniciado");
    }

    // ----------------------------------------------
    // CREAR MUTEX
    // ----------------------------------------------

    mutexPWM = xSemaphoreCreateMutex();

    if (mutexPWM == NULL) {
        Serial.println("ERROR: Mutex");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // ----------------------------------------------
    // CREAR TAREAS
    // ----------------------------------------------

    xTaskCreatePinnedToCore(Instructor, "Instructor", 4096, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(ControlPWM, "ControlPWM", 2048, NULL, 3, NULL, 1);

    xTaskCreatePinnedToCore(ControlEntradas, "Entradas", 2048, NULL, 3, &handleEntradas, 1);

    // ----------------------------------------------
    // CREAR TIMER FREERTOS
    // ----------------------------------------------

    timerParpadeo = xTimerCreate(
        "Parpadeo",
        pdMS_TO_TICKS(TIMER_PERIOD_MS),
        pdTRUE,
        NULL,
        CallbackTimer
    );

    if (timerParpadeo != NULL) {
        xTimerStart(timerParpadeo, 0);
    }
    else {
        Serial.println("ERROR: Timer");
    }

    // ----------------------------------------------
    // ACTIVAR INTERRUPCIONES
    // ----------------------------------------------

    attachInterrupt(
        digitalPinToInterrupt(PIN_ENTRADA1),
        ISR_Entrada1,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(PIN_ENTRADA2),
        ISR_Entrada2,
        CHANGE
    );

    // Leer estados iniciales
    xTaskNotify(handleEntradas, NOTIF_GPIO, eSetBits);

    Serial.println("Sistema iniciado");
}

// ==================================================
// LOOP
// ==================================================

void loop() {

    vTaskDelay(pdMS_TO_TICKS(1000));
}
