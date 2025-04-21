#ifndef CONFIG_H
#define CONFIG_H

//-------Informações sobre a planta--------

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL2HU4otiaB"
#define BLYNK_TEMPLATE_NAME "Tcc Plantinha"
#define BLYNK_AUTH_TOKEN "6eu6YBZ-GDQFj-N5gC6QZd_5fs3mxmm0"

//-------Configurações iniciais da plataforma Blynk--------

// Insira abaixo o nome da sua rede WiFi
const char* ssid = "Desktop_F4361394";

// Insira abaixo a senha da sua rede WiFi
const char* password = "7532112703667503";

// Endereço do Flask
const char* serverAddress = "http://192.168.1.102:5000/plantas";

#endif
