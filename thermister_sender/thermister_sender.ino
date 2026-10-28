/*
  ===== วิธีใช้ (อ่านก่อน) =====
  อ่านอุณหภูมิจากเทอร์มิสเตอร์ แล้วส่งเป็นตัวเลของศาเซลเซียส (เช่น 28.5) ไปที่ topic temp
  แก้ให้ตรงโจทย์: TOPIC_TEMP (แทน student_id ด้วย 68070129) และ SEND_INTERVAL
  ถ้าค่าเพี้ยนจากจริง: ปรับ TEMP_OFFSET (บวก/ลบ) เช่น -2.0 = ลดลง 2 องศา
  ถ้าค่าผิดมากผิดปกติ: เปิด Serial Monitor ดูค่า adc (ควรอยู่ราว 500-600 ที่อุณหภูมิห้อง)
*/

#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-temp-68070129";  // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_TEMP = "68070129/temp";  // topic ที่ต้อง publish (แก้ตามโจทย์)

const int TEMP_PIN = A1;                   // จุดกลางของวงจรแบ่งแรงดันต่อ A1
const float R_FIXED = 10000.0;             // ตัวต้านทานคงที่ 10k
const float R0 = 10000.0;                  // ความต้านทานเทอร์มิสเตอร์ที่ 25 C
const float BETA = 3950.0;                 // ค่า Beta ของเทอร์มิสเตอร์ (ถ้าไม่รู้ใช้ 3950)
const float TEMP_OFFSET = 0.0;             // ค่าชดเชยอุณหภูมิ (บวก/ลบ) ไว้จูนให้ตรงจริง
const unsigned long SEND_INTERVAL = 2000;  // ส่งทุกกี่ ms

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

float readTemperatureC() {                       // อ่านอุณหภูมิเป็น C
  int adc = analogRead(TEMP_PIN);                // อ่านค่า 0-1023
  if (adc <= 0 || adc >= 1023) return NAN;       // ค่าสุดขั้ว = สายหลุด/ลัดวงจร
  float rt = R_FIXED * (1023.0 / adc - 1.0);     // คำนวณความต้านทานเทอร์มิสเตอร์
  float k = 1.0 / (1.0 / 298.15 + log(rt / R0) / BETA);  // สมการ Beta ได้หน่วย Kelvin
  return k - 273.15 + TEMP_OFFSET;               // แปลงเป็น C แล้วบวกค่าชดเชย
}

/*
  ===== ทางเลือก: ถ้าใช้เซนเซอร์ MCP9700 (IC 3 ขา ไม่ต้องมีตัวต้านทาน) =====
  ต่อสาย: ด้านแบนหันเข้าหาตัว ซ้าย 5V / กลาง A1 / ขวา GND แล้วแทนฟังก์ชันด้านบนด้วยอันนี้

  float readTemperatureC() {                          // อ่านอุณหภูมิจาก MCP9700 เป็น C
    float v = analogRead(TEMP_PIN) * 5.0 / 1023.0;    // แปลงค่า ADC เป็นโวลต์
    return (v - 0.5) / 0.01 + TEMP_OFFSET;            // offset 0.5 V, ขึ้น 10 mV ต่อ 1 C
  }
*/

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

  if (millis() - lastSend >= SEND_INTERVAL) {  // ถึงเวลาส่งหรือยัง
    lastSend = millis();                       // จำเวลาที่ส่ง
    float t = readTemperatureC();              // อ่านอุณหภูมิ
    if (!isnan(t)) {                           // ส่งเฉพาะเมื่อค่าใช้ได้
      client.publish(TOPIC_TEMP, String(t, 1));  // ส่งทศนิยม 1 ตำแหน่ง
      Serial.print("adc=");                    // แสดงค่า adc ไว้ดีบัก
      Serial.print(analogRead(TEMP_PIN));
      Serial.print(" temp=");                  // แสดงอุณหภูมิที่ส่ง
      Serial.println(t);
    }
  }
}