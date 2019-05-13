/**
   Master.ino
   通过串口接收slave的数据，通过HTTP传给服务器

  Created on: 04.12.2019
  @author wangchuncheng
  @mail ttchengwang@foxmail.com
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
} server{"106.14.139.98", 80, "HomenviCollectorAlpha",
         "o/2FPLOIbSojNVOOllhA1x/2/StL54d5DnjuQK9LfvI="},
    db{"106.14.139.98", 80, "homenvi-collector", "homenvi"};

String db_uri = "/influx/write?db=";

void logToSerial(String, String = "");
void logToDB(String, String = "");

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
        String finalData = fixComData(comdata);
        logToSerial("finalData:", finalData);
        // data collections: "name1=value1,name2=value2,...."
        String payload = "collections,identifier=";
        payload.concat(server.user);
        payload.concat(' ');
        payload.concat(finalData);
        writeDB(payload);
      } else {
        // debug info
        logToDB(comdata);
      }
    }
  }
}
typedef struct {
  String name;
  int min;
  int max;
} Specification;
const Specification specifications[] = {{"humidity", 0, 100},
                                        {"celsius", -40, 80},
                                        {"heatIndexCelsius", -40, 80},
                                        {"fahrenheit", -104, 176},
                                        {"heatIndexFahrenheit", -104, 176},
                                        {"sound", 0, 1023},
                                        {"brightness", 0, 10000},
                                        {"dustDensity", 0, 500},
                                        {"gasValue", 0, 1023}};
/**
 * 串口数据可能会有错误，丢弃损坏的数据
 */
String fixComData(String comData) {
  unsigned int maxlen = 200;
  if (comData.length() > maxlen) {
    return "";
  }
  char chars[maxlen];
  comData.toCharArray(chars, maxlen);
  String collections[9];
  unsigned int i, j = 0;
  for (unsigned int i = 0; i < maxlen, chars[i] != 0, j < 9; i++) {
    if (chars[i] == ',') {
      j++;
      continue;
    }
    collections[j] += chars[i];
  }
  String res = "";
  for (i = 0; i < 9; i++) {
    String thisCollection = collections[i]; // name=value, e: humidity=50.50 
    bool correct = false;
    for (j = 0; j < 9; j++) {
      if (thisCollection.startsWith(specifications[j].name) != 0 &&
          '=' == thisCollection.charAt(specifications[j].name.length())) {
        correct = true;
        break;
      }
    }
    unsigned int len = thisCollection.length();
    if (thisCollection.charAt(len - 1) == '.') {
      thisCollection.concat("0)");
    } else if (thisCollection.charAt(len - 2) == '.') {
      thisCollection.concat("0");
    } else if (thisCollection.charAt(len - 3) != '.') {
      // 不包含小数点，或小数点位置不对
      correct = false;
    }

    if (correct) {
      res += thisCollection;
      res += ',';
    }
  }
  return res.substring(0, res.length() - 1);
}

/**
 * 写入数据库
 */
void writeDB(String payload){
  WiFiClient client;
  HTTPClient http;
  http.begin(client, db.host, db.port, db_uri);
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      logToSerial("Successfully Send collections to server!\n");
    } else {
      logToSerial("[HTTP] POST, Failed to send collections... with wrong code : ", String(httpCode));
      String error = http.getString();
      if (error.length() > 0) {
        logToSerial("[HTTP] Error info:", error);
      }
    }
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

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
      String payload = http.getString();
      logToSerial("Success notified server!\n");
      if (payload.length() > 0) {
        logToSerial("response : ", payload);
      }
    } else {
      logToSerial("[HTTP] GET, Failed to notify server! with wrong code : ", String(httpCode));
    }
  } else {
    logToSerial("[HTTP] GET, Failed to notify server! error: ", http.errorToString(httpCode));
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

void logToDB(String msg, String append){
  logToSerial(msg, append);
  String payload = "logging,identifier=";
  payload += server.user;
  payload += ' ';
  payload += "content=";
  payload += '"';
  payload += msg;
  payload += append;
  payload += '"';
  writeDB(payload);
}