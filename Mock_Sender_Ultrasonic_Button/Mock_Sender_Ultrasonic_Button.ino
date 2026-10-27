#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";          // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-door-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_DOOR = "68070129/door";  // topic ที่ต้อง publish

const int TRIG_PIN = 8;     // ขา ECHO ของ ultrasonic
const int ECHO_PIN = 7;     // ขา TRIG ของ ultrasonic
const int BUTTON_PIN = A2;  // ปุ่มต่อ A2 อีกขาลง GND

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

float readDistanceCm() {                       // อ่านระยะเป็น cm
  digitalWrite(TRIG_PIN, LOW);                 // เคลียร์ TRIG ให้เป็น LOW
  delayMicroseconds(2);                        // รอ 2 ไมโครวินาที
  digitalWrite(TRIG_PIN, HIGH);                // ส่งพัลส์ยิงเสียง
  delayMicroseconds(10);                       // นาน 10 ไมโครวินาที
  digitalWrite(TRIG_PIN, LOW);                 // หยุดพัลส์
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);   // วัดเวลาเสียงสะท้อน (timeout 30 ms)
  if (dur == 0) return 999;                    // ไม่มีเสียงสะท้อน = ไกลมาก
  return dur * 0.034 / 2;                      // แปลงเวลาเป็นระยะ cm
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
  pinMode(TRIG_PIN, OUTPUT);                   // TRIG เป็นเอาต์พุต
  pinMode(ECHO_PIN, INPUT);                    // ECHO เป็นอินพุต
  pinMode(BUTTON_PIN, INPUT_PULLUP);           // ปุ่ม: ไม่กด = HIGH, กด = LOW
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  connectMQTT();                               // ต่อ MQTT
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  if (millis() - lastSend >= 500) {            // ส่งทุก 500 ms
    lastSend = millis();                       // จำเวลาที่ส่ง
    float d = readDistanceCm();                // อ่านระยะ
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);  // กดปุ่มอยู่ไหม
    const char* msg = (d >= 20 && pressed) ? "On" : "Off";  // ระยะ >= 20 และกด = On
    client.publish(TOPIC_DOOR, msg);           // ส่งไป topic door
    Serial.print("dist=");                     // แสดงระยะ
    Serial.print(d);
    Serial.print(" pressed=");                 // แสดงสถานะปุ่ม
    Serial.print(pressed);
    Serial.print(" -> ");                      // แสดงค่าที่ส่ง
    Serial.println(msg);
  }
}