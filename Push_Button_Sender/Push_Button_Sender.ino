/*
  ===== วิธีใช้ (อ่านก่อน) =====
  โค้ดนี้ส่งข้อความ (string) ขึ้น broker ตามสถานะปุ่ม
    - กดปุ่มอยู่   -> ส่งข้อความ MSG_PRESSED   (ตั้งไว้ "On")
    - ไม่ได้กดปุ่ม -> ส่งข้อความ MSG_RELEASED  (ตั้งไว้ "Off")
  ต้องแก้ 3 จุดให้ตรงกับโจทย์:
    1) TOPIC_BUTTON  = ชื่อ topic ที่โจทย์กำหนด (แทน student_id ด้วย 68070129)
    2) MSG_PRESSED   = ข้อความตอนกด     (ตัวพิมพ์ใหญ่/เล็กต้องตรงโจทย์)
    3) MSG_RELEASED  = ข้อความตอนไม่กด  (ตัวพิมพ์ใหญ่/เล็กต้องตรงโจทย์)
  ปรับความถี่การส่งที่ SEND_INTERVAL (หน่วย ms)
  ถ้าโจทย์ให้ส่งเฉพาะตอนสถานะเปลี่ยน ให้ใช้ "แบบที่ 2" ในโค้ดใน loop()
  ต่อสาย: ปุ่มขา 1 -> A2, ขา 2 -> GND
*/

#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-button-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น

const char* TOPIC_BUTTON = "68070129/door"; // topic ที่ต้อง publish (แก้ตามโจทย์)
const char* MSG_PRESSED = "On";             // ข้อความตอนกดปุ่ม (แก้ตามโจทย์)
const char* MSG_RELEASED = "Off";           // ข้อความตอนไม่กดปุ่ม (แก้ตามโจทย์)

const int BUTTON_PIN = A2;                  // ปุ่มต่อขา A2 อีกขาต่อ GND
const unsigned long SEND_INTERVAL = 500;    // ส่งทุกกี่ ms

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด
bool lastPressed = false;   // สถานะปุ่มครั้งก่อน (ใช้กับแบบที่ 2)

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
  pinMode(BUTTON_PIN, INPUT_PULLUP);           // ปุ่ม: ไม่กด = HIGH, กด = LOW
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  connectMQTT();                               // ต่อ MQTT
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  bool pressed = (digitalRead(BUTTON_PIN) == LOW);    // กดอยู่ไหม (LOW = กด)

  // ===== แบบที่ 1 (ใช้อยู่): ส่งซ้ำทุก SEND_INTERVAL ms ตามสถานะปัจจุบัน =====
  if (millis() - lastSend >= SEND_INTERVAL) {  // ถึงเวลาส่งหรือยัง
    lastSend = millis();                       // จำเวลาที่ส่ง
    const char* msg = pressed ? MSG_PRESSED : MSG_RELEASED;  // กด = ข้อความกด, ไม่กด = ข้อความไม่กด
    client.publish(TOPIC_BUTTON, msg);         // ส่งข้อความขึ้น broker
    Serial.print("pressed=");                  // แสดงสถานะปุ่ม
    Serial.print(pressed);
    Serial.print(" -> ");                      // แสดงข้อความที่ส่ง
    Serial.println(msg);
  }

  // ===== แบบที่ 2: ส่งเฉพาะตอนสถานะเปลี่ยน (ลบ // แล้วลบแบบที่ 1 ทิ้ง) =====
  // if (pressed != lastPressed) {                              // สถานะเปลี่ยนจากครั้งก่อนไหม
  //   client.publish(TOPIC_BUTTON, pressed ? MSG_PRESSED : MSG_RELEASED);  // ส่งข้อความตามสถานะใหม่
  //   Serial.println(pressed ? MSG_PRESSED : MSG_RELEASED);    // แสดงข้อความที่ส่ง
  //   lastPressed = pressed;                                   // จำสถานะล่าสุด
  //   delay(30);                                               // หน่วงสั้นๆ กันปุ่มเด้ง
  // }
}