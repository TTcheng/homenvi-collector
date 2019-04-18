/**
   Master.ino
   通过串口接收slave的数据，通过HTTP传给服务器

  Created on: 04.12.2019
  @author wangchuncheng
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#include <WString.h>

#define LOG_SERIAL true

SoftwareSerial slaveSerial(13, 15); // RX, TX

#ifndef STASSID
#define STASSID "dtang"
#define STAPSK "dtang2018"
#endif

#define DB_NAME "homenvi"
struct {
  String host;
  uint16_t port;

  String user;
  String password;
} server{"120.27.144.202", 80, "HomenviCollectorAlpha",
         "o/2FPLOIbSojNVOOllhA1x/2/StL54d5DnjuQK9LfvI="},
    db{"132.232.181.189", 8086, "homenvi-collector", "homenvi"};

String db_uri = "/write?db=";

void logToSerial(String, String = "");

void setup() {
  db_uri.concat(DB_NAME);
  db_uri.concat("&u=");
  db_uri.concat(db.user);
  db_uri.concat("&p=");
  db_uri.concat(db.password);
  Serial.begin(115200);
  slaveSerial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  logToSerial("Connecting to ", STASSID);

  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  logToSerial("WiFi connected,IP address: ", WiFi.localIP().toString());
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
  const String encodedParams = encodeUrlParam(addresses, paramNames, 5);
  notifyServer(encodedParams);
}

void loop() {
  if (slaveSerial.available() > 0) {
    String comdata = "";
    while (slaveSerial.available() > 0) {
      comdata += char(slaveSerial.read());
      delay(1);
    }
    if (comdata.length() > 0) {
      if (comdata.indexOf('=') > 0) {
        logToSerial(comdata);
        // data collections: "name1=value1,name2=value2,...."
        String payload = "collections,identifier=";
        payload.concat(server.user);
        payload.concat(' ');
        payload.concat(comdata);
        writeDB(payload);
      } else {
        // debug info
        logToSerial(comdata);
      }
    }
  }
}

/**
 * 写入数据库
 */
void writeDB(String payload){
  WiFiClient client;
  HTTPClient http;
  http.begin(client, db.host, db.port, db_uri);
  http.addHeader("Content-Type","text/plain");
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      logToSerial("Successfully Send collections to server!\n");
      return;
    }
    logToSerial("[HTTP] POST, Failed to send collections... with wrong code : ", String(httpCode));
    return;
  } else {
    logToSerial("[HTTP] POST, Failed to send collections... , error: ", http.errorToString(httpCode));
  }
  http.end();
}

/**
 * 启动时通知服务器以记录IP，GW等地址
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

  // Serial.printf("Send get request http://%s:%d%s\n", server.host.c_str(), server.port, uri.c_str());
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      String payload = http.getString();
      logToSerial("Success notified server!\n");
      if (payload.length() > 0) {
        logToSerial("response : ", payload);
      }
      return;
    }
    logToSerial("[HTTP] GET... with wrong code : ", String(httpCode));
    return;
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
void logToSerial(String msg, String append){
  if(LOG_SERIAL){
    msg.concat(append);
    Serial.println(msg.c_str());
  }
}