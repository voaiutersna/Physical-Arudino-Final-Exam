#include <WiFiS3.h>                                   // ไลบรารี Wi-Fi ของบอร์ด UNO R4 WiFi
#include <MQTT.h>                                     // ไลบรารี MQTT

const char WIFI_SSID[]     = "iPhone";                // ชื่อ hotspot มือถือ
const char WIFI_PASSWORD[] = "12345678a";             // รหัสผ่าน hotspot

const char TOPIC_LIGHT[] = "68070129/light";          // topic ที่จะส่งค่าตัวต้านทานเข้าไป
const char TOPIC_VENUS[] = "68070129/venus";          // topic ที่จะรับอุณหภูมิ Venus
const char TOPIC_FOOD[] = "68070129/food";   // topic ส่ง "off" เมื่อระยะ > 20 cm
const int  POT_PIN = A0;                              // ขา analog ที่ต่อขากลางของตัวต้านทาน
unsigned long lastPublish = 0;                        // เวลาที่ส่งครั้งล่าสุด

const char MQTT_BROKER_ADRRESS[] = "phycom.it.kmitl.ac.th"; // broker ของสอบ
const int  MQTT_PORT = 1883;                          // ฝั่งบอร์ดใช้ 1883 (8884 คือของหน้าเว็บ)
const char MQTT_CLIENT_ID[] = "arduino-68070129";     // ชื่อ client ต้องไม่ซ้ำกับใคร
const char MQTT_USERNAME[]  = "";                     // ไม่ใช้ username
const char MQTT_PASSWORD[]  = "";                     // ไม่ใช้ password

const int  R_PIN = 3;                                 // ขาสีแดงของ Color LED
const int  G_PIN = 5;                                 // ขาสีเขียวของ Color LED
const int  B_PIN = 6;                                 // ขาสีน้ำเงินของ Color LED
const bool COMMON_ANODE = true;                       // true = ขายาวต่อ 5V (LED ของเราเป็นแบบนี้) / false ต่อลง GND
const int BRIGHTNESS = 40;                       // ความสว่าง 0-255 (น้อย = สลัว ลองปรับได้ เช่น 20, 40, 80)

const int TRIG_PIN = 7;                      // ขา TRIG ของ ultrasonic
const int ECHO_PIN = 8;                      // ขา ECHO ของ ultrasonic

const char TOPIC_TEMP[] = "68070129/temp";       // topic ส่งอุณหภูมิ
const int   TEMP_PIN = A1;                       // ขาอ่านเทอร์มิสเตอร์
const float R_FIXED  = 10000.0;                  // ตัวต้านทานคงที่ 10k
const float R0       = 10000.0;                  // ความต้านทานของ NTC ที่ 25 C
const float BETA     = 3950.0;                   // ค่า Beta ของ NTC
const float T0       = 298.15;                   // 25 C เป็นเคลวิน

WiFiClient network;                                   // object สำหรับเชื่อมต่อเครือข่าย
MQTTClient client(256);                               // MQTT client มี buffer 256 bytes

void connectWiFi() {                                  // ฟังก์ชันเชื่อมต่อ Wi-Fi
  Serial.print("Connecting WiFi");                    // แสดงว่ากำลังต่อ Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {             // วนจนกว่าจะต่อสำเร็จ
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);             // สั่งต่อ Wi-Fi
    delay(3000);                                      // รอ 3 วินาที
    Serial.print(".");                                // พิมพ์จุดแสดงว่ายังพยายามอยู่
  }                                                   // จบ while
  Serial.print("\nWiFi OK, IP = ");                   // ต่อสำเร็จ
  Serial.println(WiFi.localIP());                     // แสดง IP ที่ได้
}                                                     // จบ connectWiFi

void connectMQTT() {                                  // ฟังก์ชันเชื่อมต่อ broker
  Serial.print("Connecting MQTT");                    // แสดงว่ากำลังต่อ broker
  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) { // ลองต่อจนกว่าจะสำเร็จ
    Serial.print(".");                                // พิมพ์จุดทุกครั้งที่ล้มเหลว
    delay(1000);                                      // รอ 1 วินาทีแล้วลองใหม่
  }                                                   // จบ while
  Serial.println("\nMQTT connected");                 // ต่อ broker สำเร็จ
  client.subscribe(TOPIC_VENUS);                      // สมัครรับ topic venus (ทำหลังต่อ broker ทุกครั้ง)
}                                                     // จบ connectMQTT

float readDistanceCm() {                         // วัดระยะ คืนค่า cm
  digitalWrite(TRIG_PIN, LOW);                   // เริ่มที่ต่ำ
  delayMicroseconds(2);                          // รอ 2 us
  digitalWrite(TRIG_PIN, HIGH);                  // ยิงพัลส์
  delayMicroseconds(10);                         // ค้าง 10 us
  digitalWrite(TRIG_PIN, LOW);                   // หยุดยิง
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);// วัดเวลาสะท้อน (รอไม่เกิน 30 ms)
  if (duration == 0) return 999;                 // ไม่มีสะท้อน = ไกลมาก
  return duration * 0.034 / 2;                   // แปลงเวลาเป็น cm
}

float readTemperatureC() {                       // คืนอุณหภูมิ C (NAN ถ้าอ่านไม่ได้)
  long sum = 0;                                  // ตัวสะสมผลรวม
  for (int i = 0; i < 16; i++) {                 // อ่าน 16 ครั้ง
    sum += analogRead(TEMP_PIN);                 // บวกค่าที่อ่านได้
    delay(2);                                    // เว้นช่วงเล็กน้อย
  }
  float adc = sum / 16.0;                        // ค่าเฉลี่ย
  if (adc <= 0 || adc >= 1023) return NAN;       // สายหลุดหรือ short
  float rTh = R_FIXED * (1023.0 / adc - 1.0);    // ความต้านทานเทอร์มิสเตอร์
  float tK = 1.0 / (1.0 / T0 + log(rTh / R0) / BETA); // สมการ Beta เป็นเคลวิน
  return tK - 273.15;                            // แปลงเป็นเซลเซียส
}

