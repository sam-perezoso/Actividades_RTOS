
#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

// ============================================================
// SELECCION DEL NUCLEO
// ============================================================
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// ============================================================
// CONFIGURACION DEL TERMISTOR
// ============================================================
const int PIN_NTC = 36;
const float VCC = 3.27f;
const float R_FIJA = 6570.0f;

const float Ro = 100000.0f;
const float To = 298.15f;
const float BETA = 3950.0f;

// ============================================================
// CONFIGURACION DEL PID
// ============================================================
const float kp = 0.50f;
const float ki = 0.1f;
const float kd = 0.01f;

const float SALIDA_MIN = 0.0f;
const float SALIDA_MAX = 100.0f;

const float SETPOINT_MIN = 0.0f;
const float SETPOINT_MAX = 200.0f;

const float MAX_TEMP_SEGURA = 200.0f;

// ============================================================
// CONFIGURACION DE LA ACTUACION
// ============================================================
const int PIN_SSR = 13;

// Ventana de tiempo: 1 segundos
const uint32_t VENTANA_MS = 1000;

// Frecuencia de actualizacion de la salida
const uint32_t PERIODO_ACTUACION_MS = 10;

// Se supone que HIGH activa el SSR.
const int SSR_ON = HIGH;
const int SSR_OFF = LOW;

// ============================================================
// COMUNICACION ENTRE TAREAS
// ============================================================
static QueueHandle_t q_datos;
static QueueHandle_t q_potencia;
static SemaphoreHandle_t mutex_setpoint;

// Variables compartidas
float setpoint = 25.0f;

// ============================================================
// TAREA 1: SENSADO
// ============================================================
void sensado(void *parameter) {

  TickType_t instante = xTaskGetTickCount();

  while (1) {

    float voltaje = analogReadMilliVolts(PIN_NTC) / 1000.0f;

    float temperaturaC = NAN;

    if (voltaje > 0.0f && voltaje < VCC) {  //rango medible del ADC

      float R_NTC = R_FIJA * voltaje / (VCC - voltaje);

      if (R_NTC > 0.0f && isfinite(R_NTC)) {  //interpretación correcta de NTC

        float temperaturaK =  1.0f / ((1.0f / To) + (logf(R_NTC / Ro) / BETA));

        temperaturaC = temperaturaK - 273.15f;
      }
    }

    // Enviar la medicion mas reciente
    xQueueOverwrite(q_datos, &temperaturaC);

    vTaskDelayUntil(
        &instante,
        pdMS_TO_TICKS(100)
    );
  }
}

// ============================================================
// TAREA 2: CONTROL PID
// Entrega una potencia entre 0 y 100 %.
// ============================================================
void control(void *parameter) {

  float medicion;
  float referencia;

  float integral = 0.0f;
  float temperaturaAnterior = 0.0f;

  bool primeraMedicion = true;
  uint32_t tiempoAnterior = 0;

  while (1) {

    if (xQueueReceive( q_datos, &medicion, pdMS_TO_TICKS(1250)) != pdTRUE) {

      // Si no llega una medicion, potencia cero
      float potenciaCero = 0.0f;
      xQueueOverwrite(q_potencia, &potenciaCero);

      integral = 0.0f;
      primeraMedicion = true;

      continue;
    }

    // Obtener el setpoint
    if (xSemaphoreTake(mutex_setpoint, portMAX_DELAY) == pdTRUE) {
      referencia = setpoint;
      xSemaphoreGive(mutex_setpoint);
    }

    // Medicion invalida
    if (!isfinite(medicion)) {

      float potenciaCero = 0.0f;
      xQueueOverwrite(q_potencia, &potenciaCero);

      integral = 0.0f;
      primeraMedicion = true;

      Serial.println("Sensor invalido: potencia = 0 %");
      continue;
    }

    // Proteccion por sobretemperatura
    if (medicion >= MAX_TEMP_SEGURA) {

      float potenciaCero = 0.0f;
      xQueueOverwrite(q_potencia, &potenciaCero);

      integral = 0.0f;
      primeraMedicion = true;

      Serial.println("Sobretemperatura: potencia = 0 %");
      continue;
    }

    uint32_t tiempoActual = millis();

    // Inicializar el PID
    if (primeraMedicion) {

      tiempoAnterior = tiempoActual;
      temperaturaAnterior = medicion;
      primeraMedicion = false;

      float potenciaCero = 0.0f;
      xQueueOverwrite(q_potencia, &potenciaCero);

      continue;
    }

    float dt =
        (tiempoActual - tiempoAnterior) / 1000.0f;

    if (dt <= 0.0f) {
      continue;
    }

    // Error
    float error = referencia - medicion;

    // Derivada sobre la temperatura medida
    float derivada = -(medicion - temperaturaAnterior) / dt;

    // Integral propuesta
    float integralPropuesta = integral + error * dt;

    float salidaPropuesta = kp * error + ki * integralPropuesta + kd * derivada;

    // Anti-windup
    bool saturadoArriba = salidaPropuesta > SALIDA_MAX;

    bool saturadoAbajo = salidaPropuesta < SALIDA_MIN;

    if ((!saturadoArriba && !saturadoAbajo) || (saturadoArriba && error < 0.0f) || (saturadoAbajo && error > 0.0f)) {
      integral = integralPropuesta;
    }

    // Salida final del PID
    float salidaPID = kp * error + ki * integral + kd * derivada;

    salidaPID = constrain(salidaPID,SALIDA_MIN,SALIDA_MAX);

    // Enviar la salida al sistema de actuacion
    xQueueOverwrite(q_potencia, &salidaPID);

    Serial.print("Temperatura: ");
    Serial.print(medicion, 2);

    Serial.print(" °C | Setpoint: ");
    Serial.print(referencia, 2);

    Serial.print(" °C | Error: ");
    Serial.print(error, 2);

    Serial.print(" °C | Salida PID: ");
    Serial.print(salidaPID, 2);

    Serial.println(" %");

    // Actualizar estados del PID
    temperaturaAnterior = medicion;
    tiempoAnterior = tiempoActual;
  }
}

