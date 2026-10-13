#define BLYNK_TEMPLATE_ID "TMPL6nXDjldOr"
#define BLYNK_TEMPLATE_NAME "FallDetection"
#define BLYNK_AUTH_TOKEN "KveCGf33u8V1WhImVQS2E3YHGdjnCSYy"

#include "Audio.h"
#include "SD_MMC.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <WiFi.h>


// --- Blynk Credentials ---


// --- WiFi Credentials ---
const char ssid[] = "Xiaomi";
const char pass[] = "12345679@";

// --- API Server Settings ---
// LƯU Ý: Thay đổi IP này thành IP máy tính của bạn (chạy FastAPI)
// Ví dụ: "http://192.168.1.10:8000"
const char *server_url = "http://10.128.150.225:8000/gate";
const char *reset_url = "http://10.128.150.225:8000/reset";

// --- Notification Settings ---
#define NOTIFICATION_INTERVAL 20000 // 20 giây một lần ở Level 2
#define LEVEL_1_TIMEOUT 120000      // 2 phút
#define LEVEL_2_TIMEOUT 300000      // 5 phút


// Note off my tssk in 21/3/2026
/**
 i will update user interface to connect with code 3 api
 i will  creat al new slider to control safe duration
 next i will  creat a new button to control the system v5
 last this is fixx the picture not show in user interface and connect audio and
 microphone  to system

 you will remember this
 continue to break promblem from big step to esayest step . this is a think you
 can make now on your power. this is a good way to solve promblem  :)))

 ok let sleep now :)))
 */

// --- Audio Paths ---
const char *AUDIO_FILE_1 = "/level1.mp3"; // File âm thanh cho Level 1 (Thẻ SD)
const char *AUDIO_FILE_2 = "/level2.mp3"; // File âm thanh cho Level 2 (Thẻ SD)

// Bảng chân I2S cho Audio (Tham chiếu)
#define I2S_EN_PIN 1
#define I2S_MCLK_PIN 4
#define I2S_BCLK_PIN 5
#define I2S_DOUT_PIN 6
#define I2S_LRCK_PIN 7
#define I2S_DIN_PIN 8

// Chân nút bấm trên ESP (Ví dụ nút BOOT)
#define HW_BUTTON_PIN 0

// Trạng thái hệ thống
enum State { IDLE, LEVEL_1, LEVEL_2, SOS_TRIGGERED, WAIT_FOR_BUTTON };

State currentState = IDLE;

// Các biến lưu trữ dữ liệu từ API
String fallTimeStr = "";
String evidenceUrl = "";

// Các cờ và biến thời gian
float custom_safe_duration = 0.0; // Lưu trữ thời gian safe từ Blynk

unsigned long level1StartTime = 0;
bool level1Notified = false;
unsigned long level2StartTime = 0;
bool level2Notified = false;

// Sub-state cho LEVEL_1: phát audio → lắng nghe tuần tự
enum Level1Phase { L1_PLAYING, L1_LISTENING };
Level1Phase level1Phase = L1_PLAYING;
unsigned long listeningStartTime = 0;
const unsigned long LISTENING_WINDOW =
    3000; // Lắng nghe 3 giây sau mỗi lần phát

int blynk_v0_state = 0; // Lưu trạng thái nút V0 (I'm OK) trên blynk
unsigned long lastV6Write =
    0; // Timer cho việc giữ đèn V6 trong WAIT_FOR_BUTTON

// Khởi tạo đối tượng Audio và I2S
Audio audio;
i2s_chan_handle_t rx_chan; // Handle cho I2S RX (ghi âm)

// Danh sách số điện thoại nhận SOS (Cài đặt cố định)
String SDT[] = {"0123456789", "0987654321"};

// Khai báo hàm
void checkApiStatus();
void sendSafeStatusToServer();
void playAudio(int caseType);
bool checkVoiceCommand();
void resetSystem();
void sendSOS();
void handleStateLogic();
void triggerSOS(); // Wrapper: sendSOS() + chuyển sang WAIT_FOR_BUTTON

