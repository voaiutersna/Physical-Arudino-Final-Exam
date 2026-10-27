#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-keypad-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_KEYPAD = "68070129/keypad";       // topic ที่ publish

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
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  for (int r = 0; r < 4; r++) {                // ตั้งค่าขาแถว
    pinMode(rowPins[r], OUTPUT);               // แถวเป็นเอาต์พุต
    digitalWrite(rowPins[r], HIGH);            // เริ่มต้นเป็น HIGH
  }
  for (int c = 0; c < 4; c++) {                // ตั้งค่าขาคอลัมน์
    pinMode(colPins[c], INPUT_PULLUP);         // คอลัมน์เป็นอินพุตดึงขึ้น (กันค่ามั่ว)
  }
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  connectMQTT();                               // ต่อ MQTT
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