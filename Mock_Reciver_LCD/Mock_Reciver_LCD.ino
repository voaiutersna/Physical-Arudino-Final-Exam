#include <WiFiS3.h>            // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>              // ไลบรารี arduino-mqtt
#include <Wire.h>              // ไลบรารี I2C
#include <LiquidCrystal_I2C.h> // ไลบรารีจอ LCD แบบ I2C

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-lcd-68070129";   // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_KEYPAD = "68070129/keypad";     // topic ที่ subscribe

LiquidCrystal_I2C lcd(0x27, 16, 2);        // LCD 16x2 ที่อยู่ 0x27 (ถ้าไม่ขึ้นลอง 0x3F)

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
String history = "";        // เก็บประวัติปุ่มที่ได้รับ (สูงสุด 16 ตัว)

void showScreen(String last) {                 // วาดข้อความบนจอ
  lcd.clear();                                 // ล้างจอ
  lcd.setCursor(0, 0);                         // ไปบรรทัดบน
  lcd.print("Key: ");                          // ข้อความหัว
  lcd.print(last);                             // ปุ่มล่าสุด
  lcd.setCursor(0, 1);                         // ไปบรรทัดล่าง
  lcd.print(history);                          // ประวัติปุ่ม
}

void messageReceived(String &topic, String &payload) {  // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print(topic);                         // แสดง topic
  Serial.print(" = ");
  Serial.println(payload);                     // แสดงค่าที่ได้รับ
  if (topic != TOPIC_KEYPAD) return;           // สนใจเฉพาะ topic keypad
  history += payload;                          // ต่อค่าใหม่เข้าประวัติ
  if (history.length() > 16) {                 // เกิน 16 ตัวอักษร
    history = history.substring(history.length() - 16);  // ตัดให้เหลือ 16 ตัวท้าย
  }
  showScreen(payload);                         // อัปเดตจอ
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
  client.subscribe(TOPIC_KEYPAD);              // subscribe ทุกครั้งที่ต่อใหม่
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  lcd.init();                                  // เริ่มต้น LCD
  lcd.backlight();                             // เปิดไฟพื้นหลัง
  lcd.setCursor(0, 0);                         // ไปบรรทัดบน
  lcd.print("Connecting...");                  // แจ้งว่ากำลังเชื่อมต่อ
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  client.onMessage(messageReceived);           // ลงทะเบียนฟังก์ชันรับข้อความ
  connectMQTT();                               // ต่อ MQTT แล้ว subscribe
  lcd.clear();                                 // ล้างจอ
  lcd.print("Ready");                          // แจ้งว่าพร้อมแล้ว
}

void loop() {
  client.loop();                               // รับข้อความ ต้องเรียกทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่
}