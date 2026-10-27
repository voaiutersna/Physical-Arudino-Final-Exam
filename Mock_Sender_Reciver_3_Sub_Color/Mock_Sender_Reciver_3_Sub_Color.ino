#include <WiFiS3.h>            // ไลบรารี Wi-Fi ของ UNO R4 WiFi
#include <MQTT.h>              // ไลบรารี arduino-mqtt
#include <Wire.h>              // ไลบรารี I2C
#include <LiquidCrystal_I2C.h> // ไลบรารีจอ LCD แบบ I2C

const char* WIFI_SSID = "iPhonePut";       // ชื่อ Wi-Fi (ฮอตสปอต)
const char* WIFI_PASSWORD = "12345678a";   // รหัส Wi-Fi (ลบก่อนอัปโหลด Onlearn)
const char* MQTT_HOST = "phycom.it.kmitl.ac.th";  // broker
const int MQTT_PORT = 1883;                // พอร์ตฝั่งบอร์ด
const char* CLIENT_ID = "arduino-sentosa-68070129"; // ต้องไม่ซ้ำกับอุปกรณ์อื่น

const char* TOPIC_UNI = "68070129/queue_chotiphat";  // คิว Universal (โชติพัชร์)
const char* TOPIC_SEA = "68070129/queue_lapas";      // คิว S.E.A. Aquarium (ลภัส)
const char* TOPIC_LUGE = "68070129/queue_sumet";     // คิว Skyline Luge (สุเมธ)
const char* TOPIC_CHOICE = "68070129/sentosa_choice"; // topic ที่ต้อง publish ชื่อสถานที่

const int R_PIN = 3;        // ขาแดงต่อ D3
const int G_PIN = 5;        // ขาเขียวต่อ D5
const int B_PIN = 6;        // ขาน้ำเงินต่อ D6
const int BRIGHTNESS = 40;  // ความสว่างสูงสุด 0-255 (ต่ำไว้ไม่แสบตา)

LiquidCrystal_I2C lcd(0x27, 16, 2);        // LCD 16x2 ที่อยู่ 0x27 (ถ้าไม่ขึ้นลอง 0x3F)

WiFiClient net;             // ตัวเชื่อมต่อเครือข่าย
MQTTClient client(256);     // ตัว MQTT (buffer 256 ไบต์)

float qUni = 0, qSea = 0, qLuge = 0;             // เก็บเวลารอล่าสุดของแต่ละที่ (นาที)
bool gotUni = false, gotSea = false, gotLuge = false;  // เคยได้ค่าของแต่ละที่แล้วหรือยัง
bool dirty = false;                              // มีค่าใหม่เข้ามาที่ต้องประมวลผล

/*
  ===== ตารางสีสำหรับ setRGB(r, g, b)  (แต่ละค่า 0-255, 0 = ไม่ติด, 255 = ติดเต็ม) =====
  แดง              setRGB(255, 0,   0)
  แดงเข้ม/ส้มแดง   setRGB(255, 40,  0)
  ส้ม              setRGB(255, 90,  0)    <- ใช้ในโค้ดนี้
  ส้มอ่อน/อำพัน    setRGB(255, 130, 0)
  เหลือง           setRGB(255, 200, 0)    (ถ้าเขียวมากไปจะเหลืองอมเขียว)
  เหลืองสด         setRGB(255, 255, 0)
  เหลืองเขียว      setRGB(150, 255, 0)
  เขียว            setRGB(0,   255, 0)
  เขียวอมฟ้า       setRGB(0,   255, 120)  <- ใช้ในโค้ดนี้
  ฟ้า cyan         setRGB(0,   255, 255)
  ฟ้าอ่อน/ฟ้าน้ำ   setRGB(0,   120, 255)
  น้ำเงิน          setRGB(0,   0,   255)
  ม่วง             setRGB(160, 0,   255)
  ม่วงแดง magenta  setRGB(255, 0,   255)
  ชมพู             setRGB(255, 40,  120)
  ขาว              setRGB(255, 255, 255)
  ดับ              setRGB(0,   0,   0)
  หมายเหตุ: ไฟเขียวมักสว่างกว่าแดง/น้ำเงิน ถ้าสีออกมาเหลืองเกินไป ให้ลดค่า g ลง
  สีจริงที่เห็นจะอ่อนกว่าตารางตามค่า BRIGHTNESS เพราะถูกสเกลลงทุกสี
  ===== ถ้าใช้แบบเปิด/ปิด (true/false) ผสมสีได้ 7 สี =====
  แดง (R)             true,  false, false
  เขียว (G)           false, true,  false
  น้ำเงิน (B)         false, false, true
  เหลือง/ส้มอ่อน (R+G) true,  true,  false
  เขียวอมฟ้า (G+B)    false, true,  true
  ม่วง/magenta (R+B)  true,  false, true
  ขาว (R+G+B)         true,  true,  true
*/
void setRGB(int r, int g, int b) {                       // ตั้งสีแบบ common anode (ค่า 0-255 ของแต่ละสี)
  analogWrite(R_PIN, 255 - (r * BRIGHTNESS / 255));      // สเกลตามความสว่าง แล้วกลับค่า (255 = ดับ)
  analogWrite(G_PIN, 255 - (g * BRIGHTNESS / 255));      // เขียว
  analogWrite(B_PIN, 255 - (b * BRIGHTNESS / 255));      // น้ำเงิน
}