// BƯỚC 1: Hàm checkApiStatus để nhận các thông tin từ server
void checkApiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server_url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      JsonDocument doc;
      deserializeJson(doc, payload);

      String typeStr = doc["type"].as<String>();
      bool fallDetected = doc["fall_detected"];

      if (typeStr == "apiSendFall" && fallDetected && currentState == IDLE) {
        fallTimeStr = doc["fall_time"].as<String>();

        // Lấy domain gốc (cắt bỏ phần đuôi đằng sau mốc Port :8000)
        String baseUrl = String(server_url);
        int portIndex = baseUrl.indexOf(":8000");
        if (portIndex > 0) {
          baseUrl = baseUrl.substring(0, portIndex + 5);
        }

        evidenceUrl = baseUrl + doc["evidence"].as<String>();

        currentState = LEVEL_1;
        level1StartTime = millis();
        level1Notified = false; // Reset cờ thông báo lúc rơi ngã

        Serial.println("[CẢNH BÁO] Phát hiện ngã từ API Server!");
      }
    } else {
      Serial.printf("[API] GET failed, error: %s (Code: %d)\n",
                    http.errorToString(httpCode).c_str(), httpCode);
    }
    http.end();
  }
}

// BƯỚC 2: Hàm thực hiện POST xác nhận an toàn lên Server API
void sendSafeStatusToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    String baseUrl = String(server_url);
    int portIndex = baseUrl.indexOf(":8000");
    if (portIndex > 0) {
      baseUrl = baseUrl.substring(0, portIndex + 5);
    }
    String postUrl = baseUrl + "/response-from-esp32";

    HTTPClient http;
    http.begin(postUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"safe\": true, \"safe_duration_minutes\": " +
                     String(custom_safe_duration) + "}";

    int httpCode = http.POST(payload);
    if (httpCode > 0) {
      Serial.printf("[API] Đã gửi xác nhận an toàn lên Server API (%.1f phút), "
                    "code: %d\n",
                    custom_safe_duration, httpCode);
    } else {
      Serial.printf("[API] Lỗi khi gửi xác nhận an toàn: %s\n",
                    http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}

// BƯỚC 3: Hàm phát âm thanh dựa trên tệp trong thẻ SD
void playAudio(int caseType) {
  // Nếu đang phát rồi thì không làm gì cả để tránh bị vấp (stutter)
  if (audio.isRunning())
    return;

  // Tắt I2S RX (mic) để tránh xung đột bus (speaker và mic dùng chung
  // BCLK/LRCK)
  i2s_channel_disable(rx_chan);

  // BẬT Audio Enable (Mức logic LOW = enable)
  digitalWrite(I2S_EN_PIN, LOW);

  if (caseType == 1) {
    Serial.println("[AUDIO] Đang phát âm thanh Level 1 từ thẻ SD...");
    audio.connecttoFS(SD_MMC, AUDIO_FILE_1);
  } else if (caseType == 2) {
    Serial.println("[AUDIO] Đang phát còi hú SOS từ thẻ SD...");
    audio.connecttoFS(SD_MMC, AUDIO_FILE_2);
  }
}

// BƯỚC 4: Hàm ghi âm thanh bằng mic của esp và kiểm tra cường độ
bool checkVoiceCommand() {
  // Bật I2S RX (mic) — đảm bảo mic sẵn sàng trước khi đọc
  i2s_channel_enable(rx_chan);

  size_t bytes_read = 0;
  int16_t i2s_raw_buffer[512];      

  // Đọc dữ liệu từ I2S RX
  if (i2s_channel_read(rx_chan, i2s_raw_buffer, sizeof(i2s_raw_buffer),
                       &bytes_read, 100) == ESP_OK) {
    long sum = 0;
    for (int i = 0; i < bytes_read / 2; i++) {
      sum += abs(i2s_raw_buffer[i]);
    }
    float average_amplitude = sum / (bytes_read / 2.0);

    // Nếu cường độ âm thanh vượt ngưỡng → coi như xác nhận "tôi ổn"
    if (average_amplitude > 1500) {
      Serial.printf("[MIC] Phát hiện âm thanh cường độ cao: %.2f\n",
                    average_amplitude);
      return true;
    }
  }
  return false;
}

// Hàm khởi tạo I2S cho Ghi âm (RX) - Sử dụng I2S_NUM_1 để tránh xung đột với
// Audio library (I2S_NUM_0)
void initI2SRecording() {
  // ESP32-S3 có 2 bộ I2S. Audio library dùng I2S_NUM_0, ta dùng I2S_NUM_1 cho
  // Mic.
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
  i2s_new_channel(&chan_cfg, NULL, &rx_chan);

  i2s_std_config_t std_cfg = {
      .clk_cfg =
          I2S_STD_CLK_DEFAULT_CONFIG(16000), // Sample rate 16kHz cho ghi âm
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                  I2S_SLOT_MODE_MONO),
      .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, // Mic thường không cần MCLK
                   .bclk = (gpio_num_t)I2S_BCLK_PIN,
                   .ws = (gpio_num_t)I2S_LRCK_PIN,
                   .dout = I2S_GPIO_UNUSED,
                   .din = (gpio_num_t)I2S_DIN_PIN,
                   .invert_flags = {.mclk_inv = false,
                                    .bclk_inv = false,
                                    .ws_inv = false}},
  };
  i2s_channel_init_std_mode(rx_chan, &std_cfg);
  i2s_channel_enable(rx_chan);
  Serial.println("[HỆ THỐNG] Đã khởi tạo I2S Recording (NUM_1) thành công.");
}

