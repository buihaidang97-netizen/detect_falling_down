// --- Blynk Credentials ---
#define BLYNK_TEMPLATE_ID "TMPL6nXDjldOr"
#define BLYNK_TEMPLATE_NAME "FallDetection"
#define BLYNK_AUTH_TOKEN "KveCGf33u8V1WhImVQS2E3YHGdjnCSYy"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// --- Telegram & Xiaozhi AI Settings ---
#define TELEGRAM_BOT_TOKEN "8271771967:AAHmku0WWhAZnFm-GALfTEhB80FSmkwLKXQ"
#define TELEGRAM_CHAT_ID "5348036208"
#define DIA_CHI_NHA "Nhà số 123, Đường ABC, Quận XYZ, TP. Hà Nội"
#define XIAOZHI_ALERT_URL "http://192.168.1.100/alert" // Thay IP Xiaozhi

// --- WiFi Credentials ---
const char *ssid = "Xiaomi";
const char *pass = "12345679@";

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

// --- WebServer ---
WebServer server(80);

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

// Đối tượng WebServer đã khởi tạo ở trên

// Danh sách số điện thoại nhận SOS (Cài đặt cố định)
String SDT[] = {"0123456789", "0987654321"};

// Khai báo hàm
void checkApiStatus();
void sendSafeStatusToServer();
void triggerXiaozhi();
void resetSystem();
void sendSOS();
void handleStateLogic();
void triggerSOS(); // Wrapper: sendSOS() + chuyển sang WAIT_FOR_BUTTON
void setupWebServer();

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
      if (currentState == IDLE) {
        Serial.println("[HỆ THỐNG] Đang trong trạng thái AN TOÀN. ");
      }
      if (typeStr == "apiSendSafe") {
        float remaining_seconds = doc["safe_remaining_seconds"].as<float>();
        Blynk.virtualWrite(V5, remaining_seconds);
        Serial.printf("Thời gian duy trì còn lại: %.1f phút\n",
                      remaining_seconds / 60);
      } else if (typeStr == "apiSendFall" && fallDetected &&
                 currentState == IDLE) {
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

        Serial.println("[CẢNH BÁO] Phát hiện có người bị ngã trong nhà ");
        Serial.println("Thời gian: ");
        Serial.println(fallTimeStr);
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

// BƯỚC 3: Hàm kích hoạt Xiaozhi AI phát loa và nghe
void triggerXiaozhi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[LEVEL 1] Đang gọi điện cho Xiaozhi AI (đánh thức)...");
    HTTPClient http;
    http.begin(XIAOZHI_ALERT_URL);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("[LEVEL 1] Đã gửi lệnh cảnh báo cho Xiaozhi thành công.");
    } else {
      Serial.printf("[LEVEL 1] Lỗi gọi Xiaozhi: %s\n",
                    http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}

// BƯỚC 4: Các WebServer Handlers để nhận kết quả từ Xiaozhi
void handleSafeFromXiaozhi() {
  Serial.println("[LEVEL 1] Nhận lệnh AN TOÀN từ Xiaozhi!");
  server.send(200, "text/plain", "OK-Safe");
  sendSafeStatusToServer(); // Gửi trạng thái an toàn lên API
  resetSystem();            // Đưa hệ thống về IDLE
}

void handleSOSFromXiaozhi() {
  Serial.println("[LEVEL 1] Nhận lệnh KÊU CỨU KHẨN CẤP từ Xiaozhi!");
  server.send(200, "text/plain", "OK-SOS");
  triggerSOS(); // Gọi SOS hỏa tốc, chuyển trạng thái và gửi Telegram
}

void setupWebServer() {
  server.on("/safe", HTTP_GET, handleSafeFromXiaozhi);
  server.on("/sos", HTTP_GET, handleSOSFromXiaozhi);
  server.begin();
  Serial.println(
      "[HỆ THỐNG] WebServer nội bộ đã khởi chạy ở cổng 80 cho Xiaozhi.");
}

// BƯỚC 5: Hàm reset về trạng thái ban đầu cho hệ thống
void resetSystem() {
  Serial.println(
      "[HỆ THỐNG] Hệ thống sẽ quay trở lại  trạng thái tiếp tục giám sát ");
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
  Blynk.virtualWrite(V5, 0);         // Thiết lập lại thời gian an toàn
  Blynk.virtualWrite(V6, 0);         // Tắt đèn SOS khẩn cấp
  Blynk.virtualWrite(V4, 0);         // Tắt nút Gọi Cấp Cứu (Level 1)
  Blynk.virtualWrite(V11, 0);        // Tắt nút An Toàn
  Blynk.virtualWrite(V12, 0);        // Tắt nút SOS
}

// BƯỚC 9: Xây dựng hàm SOS() — Chỉ gửi thông tin, KHÔNG block vòng lặp
// BƯỚC 9: Xây dựng hàm SOS() — Gửi qua Telegram
void sendSOS() {
  Serial.println("==================================");
  Serial.println(
      "THỰC HIỆN GỬI SOS ĐẾN CƠ QUAN Y TẾ VÀ NGƯỜI NHÀ QUA TELEGRAM");

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // Bỏ qua SSL Certificate do Telegram dùng HTTPS

    String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
                 "/sendMessage";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // FIX 1: Chuyển sang định dạng HTML để an toàn với các ký tự đặc biệt trong
    // Link/Thời gian
    String textMsg = "🚨 <b>CẢNH BÁO PHÁT HIỆN NGÃ!</b> 🚨\n"
                     "⏰ <b>Thời gian:</b> " +
                     fallTimeStr +
                     "\n"
                     "📍 <b>Địa chỉ:</b> " +
                     String(DIA_CHI_NHA) +
                     "\n"
                     "📞 <b>SDT Người thân:</b> " +
                     SDT[0] + " / " + SDT[1] +
                     "\n"
                     "📸 <b>Trạng thái hiện tại:</b> <a href='" +
                     evidenceUrl + "'>Nhấn vào đây để xem ảnh</a>";

    Serial.println("Nội dung chuẩn bị gửi:\n" + textMsg);

    JsonDocument doc;
    doc["chat_id"] = TELEGRAM_CHAT_ID;
    doc["text"] = textMsg;
    doc["parse_mode"] = "HTML"; // FIX 2: Báo cho Telegram biết mình dùng HTML

    String payload;
    serializeJson(doc, payload);

    int httpResponseCode = http.POST(payload);

    // FIX 3: In ra chi tiết phản hồi từ Telegram để dễ bắt bệnh nếu còn lỗi
    if (httpResponseCode == 200) {
      Serial.println("[TELEGRAM] Đã gửi tin nhắn SOS thành công!");
    } else {
      Serial.printf("[TELEGRAM] Lỗi gửi SOS, HTTP Code: %d\n",
                    httpResponseCode);
      String response =
          http.getString(); // Lấy câu trả lời chi tiết của server Telegram
      Serial.println("Chi tiết lỗi từ Telegram: " + response);
    }
    http.end();
  } else {
    Serial.println("[LỖI] Không có kết nối WiFi để gửi Telegram SOS!");
  }
  Serial.println("==================================");
}
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
      Blynk.virtualWrite(V4, 1); // Bật nút Gọi Cấp Cứu (Level 1)
      level1Notified = true;
      level1Phase = L1_PLAYING; // Bắt đầu bằng phát audio
    }

    // Trong vòng 2 phút (120,000 ms), chờ phản hồi từ Xiaozhi
    if (millis() - level1StartTime < 120000) {

      if (level1Phase == L1_PLAYING) {
        // === GỌI XIAOZHI ===
        triggerXiaozhi();
        level1Phase =
            L1_LISTENING; // Chờ Xiaozhi trả kết quả ngầm qua WebServer
      }

      // Quá trình lắng nghe kết quả an toàn / báo khẩn cấp xử lý bằng ngầm
      // WebServer (loop)

      // Xử lý nút V0 (I'm OK) và V4 (Gọi Cấp Cứu) do BLYNK_WRITE() đảm nhiệm
    } else {
      // Hết 2 phút không có tín hiệu HTTP gọi về → Chuyển qua LEVEL 2
      Serial.println("[LEVEL 1] Hết 2 phút, nạn nhân không phản hồi. Chuyển "
                     "sang LEVEL 2.");
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

// [V4] Nút "Gọi Cấp Cứu" (Push) — Nạn nhân tự kích hoạt SOS tại LEVEL_1
BLYNK_WRITE(V4) {
  if (param.asInt() == 1 && currentState == LEVEL_1) {
    Serial.println("[LEVEL 1] Nạn nhân tự kích hoạt SOS khẩn cấp qua V4!");
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
  sendSafeStatusToServer(); // Gửi ngay lên server khi thay đổi giá trị
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

  // Removed Audio Enable config

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

    // --- Khởi chạy WebServer thay cho Audio/I2S ---
    setupWebServer();

    // Cấu hình Blynk (KHÔNG BLOCK HỆ THỐNG nếu WiFi rớt)
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(); // Sẽ cố gắng connect, nếu không được vẫn thoát ra chay
                     // tiep
    Serial.println("Hệ thống khởi động hoàn tất!");

  } else {
    Serial.println(
        "\n[LỖI] Không thể kết nối WiFi, vào mode offline theo dõi.");
    Serial.println("Hệ thống khởi động thất bại!");
  }
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

  // Xử lý các request từ Xiaozhi AI
  server.handleClient();
}