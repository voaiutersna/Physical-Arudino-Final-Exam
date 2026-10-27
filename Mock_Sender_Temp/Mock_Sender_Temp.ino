#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-temp-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_TEMP = "68070129/temp";  // topic ที่ต้อง publish

const int TEMP_PIN = A1;    // ขากลางของ MCP9700 ต่อ A1

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

float readTemperatureC() {                          // อ่านอุณหภูมิจาก MCP9700 เป็น C
  float v = analogRead(TEMP_PIN) * 5.0 / 1023.0;    // แปลงค่า ADC เป็นโวลต์
  return (v - 0.5) / 0.01;                          // offset 0.5 V, ขึ้น 10 mV ต่อ 1 C
}

void connectWiFi() {                           // เชื่อมต่อ Wi-Fi
  Serial.print("WiFi");                        // แสดงว่ากำลังต่อ
  while (WiFi.status() != WL_CONNECTED) {      // วนจนกว่าจะต่อได้
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);      // ลองต่อ
    Serial.print(".");                         // จุดแสดงว่ายังพยายามอยู่
    delay(3000);                               // รอ 3 วินาที
  }
  Serial.println(" OK");                       // ต่อสำเร็จ
}

void connectMQTT() {                           // เชื่อมต่อ MQTT
  Serial.print("MQTT");                        // แสดงว่ากำลังต่อ
  while (!client.connect(CLIENT_ID)) {         // วนจนกว่าจะต่อได้
    Serial.print(".");                         // จุดแสดงว่ายังพยายามอยู่
    delay(1000);                               // รอ 1 วินาที
  }
  Serial.println(" OK");                       // ต่อสำเร็จ
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  connectMQTT();                               // ต่อ MQTT
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  if (millis() - lastSend >= 2000) {           // ส่งทุก 2 วินาที
    lastSend = millis();                       // จำเวลาที่ส่ง
    float t = readTemperatureC();              // อ่านอุณหภูมิ
    client.publish(TOPIC_TEMP, String(t, 1));  // ส่งทศนิยม 1 ตำแหน่ง
    Serial.print("temp = ");                   // แสดงค่าที่ส่ง
    Serial.println(t);
  }
}