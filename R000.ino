#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Ping.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>

// #define TOUCH_SENSOR 25
// #define TOUCH_SENSOR 35
#define MAGNET_SENSOR 32
#define BUZZER 16
#define SENSORPIR 26

#define DHT_SENSOR_PIN 15
#define DHT_SENSOR_TYPE DHT22

#define SDA_PIN 5
#define RST_PIN 0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
MFRC522 rfid(SDA_PIN, RST_PIN);
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const char *ssid = "wifissid";
const char *password = "wifipass";

//------------- Buzzer Opening --------------//
int melody[] = {
    392, 523, 659, 784 // opsi pertama
};

int noteDuration[] = {
    150, 150, 150, 150, 150, 300};

int melodyLength = sizeof(melody) / sizeof(melody[0]);
//------------- End Buzzer ----------------//

//------------- Buzzer Gagal/Error ----------------//
int melody2[] = {
    330, 262, 196};

int noteDuration2[] = {
    200, 200, 300, 300};

int melodyLength2 = sizeof(melody2) / sizeof(melody2[0]);
//-------------End Buzzer Gagal/Error ----------------//
const int btn_pulang = 17;  // Pin untuk tombol Pulang
const int TOUCH_SENSOR = 4; // sensor Touch

const int selenoid = 27; // Pin untuk Selenoid
const int lampu = 13;    // Pin untuk Lampu
const int ac = 12;       // Pin untuk AC
const int colokan = 14;  // Pin untuk Colokan/terminal listrik

// Gambar warna putih, opsi pilih Black
const unsigned char logo_wifi[] PROGMEM = {
    0x07, 0xc0, 0x3f, 0xf8, 0x70, 0x1c, 0xc7, 0xc6, 0x1f, 0xf0, 0x30, 0x18, 0x07, 0xc0, 0x0c, 0x60,
    0x01, 0x00, 0x03, 0x80, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const unsigned char logo_wifi_black[] PROGMEM = {
    0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe,
    0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe};

const unsigned char logo_internet[] PROGMEM = {
    0x07, 0xc0, 0x1d, 0x70, 0x29, 0x28, 0x49, 0x24, 0x7f, 0xfc, 0x91, 0x12, 0x91, 0x12, 0xff, 0xfe,
    0x91, 0x12, 0x91, 0x12, 0x7f, 0xfc, 0x49, 0x24, 0x29, 0x28, 0x1d, 0x70, 0x07, 0xc0};

const unsigned char logo_no_internet[] PROGMEM = {
    0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe,
    0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe};

String get_mode = "http://ip_addres/erwin/public/ambil_data";               // Untuk mendeteksi apakah terhubung dengan server
String scan_kartu = "http://ip_addres/erwin/public/data_scanning";          // Scan Kartu mode stanby
String status_device = "http://ip_addres/erwin/public/status_device";       // mengirim data status all device
String status_pintu = "http://ip_addres/erwin/public/status_pintu";         // mengirim data status pintu
String status_sensorPir = "http://ip_addres/erwin/public/status_sensorPir"; // mengirim data status sensor Pir

unsigned long lastTime = 0;      // Waktu mulai req data ke database
unsigned long timerDelay = 5000; // waktu req data ke database

const unsigned long timeout = 5000; // waktu menghubungkan ke wifi

unsigned long relayStartTime = 0;         // Waktu mulai penyalakan relay
const unsigned long relayDuration = 5000; // Durasi penyalakan relay

bool relayActive = false;            // Status penyalakan relay
bool req_data = true;                // req data status ke server
bool mode_touch = false;             // Sensor sentuh
bool pir_standby = false;            // Sensor sentuh
bool scanning = true;                // proses scan
bool pulang = false;                 // status tombol pulang
bool send_data_statusDevice = false; // status tombol pulang

bool previousStatusMagnet = false; // Variabel untuk menyimpan nilai status_sensorMagnet sebelumnya
bool previousStatusPir = false;    // Variabel untuk menyimpan nilai status_sensorPir sebelumnya

String apiKey = "your_apikey"; // Kunci dari segala-galanya

void openingBuzzer()
{
  for (int i = 0; i < melodyLength; i++)
  {
    ledcWriteTone(0, melody[i]);
    delay(noteDuration[i]);
    ledcWrite(0, 0);
    delay(50);
  }
}

void succesBuzzer(int duration)
{
  ledcWriteTone(0, 1000); // Menetapkan frekuensi bunyi buzzer
  delay(duration);
  ledcWrite(0, 0); // Mematikan bunyi buzzer
  delay(50);
}

void gagalBuzzer()
{
  for (int i = 0; i < melodyLength2; i++)
  {
    ledcWriteTone(0, melody2[i]);
    delay(noteDuration2[i]);
    ledcWrite(0, 0);
    delay(50);
  }
}

void ambil_data()
{
  HTTPClient http;
  http.begin(get_mode);
  int httpcode = http.GET();
  if (httpcode > 0)
  {
    String response = http.getString();
    // Serial.println("Response: " + response);
    if (response != "")
    {
      display.drawBitmap(20, 0, logo_internet, 15, 15, 1);
      display.display();
    }
  }
  else
  {
    display.drawBitmap(20, 0, logo_internet, 15, 15, 1);
    display.display();
    delay(500);
    display.drawBitmap(20, 0, logo_no_internet, 15, 15, 0);
    display.display();
    Serial.println("Failed to make HTTP request");
  }
  http.end();
}

void checkWiFiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Not connected to WiFi, trying to connect...");
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      if (millis() - start >= timeout)
      {
        Serial.println("Gagal Menghubungkan ke Wifi!");
        Serial.println("Mode Offline");
        break;
      }
      delay(1000);
      Serial.println("Connecting to WiFi...");
      // display.clearDisplay();
      display.drawBitmap(0, 0, logo_wifi, 15, 15, 1);
      display.display();
      delay(500);
      // display.clearDisplay();
      display.drawBitmap(0, 0, logo_wifi_black, 15, 15, 0);
      display.display();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected to WiFi");
      // display.clearDisplay();
      display.drawBitmap(0, 0, logo_wifi, 15, 15, 1);
      display.display();
    }
    else
    {
      Serial.println("Failed to connect to WiFi");
    }
  }
  // else {
  //   Serial.println("Already connected to WiFi");
  // }
}

