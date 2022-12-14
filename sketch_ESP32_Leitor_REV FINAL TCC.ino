// Bibliotecas incluídas
#include <stdint.h>
#include <Arduino.h>
#include <WiFiMulti.h>  //conexão com Wifi
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#define ABNT14522 "ABNT_14522"
#include <InfluxDbClient.h>  //conexão com banco de dados InfluxDB
#include <SoftwareSerial.h>  //comunicação com a serial do ESP
#include <IEEE754tools.h>

// Configurações da rede e conexão com o banco de dados
#define WIFI_SSID "Gean"
#define WIFI_PASSWORD "rgom191f"
#define INFLUXDB_URL "http://170.233.40.16:8086"
#define INFLUXDB_TOKEN "LAf5r4q4X0LfCdhBLEIk9Tm4rsNv22V0N5jGGihJQA_U1MvOWmOVBTW3ZvklO6Lv6wu_9T4Ve_0vYjolMAhW3g=="
#define INFLUXDB_ORG "Engentel"
#define INFLUXDB_BUCKET "ESP32"
unsigned long *converte_energia(byte *message_energia);
unsigned long converteId(byte *message_id);
byte bcdToDec(byte val);

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point sensor("wifi_status");

// Códigos ABNT NBR 14522:2008 para sincronizar comunicação
#define ENQ 0x05
#define ACK 0x06
#define NAK 0x15

// Array comando 14 - Leitura das grandezas instantâneas
const PROGMEM byte comando14[66] = { 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x05, 0x0F };


