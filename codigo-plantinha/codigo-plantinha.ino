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
int pinUmidadeSolo = 0;
float umidade = 0;

//--------Configurações iniciais do sensor de temperatura--------
#define BMP280_I2C_ADDRESS 0x76  //Define o endereço I2C do BMP280. Esse endereço (0x76) é o que o ESP8266 usará para "falar" com o sensor.
Adafruit_BMP280 bmp280;

//--------Configurações iniciais do sensor de luminosidade--------
BH1750 lightMeter;

//--------Configurações do LCD
// LCD I2C no endereço 0x27, 20 colunas e 4 linhas
LiquidCrystal_I2C lcd(0x27, 20, 4);

//-------- Variaveis para controle de tempo------------------------
float umidadeMin = 30.0;  // Umidade mínima ideal do solo (%)
float umidadeMax = 60.0;  // Umidade máxima ideal do solo (%)
int horasNecessarias = 1;
int tempoLuminosidade = horasNecessarias * 3600 * 1000;  // Exemplo: 1 horas em milissegundos
unsigned long ultimaLigacaoLampada = 0;
bool lampadaLigada = false;
bool bombaLigada = false;

//------------ Definição de pinos joystick, bomba e lampada
#define BOMBA D5
#define LAMPADA D6
#define VRX_PIN D3
#define SW_PIN D4

//------------ Auxiliar para timer de uma hora
unsigned long ultimaExecucao = 0;

//------------ Variáveis globais para controle da navegação
int indicePlantaAtual = 0;  // Índice da planta atualmente selecionada
bool vrxFoiParaEsquerda = false;
bool dadosPlantasProntos = false;

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
  lcd.print("Iniciado!");

  // Inicializa o sensor de luminosidade
  lightMeter.begin();

  // Configura o D5 para controle da bomba
  pinMode(BOMBA, OUTPUT);
  digitalWrite(BOMBA, LOW);  // Garante que a bomba comece desligada

  // Configuração do joystick
  pinMode(VRX_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);  // Botão é ativo em LOW

  // Conectar ao WiFi
  conectarWiFi();

  // Fazer requisição ao servidor
  requisitarDadosPlantas();
}

//--------Loop--------

void loop() {

  // Verifica a umidade do solo
  monitorarUmidadeSolo();

  // Verifica o tempo de luminosidade que a planta precisa
  //monitorarLampada();

  // Atualização da plataforma Blynk
  Blynk.run();

  // Envia os dados para a plataforma Blynk
  Blynk.virtualWrite(V0, lightMeter.readLightLevel());
  Blynk.virtualWrite(V1, bmp280.readTemperature());
  Blynk.virtualWrite(V2, umidade);

  if (dadosPlantasProntos) {
    navegarPlantasComJoystick();
  }

  // Buscar informações no servidor a cada hora
  if (millis() - ultimaExecucao >= 3600000) {
    ultimaExecucao = millis();

    // Fazer requisição ao servidor
    requisitarDadosPlantas();
  }
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
      static DynamicJsonDocument jsonDoc(4096);
      DeserializationError error = deserializeJson(jsonDoc, response);

      if (!error) {
        plantas = jsonDoc.as<JsonArray>();
        int totalPlantas = plantas.size();
        Serial.print("Quantidade de plantas recebidas: ");
        Serial.println(totalPlantas);
        Serial.println("Dados da planta atualizados!");
        dadosPlantasProntos = true;
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
    digitalWrite(BOMBA, HIGH);
    bombaLigada = true;
  } else if (umidade > umidadeMin && bombaLigada) {
    Serial.println("Bomba desligada!");
    digitalWrite(BOMBA, LOW);
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


void mostrarComScroll(const char* label, const char* texto, int linha = 0, int delayScroll = 300) {
  int len = strlen(texto);
  int maxVisivel = 20;

  lcd.setCursor(0, linha);
  lcd.print(label);

  if (len <= maxVisivel) {
    lcd.setCursor(0, linha + 1);
    lcd.print(texto);
  } else {
    for (int i = 0; i <= len - maxVisivel; i++) {
      lcd.setCursor(0, linha + 1);
      for (int j = 0; j < maxVisivel; j++) {
        lcd.print(texto[i + j]);
      }
      delay(delayScroll);
    }

    // Volta ao início
    delay(500);
    lcd.setCursor(0, linha + 1);
    for (int j = 0; j < maxVisivel; j++) {
      lcd.print(texto[j]);
    }
  }
}


void navegarPlantasComJoystick() {
  int vrxValor = digitalRead(VRX_PIN);
  int swValor = digitalRead(SW_PIN);

  // Avança para próxima planta se o joystick foi para esquerda
  if (vrxValor == LOW && !vrxFoiParaEsquerda && plantas.size() > 0) {
    indicePlantaAtual = (indicePlantaAtual + 1) % plantas.size();
    JsonObject planta = plantas[indicePlantaAtual];

    // Mostra nome da planta no LCD
    const char* nome = plantas[indicePlantaAtual]["name"].as<const char*>();
    lcd.clear();
    mostrarComScroll("Selecionando:", nome);

    vrxFoiParaEsquerda = true;
    delay(300);  // debounce
  }

  // Reseta flag quando volta pro centro
  if (vrxValor == HIGH && vrxFoiParaEsquerda) {
    vrxFoiParaEsquerda = false;
  }

  // Seleciona planta atual ao pressionar o botão
  if (swValor == LOW && plantas.size() > 0) {
    lcd.clear();

    const char* nome = plantas[indicePlantaAtual]["name"].as<const char*>();
    float uMin = plantas[indicePlantaAtual]["umidade_solo_min"].as<float>();
    float uMax = plantas[indicePlantaAtual]["umidade_solo_max"].as<float>();
    int horas = plantas[indicePlantaAtual]["tempo_luminosidade"].as<int>();

    // Mostrar parâmetros da planta
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Planta:");
    lcd.print(nome);
    lcd.setCursor(0, 2);
    lcd.print("Umin:");
    lcd.print(uMin, 0);
    lcd.print("% Umax:");
    lcd.print(uMax, 0);
    lcd.print("%");

    lcd.setCursor(0, 3);
    lcd.print("Luz/dia: ");
    lcd.print(horas);
    lcd.print("h");

    // Atualiza os valores do sistema com os da planta
    umidadeMin = uMin;
    umidadeMax = uMax;
    horasNecessarias = horas;
    tempoLuminosidade = horasNecessarias * 3600 * 1000;

    Serial.print("Planta selecionada: ");
    Serial.println(nome);

    delay(1000);  // Tempo para usuário ler
  }
}

