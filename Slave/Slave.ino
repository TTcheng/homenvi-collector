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

/* humiture macro */
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define SOUNDPIN A0

/* dust macro */
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000 
#define        DUST_LED                        3              //drive the led of sensor
#define        DUST_PIN                        A0             //analog input

/* gas macro */
#define       GAS_DIGITAL_PIN                  4
#define       GAS_ANALOG_PIN                   A1

/* private functions */
String readHumiture();

void setup() {
  Serial.begin(9600);

  pinMode(SOUNDPIN, INPUT);
  /* init dust sensor*/
  pinMode(DUST_LED, OUTPUT);
  digitalWrite(DUST_LED, LOW); 
  pinMode(DUST_PIN, INPUT);
  /* init flammableGas sensor */
  pinMode(GAS_ANALOG_PIN, INPUT);
  pinMode(GAS_DIGITAL_PIN, INPUT);
  /* init humiture sensor*/
  pinMode(DHTPIN, INPUT);
  dht.begin();
  sendInfo("Slave setup");
}

void loop() {
  delay(5000);
  String humiture = readHumiture();
  String sound = readSound();
  String brightness = readBrightness();
  String dust = readDust();
  String flammableGas = readFlammableGas();
  String data = humiture;
  data.concat(',');
  data.concat(sound);
  data.concat(',');
  data.concat(brightness);
  data.concat(',');
  data.concat(dust);
  data.concat(',');
  data.concat(flammableGas);
  sendData(data);
}

/**
 * 采集灰尘
 */
String readDust(){
  float density, voltage;
  int   adcvalue;

  digitalWrite(DUST_LED, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(DUST_PIN);
  digitalWrite(DUST_LED, LOW);
  
  // adcvalue = DustFilter(adcvalue);
  // covert voltage (mv)
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;
  //voltage to density
  if (voltage >= NO_DUST_VOLTAGE) {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  } else
    density = 0;

  String res = "dustDensity=";
  res.concat(density);
  return res;
}

/**
 * 返回近10次采集值的平均值
 */
int DustFilter(int m) {
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;

  if (flag_first == 0) {
    flag_first = 1;

    for (i = 0, sum = 0; i < _buff_max; i++) {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  } else {
    sum -= _buff[0];
    for (i = 0; i < (_buff_max - 1); i++) {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];

    i = sum / 10.0;
    return i;
  }
}

/**
 * 采集可燃气体
 */
String readFlammableGas(){
  int gasValue=analogRead(GAS_ANALOG_PIN);
  bool gasLeakage = false;
  if (digitalRead(GAS_DIGITAL_PIN) == LOW) {
    gasLeakage = true;
  }
  String res = "gasValue=";
  res.concat(gasValue);
  return res;
}

String readBrightness(){
  return "brightness=5000";
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
