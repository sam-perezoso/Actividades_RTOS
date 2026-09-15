#if CONFIG_FREERTOS_UNICORE

static const BaseType_t app_cpu = 0;

#else

static const BaseType_t app_cpu = 1;

#endif

#define SemV 16
#define SemA 17
#define SemR 21

//====================================================
// TAREA PARA GPIO 16 - 1 Hz
//====================================================

void verde(void *parameter){

  while(1){

    digitalWrite(SemV, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    digitalWrite(SemV, LOW);

    vTaskDelay(2500 / portTICK_PERIOD_MS);

  }

}


//====================================================
// TAREA PARA GPIO 17 - 2 Hz
//====================================================

void amarillo(void *parameter){
  int i = 0;
  while(1){
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    while(i<5){
    digitalWrite(SemA, HIGH);

    vTaskDelay(50 / portTICK_PERIOD_MS);

    digitalWrite(SemA, LOW);

    vTaskDelay(50 / portTICK_PERIOD_MS);
    i++;
    }
    
    i=0;
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

}

void rojo(void *parameter){

  while(1){
    vTaskDelay(2500 / portTICK_PERIOD_MS);
    digitalWrite(SemR, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(SemR, LOW);


  }
  
}


void setup() {

  // Configuración de pines

  pinMode(SemV, OUTPUT);
  pinMode(SemA, OUTPUT);
  pinMode(SemR, OUTPUT);

  Serial.begin(9600);
  //==================================================
  // CREAR TAREA GPIO 16
  //==================================================

  xTaskCreatePinnedToCore(

    verde,       // función a llamar
    "verde",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu            // núcleo
  );


  //==================================================
  // CREAR TAREA GPIO 17
  //==================================================

  xTaskCreatePinnedToCore(

    amarillo,       // función a llamar
    "amarillo",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

  //==================================================
  // CREAR TAREA BOTON
  //==================================================

  xTaskCreatePinnedToCore(

    rojo,       // función a llamar
    "rojo",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

}


void loop() {

  // El trabajo se realiza mediante las tareas de FreeRTOS.

}