// ============================================================
// TAREA 3: LECTURA SERIAL DEL SETPOINT
// ============================================================
void serialsetpoint(void *parameter) {

  char entrada[32];
  size_t indice = 0;

  while (1) {

    while (Serial.available() > 0) {

      char caracter = Serial.read();

      if (caracter == '\n' || caracter == '\r') {

        if (indice > 0) {

          entrada[indice] = '\0';

          char *fin;
          float nuevoSetpoint = strtof(entrada, &fin);

          while (*fin == ' ' || *fin == '\t') {
            fin++;
          }

          bool valido =
              fin != entrada &&
              *fin == '\0' &&
              isfinite(nuevoSetpoint) &&
              nuevoSetpoint >= SETPOINT_MIN &&
              nuevoSetpoint <= SETPOINT_MAX;

          if (valido) {

            if (xSemaphoreTake(mutex_setpoint,portMAX_DELAY) == pdTRUE) {

              setpoint = nuevoSetpoint;
              

              Serial.print("Nuevo setpoint: ");
              Serial.print(nuevoSetpoint, 2);
              Serial.println(" °C");
              xSemaphoreGive(mutex_setpoint);
            }

          } else {
            Serial.println("Entrada invalida.");
          }

          indice = 0;
        }

      } else if (caracter != '\b' && caracter != 127) {

        if (indice < sizeof(entrada) - 1) {
          entrada[indice++] = caracter;
        } else {
          indice = 0;
          Serial.println("Entrada demasiado larga.");
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ============================================================
// TAREA 4: ACTUACION POR VENTANA DE TIEMPO
// ============================================================
void actuador(void *parameter) {

  float potenciaSolicitada = 0.0f;
  float potenciaVentana = 0.0f;

  uint32_t inicioVentana = millis();

  // Mantener el SSR apagado al iniciar
  digitalWrite(PIN_SSR, SSR_OFF);

  while (1) {

    uint32_t ahora = millis();

    // Leer la salida mas reciente del PID sin bloquear
    float nuevaPotencia;

    if (xQueueReceive(q_potencia,&nuevaPotencia,0) == pdTRUE) {

      if (isfinite(nuevaPotencia)) {
        potenciaSolicitada = constrain(nuevaPotencia, 0.0f, 100.0f);
      } else {
        potenciaSolicitada = 0.0f;
      }
    }


    if ((uint32_t)(ahora - inicioVentana) >= VENTANA_MS) {

      inicioVentana += VENTANA_MS;
      potenciaVentana = potenciaSolicitada;

      if ((uint32_t)(ahora - inicioVentana) >= VENTANA_MS) {
        inicioVentana = ahora;
      }
    }

    // Tiempo de encendido proporcional al porcentaje
    uint32_t tiempoEncendido = (uint32_t)(potenciaVentana * VENTANA_MS / 100.0f);

    uint32_t tiempoTranscurrido = ahora - inicioVentana;

    if (potenciaVentana > 0.0f && tiempoTranscurrido < tiempoEncendido) {

      digitalWrite(PIN_SSR, SSR_ON);

    } else {

      digitalWrite(PIN_SSR, SSR_OFF);
    }

    vTaskDelay(pdMS_TO_TICKS(PERIODO_ACTUACION_MS));
  }
}

// ============================================================
// CONFIGURACION INICIAL
// ============================================================
void setup() {

  Serial.begin(115200);

  pinMode(PIN_SSR, OUTPUT);
  digitalWrite(PIN_SSR, SSR_OFF);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_NTC, ADC_11db);

  // Colas de longitud 1: conservan el dato mas reciente
  q_datos = xQueueCreate(1, sizeof(float));
  q_potencia = xQueueCreate(1, sizeof(float));

  mutex_setpoint = xSemaphoreCreateMutex();

  if (q_datos == NULL || q_potencia == NULL || mutex_setpoint == NULL) {

    digitalWrite(PIN_SSR, SSR_OFF);
    Serial.println("Error al crear cola o mutex.");

    while (1) {
      digitalWrite(PIN_SSR, SSR_OFF);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }

  // Inicializar la potencia solicitada en cero
  float potenciaInicial = 0.0f;
  xQueueOverwrite(q_potencia, &potenciaInicial);

  xTaskCreatePinnedToCore(
      sensado, "sensado",
      4096, NULL, 1, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
      control, "control",
      4096, NULL, 2, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
      serialsetpoint, "serialsetpoint",
      4096, NULL, 1, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
      actuador, "actuador",
      2048, NULL, 2, NULL, app_cpu
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
