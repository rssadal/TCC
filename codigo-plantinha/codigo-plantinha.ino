//--------Arquivo de configuração
#include "config.h"

//--------Bibliotecas utilizadas no código--------
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Resultado do servidor
JsonArray plantas;

//--------Configurações iniciais do sensor de umidade do solo--------
int pinUmidadeSolo;
float umidade;

//--------Configurações iniciais do sensor de temperatura--------
#define BMP280_I2C_ADDRESS 0x76  //Define o endereço I2C do BMP280. Esse endereço (0x76) é o que o ESP8266 usará para "falar" com o sensor.
Adafruit_BMP280 bmp280;

//--------Configurações iniciais do sensor de luminosidade--------
BH1750 lightMeter;

//--------Configurações do LCD
// LCD I2C no endereço 0x27, 20 colunas e 4 linhas
LiquidCrystal_I2C lcd(0x27, 20, 4);

//-------- Variaveis para controle de tempo------------------------
float umidadeMin = 30.0;                   // Umidade mínima ideal do solo (%)
float umidadeMax = 60.0;                   // Umidade máxima ideal do solo (%)
int horasNecessarias = 1;
int tempoLuminosidade = horasNecessarias * 3600 * 1000;  // Exemplo: 1 horas em milissegundos
unsigned long ultimaLigacaoLampada = 0;
bool lampadaLigada = false; 
bool bombaLigada = false;

//------------ Definição de pinos bomba e lampada
#define BOMBA D5
#define LAMPADA D6

//--------Setup--------

void setup() {

  // Inicializa a comunicação serial
  Serial.begin(9600);

  // Inicializa a plataforma Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Inicializa o sensor de temperatura
  if (!bmp280.begin(BMP280_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1)
      ;
  }

  // Inicializa LCD
  Wire.begin(D2, D1);  // NodeMCU: SDA, SCL
  lcd.begin(20, 4);    // Inicializa o LCD
  lcd.backlight();     // Liga o backlight
  lcd.setCursor(0, 0);
  lcd.print("Iniciando!");

  // Inicializa o sensor de luminosidade
  lightMeter.begin();

  //Configura o D5 para controle da bomba
  //pinMode(BOMBA, OUTPUT);
  //digitalWrite(BOMBA, HIGH); // Garante que a bomba comece desligada

  // Conectar ao WiFi
  conectarWiFi();

  // Fazer requisição ao servidor
  requisitarDadosPlantas();
}

//--------Loop--------

void loop() {

  // Verifica a umidade do solo
  //monitorarUmidadeSolo();

  // Verifica o tempo de luminosidade que a planta precisa
  //monitorarLampada();

  // Atualização da plataforma Blynk
  Blynk.run();

  // Envia os dados para a plataforma Blynk
  Blynk.virtualWrite(V0, lightMeter.readLightLevel());
  Blynk.virtualWrite(V1, bmp280.readTemperature());
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

// Função para buscar os parâmetros da planta no banco de dados
void requisitarDadosPlantas() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverAddress);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      StaticJsonDocument<2048> json;
      DeserializationError error = deserializeJson(json, response);

      if (!error) {
        plantas = json.as<JsonArray>();
        int totalPlantas = plantas.size();
        Serial.print("Quantidade de plantas recebidas: ");
        Serial.println(totalPlantas);
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

void monitorarUmidadeSolo() {
  pinUmidadeSolo = analogRead(A0);
  umidade = 100.0 * ((978 - (float)pinUmidadeSolo) / 978);

  if (umidade < umidadeMin && !bombaLigada) {
    Serial.println("Bomba ligada!");
    digitalWrite(BOMBA, LOW);
    bombaLigada = true;
  } else if (umidade > umidadeMax && bombaLigada) {
    Serial.println("Bomba desligada!");
    digitalWrite(BOMBA, HIGH);
    bombaLigada = false;
  }
}

void monitorarLampada() {
  unsigned long agora = millis();

  // Verifica se já passou 24 horas desde a última ativação
  if (!lampadaLigada && (agora - ultimaLigacaoLampada >= 24UL * 3600 * 1000)) {
    Serial.println("Lâmpada ligada!");
    digitalWrite(LAMPADA, HIGH);
    ultimaLigacaoLampada = agora;
    lampadaLigada = true;
  }

  // Verifica se a lâmpada já ficou ligada pelo tempo necessário
  if (lampadaLigada && (agora - ultimaLigacaoLampada >= tempoLuminosidade)) {
    Serial.println("Lâmpada desligada!");
    digitalWrite(LAMPADA, LOW);
    lampadaLigada = false;
  }
}
