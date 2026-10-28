#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-light-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_LIGHT = "68070129/light"; // topic ที่ต้อง publish

const int POT_PIN = A0;     // ขากลางของ pot ต่อ A0

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

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

    // ===== แบบที่ 1 (ใช้อยู่): ค่าดิบ 0-1023 =====
    int light = analogRead(POT_PIN);           // อ่านค่า pot 0-1023
    client.publish(TOPIC_LIGHT, String(light));  // ส่งเป็นข้อความตัวเลข
    Serial.print("light = ");                  // แสดงค่าที่ส่ง
    Serial.println(light);

    // ===== แบบที่ 2: ค่า 0-100 จำนวนเต็ม (ลบ // หน้าบรรทัดแล้วลบแบบที่ 1 ทิ้ง) =====
    // int light = map(analogRead(POT_PIN), 0, 1023, 0, 100);   // อ่านแล้วแปลงเป็น 0-100
    // client.publish(TOPIC_LIGHT, String(light));              // ส่งค่า 0-100
    // Serial.print("light = ");                                // แสดงค่าที่ส่ง
    // Serial.println(light);

    // ===== แบบที่ 3: ค่า 0-100 มีทศนิยม (เช่น 50.2) =====
    // float percent = analogRead(POT_PIN) * 100.0 / 1023.0;    // คำนวณเป็นเปอร์เซ็นต์ทศนิยม
    // client.publish(TOPIC_LIGHT, String(percent, 1));         // ส่งทศนิยม 1 ตำแหน่ง
    // Serial.print("light = ");                                // แสดงค่าที่ส่ง
    // Serial.println(percent);
  }
}