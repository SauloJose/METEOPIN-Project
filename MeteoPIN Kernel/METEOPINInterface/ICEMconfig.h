/**************************************************************************
    PROTÓTIPO de Interface de configuração de estação Metereológica (ICEM)
    Arquivo de cabeçalho

    Descrição: Neste arquivo contém as variáveis necessárias para o projeto

    Por: Saulo José Almeida Silva
    Última atualização: 09/08/2023
**************************************************************************/
#ifndef ICEMCONFIG_H
#define ICEMCONFIG_H

/*==========================||BIBLIOTECAS||==============================*/
//Bibliotecas Necessárias
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


//Biblioteca do Sistema de controle dos arquivos
#include "SPIFFS.h"

/*================|| Variáveis do protótipo || =======================*/

//enumeração
enum ConectionMode { APMode, //Modo de conexão Acess Point
                     WiFiMode,  //Modo de conexão WiFi
                     BluetoothMode, //Modo de Conexão Bluetooth
                     SMSMode  //Modo de Conexão SMSM
                   };

//Como Padrão o valor começa como APMode
static ConectionMode Mode = APMode; //Modo Inicial de funcionamento do projeto

//Modelo do projeto
const char* MODELO = "MeteoPIN 1.2";
const char* TOKEN = "BMOD0112PIAL";
const char* LOCAL = "Palmeira dos Índios - AL";

//Empacotando os dados coletados para melhor administrar
typedef struct sensorData {
  //Dados de umidade, pressão, temperatura e do pluviômetro.
  float hum, press, temp, pluv;
  time_t timet;
} sensorData;

//Tempo para fazer as leituras e envio em mili segundos
#define SEND_TIME 60000 //Envia dados a cada intervalo em MS de tempo, nesse caso a cada 1 min
#define READ_TIME 3000
#define BRAIN_TIME 300
#define RECONFI_TIME 500
#define MEMORY_TIME 400
/*================|| Variáveis de Conexão com a REDE||=================*/
//Criando objeto do servidor assíncron
static AsyncWebServer server(80);

//Configurações para acesso a internet
static IPAddress localIp;
static IPAddress localGateway;
static IPAddress subnet; 


//Variáveiis para modos de conexão
//WiFi Mode
static String ssid;
static String pass;
static String ip;
static String gateway;
static String mascara;

//AcessPoint
static String ssidAP;
static String passAP;
static String ipAP;

//Bluetooth Mode


//SMS mode

//Usuário da estaçãojso
static String userM; //Email de login da plataforma/firebase
static String passM; //Senha de login da plataforma/firebase
/*================|| Variáveis de conexão com o Firebase||=================*/
//Configurações do Firebase
//OBS: O objetivo é que essas configurações sejam feitas manipulando o SPIFFS
// definindo APIKey do Firebase
#define API_KEY "AIzaSyDlLS8no97Wo4zzlsOncpLiewTAnPwzG_4"

// Indereço do RTDB
#define DATABASE_URL "https://esp32-firebase-teste-default-rtdb.firebaseio.com"

//Firebase objeto de dados
FirebaseData fdbo;
FirebaseAuth auth;
FirebaseConfig config;

//Variável para salvar o user uid do firebase
String uid;

//Número de tentativas para conexão com o firebase.
static const int tries = 60;

//Dados do sensor falso para teste
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 18000; //18 segundos
int count = 0;
bool signupOK = false;

// Parent Node ( para ser atualizado em todo loop
static String timePath = "/timestamp";            //Intervalo de tempo
static String HumPath = "/humidity";              //Humidade
static String PressPath = "/pressure";            //Pressão
static String PluvPath = "/pluviometer";          //Pluviometro
static String TempPath = "/temperature";          //Temperatura

//Parent Node
static String parentPath;
static String databasePath;

//Instante de tempo
static int timestamp ;

//Criando variável global do ntpServer
const char* ntpServer = "pool.ntp.org";

/*=======================|| Caminhos para os arquivos ||================*/

