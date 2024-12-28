#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <RTClib.h>
#include <WiFi.h> // Tambahkan untuk Wi-Fi
#include <HTTPClient.h> // Tambahkan untuk HTTP request

// Definisi untuk DHT22
#define DHTPIN 4 // GPIO untuk DHT22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Definisi untuk RTC DS3231
RTC_DS3231 rtc;

// Definisi untuk LCD 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C biasanya 0x27 atau 0x3F

// Definisi untuk TCS230
#define S0 12
#define S1 13
#define S2 14
#define S3 15
#define OUT 25

// Definisi untuk Buzzer
#define BUZZER_PIN 26

// Definisi untuk tombol
#define BUTTON1_PIN 32 // Tombol untuk Start/Restart
#define BUTTON2_PIN 33 // Tombol untuk Pause/Stop

// Wi-Fi Credential
const char* ssid = "NAMA_WIFI"; // Ganti dengan SSID Wi-Fi Anda
const char* password = "PASSWORD_WIFI"; // Ganti dengan password Wi-Fi Anda

// WhatsApp API
const char* whatsapp_api = "https://api.callmebot.com/whatsapp.php";
const char* phone_number = "+628xxxxxxxxxx"; // Nomor WhatsApp penerima (format internasional)
const char* api_key = "apikeyanda"; // Dapatkan dari CallMeBot

bool rtc_running = false;
bool rtc_paused = false;
bool buzzer_state = false;
unsigned long last_buzzer_toggle = 0;
unsigned long last_read_time = 0;
const unsigned long beep_interval = 500; // Interval beep (ms)

void setup() {
  Serial.begin(115200);

  // Inisialisasi Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke Wi-Fi...");
  }
  Serial.println("Wi-Fi Terhubung!");

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  // Inisialisasi DHT22
  dht.begin();

  // Inisialisasi RTC DS3231
  if (!rtc.begin()) {
    Serial.println("RTC tidak ditemukan!");
    lcd.print("RTC Error!");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC kehilangan daya, set ulang waktu!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inisialisasi TCS230
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);

  // Inisialisasi Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Inisialisasi tombol
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
}

void handleBuzzer(float temperature) {
  unsigned long current_time = millis();

  if (temperature > 25) {
    // Buzzer ON terus
    if (!buzzer_state) {
      buzzer_state = true;
      digitalWrite(BUZZER_PIN, HIGH);
    }
  } else {
    // Buzzer beep-beep
    if (current_time - last_buzzer_toggle > beep_interval) {
      buzzer_state = !buzzer_state;
      digitalWrite(BUZZER_PIN, buzzer_state ? HIGH : LOW);
      last_buzzer_toggle = current_time;
    }
  }
}

void sendWhatsAppMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(whatsapp_api) + "?phone=" + phone_number + "&text=" + message + "&apikey=" + api_key;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Pesan WhatsApp berhasil dikirim!");
    } else {
      Serial.println("Gagal mengirim pesan WhatsApp!");
    }
    http.end();
  } else {
    Serial.println("Wi-Fi tidak terhubung!");
  }
}

void loop() {
  unsigned long current_time = millis();

  if (rtc_running && !rtc_paused) {
    if (current_time - last_read_time > 2000) {
      last_read_time = current_time;

      // Baca data dari DHT22
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();

      // Periksa kesalahan pembacaan DHT22
      if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Gagal membaca data DHT!");
        lcd.setCursor(0, 0);
        lcd.print("DHT Error!");
        return;
      }

      // Baca waktu dari RTC
      DateTime now = rtc.now();

      // Tampilkan suhu dan kelembapan pada LCD
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print("C");

      lcd.setCursor(0, 1);
      lcd.print("Hum: ");
      lcd.print(humidity);
      lcd.print("%");

      // Kirim pesan jika suhu tinggi
      if (temperature > 25) {
        sendWhatsAppMessage("Peringatan: Suhu mencapai " + String(temperature) + "C!");
      }

      // Tangani buzzer
      handleBuzzer(temperature);
    }
  }
}
