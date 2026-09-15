#if CONFIG_FREERTOS_UNICORE

static const BaseType_t app_cpu = 0;

#else

static const BaseType_t app_cpu = 1;

#endif

#define INI 10
#define a0 17
#define a1 21
#define a2 16
#define a3 17
#define sh 21

void P0(void *parameter){
  while(1){

  }
}

void P1(void *parameter){
  while(1){

  }
}

void P2(void *parameter){
  while(1){

  }
}

void P3(void *parameter){
  while(1){

  }
}

void P4(void *parameter){
  while(1){

  }
}

void P5(void *parameter){
  while(1){

  }
}

void P6(void *parameter){
  while(1){

  }
}

void sensores(void *parameter){
  while(1){

  }
}


void setup() {

  // Configuración de pines

  pinMode(SemV, OUTPUT);
  pinMode(SemA, OUTPUT);
  pinMode(SemR, OUTPUT);

  Serial.begin(9600);


  xTaskCreatePinnedToCore(

    P0       // función a llamar
    "P0",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu            // núcleo
  );

  xTaskCreatePinnedToCore(

    P1,       // función a llamar
    "P1",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

  xTaskCreatePinnedToCore(

    P2,       // función a llamar
    "P2",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

  xTaskCreatePinnedToCore(

    P3,       // función a llamar
    "P3",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu            // núcleo
  );

  xTaskCreatePinnedToCore(

    P4,       // función a llamar
    "P4",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

  xTaskCreatePinnedToCore(

    P5,       // función a llamar
    "P5",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

  xTaskCreatePinnedToCore(

    P6,       // función a llamar
    "P6",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

    xTaskCreatePinnedToCore(

    sensores,       // función a llamar
    "sensores",    // nombre de tarea
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