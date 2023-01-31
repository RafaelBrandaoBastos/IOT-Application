#include <PubSubClient.h>
#include <WiFi.h>
#define TOPICO_PUBLISH_TEMPERATURE "ESP32RAFAELtemperatura"
#define TOPICO_PUBLISH_HUMIDITY "ESP32RAFAELumidade"
#define TOPICO_PUBLISH_LIGHT "ESP32RAFAELluz"
#define TOPICO_PUBLISH_WLEVEL "ESP32RAFAELtanque"
#define TOPICO_PUBLISH_WLEVEL2 "ESP32RAFAELchuva"
#define TOPICO_PUBLISH_SOIL"ESP32RAFAELsoil"
#define TOPICO_SUBSCRIBE_BOMBA "ESP32RAFAELbomba"
#define ID_MQTT "esp32rafael"
//DHT11
#include "DHT.h"
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//LIGHT
#include <Wire.h>
#include <BH1750.h>
bool BH1750Check = false;
BH1750 lightMeter;
//Pump
#define PUMP_PIN  12 
// WLEVEL
#define SIGNAL_PIN 35 
#define SENSOR_MIN 0
#define SENSOR_MAX 521
int Wvalue = 0; 
int Wlevel = 0; 
#define POWER_PIN  26
#define SIGNAL_PIN2 34 
#define SENSOR_MIN2 0
#define SENSOR_MAX2 521
int Wvalue2 = 0;
int Wlevel2 = 0; 
//SOIL
#define POWER_SOIL  25
#define SOIL_PIN 33
float FazLeituraUmidade(void);



const char *SSID = "moto";
const char *PASSWORD = "1234567890";
const char *BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883;
WiFiClient espClient;
PubSubClient MQTT(espClient);
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);

void initWiFi(void) {
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnectWiFi();
}

void initMQTT(void) {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("\nChegou a seguinte string via MQTT: ");
  Serial.print(msg);
  if (msg.equals("1")) {
    digitalWrite(PUMP_PIN, HIGH);
  }
  if (msg.equals("0")) {
    digitalWrite(PUMP_PIN, LOW);
  }
  Serial.println("");
}

void VerificaConexoesWiFIEMQTT(void) {
  if (!MQTT.connected())
    reconnectMQTT();
  reconnectWiFi();
}

void reconnectMQTT(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE_BOMBA); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}

void reconnectWiFi(void) {
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("\nIP obtido: ");
  Serial.println(WiFi.localIP());
}

//SENSORES

float FazLeituraUmidade(void)
{
    int ValorADC;
    float UmidadePercentual;
    ValorADC = analogRead(SOIL_PIN);   //978 -> 3,3V
    Serial.print("[Leitura ADC] ");
    Serial.print(ValorADC);
    UmidadePercentual = 100 * ((5078-(float)ValorADC) / 5078);
    Serial.print("[Umidade Percentual] ");
    Serial.print(UmidadePercentual);
    Serial.print("%");

    char uchar[4]; // Buffer big enough for 7-character float
    dtostrf(UmidadePercentual, 0, 0, uchar); // Leave room for too large numbers!
    MQTT.publish(TOPICO_PUBLISH_SOIL, uchar);

    return UmidadePercentual;
}

void dht11() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  char tchar[4]; // Buffer big enough for 7-character float
  dtostrf(t, 2, 0, tchar); // Leave room for too large numbers!
  char hchar[4]; // Buffer big enough for 7-character float
  dtostrf(h, 2, 0, hchar); // Leave room for too large numbers

  Serial.print(F("Humidity: "));
  Serial.print(hchar);
  Serial.print(F("%  Temperature: "));
  Serial.print(tchar);
  Serial.print(F(" C"));
  t = roundf(t * 100) /100;


  MQTT.publish(TOPICO_PUBLISH_TEMPERATURE, tchar);

  MQTT.publish(TOPICO_PUBLISH_HUMIDITY, hchar);
}

void gy30(){
  if (BH1750Check) {
    float lux = lightMeter.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.print(" lx");
    char lchar[4]; // Buffer big enough for 7-character float
    dtostrf(lux, 0, 0, lchar); // Leave room for too large numbers!
    MQTT.publish(TOPICO_PUBLISH_LIGHT, lchar);
  }
}

void soil(){
  digitalWrite(POWER_SOIL, HIGH);  // turn the sensor ON
  delay(10); 
  float UmidadePercentualLida;
  int UmidadePercentualTruncada;
  char FieldUmidade[11];
  UmidadePercentualLida = FazLeituraUmidade();
  digitalWrite(POWER_SOIL, LOW);  // turn the sensor ON
  delay(10);
  UmidadePercentualTruncada = (int)UmidadePercentualLida; //trunca umidade como número inteiro
}
void wlevel() {
  digitalWrite(POWER_PIN, HIGH);  // turn the sensor ON
  delay(10);                      // wait 10 milliseconds
  Wvalue = analogRead(SIGNAL_PIN); // read the analog value from sensor
  Wvalue2 = analogRead(SIGNAL_PIN2); // read the analog value from sensor2
  digitalWrite(POWER_PIN, LOW);   // turn the sensor OFF
  Wlevel = map(Wvalue, SENSOR_MIN, SENSOR_MAX, 0, 4); // 4 levels
  Wlevel2 = map(Wvalue2, SENSOR_MIN2, SENSOR_MAX2, 0, 4); // 4 levels
  Serial.print("Water level: ");
  Serial.print(Wlevel);
  Serial.print(" Chuva: ");
  if(Wlevel2 > 0) {
    Serial.print("Sim ");
    Serial.print(" Nivel: ");
    Serial.print(Wlevel2);
  } else {
    Serial.print(Wlevel2);
  }
  std::string wlevelc = std::to_string(Wlevel);
  const char *wlevelconvertido = wlevelc.c_str();
  MQTT.publish(TOPICO_PUBLISH_WLEVEL, wlevelconvertido);

  std::string wlevelc2 = std::to_string(Wlevel2);
  const char *wlevelconvertido2 = wlevelc2.c_str();
  MQTT.publish(TOPICO_PUBLISH_WLEVEL2, wlevelconvertido2);
}



void setup() {

  Serial.begin(9600);
  Serial.println("Disciplina IoT: acesso a nuvem via ESP32");
  randomSeed(analogRead(0));
  initWiFi();
  initMQTT();
  
  //PUMP
  pinMode(PUMP_PIN, OUTPUT);  
  digitalWrite(PUMP_PIN, LOW);
  
  // DHT11
  dht.begin();
  
  //LIGHT
  Wire.begin(21, 22);
  BH1750Check = lightMeter.begin();
  if (BH1750Check) {
    Serial.println(F("BH1750 Test begin"));
  }
  else {
    Serial.println(F("BH1750 Initialization FAILED"));
    while (true)
    {}
  }
  
  //WLEVEL
  pinMode(POWER_PIN, OUTPUT);  
  digitalWrite(POWER_PIN, LOW);

  //SOIL
  pinMode(POWER_SOIL, OUTPUT);  
  digitalWrite(POWER_SOIL, LOW);

}


void loop() {
  VerificaConexoesWiFIEMQTT();
  dht11();
  Serial.print(" || ");
  gy30();
  Serial.print(" || ");
  wlevel();
  Serial.print(" || ");
  soil();
  Serial.print(" || ");
  Serial.print("\n");
  Serial.print("\n");
  MQTT.loop();
  delay(5000);
}