void setup() {                                        // ทำงานครั้งเดียวตอนเริ่ม
  Serial.begin(115200);                               // เปิด Serial Monitor
  pinMode(R_PIN, OUTPUT);                             // ขา LED แดงเป็น output
  pinMode(G_PIN, OUTPUT);                             // ขา LED เขียวเป็น output
  pinMode(B_PIN, OUTPUT);                             // ขา LED น้ำเงินเป็น output
  pinMode(TRIG_PIN, OUTPUT);                     // TRIG เป็น output
  pinMode(ECHO_PIN, INPUT);                      // ECHO เป็น input
  ledSelfTest();                                      // กระพริบ แดง -> เขียว -> น้ำเงิน เพื่อเช็กวงจร
  connectWiFi();                                      // ต่อ Wi-Fi
  client.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network); // ตั้งค่า broker/port/เครือข่าย
  client.onMessage(messageReceived);                  // ถ้ามีข้อความเข้า ให้เรียกฟังก์ชัน messageReceived
  connectMQTT();                                      // ต่อ broker
}                                                     // จบ setup

void messageReceived(String &topic, String &payload) { // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print("[RX] ");                              // แสดงว่าเป็นข้อความที่รับเข้า
  Serial.print(topic);                                // แสดงชื่อ topic
  Serial.print(" = ");                                // ตัวคั่น
  Serial.println(payload);                            // แสดงเนื้อหาข้อความ

  if (topic == TOPIC_VENUS) {                         // ถ้าข้อความมาจาก topic venus
    float t = payload.toFloat();                      // แปลงข้อความเป็นตัวเลขทศนิยม
    if (t >= 36 && t <= 50)      setLed(true, false, false);  // 36-50 -> แดง
    else if (t >= 26 && t < 36)  setLed(false, false, true);  // 26-35 -> ฟ้า (ใช้ขาน้ำเงิน)
    else if (t >= 10 && t < 26)  setLed(false, true, false);  // 10-25 -> เขียว
    else                         setLed(false, false, false); // นอกช่วง -> ดับ
  }                                                   // จบ if topic
}                                                     // จบ messageReceived

void writeColor(int pin, bool state) {           // ตั้งค่าความสว่างของสี 1 ขา
  int level = state ? BRIGHTNESS : 0;            // ถ้าสีนี้ต้องติด ใช้ BRIGHTNESS ถ้าไม่ติดใช้ 0
  if (COMMON_ANODE) level = 255 - level;         // LED แบบ common anode ต้องกลับค่า (ค่าต่ำ = สว่าง)
  analogWrite(pin, level);                       // ส่งสัญญาณ PWM ออกขา
}

void setLed(bool r, bool g, bool b) {            // ฟังก์ชันเดิม เรียกใช้ที่อื่นได้เหมือนเดิม
  writeColor(R_PIN, r);                          // สีแดง
  writeColor(G_PIN, g);                          // สีเขียว
  writeColor(B_PIN, b);                          // สีน้ำเงิน
}                                                   // จบ setLed

void ledSelfTest() {                                  // ทดสอบ LED ตอนเปิดเครื่อง (ลบทิ้งได้ภายหลัง)
  setLed(true, false, false);  delay(500);            // แดง 0.5 วินาที
  setLed(false, true, false);  delay(500);            // เขียว 0.5 วินาที
  setLed(false, false, true);  delay(500);            // น้ำเงิน 0.5 วินาที
  setLed(false, false, false);                        // ดับ
}                                                     // จบ ledSelfTest

void loop() {                                         // ทำงานวนซ้ำตลอด
  client.loop();                                      // รักษาการเชื่อมต่อ + รับข้อความ (ห้ามลืม)
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // Wi-Fi หลุดให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // broker หลุดให้ต่อใหม่

  if (millis() - lastPublish >= 2000) {               // ถ้าผ่านไปครบ 2 วินาทีแล้ว
    lastPublish = millis();                           // จดเวลาไว้
    int light = analogRead(POT_PIN);                  // อ่านค่าตัวต้านทาน ได้ 0-1023
    //int light = random(0, 1024);                      // test
    client.publish(TOPIC_LIGHT, String(light));       // ส่งค่า (แปลงเป็นข้อความก่อน) ขึ้น topic light
    Serial.print("[TX] light = ");                    // แสดงว่าส่งอะไรไป
    Serial.println(light);                            // แสดงค่าที่ส่ง
    
    float dist = readDistanceCm();               // วัดระยะ
    Serial.print("[DIST] ");                     // แสดงระยะใน Serial
    Serial.print(dist);                          // ค่าระยะ
    Serial.println(" cm");                       // หน่วย
    if (dist > 20) {                             // ถ้าไกลกว่า 20 cm
      client.publish(TOPIC_FOOD, "off");         // ส่ง off ไป 68070129/food
      Serial.println("[TX] food = off");         // แสดงใน Serial
    }

    float temp = readTemperatureC();             // อ่านอุณหภูมิ
    if (!isnan(temp)) {                          // ถ้าอ่านได้ปกติ
      client.publish(TOPIC_TEMP, String(temp, 1)); // ส่งไป 68070129/temp ทศนิยม 1 ตำแหน่ง
      Serial.print("[TX] temp = ");              // แสดงใน Serial
      Serial.println(temp, 1);                   // ค่าอุณหภูมิ
    }
  }                                                   // จบ if
  
}                                                     // จบ loop