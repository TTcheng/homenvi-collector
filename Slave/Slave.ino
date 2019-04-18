/**
   Slave.ino
  采集数据并格式化，然后通过串口传给Master

  Data format: "name1=value1,name2=value2,..."
  
  Info format: "sentence not contains '='"

  Created on: 04.18.2019
  @author wangchuncheng
*/
#include <WString.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define SOUNDPIN A0

String readHumiture();

void setup() {
  Serial.begin(9600);

  pinMode(DHTPIN, INPUT);
  pinMode(SOUNDPIN, INPUT);
  dht.begin();
  sendInfo("Slave setup");
}

void loop() {
  delay(5000);
  String humiture = readHumiture();
  // String sound = readSound();
  // String brightness = readBrightness();
  String data = humiture;
  // data.concat(',');
  // data.concat(sound);
  // data.concat(',');
  // data.concat(brightness);
  sendData(data);
}

/**
 * 读取噪音
 */
String readSound() {
  int soundValue = analogRead(SOUNDPIN);
  String sound = "sound=";
  sound.concat(soundValue);
  return sound;
}

/**
 * 读取温湿度
 */
String readHumiture() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    sendInfo("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  String humiture = "humidity=";
  humiture.concat(h);
  humiture.concat(",celsius=");
  humiture.concat(t);
  humiture.concat(",fahrenheit=");
  humiture.concat(f);
  humiture.concat(",heatIndexCelsius=");
  humiture.concat(hic);
  humiture.concat(",heatIndexFahrenheit=");
  humiture.concat(hif);
  return humiture;
}

/**
 * 发送数据到master,format: "name1=value1,name2=value2...."
 */
void sendData(String data) { Serial.write(data.c_str()); }
/**
 * 发送信息到master，注意内容应不含'='。
 * info shouldn't contains '='
 */
void sendInfo(String info) { Serial.write(info.c_str()); }