// String uidString = "";
// void scanner() {
//   if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
//     // digitalWrite(BUZZER, HIGH);
//     Serial.print("\nCard UID: ");
//     uidString = "";  // Reset nilai UID sebelum mengisi ulang
//     for (byte i = 0; i < rfid.uid.size; i++) {
//       Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
//       int uid = rfid.uid.uidByte[i];
//       Serial.print(uid);
//       uidString += String(uid, DEC);
//     }
//     rfid.PICC_HaltA();
//     rfid.PCD_StopCrypto1();
//     delay(200);
//     // digitalWrite(BUZZER, LOW);
//   }
// }

void send_status_device()
{
  int status_sensorMagnet = digitalRead(MAGNET_SENSOR);

  WiFiClient client;
  HTTPClient http3;

  int status_pintu = status_sensorMagnet;
  int statusLampu = 1;
  int statusAc = 1;
  int statusTerminal = 1;
  int sensorGerak = 0;

  // Membuat string data untuk dikirim dalam format x-www-form-urlencoded
  String data = "apiKey=" + apiKey + "&status_pintu=" + status_pintu + "&status_lampu=" + statusLampu + "&status_ac=" + statusAc + "&status_terminal=" + statusTerminal + "&sensor_gerak=" + sensorGerak;

  http3.begin(client, status_device);
  http3.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http3.POST(data);

  // Menampilkan kode respons dari server
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Jika data berhasil dikirim, tampilkan respons dari server
  if (httpResponseCode > 0)
  {
    String response = http3.getString();
    Serial.print("Response: ");
    Serial.println(response);
  }
  else
  {
    Serial.println("Failed to make HTTP request");
  }
  send_data_statusDevice = false;
  http3.end();
}

void send_status_pulang()
{
  int status_sensorMagnet = digitalRead(MAGNET_SENSOR);

  WiFiClient client;
  HTTPClient http4;

  int status_pintu = status_sensorMagnet;
  int statusLampu = 0;
  int statusAc = 0;
  int statusTerminal = 0;

  // Membuat string data untuk dikirim dalam format x-www-form-urlencoded
  String data = "apiKey=" + apiKey + "&status_pintu=" + status_pintu + "&status_lampu=" + statusLampu + "&status_ac=" + statusAc + "&status_terminal=" + statusTerminal;

  http4.begin(client, status_device);
  http4.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http4.POST(data);

  // Menampilkan kode respons dari server
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Jika data berhasil dikirim, tampilkan respons dari server
  if (httpResponseCode > 0)
  {
    String response = http4.getString();
    Serial.print("Response: ");
    Serial.println(response);
  }
  else
  {
    Serial.println("Failed to make HTTP request");
  }
  pulang = false;
  http4.end();
}

