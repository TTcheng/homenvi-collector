
/**
   Homenvi-collector.ino

  Created on: 04.12.2019
  @author wangchuncheng
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WString.h>
#include "DHT.h"

#define SERIAL_DEBUG true

#define DHTPIN D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#ifndef STASSID
#define STASSID "dtang"
#define STAPSK "dtang2018"
#endif

#define DB_NAME "homenvi"

const char *ssid = STASSID;
const char *password = STAPSK;

String db_uri = "/write?db=";

struct {
  String host;
  uint16_t port;
  String user;
  String password;
} server{"120.27.144.202", 80, "HomenviCollectorAlpha", "o/2FPLOIbSojNVOOllhA1x/2/StL54d5DnjuQK9LfvI="},
    db{"132.232.181.189", 8086, "homenvi-collector", "homenvi"};

void logToSerial(String, String = "");

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  logToSerial("Connecting to ", ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
   would try to act as both a client and an access-point and could cause
   network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  logToSerial("WiFi connected,IP address: ", WiFi.localIP().toString());
  logToSerial("Prepare params...");
  const String paramNames[] = {
        "ipAddress", 
        "macAddress", 
        "dnsAddress",
        "subnetMaskAddress", 
        "gatewayAddress"
    };
  const String addresses[] = {
      WiFi.localIP().toString(), 
      WiFi.macAddress(), 
      WiFi.dnsIP().toString(),
      WiFi.subnetMask().toString(), 
      WiFi.gatewayIP().toString()
    };
  logToSerial("encode Params...");
  const String encodedParams = encodeUrlParam(addresses, paramNames, 5);
  Serial.println("Notify server...");
  notifyServer(encodedParams);

  // dht humidity and temperature
  logToSerial(">>>>>>>>> init <<<<<<<<<<");
  db_uri.concat(DB_NAME);
  db_uri.concat("&u=");
  db_uri.concat(db.user);
  db_uri.concat("&p=");
  db_uri.concat(db.password);

  logToSerial(">>>>>>>>> DHT begin <<<<<<<<<<");
  dht.begin();
}

void loop() {
  delay(5000);
  readHumiture();
}

/**
 * 读取温湿度
 */
void readHumiture() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    logToSerial("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  String payload = "humiture,identifier=";
  payload.concat(server.user);
  payload.concat(" humidity=");
  payload.concat(h);
  payload.concat(",celsius=");
  payload.concat(t);
  payload.concat(",fahrenheit=");
  payload.concat(f);
  payload.concat(",heatIndexCelsius=");
  payload.concat(hic);
  payload.concat(",heatIndexFahrenheit=");
  payload.concat(hif);
  writeDB(payload);
}

/**
 * 写入数据库
 */
void writeDB(String payload){
  WiFiClient client;
  HTTPClient http;
  http.begin(client, db.host, db.port, db_uri);
  logToSerial("humiture:", payload);
  http.addHeader("Content-Type","text/plain");
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      String response = http.getString();
      logToSerial(response);
      logToSerial("Successful Send humiture data to server!");
      return;
    }
    logToSerial("Send humiture... with wrong code : ", String(httpCode));
  } else {
    logToSerial("Send humiture... failed, error: ", http.errorToString(httpCode));
  }
  http.end();
}

/**
 * 启动时通知服务器
 */
void notifyServer(const String encodedParams) {
  WiFiClient client;
  HTTPClient http;  
  String uri = "/api/openApi/collectors/refresh";
  uri.concat("?identifier=");
  uri.concat(server.user);
  uri.concat("&password=");
  uri.concat(server.password);
  uri.concat("&");
  uri.concat(encodedParams);
  http.begin(client, server.host, server.port, uri);

  Serial.printf("Send get request http://%s:%d%s\n", server.host.c_str(), server.port, uri.c_str());
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      String payload = http.getString();
      logToSerial(payload);
      logToSerial("Success notified server!");
      return;
    }
    logToSerial("[HTTP] GET... with wrong code : ", String(httpCode));
  } else {
    logToSerial("[HTTP] GET... failed, error: ", http.errorToString(httpCode));
  }
  http.end();
}

/**
 * 将参数编码到url
 *
 * @param params 参数数组
 * @param paramNames 参数名数组
 * @param len 参数个数
 * @return 结果串
 */

String encodeUrlParam(const String params[], const String paramNames[], unsigned int len) {
  String res;
  for (unsigned int i = 0; i < len; i++) {
    res.concat(paramNames[i]);
    res.concat("=");
    res.concat(params[i]);
    res.concat("&");
  }
  return res;
}

/**
 * Serial log
 */
void logToSerial(String msg, String param){
  if(SERIAL_DEBUG){
    msg.concat(param);
    Serial.println(msg.c_str());
  }
}