void messageReceived(String &topic, String &payload) {   // ถูกเรียกเมื่อมีข้อความเข้า
  Serial.print(topic);                                   // แสดง topic
  Serial.print(" = ");
  Serial.println(payload);                               // แสดงค่าที่ได้รับ
  if (topic == TOPIC_UNI) { qUni = payload.toFloat(); gotUni = true; }       // เก็บค่า Universal
  else if (topic == TOPIC_SEA) { qSea = payload.toFloat(); gotSea = true; }  // เก็บค่า Aquarium
  else if (topic == TOPIC_LUGE) { qLuge = payload.toFloat(); gotLuge = true; } // เก็บค่า Luge
  else return;                                           // topic อื่นไม่สนใจ
  dirty = true;                                          // บอก loop ว่ามีค่าใหม่
}

void process() {                                         // ประมวลผลจากค่าที่เก็บไว้ทั้ง 3 ตัว
  const char* name = "Universal";                        // เริ่มสมมติว่า Universal น้อยสุด
  float best = qUni;                                     // ค่าต่ำสุดตอนนี้
  if (qSea < best) { best = qSea; name = "Aquarium"; }   // Aquarium น้อยกว่า
  if (qLuge < best) { best = qLuge; name = "Luge"; }     // Luge น้อยกว่า
  float avg = (qUni + qSea + qLuge) / 3.0;               // ค่าเฉลี่ยทั้ง 3 ที่

  client.publish(TOPIC_CHOICE, name);                    // ส่งชื่อสถานที่คิวน้อยสุด

  lcd.clear();                                           // ล้างจอ
  lcd.setCursor(0, 0);                                   // บรรทัดที่ 1
  lcd.print(name);                                       // ชื่อสถานที่
  lcd.print(" (");
  lcd.print((int)best);                                  // เวลารอของที่นั่น
  lcd.print("m)");
  lcd.setCursor(0, 1);                                   // บรรทัดที่ 2
  lcd.print("Avg: ");
  lcd.print((int)avg);                                   // ค่าเฉลี่ย
  lcd.print("m");

  // ----- เลือกสีตามค่าเฉลี่ย (เปลี่ยนสีได้จากตารางสีด้านบน) -----
  if (avg > 60) {                                        // เฉลี่ย > 60 นาที = คนเยอะมาก
    setRGB(255, 0, 0);                                   // แดง
    // ทางเลือก: setRGB(255, 40, 0) แดงเข้ม   |  setColor แบบ bool: (true, false, false)
  } else if (avg >= 30) {                                // เฉลี่ย 30-60 นาที = คนปานกลาง
    setRGB(255, 90, 0);                                  // ส้ม
    // ทางเลือก: setRGB(255, 130, 0) ส้มอ่อน  |  setRGB(255, 200, 0) เหลือง
    //           แบบ bool: (true, true, false) = เหลือง/ส้มอ่อน
  } else {                                               // เฉลี่ย < 30 นาที = คิวโล่ง
    setRGB(0, 255, 120);                                 // เขียวอมฟ้า
    // ทางเลือก: setRGB(0, 255, 255) ฟ้า cyan  |  setRGB(0, 255, 0) เขียวล้วน
    //           แบบ bool: (false, true, true) = เขียวอมฟ้า
  }

  Serial.print(name); Serial.print(" best="); Serial.print(best);   // ดีบักใน Serial
  Serial.print(" avg="); Serial.println(avg);
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
  client.subscribe(TOPIC_UNI);                 // subscribe 3 topic ทุกครั้งที่ต่อใหม่
  client.subscribe(TOPIC_SEA);
  client.subscribe(TOPIC_LUGE);
}

void setup() {
  Serial.begin(115200);                        // เปิด Serial Monitor
  pinMode(R_PIN, OUTPUT);                      // ขา RGB เป็นเอาต์พุต
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  setRGB(0, 0, 0);                             // เริ่มต้นให้ดับ
  lcd.init();                                  // เริ่มต้น LCD
  lcd.backlight();                             // เปิดไฟพื้นหลัง
  lcd.print("Connecting...");                  // แจ้งว่ากำลังเชื่อมต่อ
  connectWiFi();                               // ต่อ Wi-Fi
  client.begin(MQTT_HOST, MQTT_PORT, net);     // ตั้งค่า broker
  client.onMessage(messageReceived);           // ลงทะเบียนฟังก์ชันรับข้อความ
  connectMQTT();                               // ต่อ MQTT แล้ว subscribe
  lcd.clear();                                 // ล้างจอ
  lcd.print("Waiting data...");                // รอข้อมูลครบ 3 ที่
}

void loop() {
  client.loop();                               // รับข้อความ ต้องเรียกทุกรอบ
  if (WiFi.status() != WL_CONNECTED) connectWiFi();   // หลุด Wi-Fi ให้ต่อใหม่
  if (!client.connected()) connectMQTT();             // หลุด MQTT ให้ต่อใหม่

  if (dirty && gotUni && gotSea && gotLuge) {  // มีค่าใหม่ และได้ครบทั้ง 3 ที่แล้ว
    dirty = false;                             // เคลียร์ flag
    process();                                 // คำนวณ แสดงผล และ publish
  }
}