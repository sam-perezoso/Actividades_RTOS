#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Configuración de entradas y salidas
#define LED_PIN 2
#define motor 19
#define asientoD 16
#define asientoT 17
#define cinturonD 5
#define cinturonT 18

// Identificador del temporizador
TimerHandle_t temporizadorLED;

// Sensor
Adafruit_MPU6050 mpu;

// Variables compartidas
volatile bool sM = false;
volatile bool blink = false;

volatile bool sD = false;
volatile bool sT = false;

volatile bool scD = false;
volatile bool scT = false;

// Memoria de uso previo del asiento trasero
volatile bool FT = false;


// Callback del temporizador
void callbackLED(TimerHandle_t xTimer) {

    if (blink == true) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    else {
        digitalWrite(LED_PIN, LOW);
    }
}


// Interrupción del motor
void IRAM_ATTR stmotor() {

    sM = !digitalRead(motor);
    FT = false;
}


// Tarea de lectura de sensores
void estados(void *parameter) {

    bool saT = false;

    while (1) {

        // Lectura de ocupación
        sD = !digitalRead(asientoD);
        sT = !digitalRead(asientoT);

        // Lectura de cinturones
        scD = !digitalRead(cinturonD);
        scT = !digitalRead(cinturonT);

        // Detectar transición ocupado -> desocupado
        if (saT && !sT) {
            FT = true;
        }

        // Actualizar estado anterior
        saT = sT;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


// Tarea de evaluación de condiciones
void control(void *parameter) {

    while (1) {

        if (sM && sD) {

            if (sT && FT) {

                // Segunda ocupación o posterior
                blink = !(scD && scT);

            }
            else {

                // Primera ocupación o asiento trasero vacío
                blink = !scD;

            }

        }
        else {

            // Motor apagado o asiento delantero desocupado
            blink = false;

        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void acelerometro(void *parameter) {

    sensors_event_t a, g, temp;

    float aceleracionAbs;

    while (1) {

        // Obtener mediciones del sensor
        mpu.getEvent(&a, &g, &temp);

        // Calcular magnitud de aceleracion
        aceleracionAbs = sqrtf(
            a.acceleration.x * a.acceleration.x +
            a.acceleration.y * a.acceleration.y +
            a.acceleration.z * a.acceleration.z
        );

        // Mostrar resultado
        Serial.print("Aceleracion absoluta: ");
        Serial.print(aceleracionAbs, 3);
        Serial.println(" m/s^2");

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void setup() {

    Serial.begin(115200);

    // Inicializar MPU6050
    if (!mpu.begin(0x68, &Wire)) {

        Serial.println("Error: MPU6050 no encontrado");

        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Serial.println("MPU6050 inicializada correctamente");

    // Configuracion del acelerometro
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(motor, INPUT_PULLUP);
    pinMode(asientoD, INPUT_PULLUP);
    pinMode(asientoT, INPUT_PULLUP);
    pinMode(cinturonD, INPUT_PULLUP);
    pinMode(cinturonT, INPUT_PULLUP);

    // Lectura inicial del motor
    sM = !digitalRead(motor);

    // Crear temporizador
    temporizadorLED = xTimerCreate("callbackLED",pdMS_TO_TICKS(500),pdTRUE,NULL,callbackLED);

    // Crear tareas
    xTaskCreatePinnedToCore(control,"control",4096,NULL,2,NULL,app_cpu);

    xTaskCreatePinnedToCore(estados,"estados",2048,NULL,2,NULL,app_cpu);

    xTaskCreatePinnedToCore(
        acelerometro,
        "acelerometro",
        4096,
        NULL,
        1,
        NULL,
        app_cpu
    );
    // Interrupción del motor
    attachInterrupt(digitalPinToInterrupt(motor),stmotor,CHANGE);


    // Iniciar temporizador
    if (temporizadorLED != NULL) {
        xTimerStart(temporizadorLED, 0);
    }
}


void loop() {

}
