/**************************************************************************
    PROTÓTIPO de Interface de configuração de estação Metereológica (ICEM)
    Criação das funções

    Descrição: Neste arquivo contém as variáveis necessárias para o projeto

    Por: Saulo José Almeida Silva
    Última atualização: 09/08/2023
**************************************************************************/
#include "ICEMconfig.h"

/*==========================* Funções *====================================*/
/*================| Funções para coleta de tempo | ========================*/
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[METEOPIN]: Falhou em obter o tempo");
    return (0);
  }
  time(&now);
  return now;
}

/*===========| Funções para configuração do Firebase | ====================*/
//Função para realizar as configurações do Firebase
bool configFirebase(FirebaseData* fdbo, FirebaseAuth* auth, FirebaseConfig* config ) {
  int i = 1;

  //API key
  config->api_key = API_KEY;
  //Conexão com o Real Time Database (RTDB)
  config->database_url = DATABASE_URL;
  
  //Dados de autenticação com Firebase;
  auth->user.email = userM.c_str();
  auth->user.password = passM.c_str();

  //Conexão com o Real Time Database (RTDB)
  config->database_url = DATABASE_URL;

  Serial.println("================================================");
  Serial.println("[CONFIGFIREBASE]: Usuário e senha");
  Serial.print("user: ");
  Serial.println(userM);
  Serial.print("senha: ");
  Serial.println(passM);
  Serial.print("Database: ");
  Serial.println(DATABASE_URL);
  Serial.print("API Key: ");
  Serial.println(API_KEY);
  Serial.println("==================================================");
  Firebase.reconnectWiFi(true);
  fdbo->setResponseSize(4096);

  //Callback para geração de token
  config->token_status_callback = tokenStatusCallback;

  //Número máximo de geração de tokens
  config->max_token_generation_retry = 5;

  //Iniciando a biblioteca com o Firebase Auth e a configuração
  Firebase.begin(config, auth);

  // Autenticando informações
  Serial.println("[Firebase]: Buscando o UID do usuário");
  while ((auth->token.uid) == "" && (i <= tries))
  {
    Serial.print(".");
    delay(500);
    i = i + 1;
  }
  Serial.print("\nTentativas necessárias para conectar: ");
  Serial.print(i);
  //verifica se ocorreu um erro aqui
  if (i >= tries) {
    Serial.println("\n[Firebase]: Email e senha incorretos ou existe um problema de conexão com o firebase");
    //Resetando WiFi para realizar novamento as configurações
    return false;
  } else {
    //tenta adquirir o token do uid
    uid = auth->token.uid.c_str();
    Serial.print("\n\n[Firebase]: UID do Usuário: ");
    Serial.println(uid);

    // configurando o caminho para carregar os dados no RTBD
    databasePath = "/UsersData/" + uid + "/readings";

    Serial.println("\n======================================\n");
    return true; // Caso não tenha ocorrido um erro
  }
}


//Função para enviar os dados dos sensores para o Real Time Data Base configurado
bool sendFirebase(FirebaseJson* json, FirebaseData* fdbo, int timestamp) {
  //Serial.println("=====>[DEBUG 2]: Tentando enviar o arquivo para o Firebase");
  parentPath = databasePath + "/" + String(timestamp);
  // Tente atribuir o JSON ao banco de dados
  if (Firebase.RTDB.setJSON(fdbo, parentPath.c_str(), json)) {
    Serial.println("=====>[FIREBASE]: JSON atribuído com sucesso!");
  } else {
    Serial.printf("=====>[DEBUG 2]: Erro ao atribuir JSON: %s\n", fdbo->errorReason().c_str());
  }
  //Serial.println("=====>[DEBUG 2]: Tudo certo.");
  return true;
}

// Função para gerar o json que será enviado para o firebase
void makeJsonFirebase(sensorData Sensor, FirebaseJson* js)
{ //Construindo Json para enviar os dados ao Firebase
  // Setando temperatura
  js->set(TempPath.c_str(), String(Sensor.temp));

  // Setando pressão
  js->set(PressPath.c_str(), String(Sensor.press));

  // Setando humidade
  js->set(HumPath.c_str(), String(Sensor.hum));

  // Setando pluviômetro
  js->set(PluvPath.c_str(), String(Sensor.pluv));

  // Setando intervalo de tempo
  js->set(timePath.c_str(), String(Sensor.timet));

}

