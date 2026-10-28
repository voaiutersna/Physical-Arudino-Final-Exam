#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-light-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_LIGHT = "68070129/light"; // topic ที่ publish และ subscribe

const int POT_PIN = A0;     // ขากลางของ pot ต่อ A0
const int LED_PIN = 3;      // LED ต่อ D3
const int THRESHOLD = 500;  // ค่าตั้งแต่เท่านี้ขึ้นไป = ติด
const bool LED_ACTIVE_HIGH = true;  // ขายาวต่อ D3 = true / ขายาวต่อ 5V = false

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

void setLed(bool on) {                                 // เปิด/ปิด LED
  digitalWrite(LED_PIN, (on == LED_ACTIVE_HIGH) ? HIGH : LOW);  // กลับระดับให้ถ้าต่อแบบ active low
}

void messageReceived(String &topic, String &payload) { // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print(topic);                                 // แสดง topic
  Serial.print(" = ");
  Serial.println(payload);                             // แสดงค่าที่ได้รับ
  if (topic != TOPIC_LIGHT) return;                    // สนใจเฉพาะ topic light

  int v = payload.toInt();                             // แปลงข้อความเป็นเลข (0-1023)

  if (v >= THRESHOLD) {                                // ค่ามากกว่าหรือเท่ากับเกณฑ์
    setLed(true);                                      // เปิด LED
  } else {                                             // ค่าน้อยกว่าเกณฑ์
    setLed(false);                                     // ปิด LED
  }

  // ทางเลือก: หรี่แสงด้วย PWM แทนเปิดเต็ม (D3 รองรับ analogWrite)
  //   analogWrite(LED_PIN, 40);   // ติดเบาๆ (0-255)  | ถ้า active low ใช้ 255 - 40
  // ทางเลือก: เปิดเฉพาะช่วง เช่น 200-600
  //   setLed(v >= 200 && v <= 600);
  // ทางเลือก: รับข้อความ on/off
  //   if (payload == "on") setLed(true); else if (payload == "off") setLed(false);
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
  client.subscribe(TOPIC_LIGHT);               // subscribe topic เดียวกับที่ publish
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  pinMode(LED_PIN, OUTPUT);                    // ขา LED เป็นเอาต์พุต
  setLed(false);                               // เริ่มต้นให้ดับ
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  client.onMessage(messageReceived);           // ลงทะเบียนฟังก์ชันรับข้อความ
  connectMQTT();                               // ต่อ MQTT แล้ว subscribe
}

void loop() {
  client.loop();                               // ให้ MQTT ทำงานทุกรอบ (รับข้อความด้วย)
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  if (millis() - lastSend >= 2000) {           // ส่งทุก 2 วินาที
    lastSend = millis();                       // จำเวลาที่ส่ง
    int light = analogRead(POT_PIN);           // อ่านค่า pot 0-1023
    client.publish(TOPIC_LIGHT, String(light));  // ส่งเป็นข้อความตัวเลข
    Serial.print("light = ");                  // แสดงค่าที่ส่ง
    Serial.println(light);
  }
}