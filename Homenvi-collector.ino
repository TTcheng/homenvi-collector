
/**
   Homenvi-collector.ino

  Created on: 04.12.2019
  @author wangchuncheng
*/
#include <WString.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID "dtang"
#define STAPSK "dtang2018"
#endif

#define HTTP_PREFIX "http://"

const char *ssid = STASSID;
const char *password = STAPSK;

struct
{
    String host;
    uint16_t port;
    String user;
    String password;
} server{"120.27.144.202", 80, "HomenviCollectorAlpha", "o/2FPLOIbSojNVOOllhA1x/2/StL54d5DnjuQK9LfvI="},
    db{"132.232.181.189:8086", 8086, "admin", "admin"};

String encodeUrlParam(const String params[], const String paramNames[], unsigned int len);
void notifyServer(const String encodedParams);

void setup()
{
    Serial.begin(115200);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Prepare params...");
    const String paramNames[] = {
        "ipAddress",
        "macAddress",
        "dnsAddress",
        "subnetMaskAddress",
        "gatewayAddress"
    };
    String test = "java";
    const String addresses[] = {
        WiFi.localIP().toString(),
        WiFi.macAddress(),
        WiFi.dnsIP().toString(),
        WiFi.subnetMask().toString(),
        WiFi.gatewayIP().toString()
    };
    Serial.printf("%s\n",addresses[0].c_str());
    Serial.println("encode Params...");
    const String encodedParams = encodeUrlParam(addresses, paramNames, 5);
    Serial.println("Notify server.");
    notifyServer(encodedParams);
}

void loop()
{
    Serial.println("... loop ...");
    delay(10000);
}

void notifyServer(const String encodedParams)
{
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
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT)
        {
            String payload = http.getString();
            Serial.println(payload);
            return;
        }
        Serial.printf("[HTTP] GET... with wrong code : %d", httpCode);
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

/**
 * 将参数编码到url
 *
 * @param params 参数
 * @param paramNames 参数名
 * @return 结果串的起始位置
 */
String encodeUrlParam(const String params[], const String paramNames[], unsigned int len) {
    int neededSize = len * 2;  // 需要的字符数量
    unsigned int i,j;
    for (i = 0; i < len; ++i) {
        neededSize += params[i].length();
        neededSize += paramNames[i].length();
    }
    neededSize += 1;
    char res[neededSize];
    unsigned int index = 0;
    // char *resPointer = res;
    // 分配空间
    // 第一个参数
    // 第一个参数前有?没有&
    // res[index++] = '?';
    unsigned int tmp = paramNames[0].length();
    for (i = 0; i < tmp; ++i) {
        res[index++] = paramNames[0][i];
    }
    res[index++] = '=';
    tmp = params[0].length();
    for (i = 0; i < tmp; ++i) {
        res[index++] = params[0][i];
    }

    // 后续参数
    for (i = 1; i < len; ++i) {
        res[index++] = '&';
        tmp = paramNames[i].length();
        for (j = 0; j < tmp; ++j) {
            res[index++] = paramNames[i][j];
        }
        res[index++] = '=';
        tmp = params[i].length();
        for (j = 0; j < tmp; ++j) {
            res[index++] = params[i][j];
        }
    }
    // 添加字符串结束符'\0'
    while (index < neededSize){
        res[index++] = '\0';
    }
    // 打印结果
    printf("encodedParams:%s\n", res);
    return res;
}