//Resetando os arquivos do Firebase
bool resetFirebase() {
  Serial.println("===========================================");
  Serial.println("[SPIFFS]: Resetando arquivos do firebase...");
  //Resetando configurações de usuário e senha
  writeFile(SPIFFS, userMPath, "");
  writeFile(SPIFFS, passMPath, "");
  Serial.println("[SPIFFS]: Arquivos do firebase resetados com sucesso.");
  Serial.println("===========================================\n");
}
/*================| Funções para gerenciar os arquivos |==================*/
//==============|| Sistema de gerenciamento de arquivos ||=================

//Função para inicializar o sistemade arquivos
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS]: Ocorreu um erro ao iniciar o sistema de arquivos.");
  }
  Serial.println("[SPIFFS]: Sistema iniciado com sucesso!");
}

//Funções para gerenciamento do SPIFFS
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("[SPIFFS]: Lendo arquivo %s\r\n", path);

  //Abrindo arquivo  do SPIFFS
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("[SPIFFS]: Ocorreu uma falha ao abrir o arquivo para leitura.");
    return String(); //?
  }

  String fileContent; //Conteúdo do arquivo tzt

  while (file.available()) {
    fileContent = file.readStringUntil('\n'); //Faz a leitura até o pular de linha
    break;
  }

  return fileContent; //Retorna o conteúdo lido do arquivo TXT
}

//Função para escrever em um arquivo
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("[SPIFFS]: Escrevendo no arquivo %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("[SPIFFS]: Falha ao tentar abrir o arquivo para escrita.");
    return;
  }

  if (file.print(message)) { //Imprimindo mensagm no arquivo
    Serial.println("[SPIFFS]: Arquivo foi escrito.");
  } else {
    Serial.println("[SPIFFS]: Escrita falhou.");
  }
}


//Configurando arquivos
void configure() {
  /*Carregando arquivos do SPIFFS;*/
  // Carregando os arquivos salvos no SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  mascara = readFile(SPIFFS, mascaraPath);

  // carregando os arquivos para AcessPoint
  ssidAP = readFile(SPIFFS, ssidAPPath);
  ipAP = readFile(SPIFFS, ipAPPath);
  passAP = readFile(SPIFFS, passAPPath);

  //Carregando arquivos de login do usuário
  userM = readFile(SPIFFS, userMPath);
  passM = readFile(SPIFFS, passMPath);

  // Imprimindo configurações salvas no SPIFFS
  Serial.println("===========================================");
  Serial.println("[SPIFFS]: Arquivos carregados da flash:");
  Serial.println("[SPIFFS]: Dados de conexão WiFi:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASSWORD: ");
  Serial.println(pass);
  Serial.print("IP: ");
  Serial.println(ip);
  Serial.print("GATEWAY: ");
  Serial.println(gateway);
  Serial.print("Mascara de Rede: ");
  Serial.println(mascara);
  Serial.println("[SPIFFS]: Dados de configuração do AcessPoint:");
  Serial.print("SSID do AP: ");
  Serial.println(ssidAP);
  Serial.print("PASSWORD: ");
  Serial.println(passAP);
  Serial.print("IP do AcessPoint: ");
  Serial.println(ipAP);
  Serial.println("[SPIFFS]: Usuário cadastrado da tomada:");
  Serial.print("Usuário(Email): ");
  Serial.println(userM);
  Serial.print("Senha: ");
  Serial.println(passM);
  Serial.println("===========================================\n");
}

//Reseta o SPIFFS a configuração inicial, com arquivos limpos.
void resetSPIFFS() {
  Serial.println("===========================================");
  Serial.println("[SPIFFS]: Resetando arquivos...");
  // Resetando o arquivos do SPIFFS;

  // Resetando arquivos de ConexãoWiFi
  writeFile(SPIFFS, ssidPath, "");
  writeFile(SPIFFS, passPath, "");
  writeFile(SPIFFS, ipPath, "");
  writeFile(SPIFFS, gatewayPath, "");
  writeFile(SPIFFS, mascaraPath, "");

  // Resetando arquivos de configuração interna do AcessPoint
  writeFile(SPIFFS, ssidAPPath, "");
  writeFile(SPIFFS, ipAPPath, "192.168.4.1");
  writeFile(SPIFFS, passAPPath, "");

  //Resetando configurações de usuário e senha
  writeFile(SPIFFS, userMPath, "");
  writeFile(SPIFFS, passMPath, "");
  Serial.println("[SPIFFS]: Arquivos resetados com sucesso.");
  Serial.println("===========================================\n");
  Serial.println("===========================================\n");

}
/*=================| FUNÇÕES PARA CONFIGURAR A INTERFACE | ================*/

