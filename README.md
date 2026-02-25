# 🏥 Smart Medicine Room - ESP32 IoT + Digital Twin

ระบบห้องเก็บยาอัจฉริยะ (Smart Medicine Storage Room)  
พัฒนาโดยใช้ ESP32 + Web Dashboard + MySQL + Telegram Notification  
รองรับการทำงานแบบ Real-Time และ Edge Computing

# 📌 1. Project Overview

Smart Medicine Room คือระบบ IoT สำหรับควบคุมและตรวจสอบสภาพแวดล้อมในห้องเก็บยา  
รองรับการ:

- ตรวจวัดอุณหภูมิและความชื้น
- ตรวจจับการเปิดประตู / การเข้าใช้งาน
- แจ้งเตือนผ่าน Telegram เมื่อเกิดเหตุผิดปกติ
- ควบคุมอุปกรณ์ผ่าน Web Dashboard
- ทำงานอัตโนมัติแม้ไม่มีอินเทอร์เน็ต (Edge Logic)

แนวคิดหลักของระบบนี้คือ:

> Digital Twin Concept  
> อุปกรณ์จริงทำงานสอดคล้องกับระบบบนเว็บแบบ Real-Time


# 🧠 2. System Architecture
ESP32 (Sensor + Actuator)
│
│ HTTP API
▼
PHP API (XAMPP)
│
▼
MySQL Database
│
▼
Web Dashboard (Real-Time)
│
▼
Telegram Notification


---

# 🔌 3. Hardware Components

## Microcontroller
- ESP32 Dev Board

## Input (Sensors)
- DHT22 (Temperature & Humidity)
- IR Sensor (Entry Detection)

## Output (Actuators)
- Relay (ควบคุมไฟ)
- Servo Motor (เปิด/ปิดประตูยา)
- Servo (พัดลมส่าย)
- Buzzer (แจ้งเตือน)
- Status LED

---

# 📍 4. Pin Mapping

| Device | Pin |
|--------|-----|
| DHT22 | 27 |
| IR Sensor | 14 |
| Relay | 26 |
| LED | 4 |
| Buzzer | 25 |
| Fan Servo | 18 |
| Door Servo | 19 |

---

# 🌐 5. WiFi Configuration

ระบบไม่ฝัง SSID/Password ในโค้ด

วิธีตั้งค่า:
1. เปิดเครื่อง ESP32
2. จะมี WiFi ชื่อ `IOT-SETUP`
3. เข้าไปตั้งค่า WiFi
4. กรอก BASE_URL และ deviceId
5. ระบบจะบันทึกใน Preferences

---

# 🗄 6. API Endpoints

| Endpoint | Description |
|-----------|-------------|
| sensor_push.php | รับข้อมูล Sensor |
| cmd_get.php | ดึงคำสั่งจาก Dashboard |
| cmd_done.php | แจ้งว่า Command ทำเสร็จ |
| entry_push.php | บันทึกการเข้าใช้งาน |
| telegram_entry.php | แจ้งเตือนการเข้า |
| telegram_critical.php | แจ้งเตือนค่าวิกฤต |

---

# 🔐 7. Security Layer

ระบบมีการป้องกัน:

- API Key Header (`x-api-key`)
- Device Token (`x-device-token`)
- แยก secrets.h ไม่ push ขึ้น GitHub
- Rate Limiting (ฝั่ง Server)

---

# 📊 8. Dashboard Features

- แสดงค่าอุณหภูมิ/ความชื้นแบบ Real-Time
- แสดงสถานะ IR
- ควบคุม Relay
- เปิด/ปิด Door
- ควบคุม Fan
- ดู Log การเข้าใช้งาน

---

# 🚨 9. Notification System

## 🔔 Entry Alert
- เมื่อ IR ตรวจจับการเข้า
- แจ้ง Telegram พร้อม Temp/Humidity

## 🔥 Critical Alert
- อุณหภูมิ < 0°C หรือ > 30°C
- ความชื้น < 40% หรือ > 80%
- แจ้งซ้ำทุก 5 วินาทีหากยังผิดปกติ

---

# ⚙️ 10. Automation Logic (Edge Computing)

ระบบทำงานแม้เน็ตหลุด:

- Relay เปิดเมื่อมีการเข้า
- Fan ทำงานเมื่อร้อน
- LED/Buzzer แจ้งเตือนทันที
- Critical Check ทำใน ESP32 โดยตรง

---

# 🧪 11. Wokwi Simulation

ลิงก์จำลองระบบ:

(ใส่ลิงก์ Wokwi ตรงนี้)


---

# 🛠 12. Installation Guide

## Firmware Setup

1. เปิด Arduino IDE
2. ติดตั้ง:
   - WiFiManager
   - ESP32Servo
   - ArduinoJson
3. สร้างไฟล์ `secrets.h`

Upload ไป ESP32

Server Setup

ติดตั้ง XAMPP

วางโฟลเดอร์ api ไว้ใน:

C:\xampp\htdocs\iot\

Import MySQL Database

แก้ db.php ให้ตรงกับเครื่อง

Dashboard

เปิดผ่าน:

http://localhost/iot/dashboard/
📁 13. Project Structure
Smart-Medicine-Room
│
├── firmware
├── api
├── dashboard
├── .gitignore
└── README.md
🎓 14. Deliverables

Physical Model (Diorama)

ESP32 Device

Web Dashboard

Database

GitHub Source Code

Wokwi Simulation

Telegram Notification

👨‍💻 Developer

Name: First
Major: Computer Engineering
Project: IoT Mini Project

📜 License

Educational Use Only


---

# 🚀 หลังจากวางแล้ว

รัน:

```bash
git add README.md
git commit -m "docs: full project documentation"
git push
