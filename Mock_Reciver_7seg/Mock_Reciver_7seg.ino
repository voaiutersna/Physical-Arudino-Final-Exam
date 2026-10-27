#include <WiFiS3.h>                                   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>                                     // ไลบรารี MQTT

const char WIFI_SSID[]     = "iPhonePut";                // ชื่อ hotspot
const char WIFI_PASSWORD[] = "12345678a";             // รหัส hotspot

const char MQTT_BROKER_ADRRESS[] = "phycom.it.kmitl.ac.th"; // broker ของสอบ
const int  MQTT_PORT = 1883;                          // พอร์ต MQTT ของบอร์ด
const char MQTT_CLIENT_ID[] = "arduino-7seg-68070129"; // ต้องไม่ซ้ำกับโค้ดอื่น
const char TOPIC_PROB[] = "68070129/prob_stat";       // topic รับเลข 0-9

const int  segPins[7] = {1, 2, 3, 4, 5, 6, 7};        // ขา a b c d e f g ตามที่ต่อ = D1-D7
const bool COMMON_CATHODE_7SEG = true;                // true = COM ลง GND / false = COM ไป 5V
const byte digits[10][7] = {                          // ส่วนที่ติดของเลข 0-9 (a b c d e f g)
  {1,1,1,1,1,1,0}, {0,1,1,0,0,0,0}, {1,1,0,1,1,0,1}, {1,1,1,1,0,0,1}, {0,1,1,0,0,1,1},
  {1,0,1,1,0,1,1}, {1,0,1,1,1,1,1}, {1,1,1,0,0,0,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1} };

WiFiClient network;                                   // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);                               // MQTT client

void showDigit(int n) {                               // แสดงเลข 0-9 (นอกช่วง = ดับหมด)
  for (int i = 0; i < 7; i++) {                       // วนทั้ง 7 ส่วน
    bool on = (n >= 0 && n <= 9) && digits[n][i];     // ส่วนนี้ต้องติดไหม
    digitalWrite(segPins[i], COMMON_CATHODE_7SEG ? on : !on); // common anode กลับค่า
  }
}

void messageReceived(String &topic, String &payload) {// ทำงานเมื่อมีข้อความเข้า
  Serial.print("[RX] ");                              // แสดงใน Serial
  Serial.print(topic);                                // ชื่อ topic
  Serial.print(" = ");                                // ตัวคั่น
  Serial.println(payload);                            // ข้อความ
  if (topic == TOPIC_PROB) {                          // ถ้ามาจาก topic prob_stat
    showDigit(payload.toInt());                       // แสดงเลขที่รับ
  }
}

void connectWiFi() {                                  // ต่อ Wi-Fi
  Serial.print("Connecting WiFi");                    // แจ้งสถานะ
  while (WiFi.status() != WL_CONNECTED) {             // วนจนต่อติด
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             // ลองต่อ
    delay(3000);                                      // รอ 3 วินาที
    Serial.print(".");                                // พิมพ์จุด
  }
  Serial.println("\nWiFi OK");                        // ต่อติดแล้ว
}

void connectMQTT() {                                  // ต่อ broker
  Serial.print("Connecting MQTT");                    // แจ้งสถานะ
  while (!client.connect(MQTT_CLIENT_ID, "", "")) {   // วนจนต่อติด
    Serial.print(".");                                // พิมพ์จุด
    delay(1000);                                      // รอ 1 วินาที
  }
  Serial.println("\nMQTT connected");                 // ต่อติดแล้ว
  client.subscribe(TOPIC_PROB);                       // สมัครรับ topic prob_stat
}

void setup() {                                        // ทำครั้งเดียวตอนเริ่ม
  Serial.begin(115200);                               // เปิด Serial
  for (int i = 0; i < 7; i++) pinMode(segPins[i], OUTPUT); // ขา 7-segment เป็น output
  showDigit(-1);                                      // เริ่มดับทุกส่วน
  connectWiFi();                                      // ต่อ Wi-Fi
  client.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network); // ตั้งค่า broker
  client.onMessage(messageReceived);                  // กำหนดฟังก์ชันรับข้อความ
  connectMQTT();                                      // ต่อ broker และ subscribe
}

void loop() {                                         // ทำซ้ำตลอด
  client.loop();                                      // รับข้อความ + keep-alive (ต้องเรียกทุกรอบ)
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // Wi-Fi หลุด ต่อใหม่
  if (!client.connected()) connectMQTT();             // broker หลุด ต่อใหม่
}