//Callback para processar informações do servidor
String processor(const String& var) {

  //PlaceHolders para os dados do modelo
  if (var == "modelo") {
    return F(MODELO);
  }
  if (var == "token") {
    return F(TOKEN);
  }
  if (var == "localização") {
    return F(LOCAL);
  }
  if (var == "modo") { //Modo de conexão WiFi
    if (Mode == APMode) {
      return F("AcessPoint (Apenas para configurações)");
    }
    else if (Mode == WiFiMode) {
      return F("WIFI Client");
    }
    else if (Mode == BluetoothMode) {
      return F("BlueTooth");
    }
    else if (Mode == SMSMode) {
      return F("Via SMS");
    }
  }
  if (var == "atualizacao") {
    return F("12/2022");
  }

  //Valor configurado para o SSID do acessPoint
  if (var == "SAP") { //Nome do AcessPoint
    if (ssidAP == "") {
      return F("IDEM_config");
    } else {
      return F(ssidAP.c_str());
    }
  }

  //Valor da senha do AcessPoint
  if (var == "PAP") { //Senha do AcessPoint
    if (passAP == "") {
      return F("( Não tem senha)");
    } else {
      return F(passAP.c_str());
    }
  }

  //Ip configurado para o AcessPoint
  if (var == "IAP") { // Ip do AcessPoint
    return F("192.168.4.1");
  }

  //Usuário do MeteoPIN
  if (var == "UserMeteoPIN" ) {
    if (userM == "") {
      return F("Usuário não cadastrado");
    } else {
      return F(userM.c_str());
    }
  }

  //Senha do usuário do MeteoPIN
  if (var == "PassMeteoPIN") {
    if (passM == "") {
      return F("Senha não cadastrada");
    } else {
      return F(passM.c_str());
    }
  }
  return String();
}

//Atualiza PlaceHolders da Configuração
/*OBS: Por algum motivo essa função não está sendo chamada
  e o arquivo html de configurações não está recebendo os valores.*/
String ConfigProcessor(const String& var) {
  //Valor configurado para o SSID do acessPoint
  if (var == "SAP") {
    if (ssidAP == "") {
      return F("IDEM_config");
    } else {
      return F(ssidAP.c_str());
    }
  }

  //Valor da senha do AcessPoint
  if (var == "PAP") {
    if (passAP == "") {
      return F("( Não tem senha)");
    } else {
      return F(passAP.c_str());
    }
  }

  //Ip configurado para o AcessPoint
  if (var == "IAP") {
    return F("192.168.4.1");
  }

  return String();
}