// BƯỚC 5: Hàm reset về trạng thái ban đầu cho hệ thống
void resetSystem() {
  Serial.println("[HỆ THỐNG] Resetting system to IDLE...");
  currentState = IDLE;
  level1Notified = false;
  level2Notified = false;
  blynk_v0_state = 0;
  lastV6Write = 0;
  fallTimeStr = "";
  evidenceUrl = "";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(reset_url);
    http.POST("");
    http.end();
  }

  // Khôi phục TẤT CẢ các chân Blynk về trạng thái an toàn ban đầu
  Blynk.virtualWrite(V0, 0);         // Tắt nút I'm OK
  Blynk.virtualWrite(V1, "");        // Xóa Terminal cảnh báo
  Blynk.setProperty(V3, "urls", ""); // Xóa ảnh bằng chứng
  Blynk.virtualWrite(V6, 0);         // Tắt đèn SOS khẩn cấp
  Blynk.virtualWrite(V10, 0);        // Tắt nút Gọi Cấp Cứu (Level 1)
  Blynk.virtualWrite(V11, 0);        // Tắt nút An Toàn
  Blynk.virtualWrite(V12, 0);        // Tắt nút SOS
}

// BƯỚC 9: Xây dựng hàm SOS() — Chỉ gửi thông tin, KHÔNG block vòng lặp
void sendSOS() {
  Serial.println("==================================");
  Serial.println("THỰC HIỆN GỬI SOS ĐẾN CƠ QUAN Y TẾ VÀ NGƯỜI NHÀ");

  String msg = "Thông cáo: Bạn có người nhà đang nguy hiểm!!!\n";
  msg += "Thời gian: " + fallTimeStr + "\n";
  msg += "Bằng chứng: " + evidenceUrl + "\n";
  msg += "SĐT người thân: " + SDT[0] + "\n";

  Serial.println(msg);

  for (int i = 0; i < sizeof(SDT) / sizeof(SDT[0]); i++) {
    Serial.println("-> Đang gửi thông báo đến SĐT: " + SDT[i]);
  }
  Serial.println("==================================");
  // Sau khi gửi SOS, nơi gọi hàm phải set currentState = WAIT_FOR_BUTTON
  // Việc bật V6 liên tục sẽ do handleStateLogic() xử lý (non-blocking)
}

// Wrapper tiện ích: Gọi SOS và chuyển sang trạng thái chờ xác nhận
void triggerSOS() {
  sendSOS();
  currentState = WAIT_FOR_BUTTON;
  lastV6Write = 0; // Reset timer để V6 bật ngay lập tức
}

