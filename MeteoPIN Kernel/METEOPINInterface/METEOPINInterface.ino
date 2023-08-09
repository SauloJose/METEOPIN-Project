/**************************************************************************
    PROTÓTIPO de Interface de configuração de estação Metereológica (ICEM)
    Arquivo fonte

    Descrição: Este projeto utiliza do SPIFFS do ESP32 para gerar uma
               interface de configurações de conexão e análise de dados
               conectando com WiFi ou SIM800

    Por: Saulo José Almeida Silva
    Última atualização: 09/08/2023
**************************************************************************/

/****************************************************************************************************
  |     NOME       |     CORE       |    PRIORIDADE     |                 DESCRIÇÃO                 |
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskBrain     |      01        |         03        |    Tem como objetivo analisar os dados    |
  |                |                |                   |  Colhidos pela tarefa sensor e então tomar|
  |                |                |                   |  decisões de ataque e defesa.             |
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskSensor    |      00        |         02        |  Tarefa responsável por analisar o ambi-  |
  |                |                |                   |ente e enviar as informações para o cerebro|
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskReconfig  |      01        |         02        |  Responsável por ligar ou desligar o robô |
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskPluvio    |      00        |         04        |  Responsável por realizar os movimentos   |
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskFireBase  |      00        |         01        | Responsável por enviar os dados para o BD |
  |----------------|----------------|-------------------|-------------------------------------------|
  |-vTaskMemory    |      01        |         05        | Leitura/escrita de arquivos               |
****************************************************************************************************/
//Incluindo o arquivo de configurações da interface
#include "ICEMconfig.h"

//Gerando filas
QueueHandle_t xQueueSensor;      //Fila com dados dos sensores
QueueHandle_t xQueueFirebase;    //Fila com dados para o Firebase
QueueHandle_t xQueuePluvio;      //Fila com dados para o pluviometro
QueueHandle_t xQueueReconfig;    //Fila com dados para reconfigurar
QueueHandle_t xQueueReadMemory;  //Fila com dados solicitados memória
QueueHandle_t xQueueSendMemory;  //Fila com dados para leitura da memória

//Identificadores das tarefas
TaskHandle_t vTaskBrainHandle;
TaskHandle_t vTaskSensorHandle;
TaskHandle_t vTaskReconfigHandle;
TaskHandle_t vTaskPluvioHandle;
TaskHandle_t vTaskFirebaseHandle;
TaskHandle_t vTaskMemoryHandle;


//Protótipo das tarefas
//tarefa principal do sistema
void vTaskBrain(void* pvParamaters);

//tarefa de leitura dos sensores
void vTaskSensor(void* pvParamaters);

//tarefa que verifica se foi solicitado a reconfiguração
void vTaskReconfig(void* pvParamaters);

//tarefa para leitura dos dados do pluviometer
void vTaskPluvio(void* pvParamaters);

//tarefa para preparar os dados e enviar ao firebase
void vTaskFirebase(void* pvParamaters);

//tarefa para trabalhar com arquivos do SPIFFS do Microcontrolador
void vTaskMemory(void* pvParamaters);

//Configurações iniciais
void setup() {
  //Iniciando a Serial
  Serial.begin(115200);

  //Iniciando o SPIFFS (Sistema de arquivos)
  initSPIFFS();

  //Configurando os arquivos do SPIFFS, ou seja, faz a leitura dos arquivos e salva na memória
  configure();

  //Gerando servidor assincrono
  if (initWiFi()) {
    //Inicializando a comunicação com o Firebase
    configFirebase(&fdbo, &auth, &config);

    //Iniciando o freeRTOS
    initFreeRTOS();
    
    //Iniciando webserver com WiFi
    initServerWEB(&server);
  } else {
    //Iniciando conexão AcessPoint para configuração dos dados
    initServerAP(&server);
  }

  //Iniciando o FreeRTOS[
}

void loop() {
  //Por enquanto, não precisa de nada na loop.
  vTaskDelete(NULL);
}