//Configura o servidor dentro da rede.
bool initServerAP(AsyncWebServer* server) {

  //Cria acessPoint para configurar os dados
  Serial.println("[SERVIDOR]: Gerando AcessPoint para configurar rede.");

  //verifica se existe um acesspoint cadastrado
  if (ssidAP == "") {
    Serial.println("[WIFI]: Gerando AcessPoint Padrão");
    WiFi.softAP("ICEM-Config", NULL); //Criando acesspoint com dados padrões
  } else {
    Serial.println("[WIFI]: Configurando AcessPoint com os dados cadastrados...");
    WiFi.softAP(ssidAP.c_str(), passAP.c_str()); //Configura AcessPoint com dados escolhidos
  }


  IPAddress IP = WiFi.softAPIP(); //Retomando endereço de IP
  Serial.print("[SERVIDOR]: AcessPoint criado no endereço de IP:");
  Serial.println(IP);


  //Criando rotas do servidor via URL
  //Rota principal, apresentação do servidor
  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) { //Página Inicial
    request->send(SPIFFS, "/welcome.html", "text/html"); //Página inicial de boas vindas
    //Depois redireciona para a página inicial
  });

  //página principal
  server->on("/home.html", HTTP_GET, [](AsyncWebServerRequest * request) { //Página Inicial
    request->send(SPIFFS, "/home.html", "text/html", false, processor); //Página inicial de boas vindas
    //Depois redireciona para a página inicial
  });

  //Página de configurações
  server->on("/configuracoes", HTTP_GET, [](AsyncWebServerRequest * request) { //Página Inicial
    request->send(SPIFFS, "/configuracoes.html", "text/html", false, processor); //Página inicial de boas vindas
  });

  //Página de comunicação
  server->on("/comunicacao", HTTP_GET, [](AsyncWebServerRequest * request) { // pagina de comunicação
    request->send(SPIFFS, "/comunicacao.html", "text/html");
  });

  //Página do WiFi Manager
  server->on("/WiFiManager.html", HTTP_GET, [](AsyncWebServerRequest * request) { //Página do WiFI Menageer
    request->send(SPIFFS, "/WiFiManager.html", "text/html");
  });

  //Página de informações
  server->on("/info.html", HTTP_GET, [](AsyncWebServerRequest * request) { //Página de informações
    request->send(SPIFFS, "/info.html", "text/html");
  });

  //Rota de finalização de configuração
  server->on("/finish.html", HTTP_GET, [](AsyncWebServerRequest * request) { //Página final
    request->send(SPIFFS, "/finish.html", "text/html");
  });

  server->serveStatic("/", SPIFFS, "/"); //Diz ao servidor qual o diretório onde ele deve buscar os arquivos

  //Tratando requisições HTTP_POST
  //Trata os dados vindos da página do wifi
  server->on("/WiFiManager.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        // HTTP POST ssid
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          Serial.print("[SERVIDOR]: SSID mudado para  ");
          Serial.println(ssid);
          //Guardando arquivo
          writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          Serial.print("[SERVIDOR}: Senha mudada para ");
          Serial.println(pass);
          //Guardando no arquivo
          writeFile(SPIFFS, passPath, pass.c_str());
        }
        // HTTP POST ip
        if (p->name() == PARAM_INPUT_3) {
          ip = p->value().c_str();
          Serial.print("[SERVIDOR]: IP Address mudado para: ");
          Serial.println(ip);
          // Escrevendo no arquivo
          writeFile(SPIFFS, ipPath, ip.c_str());
        }
        // HTTP POST gateway
        if (p->name() == PARAM_INPUT_4) {
          gateway = p->value().c_str();
          Serial.print("[SERVIDOR]: Gateway mudado para: ");
          Serial.println(gateway);
          // Escrevendo no arquivo
          writeFile(SPIFFS, gatewayPath, gateway.c_str());
        }

        // HTTP Post mascara de rede
        if (p->name() == PARAM_INPUT_41) {
          mascara = p->value().c_str();
          if (p->value() == "1") {
            mascara = "255.000.000.000";
          }
          else if (p->value() == "2") {
            mascara = "255.255.000.000";
          }
          else if (p->value() == "3") {
            mascara = "255.255.255.000";
          } else {
            mascara = "Erro ao receber o valor.";
          }
          Serial.print("[SERVIDOR]: mascara mudado para  ");
          Serial.println(mascara);

          //Guardando arquivo
          writeFile(SPIFFS, mascaraPath, mascara.c_str());
        }

        //Debug
        //Serial.printf("[SERVIDOR]: POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->redirect("/finish.html");
    delay(3000);
    ESP.restart(); //Reinicia o ESP
  });

  //Tratando dados da configuração do AcessPoint
  server->on("/configuracoes.html", HTTP_POST, [](AsyncWebServerRequest * request) {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        // HTTP POST ssid AcessPoint
        if (p->name() == PARAM_INPUT_5) {
          ssidAP = p->value().c_str();
          Serial.print("[SERVIDOR]: SSID do AcessPoint mudado para: ");
          Serial.println(ssidAP);
          // Escrevendo no arquivo
          writeFile(SPIFFS, ssidAPPath, ssidAP.c_str());
        }
        // HTTP POST password do AcessPoint
        if (p->name() == PARAM_INPUT_6) {
          passAP = p->value().c_str();
          Serial.print("[SERVIDOR]: senha do AcessPoint mudada para: ");
          Serial.println(passAP);
          // Escrevendo no arquivo
          writeFile(SPIFFS, passAPPath, passAP.c_str());
        }
        // HTTP POST ip do AcessPoint
        if (p->name() == PARAM_INPUT_7) {
          ipAP = p->value().c_str();
          Serial.print("[SERVIDOR]: IP do AcessPoint configurado para: ");
          Serial.println(ipAP);
          // Escrevendo no arquivo
          writeFile(SPIFFS, ipAPPath, ipAP.c_str());
        }

        if (p->name() == PARAM_INPUT_8) { //Quando chegar um parâmetro aqui ele reseta o spiffs
          Serial.println("[SERVIDOR]: Resetando os arquivos do SPIFFS...");
          resetSPIFFS();
        }

        //Usuário da plataforma MeteoPIN
        if (p->name() == PARAM_INPUT_9) {
          userM = p->value().c_str();
          Serial.print("[SERVIDOR]: Usuário da plataforma logado ");
          Serial.println(userM);

          // Escrevendo no arquivo
          writeFile(SPIFFS, userMPath, userM.c_str());
        }

        //Senha do usuário da plataforma MeteoPIN
        if (p->name() == PARAM_INPUT_10) {
          passM = p->value().c_str();
          Serial.print("[SERVIDOR]: Senha do usuário da plataforma ");
          Serial.println(passM);

          // Escrevendo no arquivo
          writeFile(SPIFFS, passMPath, passM.c_str());
        }
        //Debug
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->redirect("/finish.html");
    delay(3000);
    ESP.restart();
  });

  //Permitindo que novos usuários usem
  server->begin(); //Iniciando o servidor.

  //Configurado para modo AcessPoint
  Mode = APMode;
  return 1;
}

