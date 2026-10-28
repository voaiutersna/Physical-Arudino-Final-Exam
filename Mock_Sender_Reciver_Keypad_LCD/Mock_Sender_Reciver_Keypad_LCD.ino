#include <WiFiS3.h>            // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>              // ไลบรารี arduino-mqtt
#include <Wire.h>              // ไลบรารี I2C
#include <LiquidCrystal_I2C.h> // ไลบรารีจอ LCD แบบ I2C

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-keypad-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_KEYPAD = "68070129/keypad";       // topic ที่ publish

LiquidCrystal_I2C lcd(0x27, 16, 2);        // LCD 16x2 ที่อยู่ 0x27 (ถ้าไม่ขึ้นลอง 0x3F)


const int rowPins[4] = {1, 2, 3, 4};       // ขา keypad 1-4 (แถว) ต่อ D1-D4
const int colPins[4] = {5, 6, 7, 8};       // ขา keypad 5-8 (คอลัมน์) ต่อ D5-D8
const char keys[4][4] = {                  // ตารางตัวอักษรของปุ่ม
  {'1', '2', '3', 'A'},                    // แถวที่ 1
  {'4', '5', '6', 'B'},                    // แถวที่ 2
  {'7', '8', '9', 'C'},                    // แถวที่ 3
  {'*', '0', '#', 'D'}                     // แถวที่ 4
};

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
char lastKey = 0;           // ปุ่มที่กดค้างอยู่ (0 = ไม่มี)
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


char readKey() {                                       // สแกนหาปุ่มที่ถูกกด
  for (int r = 0; r < 4; r++) {                        // ไล่ทีละแถว
    digitalWrite(rowPins[r], LOW);                     // ดึงแถวนี้เป็น LOW
    for (int c = 0; c < 4; c++) {                      // ไล่ทีละคอลัมน์
      if (digitalRead(colPins[c]) == LOW) {            // คอลัมน์เป็น LOW = ปุ่มถูกกด
        digitalWrite(rowPins[r], HIGH);                // คืนแถวเป็น HIGH
        return keys[r][c];                             // คืนตัวอักษรของปุ่ม
      }
    }
    digitalWrite(rowPins[r], HIGH);                    // คืนแถวเป็น HIGH
  }
  return 0;                                            // ไม่มีปุ่มถูกกด
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
  for (int r = 0; r < 4; r++) {                // ตั้งค่าขาแถว
    pinMode(rowPins[r], OUTPUT);               // แถวเป็นเอาต์พุต
    digitalWrite(rowPins[r], HIGH);            // เริ่มต้นเป็น HIGH
  }
  for (int c = 0; c < 4; c++) {                // ตั้งค่าขาคอลัมน์
    pinMode(colPins[c], INPUT_PULLUP);         // คอลัมน์เป็นอินพุตดึงขึ้น (กันค่ามั่ว)
  }
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  client.onMessage(messageReceived);           // ลงทะเบียนฟังก์ชันรับข้อความ (บรรทัดที่ขาด)
  connectMQTT();                               // ต่อ MQTT
  lcd.clear();                                 // ล้างจอ
  lcd.print("Ready");                          // แจ้งว่าพร้อมแล้ว
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  char k = readKey();                          // อ่านปุ่มตอนนี้
  if (k != 0 && k != lastKey) {                // มีปุ่มใหม่ที่เพิ่งถูกกด
    client.publish(TOPIC_KEYPAD, String(k));   // ส่งตัวอักษรขึ้น broker
    Serial.print("key = ");                    // แสดงค่าที่ส่ง
    Serial.println(k);
    delay(30);                                 // หน่วงสั้นๆ กันปุ่มเด้ง
  }
  lastKey = k;                                 // จำสถานะ (ปล่อยแล้วเป็น 0 กดซ้ำได้)
}

/*
  ========== เช็กลิสต์แก้ปัญหา Arduino + MQTT ==========

  [ลำดับเช็กเร็ว]
  1) Serial (115200) ขึ้น WiFi OK / MQTT OK ไหม
  2) ค่าที่ส่ง/รับขึ้นใน Serial ไหม
  3) ขึ้นที่หน้าเว็บ phycom หรือ HiveMQ ไหม
  -> ดูว่าติดขั้นไหน แล้วแก้เฉพาะขั้นนั้น

  [รับข้อความ (subscribe) ไม่ทำงาน]
  - ลืม client.onMessage(messageReceived) ใน setup()
  - ลืม client.subscribe(...) หรือไม่ได้ใส่ใน connectMQTT() (ต่อใหม่แล้วต้อง subscribe ซ้ำ)
  - ลืม client.loop() ใน loop()
  - มี delay() ยาวใน loop() ข้อความจะมาช้าหรือหลุด
  - ชื่อ topic ไม่ตรง แม้ตัวเดียว (Keypad กับ keypad) -> ดูที่ Serial ว่า topic ที่ได้รับคืออะไร

  [ส่งข้อความ (publish) ไม่ขึ้น]
  - ไม่ขึ้น MQTT OK -> เช็ก Wi-Fi: ชื่อ, รหัส, เปิดฮอตสปอตค้างไว้
  - client ID ซ้ำกับอุปกรณ์อื่น (ตัวใหม่เตะตัวเก่าหลุด)
  - ตัวพิมพ์ใหญ่/เล็กของข้อความต้องตรงโจทย์ (on/On, off/Off)
  - แทน student_id ด้วย 68070129 ทุก topic

  [ค่า sensor เพี้ยน]
  - ใช้สูตรผิดตัว: MCP9700 -> (v - 0.5) / 0.01 | เทอร์มิสเตอร์ -> สมการ Beta
  - พิมพ์ค่า adc ออก Serial ดู (ค้าง 0 หรือ 1023 = สายหลุด/ลัดวงจร)
  - 5V/GND ต้องร่วมกัน และ rail breadboard บางรุ่นขาดกลาง
  - ultrasonic ได้ 999 ตลอด: TRIG/ECHO สลับ หรือ VCC ไม่ต่อ 5V

  [output ไม่ติด]
  - RGB common anode: ขายาว -> 5V และต้องกลับค่า (255 - ...)
  - LCD ไม่ขึ้น: ลอง 0x3F | สว่างแต่ไม่มีตัวอักษร: หมุน contrast | SDA -> A4, SCL -> A5
  - 7-seg: ตัวเลขผิด -> เช็กลำดับขา a-g ให้ตรงกับตาราง digits
  - 7-seg: common cathode ใช้ COMMON_CATHODE_7SEG = true, common anode = false

  [อัปโหลด/เครื่องมือ]
  - No device found: ถอด/เสียบ USB ใหม่, เลือกพอร์ตใหม่, กด RESET สองครั้งติดกัน
  - Serial Monitor ว่าง: ตั้ง baud 115200 แล้วกด RESET
  - expected '}' / redefinition: โค้ดวางซ้ำหรือขาดวงเล็บ -> เลือกทั้งหมดแล้ววางใหม่ชุดเดียว
  - คำเตือน claims to run on avr (LiquidCrystal I2C) ไม่ใช่ error ข้ามได้

  [ความปลอดภัย]
  - เช็ก 5V/GND ก่อนเสียบ USB ทุกครั้ง และถอด USB ก่อนเปลี่ยนสาย
  - ลบรหัสฮอตสปอตออกก่อนอัปโหลดขึ้น Onlearn
*/