/*=======================================|| Tarefas ||===================================================*/
//tarefa principal do sistema
void vTaskBrain(void* pvParamaters) {
  (void*)pvParamaters;
  sensorData dataRead;
  Serial.println("[METEOPIN]: Tarefa central de processamento foi iniciada...");
  for (;;) {
    if (xQueueReceive(xQueueSensor, &dataRead, portMAX_DELAY) == pdTRUE) {
      //Toma decisões caso tenha dados dos sensores



      //Envia os dados para a tarefa do Firebase, para enviar ao banco
      xQueueSend(xQueueFirebase, &dataRead, portMAX_DELAY);

    }
    //Tempo de pensamento
    vTaskDelay(pdMS_TO_TICKS(BRAIN_TIME));
  }
}

//tarefa de leitura dos sensores e enviar os dados para o cérebro
void vTaskSensor(void* pvParamaters) {
  (void*)pvParamaters;
  //Criando a estrutura responsável por guardar os dados coletados
  sensorData dataRead;
  int cm;
  int timestamp;
  Serial.println("[METEOPIN]: Tarefa do Sensor foi iniciada...");
  for (;;) {

    if (xQueueReceive(xQueuePluvio, &cm, portMAX_DELAY) == pdTRUE) {
      //coletando tempo
      timestamp = getTime();

      //coletando os dados e salvando na estrutura
      /*dataRead.temp = BMEInclude.readTemperature();
      dataRead.pres = BMEInclude.readPressure() / 100.0;
      dataRead.hum = BMEInclude.readHumidity();*/

      dataRead.temp = 1000000;
      dataRead.press = 2000000;
      dataRead.hum = 300000;

      dataRead.pluv = cm; //falta programar isso
      dataRead.timet = timestamp;

      //enviando para a fila
      xQueueSend(xQueueSensor, &dataRead, portMAX_DELAY);

      //delay para não dar erro
      vTaskDelay(pdMS_TO_TICKS(READ_TIME));
    }
  }
}

//tarefa que verifica se foi solicitado a reconfiguração
void vTaskReconfig(void* pvParamaters) {
  (void*)pvParamaters;
  sensorData dataRead;
  
  Serial.println("[METEOPIN]: Tarefa de reconfiguração foi iniciada....");
  for (;;) {
    //Essa tarefa por enquanto é inútil, pois a função dela é resetar as configurações
    //quando uma interrupção (botão) for iniciada
    if (xQueueReceive(xQueueReconfig, &dataRead, portMAX_DELAY) == pdTRUE) {
      resetSPIFFS(); //Zerando a memória do ESP32
      ESP.restart(); //Reiniciando o ESP para poder realizar novamente a configuração.
    }
    vTaskDelay(pdMS_TO_TICKS(RECONFI_TIME));
    vTaskDelete(NULL);
  }
}

//tarefa para leitura dos dados do pluviometer
void vTaskPluvio(void* pvParamaters) {
  (void*)pvParamaters;
  float cm = 0.0;
  Serial.println("[METEOPIN]: Tarefa de contagem do pluviômetro foi iniciada....");
  for (;;) {
    //fazendo a leitura e enviando o valor
    xQueueSend(xQueuePluvio, &cm, portMAX_DELAY);

    //delay para não dar erro
    vTaskDelay(pdMS_TO_TICKS(BRAIN_TIME));
  }
}

