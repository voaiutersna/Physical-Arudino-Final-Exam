#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-venus-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_VENUS = "68070129/venus"; // topic ที่ต้อง subscribe

const int R_PIN = 3;        // ขาแดงต่อ D3
const int G_PIN = 5;        // ขาเขียวต่อ D5
const int B_PIN = 6;        // ขาน้ำเงินต่อ D6
const int BRIGHTNESS = 40;  // ความสว่าง 0-255 (ต่ำไว้ไม่แสบตา)

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)

void setColor(bool r, bool g, bool b) {                  // ตั้งสีแบบ common anode
  analogWrite(R_PIN, r ? 255 - BRIGHTNESS : 255);        // ค่าต่ำ = ติด, 255 = ดับ
  analogWrite(G_PIN, g ? 255 - BRIGHTNESS : 255);        // เขียว
  analogWrite(B_PIN, b ? 255 - BRIGHTNESS : 255);        // น้ำเงิน
}

void messageReceived(String &topic, String &payload) {   // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print(topic);                                   // แสดง topic
  Serial.print(" = ");
  Serial.println(payload);                               // แสดงค่าที่ได้รับ
  if (topic != TOPIC_VENUS) return;                      // สนใจเฉพาะ topic venus
  int v = payload.toInt();                               // แปลงข้อความเป็นเลข
  if (v >= 36 && v <= 50) setColor(true, false, false);        // 36-50 = แดง
  else if (v >= 26 && v <= 35) setColor(false, false, true);   // 26-35 = น้ำเงิน
  else if (v >= 10 && v <= 25) setColor(false, true, false);   // 10-25 = เขียว
  else setColor(false, false, false);                          // นอกช่วง = ดับ
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
  client.subscribe(TOPIC_VENUS);               // subscribe ทุกครั้งที่ต่อใหม่
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  pinMode(R_PIN, OUTPUT);                      // ขาแดงเป็นเอาต์พุต
  pinMode(G_PIN, OUTPUT);                      // ขาเขียวเป็นเอาต์พุต
  pinMode(B_PIN, OUTPUT);                      // ขาน้ำเงินเป็นเอาต์พุต
  setColor(false, false, false);               // เริ่มต้นให้ดับ
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  client.onMessage(messageReceived);           // ลงทะเบียนฟังก์ชันรับข้อความ
  connectMQTT();                               // ต่อ MQTT แล้ว subscribe
}

void loop() {
  client.loop();                               // รับข้อความ ต้องเรียกทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่
}