/*Parâmetros do formulário HTTP POST*/

//Parâmetros para WiFiManager
const char* PARAM_INPUT_1 = "ssid";      //Nome da rede sem fio
const char* PARAM_INPUT_2 = "pass";      //Senha da rede sem fio
const char* PARAM_INPUT_3 = "ip";        //Endereço IP cadastrado para o web server
const char* PARAM_INPUT_4 = "gateway";   //Gateway do roteador WiFi
const char* PARAM_INPUT_41 = "mascara";  //Mascara de rede do gateway

//Parâmetros para configurações de AcessPoint
const char* PARAM_INPUT_5 = "ssidAP";     //Nome do acessPoint
const char* PARAM_INPUT_6 = "passAP";     //Senha para o acessPoint
const char* PARAM_INPUT_7 = "ipAP";       //IP que foi configurado
const char* PARAM_INPUT_8 = "OK";         //Serve apenas para resetar as configurações

//Parâmetros para entrar com usuário e senha
const char* PARAM_INPUT_9 = "UserMeteoPin";     //Usuário do MeteoPin
const char* PARAM_INPUT_10 = "PassMeteoPin";    //Senha do user MeteoPIN

//Caminhos para os arquivos no sistema de arquivos
//arquivos da configuração WiFi
const char* ssidPath = "/ssid.txt";
const char* passPath = "/password.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";
const char* mascaraPath = "/mascara.txt";

//arquivos da configuração do AcessPoint
const char* ssidAPPath = "/ssidAP.txt";
const char* passAPPath = "/passwordAP.txt";
const char* ipAPPath = "/ipAP.txt";

//arquivos da configuração do user do MeteoPIN
const char* userMPath = "/userM.txt";
const char* passMPath = "/passM.txt";
/*==========================* Funções *====================================*/
/*================| Funções para coleta de tempo | ========================*/
//Função para retornar o tempo atual do ESP32
unsigned long getTime();


/*===========| Funções para configuração do Firebase | ====================*/
//Função para realizar as configurações da conexão com o Firebase
bool configFirebase(FirebaseData* fdbo, FirebaseAuth* auth, FirebaseConfig* config );

//Função para enviar os dados dos sensores para o Real Time Data Base configurado
bool sendFirebase(FirebaseJson* json, FirebaseData* fdbo, int timestamp);

//Função responsável por gerar o json que será enviado para o Firebase
void makeJsonFirebase(sensorData Sensor, FirebaseJson* js);

//resetando os arquivos do Firebase
bool resetFirebase();
/*================| Funções para gerenciar os arquivos |===================*/
//Sistema de gerenciamento de arquivos

//Função para inicializar o sistemade arquivos
void initSPIFFS();

//Funções para gerenciamento do SPIFFS
String readFile(fs::FS &fs, const char * path);

//Função para escrever em um arquivo
void writeFile(fs::FS &fs, const char * path, const char * message);

//configurar todos os arquivos
void configure();

//Função para resetar todos os arquivos ao modo de fabricante
void resetSPIFFS();

//resetar dados cadastrados no WiFi, caso eles não sejam verdadeiros.
void resetWiFiConfig();

/*=================| FUNÇÕES PARA CONFIGURAR A INTERFACE | ================*/

//Atualiza PlaceHolders
String processor(const String& var);

//Atualiza placeHolders de configurações
String ConfigProcessor(const String& var);

/*=================| FUNÇÕES PARA CONFIGURAÇÃO DE REDE|===================*/

//Iniciar configuração WiFi
bool initWiFi();

//Configura o servidor dentro da rede.
bool initServerAP(AsyncWebServer* server);

//configura o servidor em modo estação.
bool initServerWEB(AsyncWebServer* server);

/*=================| Definindo tarefas do FreeRTOS|======================*/
//inicializa o Sistema Operacional FreeRTOS para o Kernel
void initFreeRTOS();


/*=================| Definindo funções do BME|==========================*/
void initBME();


#endif