//tarefa para preparar os dados e enviar ao firebase
void vTaskFirebase(void* pvParamaters) {
  (void*)pvParamaters;
  sensorData dataRead;
  FirebaseJson json;
  Serial.println("[MeteoPIN]: Tarefa de envio para o Firebase foi iniciada...");

  for (;;) {
    if (xQueueReceive(xQueueFirebase, &dataRead, portMAX_DELAY) == pdTRUE) {
      //Cria o json
      makeJsonFirebase(dataRead, &json);

      //Envia o Json para o Firebase para o banco de dados cadastrado.
      if (Firebase.ready()) { //Se o firebase estiver disponível
        sendFirebase(&json, &fdbo, dataRead.timet);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(SEND_TIME));
  }
}

//tarefa para trabalhar com arquivos do SPIFFS do Microcontrolador
void vTaskMemory(void* pvParamaters) {
  (void*)pvParamaters;
  sensorData dataRead;
  Serial.println("[MeteoPIN]: Tarefa de gerenciamento de memória foi iniciada");
  //Início do código

  for (;;) {
    if ((xQueueReceive(xQueueReadMemory, &dataRead, portMAX_DELAY) == pdTRUE)) {
      //Recebe instruções para guardar na memória.


      //Verifica se foi solicitada algum valor?
    }
    vTaskDelay(pdMS_TO_TICKS(MEMORY_TIME));
  }
}


/*============================|| DEFINIÇÃO DA FUNÇÃO DE INICIALIZAÇÃO DO FREERTOS||==============================*/
//Procedimento inicial para iniciar o sistema
void initFreeRTOS() {
  Serial.println("[FreeRTOS]: Iniciando configurações do sistema...");
  BaseType_t returnSensor, returnBrain, returnReconfig, returnPluvio, returnFirebase, returnMemory;

  //Gerando as filas necessárias
  xQueueSensor = xQueueCreate(10, sizeof(sensorData));
  xQueuePluvio = xQueueCreate(1, sizeof(float));
  xQueueReadMemory = xQueueCreate(5, sizeof(sensorData));
  xQueueSendMemory = xQueueCreate(5, sizeof(sensorData));
  xQueueReconfig = xQueueCreate(1, sizeof(int));
  xQueueFirebase = xQueueCreate(10, sizeof(sensorData));

  //dando início as tarefas
  if (xQueueSensor != NULL && xQueuePluvio != NULL && xQueueReadMemory != NULL && xQueueSendMemory != NULL && xQueueReconfig != NULL && xQueueFirebase != NULL) {
    Serial.println("[FreeRTOS]: Filas criadas com sucesso");

    //Tarefa de aquisição dos dados dos sensores
    returnSensor = xTaskCreatePinnedToCore(vTaskSensor, "TaskSensor", configMINIMAL_STACK_SIZE + 1024, NULL, 2, &vTaskSensorHandle, 0);
    if (returnSensor != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa do sensor.");
      while (1) {};
    }

    //Tarefa de aquisição dos dados do pluviometro
    returnPluvio = xTaskCreatePinnedToCore(vTaskPluvio, "TaskPluvio", configMINIMAL_STACK_SIZE + 1024, NULL, 4, &vTaskPluvioHandle, 0);
    if (returnPluvio != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa do pluviometro.");
      while (1) {};
    }

    //Tarefa de aquisição dos dados do cérebro
    returnBrain = xTaskCreatePinnedToCore(vTaskBrain, "TaskBrain", configMINIMAL_STACK_SIZE + 1024, NULL, 5, &vTaskBrainHandle, 1);
    if (returnBrain != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa princial (brain).");
      while (1) {};
    }

    //Tarefa para verificar se a reconfiguração foi solicitada
    returnReconfig = xTaskCreatePinnedToCore(vTaskReconfig, "TaskReconfig", configMINIMAL_STACK_SIZE, NULL, 8, &vTaskReconfigHandle, 1);
    if (returnReconfig != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa de reconfiguração.");
      while (1) {};
    }

    //Tarefa para realizar a manipulação dos dados do SPIFFS
    returnMemory = xTaskCreatePinnedToCore(vTaskMemory, "TaskMemory", configMINIMAL_STACK_SIZE + 1024, NULL, 6, &vTaskMemoryHandle, 1);
    if (returnMemory != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa de controle da memória");
      while (1) {};
    }

    //Tarefa para enviar dados para o firebase
    returnFirebase = xTaskCreatePinnedToCore(vTaskFirebase, "TaskFirebase", configMINIMAL_STACK_SIZE + 5120, NULL, 7, &vTaskFirebaseHandle, 0);
    if (returnFirebase != pdTRUE) {
      Serial.println("[FreeRTOS]: Não foi possível gerar a tarefa de gestão do firebase.");
      while (1) {};
    }

    Serial.println("[FreeRTOS]: Sistema configurado com sucesso.\n\n");
  } else {
    Serial.println("[FreeRTOS]: Não foi possível gerar alguma das filas solicitadas, portanto, o esp32 irá reiniciar...");
    while (1) {};
  }
}
