// Bibliotecas MQTT e ESP8266
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Bibliotecas Sensores
#include <Wire.h>
#include <OneWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <DallasTemperature.h>
#include <ADS1115_WE.h> 

// Configuração de rede e servidor
#define WIFI_SSID "ALUNOS_AP_42"
#define WIFI_PASSWORD ""
#define MQTT_HOST IPAddress(192, 100, 40, 253)
#define MQTT_PORT 1883

// Define o caminho das publicações
#define MQTT_PUB_STATUS "esp/status"
#define MQTT_PUB_TEMP "esp/bme280/temperatura"
#define MQTT_PUB_UMI "esp/bme280/umidade"
#define MQTT_PUB_PRES "esp/bme280/pressao"
#define MQTT_PUB_LUX "esp/bh1750/luminosidade"
#define MQTT_PUB_TEMPSOLO "esp/ds18b20/temperatura"
#define MQTT_PUB_UMISOLO "esp/hd38/umidade"
#define MQTT_PUB_UV "esp/gyml8511/uv"

// Variáveis que guardam a leitura dos sensores
float temp;
float umi;
float pres;
uint16_t lux;
float tempSolo;
float umiSolo;
int uv;

// I2C
#define SDA D1
#define SCL D2
#define I2C_ADDRESS 0x48
ADS1115_WE adc(I2C_ADDRESS);

#define ONE_WIRE_BUS D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
Adafruit_BME280 bme;
BH1750 lightMeter;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando à rede WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void setupBME280() {
  if (!bme.begin(0x76)) {
    Serial.println("Não foi possivel encontrar o sensor BME280!");
    while (1);
  }
}

void setupBH1750() {
  if (!lightMeter.begin()) {
    Serial.println("Não foi possivel encontrar o sensor BH1750!");
    while (1);
  }
}

void setupDS18B20() {
  DS18B20.begin();
}

void setupADS1115() {
  if(!adc.init()){
    Serial.println("ADS1115 não conectado!");
    while (1);
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_6144);
  adc.setCompareChannels(ADS1115_COMP_3_GND);
  adc.setMeasureMode(ADS1115_CONTINUOUS);
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  voltage = adc.getResult_V();
  return voltage;
}

void reconnect() {
  // Mensagem lastWill
  byte willQoS = 0;
  const char* willMessage = "OFF_LINE";
  boolean willRetain = true;
  // Loop até reconectar
  while (!client.connected()) {
    Serial.print("Conectando com o MQTT...");
    // Cria identificação randômica do cliente
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_PUB_STATUS, willQoS, willRetain, willMessage)) {
      Serial.println("conectado");
      // Uma vez conectado publica anúncio:
      char* message = "ON_LINE";
      int length = strlen(message);
      boolean retained = true; 
      client.publish(MQTT_PUB_STATUS, (byte*)message, length, retained);
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void publishSensors() {
     // Converte o valor para um array de char
    char tempString[8];
    dtostrf(temp, 1, 2, tempString);
    Serial.print("Temperatura: ");
    Serial.println(tempString);
    client.publish(MQTT_PUB_TEMP, tempString);

    char umiString[8];
    dtostrf(umi, 1, 2, umiString);
    Serial.print("Umidade: ");
    Serial.println(umiString);
    client.publish(MQTT_PUB_UMI, umiString);

    char presString[8];
    dtostrf(pres, 1, 2, presString);
    Serial.print("Pressão: ");
    Serial.println(presString);
    client.publish(MQTT_PUB_PRES, presString);

    char luxString[8];
    dtostrf(lux, 1, 2, luxString);
    Serial.print("Luminosidade: ");
    Serial.println(luxString);
    client.publish(MQTT_PUB_LUX, luxString);

    char tempSoloString[8];
    dtostrf(tempSolo, 1, 2, tempSoloString);
    Serial.print("Temperatura do Solo: ");
    Serial.println(tempSoloString);
    client.publish(MQTT_PUB_TEMPSOLO, tempSoloString);

    char umiSoloString[8];
    dtostrf(umiSolo, 1, 2, umiSoloString);
    Serial.print("Umidade do Solo: ");
    Serial.println(umiSoloString);
    client.publish(MQTT_PUB_UMISOLO, umiSoloString);

    char uvString[8];
    dtostrf(uv, 1, 2, uvString);
    client.publish(MQTT_PUB_UV, uvString);
}

void nivelUV() {
  float valorSensorUV = map(readChannel(ADS1115_COMP_1_GND), 0, 3.3, 0, 1023);
  float valorSensorRef = readChannel(ADS1115_COMP_2_GND);

  if(valorSensorUV <= 227) uv = 0;
  else if (valorSensorUV > 227 && valorSensorUV <= 318)   uv = 1;
  else if (valorSensorUV > 318 && valorSensorUV <= 408)   uv = 2;
  else if (valorSensorUV > 408 && valorSensorUV <= 503)   uv = 3;
  else if (valorSensorUV > 503 && valorSensorUV <= 606)   uv = 4;
  else if (valorSensorUV > 606 && valorSensorUV <= 696)   uv = 5;
  else if (valorSensorUV > 696 && valorSensorUV <= 795)   uv = 6;
  else if (valorSensorUV > 795 && valorSensorUV <= 881)   uv = 7;
  else if (valorSensorUV > 881 && valorSensorUV <= 976)   uv = 8;
  else if (valorSensorUV > 976 && valorSensorUV <= 1079)  uv = 9;
  else if (valorSensorUV > 1079 && valorSensorUV <= 1170) uv = 10;
  else uv = 11;

  Serial.print("Valor Sensor UV: ");
  Serial.println(valorSensorUV);
  Serial.print("Valor Referencia: ");
  Serial.println(valorSensorRef);
  Serial.print("Nível UV: ");
  Serial.println(uv);  
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  Wire.begin(SDA, SCL);
  setupADS1115();
  setupBME280();
  setupBH1750();
  setupDS18B20();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // Leitura dos sensores
    temp = bme.readTemperature();
    umi = bme.readHumidity();
    pres = bme.readPressure()/100.0F;
    lux = lightMeter.readLightLevel();
    DS18B20.requestTemperatures();
    tempSolo = DS18B20.getTempCByIndex(0);
    umiSolo = map(readChannel(ADS1115_COMP_0_GND), 0, 5, 100, 0);
    nivelUV();
  
    publishSensors();
  }
}