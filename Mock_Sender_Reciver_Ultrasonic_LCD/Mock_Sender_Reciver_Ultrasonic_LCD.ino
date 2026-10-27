#include <WiFiS3.h>            // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>              // ไลบรารี arduino-mqtt
#include <Wire.h>              // ไลบรารี I2C
#include <LiquidCrystal_I2C.h> // ไลบรารีจอ LCD แบบ I2C

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-taxi-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_TAXI = "68070129/taxi_model";   // topic ที่ต้อง publish

const int TRIG_PIN = 8;     // ขา TRIG ของ ultrasonic ต่อ D8
const int ECHO_PIN = 7;     // ขา ECHO ของ ultrasonic ต่อ D7

LiquidCrystal_I2C lcd(0x27, 16, 2);        // LCD 16x2 ที่อยู่ 0x27 (ถ้าไม่ขึ้นลอง 0x3F)

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด
String lastModel = "";      // รุ่นที่แสดงบน LCD ตอนนี้ (กันจอกะพริบ)

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

void showModel(String model) {                 // แสดงรุ่นรถบน LCD
  lcd.clear();                                 // ล้างจอ
  lcd.setCursor(0, 0);                         // บรรทัดที่ 1
  lcd.print("Taxi Model:");                    // ข้อความหัว
  lcd.setCursor(0, 1);                         // บรรทัดที่ 2
  lcd.print(model);                            // ชื่อรุ่น
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
  lcd.init();                                  // เริ่มต้น LCD
  lcd.backlight();                             // เปิดไฟพื้นหลัง
  lcd.print("Connecting...");                  // แจ้งว่ากำลังเชื่อมต่อ
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  connectMQTT();                               // ต่อ MQTT
  lcd.clear();                                 // ล้างจอ
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  if (millis() - lastSend >= 1000) {           // ทำทุก 1 วินาที
    lastSend = millis();                       // จำเวลาที่ทำ
    float d = readDistanceCm();                // อ่านระยะ
    String model;                              // ชื่อรุ่นที่ได้
    if (d < 15) model = "Yaris";               // น้อยกว่า 15 cm = Yaris
    else if (d <= 30) model = "Altis";         // 15-30 cm = Altis
    else model = "Accord";                     // มากกว่า 30 cm = Accord
    client.publish(TOPIC_TAXI, model);         // ส่งชื่อรุ่นขึ้น broker
    if (model != lastModel) {                  // อัปเดตจอเฉพาะเมื่อรุ่นเปลี่ยน
      showModel(model);                        // แสดงบน LCD
      lastModel = model;                       // จำรุ่นล่าสุด
    }
    Serial.print("dist=");                     // แสดงระยะและรุ่นใน Serial
    Serial.print(d);
    Serial.print(" -> ");
    Serial.println(model);
  }
}