void setup() {

  Serial.setRxBufferSize(258);
  Serial.begin(9600);

  // Connect WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  // Add constant tags - only once
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

//####################################################################################################################################################

void send_comando14() {
  for (int k = 0; k < 66; k++) {
    Serial.write(comando14[k]);
  }
}

//####################################################################################################################################################

// Pintar em BIN
void printBinary(unsigned long value) {
  for (int mask = 0x80; mask; mask >>= 1) {
    if (mask & value)
      Serial.print('1');
    else
      Serial.print('0');
  }
}

/*
// Converte formato BCD para decimal.
byte bcdToDec(byte val) {
	return( (val/16*10) + (val%16) );
}
*/

//BCD para decimal
inline uint8_t bcdToDec(uint8_t bcd) {
  // e.g. 0x14 -> 14
  return (0x0F & bcd) + 10 * ((0xF0 & bcd) >> 4);
}

// Conversão para IEEE-754 32-bit floating point
float floatizeMe(unsigned int myNumba) {
  //// myNumba comes in as 32 bits or 8 byte

  unsigned int sign = myNumba >> 31;
  signed int exponent = ((myNumba >> 23) & 0xff) - 0x7F;
  unsigned int mantissa = myNumba << 9;
  float value = 0;
  float mantissa2;

  value = 0.5f;
  mantissa2 = 0.0f;
  while (mantissa) {
    if (mantissa & 0x80000000)
      mantissa2 += value;
    mantissa <<= 1;
    value *= 0.5f;
  }

  value = (1.0f + mantissa2) * (pow(2, exponent));
  if (sign) value = -value;

  return value;
}
//####################################################################################################################################################
/*
// IEEE 754 floating point representation
// into real value
 
#include<bits/stdc++.h>
using namespace std;
 
typedef union {
 
    float f;
    struct
    {
 
        // Order is important.
        // Here the members of the union data structure
        // use the same memory (32 bits).
        // The ordering is taken
        // from the LSB to the MSB.
        unsigned int mantissa : 23;
        unsigned int exponent : 8;
        unsigned int sign : 1;
 
    } raw;
} myfloat;
 
// Function to convert a binary array
// to the corresponding integer
unsigned int convertToInt(unsigned int* arr, int low, int high)
{
    unsigned int f = 0, i;
    for (i = high; i >= low; i--) {
        f = f + arr[i] * pow(2, high - i);
    }
    return f;
}*/

//####################################################################################################################################################


void loop() {

  byte rb[258] = {};
  unsigned long multiplicador1[4] = { 1000000UL, 10000UL, 100UL, 1UL };
  unsigned long multiplicador2[5] = { 10000000UL, 1000000UL, 10000UL, 100UL, 1UL };
  
  //Envia o comando para o medidor a cada 5s conforme configurado
  
  send_comando14();

  // Lê os dados recebidos na serial e armazena no array de 258 posições
  if (Serial.available() > 0) {

    memset(rb, 0, sizeof(rb));
    for (int j = 0; j < 258; j++) {
      rb[j] = Serial.read();
    }
    while(Serial.available()){
      int esvazia_buffer = Serial.read();
    }
  }
delay(5000);

  // Extrai os dados para enviar ao banco de dados

  //Localização - dados inseridos manualmente
  float lat;
  float lon;
  lat = -27.64432410182542;
  lon = -52.279147255560105;

  // Extrai o Serial Number
  unsigned long serial_number = 0;
  unsigned long serial_number_1 = 0;  
  unsigned long serial_number_2 = 0;  
  serial_number = (rb[1] <<24) | (rb[2] <<16) | (rb[3] <<8) | (rb[4]);
  for (int j = 0; j <= 3; j++) {
  serial_number_1 = serial_number + (unsigned long)(bcdToDec((rb[j + 1]) * multiplicador1[j]));
  serial_number_2 = bcdToDec(serial_number_1);
  }
   

  Serial.println();
  Serial.print("SERIAL DO MEDIDOR= ");
  Serial.print(serial_number_1);
  Serial.println();
  Serial.print("SERIAL DO MEDIDOR 2= ");
  Serial.print(serial_number_2);
  
  

  // Extrai Tensão de fase A
  unsigned long va = 0;
  float va1 = 0;
  va = (rb[14] << 24) | (rb[13] << 16) | (rb[12] << 8) | (rb[11]);
  for (int j = 0; j <= 4; j++) {
    va1 = floatizeMe(va + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VA= ");
  Serial.print(va1);
  Serial.print(" V");

  


  // Extrai Tensão de fase B
  unsigned long vb = 0;
  float vb1 = 0;
  vb = (rb[18] << 24) | (rb[17] << 16) | (rb[16] << 8) | (rb[15]);
  for (int j = 0; j <= 4; j++) {
    vb1 = floatizeMe(vb + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VB= ");
  Serial.print(vb1);
  Serial.print(" V");

  // Extrai Tensão de fase C
  unsigned long vc = 0;
  float vc1 = 0;
  vc = (rb[22] << 24) | (rb[21] << 16) | (rb[20] << 8) | (rb[19]);
  for (int j = 0; j <= 4; j++) {
    vc1 = floatizeMe(vc + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VC= ");
  Serial.print(vc1);
  Serial.print(" V");

// Extrai Tensão de lina AB
  unsigned long vab = 0;
  float vab1 = 0;
  vab = (rb[26] << 24) | (rb[25] << 16) | (rb[24] << 8) | (rb[23]);
  for (int j = 0; j <= 4; j++) {
    vab1 = floatizeMe(vab + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VAB= ");
  Serial.print(vab1);
  Serial.print(" V");

  // Extrai Tensão de linha BC
  unsigned long vbc = 0;
  float vbc1 = 0;
  vbc = (rb[30] << 24) | (rb[29] << 16) | (rb[28] << 8) | (rb[27]);
  for (int j = 0; j <= 4; j++) {
    vbc1 = floatizeMe(vbc + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VBC= ");
  Serial.print(vbc1);
  Serial.print(" V");

  // Extrai Tensão de linha CA
  unsigned long vca = 0;
  float vca1 = 0;
  vca = (rb[34] << 24) | (rb[33] << 16) | (rb[32] << 8) | (rb[31]);
  for (int j = 0; j <= 4; j++) {
    vca1 = floatizeMe(vca + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("VCA= ");
  Serial.print(vca1);
  Serial.print(" V");

  // Extrai corrente de fase A
  unsigned long ia = 0;
  float ia1 = 0;
  ia = (rb[38] << 24) | (rb[37] << 16) | (rb[36] << 8) | (rb[35]);
  for (int j = 0; j <= 4; j++) {
    ia1 = floatizeMe(ia + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("IA= ");
  Serial.print(ia1);
  Serial.print(" A");

  // Extrai corrente de fase B
  unsigned long ib = 0;
  float ib1 = 0;
  ib = (rb[42] << 24) | (rb[41] << 16) | (rb[40] << 8) | (rb[39]);
  for (int j = 0; j <= 4; j++) {
    ib1 = floatizeMe(ib + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("IB= ");
  Serial.print(ib1);
  Serial.print(" A");

  // Extrai Corrente de fase C
  unsigned long ic = 0;
  float ic1 = 0;
  ic = (rb[46] << 24) | (rb[45] << 16) | (rb[44] << 8) | (rb[43]);
  for (int j = 0; j <= 4; j++) {
    ic1 = floatizeMe(ic + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("IC= ");
  Serial.print(ic1);
  Serial.print(" A");

  // Extrai potência ativa de fase A
  unsigned long pow_a = 0;
  float pow_a1 = 0;
  pow_a = (rb[54] << 24) | (rb[53] << 16) | (rb[52] << 8) | (rb[51]);
  for (int j = 0; j <= 4; j++) {
    pow_a1 = floatizeMe(pow_a + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA ATIVA A= ");
  Serial.print(pow_a1);
  Serial.print(" kW");

  // Extrai potência ativa de fase B
  unsigned long pow_b = 0;
  float pow_b1 = 0;
  pow_b = (rb[58] << 24) | (rb[57] << 16) | (rb[56] << 8) | (rb[55]);
  for (int j = 0; j <= 4; j++) {
    pow_b1 = floatizeMe(pow_b + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA ATIVA B= ");
  Serial.print(pow_b1);
  Serial.print(" kW");

  // Extrai potência ativa de fase C
  unsigned long pow_c = 0;
  float pow_c1 = 0;
  pow_c = (rb[62] << 24) | (rb[61] << 16) | (rb[60] << 8) | (rb[59]);
  for (int j = 0; j <= 4; j++) {
    pow_c1 = floatizeMe(pow_c + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA ATIVA C= ");
  Serial.print(pow_c1);
  Serial.print(" kW");


  // Extrai potência retiva de fase A
  unsigned long powr_a = 0;
  float powr_a1 = 0;
  powr_a = (rb[70] << 24) | (rb[69] << 16) | (rb[68] << 8) | (rb[67]);
  for (int j = 0; j <= 4; j++) {
    powr_a1 = floatizeMe(powr_a + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA REATIVA A= ");
  Serial.print(powr_a1);
  Serial.print(" kVAr");

  // Extrai potência reativa de fase B
  unsigned long powr_b = 0;
  float powr_b1 = 0;
  powr_b = (rb[74] << 24) | (rb[73] << 16) | (rb[72] << 8) | (rb[71]);
  for (int j = 0; j <= 4; j++) {
    powr_b1 = floatizeMe(powr_b + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA REATIVA B= ");
  Serial.print(powr_b1);
  Serial.print(" kVAr");

  // Extrai potência reativa de fase C
  unsigned long powr_c = 0;
  float powr_c1 = 0;
  powr_c = (rb[78] << 24) | (rb[77] << 16) | (rb[76] << 8) | (rb[75]);
  for (int j = 0; j <= 4; j++) {
    powr_c1 = floatizeMe(powr_c + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("POTENCIA REATIVA C= ");
  Serial.print(powr_c1);
  Serial.print(" kVAr");


  // Extrai frequencia da rede
  unsigned long freq = 0;
  float freq1 = 0;
  freq = (rb[186] << 24) | (rb[185] << 16) | (rb[184] << 8) | (rb[183]);
  for (int j = 0; j <= 4; j++) {
    freq1 = floatizeMe(freq + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("FREQUENCIA DA REDE= ");
  Serial.print(freq1);
  Serial.print(" Hz");

   // Extrai temperatura interna
  unsigned long temp = 0;
  float temp1 = 0;
  freq = (rb[182] << 24) | (rb[181] << 16) | (rb[180] << 8) | (rb[179]);
  for (int j = 0; j <= 4; j++) {
    temp1 = floatizeMe(temp + (bcdToDec(rb[j + 5]) * multiplicador2[j]));
  }

  Serial.println();
  Serial.print("TEMPERATURA INTERNA= ");
  Serial.print(temp1);
  Serial.print(" ºC");

  // Extrai modelo do medidor
  unsigned long mod = 0;
  mod = rb[196];


  Serial.println();
  Serial.print("MODELO DO MEDIDOR= ");
  Serial.print(mod);
  Serial.println();

  delay(1000);


  // Store measured value into point
  sensor.clearFields();

  // Report data to Influxdb
  sensor.addField("rssi", WiFi.RSSI());
  sensor.addField("serial_number", serial_number_1);
  sensor.addField("va1", va1);
  sensor.addField("vb1", vb1);
  sensor.addField("vc1", vc1);
  sensor.addField("vab1", vab1);
  sensor.addField("vbc1", vbc1);
  sensor.addField("vca1", vca1);
  sensor.addField("ia1", ia1);
  sensor.addField("ib1", ib1);
  sensor.addField("ic1", ic1);
  sensor.addField("pow_a1", pow_a1);
  sensor.addField("pow_b1", pow_b1);
  sensor.addField("pow_c1", pow_c1);
  sensor.addField("powr_a1", powr_a1);
  sensor.addField("powr_b1", powr_b1);
  sensor.addField("powr_c1", powr_c1);
  sensor.addField("freq", freq1);
  sensor.addField("temp", temp1);
  //sensor.addField("mod1", mod1);
  sensor.addField("latitude", lat);
  sensor.addField("longitude", lon);


  // Print what are we exactly writing
    //Serial.println("");
    //Serial.println(client.pointToLineProtocol(sensor));


  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
delay(1000);
  
}
