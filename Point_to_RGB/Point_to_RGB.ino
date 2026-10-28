#include <WiFiS3.h>   // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>     // ไลบรารี arduino-mqtt

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-light-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น
const char* TOPIC_LIGHT = "68070129/light"; // topic ที่ publish และ subscribe

const int POT_PIN = A0;     // ขากลางของ pot ต่อ A0
const int R_PIN = 3;        // ขาแดงต่อ D3
const int G_PIN = 5;        // ขาเขียวต่อ D5
const int B_PIN = 6;        // ขาน้ำเงินต่อ D6
const int BRIGHTNESS = 40;  // ความสว่างสูงสุด 0-255 (ต่ำไว้ไม่แสบตา)
const bool COMMON_ANODE = true;  // ขายาวต่อ 5V = true / ต่อ GND = false

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)
unsigned long lastSend = 0; // เวลาที่ส่งครั้งล่าสุด

/*
  ===== ตารางสีสำหรับ setRGB(r, g, b)  (แต่ละค่า 0-255) =====
  แดง            setRGB(255, 0,   0)
  ส้ม            setRGB(255, 90,  0)
  เหลือง         setRGB(255, 200, 0)
  เขียว          setRGB(0,   255, 0)
  เขียวอมฟ้า     setRGB(0,   255, 120)
  ฟ้า cyan       setRGB(0,   255, 255)
  น้ำเงิน        setRGB(0,   0,   255)
  ม่วง           setRGB(160, 0,   255)
  ชมพู           setRGB(255, 40,  120)
  ขาว            setRGB(255, 255, 255)
  ดับ            setRGB(0,   0,   0)
*/
void setRGB(int r, int g, int b) {                       // ตั้งสี (ค่า 0-255 ของแต่ละสี)
  r = r * BRIGHTNESS / 255;                              // สเกลตามความสว่าง
  g = g * BRIGHTNESS / 255;
  b = b * BRIGHTNESS / 255;
  if (COMMON_ANODE) {                                    // common anode: ค่าต่ำ = ติด
    r = 255 - r; g = 255 - g; b = 255 - b;               // กลับค่า (255 = ดับ)
  }
  analogWrite(R_PIN, r);                                 // ส่งค่าไปขาแดง
  analogWrite(G_PIN, g);                                 // ส่งค่าไปขาเขียว
  analogWrite(B_PIN, b);                                 // ส่งค่าไปขาน้ำเงิน
}

void messageReceived(String &topic, String &payload) {   // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print(topic);                                   // แสดง topic
  Serial.print(" = ");
  Serial.println(payload);                               // แสดงค่าที่ได้รับ
  if (topic != TOPIC_LIGHT) return;                      // สนใจเฉพาะ topic light

  int v = payload.toInt();                               // แปลงข้อความเป็นเลข (0-1023)

  // ----- เงื่อนไข: ค่าเท่านี้ -> สีนี้ (ปรับช่วงและสีตามโจทย์) -----
  if (v >= 800) {                                        // 800-1023
    setRGB(255, 0, 0);                                   // แดง
  } else if (v >= 500) {                                 // 500-799
    setRGB(255, 90, 0);                                  // ส้ม
  } else if (v >= 200) {                                 // 200-499
    setRGB(0, 255, 0);                                   // เขียว
  } else {                                               // 0-199
    setRGB(0, 0, 255);                                   // น้ำเงิน
  }

  /*
    ===== ตัวอย่างรูปแบบเงื่อนไขอื่น (ไว้ดูเพิ่ม) =====
    1) ค่าเท่ากับเลขตรงๆ:        if (v == 25) setRGB(255, 255, 0);       // เท่ากับ 25 = เหลือง
    2) มากกว่า / น้อยกว่า:       if (v > 60) setRGB(255, 0, 0);          // มากกว่า 60 = แดง
                                  else if (v < 30) setRGB(0, 255, 120);   // น้อยกว่า 30 = เขียวอมฟ้า
    3) อยู่ในช่วง (รวมปลายทาง):  if (v >= 30 && v <= 60) setRGB(255, 90, 0);   // 30-60 = ส้ม
    4) ข้อความ (ไม่ใช่ตัวเลข):   if (payload == "on") setRGB(255, 255, 255);   // "on" = ขาว
                                  else if (payload == "off") setRGB(0, 0, 0);   // "off" = ดับ
    5) ค่าทศนิยม:                float f = payload.toFloat();                  // แปลงเป็นทศนิยม
                                  if (f >= 28.5) setRGB(255, 0, 0);            // ตั้งแต่ 28.5 ขึ้นไป = แดง
    6) หลายเงื่อนไขพร้อมกัน:     if (v > 20 && v < 40) ...                    // และ (&&)
                                  if (v < 10 || v > 50) ...                    // หรือ (||)
    หมายเหตุ: ใช้ else if ต่อกันเพื่อให้ทำแค่เงื่อนไขเดียว และเรียงจากเงื่อนไขเฉพาะไปกว้าง
  */
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
  pinMode(R_PIN, OUTPUT);                      // ขา RGB เป็นเอาต์พุต
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  setRGB(0, 0, 0);                             // เริ่มต้นให้ดับ
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