//configura o servidor em modo estação.
bool initServerWEB(AsyncWebServer* server) {
  //caso consiga acessar a internet, gera o código
  /*Processando páginas do servidor*/
  Serial.println("[SERVIDOR]: Por enquanto, não tem página...");
  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send(SPIFFS, "/index.html", "text/html", false, processor);
  });


  /* Processando PlaceHolders*/
  server->serveStatic("/", SPIFFS, "/"); //Diz ao servidor onde ele deve buscar os arquivos.

  //Iniciando comunicação
  server->begin();
  return 1;
}


/*=================| FUNÇÕES PARA CONFIGURAÇÃO DE REDE|===================*/
bool initWiFi() {

  // Variáveis para contar tempo de conexão
  unsigned long previousMillis = 0;
  const long interval = 10000;

  //Verifica logo se existe as configurações de email e senha da plataforma
  if (userM == "" || passM == "") {
    return false;
  }
  // Verifica se existe ssid ou ip
  if (ssid == "" || ip == "")
  {
    Serial.println("[WIFI]: endereços de SSID ou IP indefinidos");
    resetWiFiConfig();
    return false;
  }

  /* Esse trecho do código está gerando problemas para que o ESP32 envie os dados
      ao servidor. Portnato, será retirado para ser analizado em versões futuras
    // Configurando para modo estação
    WiFi.mode(WIFI_STA);

    // Pego os dados da conexão
    localIp.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());
    subnet.fromString(mascara.c_str());

    // Configurando gateway e IP
    if (!WiFi.config(localIp, localGateway, subnet))
    {
    Serial.println("[WIFI]: STA falhou em configurar");
    resetWiFiConfig();
    return false;
    }
  */

  // Acessando WiFI
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("[WIFI]: Conectando...");

  // Atualiza tempo
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  // Verificando se foi realizado a conexão
  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      Serial.println("[WIFI]: Conexão falhou.");
      Serial.println("[WIFI]: Provavelmente o endereço e senha estão indisponíveis no momento.\n outro possível Caso é o IP que você tentou configura...\n Por favor, reveja os dados cadastrados e tente novamente.");
      resetWiFiConfig();
      return false;
    }
  }

  Serial.print("[WIFI]: Conexão realizada com sucesso.\n[WIFI]:IP configurado para: ");
  Serial.println(WiFi.localIP());
  Serial.println("\n");

  // Configurando o timer do esp32
  configTime(-3 * 3600, 0, ntpServer);

  // Configurando Modo do servidor.
  Mode = WiFiMode; // Modificando o modo de funcionamento para Modo WiFI
  return true;
}

//Resetando informações de WiFi do Microcontrolador
void resetWiFiConfig() {
  Serial.println("[WIFI]: Resetando configuração do WiFi...");

  //Resetando arquivos de ConexãoWiFi
  writeFile(SPIFFS, ssidPath, "");
  writeFile(SPIFFS, passPath, "");
  writeFile(SPIFFS, ipPath, "");
  writeFile(SPIFFS, gatewayPath, "");
  writeFile(SPIFFS, mascaraPath, "");

  Serial.println("[WIFI]: Tarefa Concluída.\n");

}

/*=================| Definindo funções do BME|==========================*/
//Iniciando o BME
void initBME(){
  
}