// BƯỚC 6, 7, 8: Hàm xử lý loop logic cho hệ thống
void handleStateLogic() {
  switch (currentState) {

  // ── GIAI ĐOẠN 1: PHÁT HIỆN NGÃ, NẠN NHÂN CÒN NHẬN THỨC ──
  // Logic tuần tự: Phát audio → Lắng nghe 3s → Lặp lại (trong 2 phút)
  case LEVEL_1: {
    // Thông báo 1 lần duy nhất khi mới vào Level 1
    if (!level1Notified) {
      Serial.println(
          "[LEVEL 1] Phát hiện ngã! Đang chờ phản hồi từ nạn nhân...");
      Blynk.virtualWrite(V0,
                         1); // Bật nút I'm OK (đỏ) — nạn nhân gạt về 0 nếu ổn
      Blynk.virtualWrite(V10, 1); // Bật nút Gọi Cấp Cứu (Level 1)
      level1Notified = true;
      level1Phase = L1_PLAYING; // Bắt đầu bằng phát audio
    }

    // Trong vòng 2 phút (120,000 ms): chu kỳ phát audio → lắng nghe
    if (millis() - level1StartTime < 120000) {

      if (level1Phase == L1_PLAYING) {
        // === PHASE 1: Phát audio cảnh báo ===
        playAudio(1);
        // Khi audio phát xong → chuyển sang lắng nghe
        if (!audio.isRunning() && level1Notified) {
          level1Phase = L1_LISTENING;
          listeningStartTime = millis();
          Serial.println("[LEVEL 1] Audio xong, bắt đầu lắng nghe...");
        }
      } else if (level1Phase == L1_LISTENING) {
        // === PHASE 2: Lắng nghe mic (3 giây) ===
        bool saySafe = checkVoiceCommand();
        if (saySafe) {
          Serial.println("[LEVEL 1] Xác nhận an toàn từ giọng nói!");
          i2s_channel_disable(rx_chan); // Tắt mic
          sendSafeStatusToServer();     // Gửi trạng thái an toàn lên API
          resetSystem();
          break;
        }
        // Hết cửa sổ lắng nghe 3s → phát lại audio
        if (millis() - listeningStartTime >= LISTENING_WINDOW) {
          Serial.println(
              "[LEVEL 1] Không phát hiện phản hồi, phát lại audio...");
          level1Phase = L1_PLAYING;
        }
      }

      // Xử lý nút V0 (I'm OK) và V10 (Gọi Cấp Cứu) do BLYNK_WRITE() đảm nhiệm
    } else {
      // Hết 2 phút không phản hồi → Chuyển qua LEVEL 2
      Serial.println("[LEVEL 1] Hết 2 phút, nạn nhân không phản hồi. Chuyển "
                     "sang LEVEL 2.");
      i2s_channel_disable(rx_chan); // Tắt mic khi rời LEVEL_1
      currentState = LEVEL_2;
      level2StartTime = millis();
      level2Notified = false;
    }
    break;
  }

  // ── GIAI ĐOẠN 2: NẠN NHÂN KHÔNG NHẬN THỨC — CHỜ NGƯỜI NHÀ ──
  case LEVEL_2: {
    // Đẩy thông báo & bật các nút hành động 1 lần khi vào Level 2
    if (!level2Notified) {
      String msg = "Cảnh báo: Có người bị ngã hãy check cam ngay\n";
      msg += "Giờ ngã: " + fallTimeStr;
      Blynk.virtualWrite(V1, msg);                // Gửi vào Terminal V1
      Blynk.setProperty(V3, "urls", evidenceUrl); // Tải URL ảnh bằng chứng
      Blynk.virtualWrite(V11, 1); // Bật nút "An Toàn" (xanh) cho người nhà
      Blynk.virtualWrite(V12, 1); // Bật nút "SOS" (đỏ) cho người nhà
      level2Notified = true;
      Serial.println("[LEVEL 2] Đã đẩy cảnh báo lên Blynk (V1, V2, V3, V11, "
                     "V12). Đang đếm 3 phút...");
    }

    // Quá 3 phút (180,000 ms) mà người nhà không bấm → Tự động gửi SOS
    if (millis() - level2StartTime >= 180000) {
      Serial.println(
          "[LEVEL 2] Quá 3 phút không phản hồi. Tự động kích hoạt SOS!");
      triggerSOS();
    }
    // Xử lý nút V11 (An Toàn) và V12 (SOS) do BLYNK_WRITE() đảm nhiệm
    break;
  }

  // ── TRẠNG THÁI CHỜ XÁC NHẬN SAU SOS ──
  case WAIT_FOR_BUTTON: {
    // Giữ đèn V6 luôn sáng đỏ, cứ mỗi 500ms ghi 1 lần (non-blocking)
    unsigned long now = millis();
    if (now - lastV6Write >= 500) {
      Blynk.virtualWrite(V6, 1); // Duy trì đèn đỏ khẩn cấp
      lastV6Write = now;
    }
    // Xử lý nút V6 (xác nhận đã biết) do BLYNK_WRITE() đảm nhiệm
    break;
  }

  case IDLE:
  default:
    break;
  }
}

// ================= XỬ LÝ SỰ KIỆN TỪ BLYNK APP =================

// [V0] Nút "I'm OK" (Switch) — Nạn nhân xác nhận an toàn tại LEVEL_1
// Hệ thống ghi V0=1 (bật đỏ). Nạn nhân gạt về OFF (=0) → xác nhận ổn.
BLYNK_WRITE(V0) {
  blynk_v0_state = param.asInt();
  if (blynk_v0_state == 0 && currentState == LEVEL_1) {
    Serial.println(
        "[LEVEL 1] Nạn nhân xác nhận AN TOÀN qua nút I'm OK trên Blynk!");
    sendSafeStatusToServer(); // Gửi trạng thái an toàn lên API
    resetSystem();            // Đưa hệ thống về IDLE
  }
}

// [V10] Nút "Gọi Cấp Cứu" (Push) — Nạn nhân tự kích hoạt SOS tại LEVEL_1
BLYNK_WRITE(V10) {
  if (param.asInt() == 1 && currentState == LEVEL_1) {
    Serial.println("[LEVEL 1] Nạn nhân tự kích hoạt SOS khẩn cấp qua V10!");
    triggerSOS(); // Gọi SOS và chuyển sang WAIT_FOR_BUTTON
  }
}

