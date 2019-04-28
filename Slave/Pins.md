## 传感器连接Nano引脚对照表

| Sensor       | Power    | Analog | Digital | Other                 |
| ------------ | -------- | ------ | ------- | --------------------- |
| DHT humiture | 3.3-6.0V |        | 2       |                       |
| dust         | 2.5-5.5V | A0     |         | LED drive: 3          |
| gas          | 2.5-5.0V | A1     | 4(INT)  |                       |
| sound        | 3.3-5.3V | A2     | 6       |                       |
| brightness   | 3.3-5.0V |        | 5(INT)  | I2C: SCL(A5), SDA(A4) |

## Nano引脚图

![nano_pins](assets/NanoPin.jpg)