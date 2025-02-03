
//-------Informações sobre a planta--------

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2HU4otiaB"
#define BLYNK_TEMPLATE_NAME "Tcc Plantinha"
#define BLYNK_AUTH_TOKEN "6eu6YBZ-GDQFj-N5gC6QZd_5fs3mxmm0"

//--------Bibliotecas utilizadas no código--------

#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_Sensor.h>
#include <Adafruit_ST7735.h>
#include <BH1750.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Valores mínimos e máximos de umidade do solo, temperatura e luminosidade 
int u_min = 15;
int u_max = 35;
int t_min = 10;
int t_max = 40;
int l_min = 700;
int l_max = 40000;

//-------Configurações iniciais da plataforma Blynk--------


// Insira abaixo o seu Auth Token
char auth[] = "x3SUJMQhZkRsTaSyQKB8BV0IMEXjt11q";

// Insira abaixo o nome da sua rede WiFi
const char* ssid = "Alexandre";

// Insira abaixo a senha da sua rede WiFi
const char* password  = "96215288";

 // Endereço do Flask
const char* serverAddress = "http://192.168.1.155:5000/plantas";

// Resultado do servidor
JsonArray plantas;

//--------Configurações iniciais do display LCD TFT--------

//Definição dos pinos
#define lampada  D1    // Relé da lâmpada no pino D1
#define bombaAgua D2   // Relé da bomba de água no pino D2

//--------Configurações iniciais do sensor de umidade do solo--------

int AOUT;
float umidade;

//--------Configurações iniciais do sensor de temperatura--------

#define BMP280_I2C_ADDRESS  0x76
Adafruit_BMP280 bmp280;

//--------Configurações iniciais do sensor de luminosidade--------
BH1750 lightMeter;

//-------- Variaveis para controle de tempo------------------------
float umidadeMin = 30.0; // Umidade mínima ideal do solo (%)
float umidadeMax = 60.0; // Umidade máxima ideal do solo (%)
int tempoLuminosidade = 10 * 3600 * 1000; // Exemplo: 10 horas em milissegundos
unsigned long ultimaLigacaoLampada = 0;

//--------Setup--------

void setup(){
  
  // Inicializa a comunicação serial
  Serial.begin(9600);
  
  // Inicializa a plataforma Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  
  // Inicializa o sensor de temperatura
  if(!bmp280.begin(BMP280_I2C_ADDRESS))
  {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while(1);
  }

  // Inicializa o sensor de luminosidade
  lightMeter.begin();
  
  // Inicializa o display LCD TFT
  tft.initR(INITR_BLACKTAB);
  tft.fillRect( 0, 0, 128, 160, PRETO);

  // Conectar ao WiFi
  conectarWiFi();

  // Fazer requisição ao servidor
  requisitarDadosPlantas();
  
}

//--------Loop--------

void loop(){

  // Verifica a luminosidade ambiente
  uint16_t luminosidade = lightMeter.readLightLevel();
  Serial.print("Luminosidade= ");
  Serial.println(luminosidade);
  
  // Verifica a temperatura ambiente
  float temperatura = bmp280.readTemperature();
  Serial.print("Temperatura = ");
  Serial.println(temperatura);
  
  // Verifica a umidade do solo
  monitorarUmidadeSolo();

  // Atualização da plataforma Blynk                            
  Blynk.run();
  
  // Envia os dados para a plataforma Blynk
  Blynk.virtualWrite(V0, luminosidade);
  Blynk.virtualWrite(V1, temperatura);
  Blynk.virtualWrite(V2, umidade);
  
}
//--------- Funções

void conectarWiFi() {
    Serial.print("Conectando-se ao WiFi");
    WiFi.begin(ssid, password);

    // Aguarda conexão
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado ao WiFi");
}

void printarPlantas(JsonArray plantas) {
    for (JsonObject planta : plantas) {
        Serial.print("ID: ");
        Serial.println(planta["id"].as<int>());
        Serial.print("Nome: ");
        Serial.println(planta["name"].as<const char*>());
        Serial.print("Tempo de Luminosidade: ");
        Serial.println(planta["tempo_luminosidade"].as<int>());
        Serial.println("--------------------");
    }
}

void fazerRequisicao() {
    String response = "";
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        http.begin(client, serverAddress);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
            response = http.getString();
        } else {
            Serial.print("Erro na requisição: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    }

    if (response != "") {
        // Processar JSON recebido
        StaticJsonDocument<1024> json;
        DeserializationError error = deserializeJson(json, response);

        if (!error) {
            plantas = json.as<JsonArray>();
            printarPlantas(plantas); // Chama a função para imprimir os dados
        } else {
            Serial.println("Erro ao processar JSON!");
        }
    }
}

// Função para buscar os parâmetros da planta no banco de dados
void requisitarDadosPlantas() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverAddress);
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
            String response = http.getString();
            StaticJsonDocument<1024> json;
            DeserializationError error = deserializeJson(json, response);

            if (!error) {
                JsonObject planta = json[0]; // Pegando a primeira planta da resposta
                umidadeMin = planta["umidade_solo_min"];
                umidadeMax = planta["umidade_solo_max"];
                tempoLuminosidade = (int)planta["tempo_luminosidade"] * 3600 * 1000;
                Serial.println("Dados da planta atualizados!");
            } else {
                Serial.println("Erro ao processar JSON!");
            }
        } else {
            Serial.print("Erro na requisição: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
}

// Função para monitorar a umidade do solo e controlar a bomba de água
void monitorarUmidadeSolo() {

    AOUT = analogRead(A0);
    umidade = 100 * ((978-(float)AOUT) / 978);
    
    Serial.print("Umidade do solo = ");
    Serial.println(umidade);

    if (umidade < umidadeMin) {
        Serial.println("Bomba ligada!");
        digitalWrite(bombaAgua, HIGH);
    } else if (umidade >= umidadeMax) {
        Serial.println("Bomba desligada!");
        digitalWrite(bombaAgua, LOW);
    }
}

void controlarLampada() {
    unsigned long agora = millis();
    bool lampadaLigada = false;
    // Verifica se já passou 24 horas desde a última ativação
    if (!lampadaLigada && (agora - ultimaLigacaoLampada >= 24 * 3600 * 1000)) {
        Serial.println("Lâmpada ligada!");
        digitalWrite(lampada, HIGH);
        ultimaLigacaoLampada = agora;
        lampadaLigada = true;
    }

    // Verifica se a lâmpada já ficou ligada pelo tempo necessário
    if (lampadaLigada && (agora - ultimaLigacaoLampada >= tempoLuminosidade)) {
        Serial.println("Lâmpada desligada!");
        digitalWrite(lampada, LOW);
        lampadaLigada = false;
    }
}