// [V11] Nút "An Toàn" (Push) — Người nhà xác nhận nạn nhân ổn tại LEVEL_2
BLYNK_WRITE(V11) {
  if (param.asInt() == 1 && currentState == LEVEL_2) {
    Serial.println("[LEVEL 2] Người nhà xác nhận AN TOÀN qua V11!");
    Blynk.virtualWrite(V12, 0); // Tắt nút SOS còn lại
    sendSafeStatusToServer();   // Gửi trạng thái an toàn lên API
    resetSystem();              // Đưa hệ thống về IDLE
  }
}

// [V12] Nút "SOS" (Push) — Người nhà chủ động gọi cấp cứu tại LEVEL_2
BLYNK_WRITE(V12) {
  if (param.asInt() == 1 && currentState == LEVEL_2) {
    Serial.println("[LEVEL 2] Người nhà chủ động kích hoạt SOS qua V12!");
    triggerSOS(); // Gọi SOS và chuyển sang WAIT_FOR_BUTTON
  }
}

// [V5] Nhận thời gian an toàn tùy chỉnh từ Blynk Slider/Numeric Input
BLYNK_WRITE(V5) {
  custom_safe_duration = param.asFloat();
  Serial.printf("[BLYNK] Đã cập nhật thời gian an toàn tùy chỉnh: %.1f phút\n",
                custom_safe_duration);
}

// [V6] Đèn SOS khẩn cấp — Người nhà xác nhận đã biết, reset hệ thống
BLYNK_WRITE(V6) {
  int v6_state = param.asInt();
  // Khi người dùng bấm nút V6 (giá trị = 1) để xác nhận đã biết thông tin
  if (v6_state == 1 && currentState == WAIT_FOR_BUTTON) {
    Serial.println(
        "[HỆ THỐNG] Người nhà xác nhận ĐÃ BIẾT tin cấp cứu. Reset về IDLE.");
    resetSystem();
  }
}

void setup() {
  Serial.begin(115200);

  // Cấu hình nút bấm trên ESP (Thường PullUp nội bên trong)
  pinMode(HW_BUTTON_PIN, INPUT_PULLUP);

  // Cấu hình chân Audio Enable (Mức logic theo bảng: High level disable, Low
  // level enable) // BƯỚC 3
  pinMode(I2S_EN_PIN, OUTPUT);
  digitalWrite(I2S_EN_PIN, HIGH);

  // Kết nối WiFi (Non-blocking hoặc Timeout ngắn)
  Serial.println("Đang cấu hình WiFi...");
  WiFi.begin(ssid, pass);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi connected! IP address: ");
    Serial.println(WiFi.localIP());

    // --- Khởi tạo Audio & SD Card & I2S Recording ---
    audio.setPinout(I2S_BCLK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN);
    audio.setVolume(21); // Âm lượng tối đa

    // Cấu hình chân SDIO cho ESP32-S3
    SD_MMC.setPins(38, 40, 39, 41, 48, 47);
    if (!SD_MMC.begin("/sdcard", false)) { // false = 4-bit mode
      Serial.println("[LỖI] Không thể mount thẻ nhớ SD!");
    } else {
      Serial.println("[HỆ THỐNG] Đã mount thẻ nhớ SD thành công.");
    }

    initI2SRecording();

    // Cấu hình Blynk (KHÔNG BLOCK HỆ THỐNG nếu WiFi rớt)
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk
        .connect(); // Sẽ cố gắng connect, nếu không được vẫn thoát ra chay tiep
  } else {
    Serial.println(
        "\n[LỖI] Không thể kết nối WiFi, vào mode offline theo dõi.");
  }

  Serial.println("Hệ thống khởi động hoàn tất!");
}

void loop() {
  Blynk.run();

  // Kiểm tra API mỗi 2 giây nếu đang ở trạng thái IDLE
  static unsigned long lastApiCheck = 0;
  if (millis() - lastApiCheck > 2000) {
    lastApiCheck = millis();
    if (currentState == IDLE) {
      checkApiStatus();
    }
  }

  handleStateLogic();

  // Cập nhật Audio và Loa
  audio.loop();

  // Tắt Audio EN khi không phát nữa
  static bool lastRunning = false;
  bool isRunning = audio.isRunning();
  if (lastRunning && !isRunning) {
    digitalWrite(I2S_EN_PIN, HIGH);
    Serial.println("[AUDIO] Đã phát xong, tắt Audio EN.");
  }
  lastRunning = isRunning;
}