void setup()
{
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Jika oled tidak terdetesi maka loop tidak akan di proses
  }
  dht_sensor.begin(); // initialize the DHT sensor
  SPI.begin();
  rfid.PCD_Init();

  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER, 0);

  openingBuzzer();

  pinMode(TOUCH_SENSOR, INPUT_PULLUP);
  pinMode(MAGNET_SENSOR, INPUT_PULLUP);
  pinMode(SENSORPIR, INPUT);

  pinMode(btn_pulang, INPUT_PULLUP);

  pinMode(selenoid, OUTPUT);
  pinMode(lampu, OUTPUT);
  pinMode(ac, OUTPUT);
  pinMode(colokan, OUTPUT);

  digitalWrite(selenoid, LOW);
  digitalWrite(lampu, LOW);
  digitalWrite(ac, LOW);
  digitalWrite(colokan, LOW);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(41, 20);
  display.println(F("SMART"));
  display.setCursor(35, 43);
  display.println(F("OFFICE"));
  display.display();
  delay(3000);
  checkWiFiConnection();
}

void loop()
{
  checkWiFiConnection();

  float tempC = dht_sensor.readTemperature();
  if (isnan(tempC))
  {
    Serial.println("Sensor Suhu Tidak Terhubung");
  }
  else
  {
    // Serial.println();
    // Serial.print("Temperature: ");
    // Serial.print(tempC);
    // Serial.print("°C\n");
  }

  int status_btnPulang = digitalRead(btn_pulang);
  int status_sensorTouch = digitalRead(TOUCH_SENSOR);
  int status_sensorMagnet = digitalRead(MAGNET_SENSOR);
  int status_pirSensor = digitalRead(SENSORPIR);

  // Serial.print("Sensor Touch  : ");
  // Serial.println(status_sensorTouch);
  // Serial.print("Sensor Magnet : ");
  // Serial.println(status_sensorMagnet);

  if (relayActive && millis() - relayStartTime >= relayDuration)
  {
    digitalWrite(selenoid, LOW); // Matikan relay
    Serial.println("Terkunci");
    scanning = true;
    relayActive = false; // Set status penyalakan relay menjadi tidak aktif
  }

  if (pulang)
  {
    int status_btnPulang = digitalRead(btn_pulang);

    if (status_btnPulang == 0)
    {
      pir_standby = true;
      Serial.println("Tombol di Tekan");
      digitalWrite(lampu, LOW);   // Matikan Lampu
      digitalWrite(ac, LOW);      // Matikan AC
      digitalWrite(colokan, LOW); // Matikan Relay Colokan/Terminal
      send_status_pulang();
    }
  }

  if (pir_standby)
  {
    int status_pirSensor = digitalRead(SENSORPIR);

    if (status_pirSensor != previousStatusPir)
    {
      // Serial.println("Gerakan terdeteksi");
      WiFiClient client;
      HTTPClient http6;

      String data = "apiKey=" + apiKey + "&sensor_gerak=" + status_pirSensor;

      if (WiFi.status() == WL_CONNECTED)
      {
        http6.begin(client, status_sensorPir);
        http6.addHeader("Content-Type", "application/x-www-form-urlencoded");

        int httpResponseCode = http6.POST(data);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Jika data berhasil dikirim, tampilkan respons dari server
        if (httpResponseCode > 0)
        {
          String response = http6.getString();
          Serial.print("Response: ");
          Serial.println(response);
        }
        else
        {
          Serial.println("Failed to make HTTP request");
        }
        http6.end();
      }
      previousStatusPir = status_pirSensor;
    }
  }
  if (mode_touch)
  {
    int status_sensorTouch = digitalRead(TOUCH_SENSOR);
    int status_sensorMagnet = digitalRead(MAGNET_SENSOR);

    if (status_sensorMagnet != previousStatusMagnet)
    {
      WiFiClient client;
      HTTPClient http5;

      String data = "apiKey=" + apiKey + "&status_pintu=" + status_sensorMagnet;

      if (WiFi.status() == WL_CONNECTED)
      {
        http5.begin(client, status_pintu);
        http5.addHeader("Content-Type", "application/x-www-form-urlencoded");
        Serial.print("Data Yang di  kirim : ");
        Serial.println(status_sensorMagnet);

        int httpResponseCode = http5.POST(data);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Jika data berhasil dikirim, tampilkan respons dari server
        if (httpResponseCode > 0)
        {
          String response = http5.getString();
          Serial.print("Response: ");
          Serial.println(response);
        }
        else
        {
          Serial.println("Failed to make HTTP request");
        }
        http5.end();
      }
      previousStatusMagnet = status_sensorMagnet; // Simpan nilai status_sensorMagnet saat ini sebagai nilai sebelumnya
    }

    if (status_sensorTouch == 1 && !relayActive)
    {
      Serial.println("Kunci Terbuka");
      digitalWrite(selenoid, HIGH); // Nyalakan relay
      relayStartTime = millis();    // Catat waktu mulai penyalakan
      relayActive = true;           // Set status penyalakan relay menjadi aktif

      Serial.print("Sensor Touch   : ");
      Serial.println(status_sensorTouch);
      Serial.print("Sensor Magnet  : ");
      Serial.println(status_sensorMagnet);
      Serial.print("Sensor Gerak   : ");
      Serial.println(status_pirSensor);
    }
  }

  if (send_data_statusDevice)
  {
    send_status_device();
  }

  // display.setTextColor(WHITE);
  // display.setTextSize(2);
  // display.setCursor(40, 0);
  // display.print(tempC);
  // display.println("°C");
  // display.display();
  delay(500);

  if (req_data && (millis() - lastTime) > timerDelay)
  {
    ambil_data();
    lastTime = millis();
  }

  if (scanning)
  {
    // scanner();
    String uidString = "";

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
      Serial.print("\nCard UID: ");
      uidString = ""; // Reset nilai UID sebelum mengisi ulang
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        int uid = rfid.uid.uidByte[i];
        Serial.print(uid);
        uidString += String(uid, DEC);
      }
      // rfid.PICC_HaltA();
      // rfid.PCD_StopCrypto1();
      // delay(200);
    }

    display.setTextColor(BLACK);
    display.setTextSize(2);
    display.setCursor(41, 20);
    display.println(F("SMART"));
    display.setCursor(35, 43);
    display.println(F("OFFICE"));

    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(39, 20);
    display.println(F("SCAN"));
    display.setCursor(35, 43);
    display.println(F("KARTU"));
    display.display();

    if (uidString != "")
    {
      WiFiClient client;
      HTTPClient http2;

      http2.begin(client, scan_kartu);
      http2.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String data = "uid=" + uidString + "&apiKey=" + apiKey;

      Serial.println();
      Serial.print("Data: ");
      Serial.println(data);

      int send_data = http2.POST(data);

      Serial.println();
      String response2 = http2.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(send_data);
      Serial.print("Response: ");
      Serial.println(response2);

      StaticJsonDocument<400> doc;
      DeserializationError error = deserializeJson(doc, response2);

      if (error)
      {
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
        return;
      }

      const char *statusValue = doc["status"];
      Serial.print("Status: ");
      Serial.println(statusValue);

      if (strcmp(statusValue, "success") == 0)
      {
        mode_touch = true;             // mengaktifkan tombol Touch
        pulang = true;                 // mengaktifkan tombol Pulang
        pir_standby = false;           // Mematikan sensor Pir
        send_data_statusDevice = true; // Mengaktifkan pengiriman data ke server

        for (int i = 0; i < 2; i++)
        {
          succesBuzzer(150);
        }

        display.setTextColor(BLACK);
        display.setTextSize(2);
        display.setCursor(39, 20);
        display.println(F("SCAN"));
        display.setCursor(35, 43);
        display.println(F("KARTU"));

        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(39, 20);
        display.println(F("SCAN"));
        display.setCursor(20, 43);
        display.println(F("SUCCESS"));
        display.display();

        digitalWrite(lampu, HIGH);
        digitalWrite(ac, HIGH);
        digitalWrite(colokan, HIGH);

        if (!relayActive)
        {
          digitalWrite(selenoid, HIGH); // Nyalakan relay
          relayStartTime = millis();    // Catat waktu mulai penyalakan
          relayActive = true;           // Set status penyalakan relay menjadi aktif
        }

        // send_status_device();
        // digitalWrite(selenoid, HIGH);
        // delay(5000);
        // digitalWrite(selenoid, LOW);

        delay(1000);
        display.setTextColor(BLACK);
        display.setTextSize(2);
        display.setCursor(39, 20);
        display.println(F("SCAN"));
        display.setCursor(20, 43);
        display.println(F("SUCCESS"));
        display.display();
      }

      if (strcmp(statusValue, "error") == 0)
      {
        gagalBuzzer();
        display.setTextColor(BLACK);
        display.setTextSize(2);
        display.setCursor(39, 20);
        display.println(F("SCAN"));
        display.setCursor(35, 43);
        display.println(F("KARTU"));

        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(35, 20);
        display.println(F("TIDAK"));
        display.setCursor(15, 43);
        display.println(F("TERDAFTAR"));
        display.display();

        delay(2000);
        display.setTextColor(BLACK);
        display.setTextSize(2);
        display.setCursor(35, 20);
        display.println(F("TIDAK"));
        display.setCursor(15, 43);
        display.println(F("TERDAFTAR"));
        display.display();
      }
      http2.end();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      delay(200);
      uidString = "";
    }
  }
}
