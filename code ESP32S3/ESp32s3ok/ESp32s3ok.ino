// --- Blynk Credentials ---
#define BLYNK_TEMPLATE_ID "TMPL6nXDjldOr"
#define BLYNK_TEMPLATE_NAME "FallDetection"
#define BLYNK_AUTH_TOKEN "KveCGf33u8V1WhImVQS2E3YHGdjnCSYy"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <WiFi.h>


// --- WiFi Credentials ---
const char ssid[] = "Xiaomi";
const char pass[] = "12345679@";

// --- API Server Settings ---
// LƯU Ý: Thay đổi IP này thành IP máy tính của bạn (chạy FastAPI)
// Ví dụ: "http://192.168.1.10:8000"
const char *server_url = "http://10.128.150.225:8000/status";
const char *reset_url  = "http://10.128.150.225:8000/reset";

// --- Notification Settings ---
#define NOTIFICATION_INTERVAL 20000 // 20 giây một lần ở Level 2
#define LEVEL_1_TIMEOUT 120000      // 2 phút
#define LEVEL_2_TIMEOUT 300000      // 5 phút

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
bool isApiPaused = false;
unsigned long apiPauseStartTime = 0;
const unsigned long API_PAUSE_DURATION = 15 * 60 * 1000; // 15 phút

unsigned long level1StartTime = 0;
bool level1Notified = false;
unsigned long level2StartTime = 0;
bool level2Notified = false;

int blynk_v0_state = 0; // Lưu trạng thái nút V0 trên blynk

// Danh sách số điện thoại nhận SOS (Cài đặt cố định)
String SDT[] = {"0123456789", "0987654321"};

// Khai báo hàm
void checkApiStatus();
void pauseApiRetrieval();
void playAudio(int caseType);
bool checkVoiceCommand();
void resetSystem();
void sendSOS();
void handleStateLogic();

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

      bool fallDetected = doc["fall_detected"];

      if (fallDetected && currentState == IDLE) {
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

// BƯỚC 2: Hàm không truy xuất API trong 15 phút
void pauseApiRetrieval() {
  isApiPaused = true;
  apiPauseStartTime = millis();
  Serial.println("[HỆ THỐNG] Tạm dừng truy xuất API trong 15 phút.");
}

// BƯỚC 3: Hàm phát âm thanh dựa trên case
void playAudio(int caseType) {
  // Low level enable Audio dựa theo bảng I2S En
  digitalWrite(I2S_EN_PIN, LOW);

  if (caseType == 1) {
    // Phát âm thanh case 1
    // TODO: Bổ sung code đọc file am thanh 1 ra I2S tại đây
  } else if (caseType == 2) {
    // Phát âm thanh case 2
    // TODO: Bổ sung code đọc file am thanh 2 ra I2S tại đây
  }

  // High level disable Audio sau khi dùng
  digitalWrite(I2S_EN_PIN, HIGH);
}

// BƯỚC 4: Hàm ghi âm thanh bằng mic của esp
bool checkVoiceCommand() {
  // Ghi âm bằng I2S (I08 làm DIN) và phân tích giọng nói "tôi ổn"
  // Logic giả lập trả về False hiện tại
  return false;
}

// BƯỚC 5: Hàm reset về trạng thái ban đầu cho hệ thống
void resetSystem() {
  Serial.println("[HỆ THỐNG] Resetting system to IDLE...");
  currentState = IDLE;
  level1Notified = false;
  blynk_v0_state = 0;
  fallTimeStr = "";
  evidenceUrl = "";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(reset_url);
    http.POST("");
    http.end();
  }

  // Khôi phục các chân trên app Blynk về trạng thái an toàn ban đầu
  Blynk.virtualWrite(V0, 0);
  Blynk.virtualWrite(V1, "");
  Blynk.virtualWrite(V2, "");
  Blynk.setProperty(V3, "urls", ""); // Xóa hình ảnh nếu hỗ trợ
  Blynk.virtualWrite(V6, 0);
  Blynk.virtualWrite(V11, 0);
  Blynk.virtualWrite(V12, 0);
}

// BƯỚC 9: Xây dựng hàm SOS()
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
}

// BƯỚC 6, 7, 8: Hàm xử lý loop logic cho hệ thống
void handleStateLogic() {
  switch (currentState) {
  // PHÁT HIỆN NGÃ VÀ NẠN NHÂN CÒN NHẬN THỨC
  case LEVEL_1: {
    // Nếu chưa thông báo thì đẩy lên serial và app 1 lần duy nhất
    if (!level1Notified) {
      blynk_v0_state = 1;
      Blynk.virtualWrite(V0, 1);
      Serial.println("[LEVEL 1] Hệ thống phát hiện ngã!");
      level1Notified = true;
    }

    // Liên tục chạy trong 2 phút (120,000 ms), kết hợp chạy âm thanh và mic
    if (millis() - level1StartTime < 120000) {
      Blynk.run(); // Luôn duy trì kết nối Blynk liên tục tránh timeout
      playAudio(1);                       // Phát âm thanh cảnh báo level 1
      bool saySafe = checkVoiceCommand(); // Ghi âm và đối chiếu (Đảm bảo logic bên trong hàm này Non-blocking - không dùng delay)

      // Hoặc nói tôi ổn, hoặc nhấn V0 trên đt dể về 0
      if (saySafe || blynk_v0_state == 0) {
        Serial.println(
            "[HỆ THỐNG] Xác nhận an toàn từ giọng nói hoặc Blynk V0!");
        resetSystem();
      }
    } else {
      // Hết 2 phút mà không có xác nhận -> Chuyển qua LEVEL 2
      currentState = LEVEL_2;
      level2StartTime = millis();
      level2Notified = false;
    }
    break;
  }
  // NẠN NHÂN KHÔNG NHẬN THỨC
  case LEVEL_2: {
    // BƯỚC 7 phần 1: Gửi thông báo label và picture
    if (!level2Notified) {
      Blynk.virtualWrite(V1, "Cảnh báo: Có người bị ngã hãy check cam ngay");
      Blynk.virtualWrite(V2, fallTimeStr);
      Blynk.setProperty(V3, "urls", evidenceUrl); // Tải url hình ảnh

      level2Notified = true;
      Serial.println("[LEVEL 2] Đã đẩy cảnh báo lên Blynk (V1, V2, V3)");
    }

    // BƯỚC 8: Quá 5 phút (300,000 ms) mà V11, V12 vẫn 0 (chưa ai bấm)
    if (millis() - level2StartTime >= 300000) {
      Serial.println("[LEVEL 2] Quá 5 phút không phản hồi. Gửi SOS khẩn cấp!");
      Blynk.virtualWrite(
          V6, 1); // Cảnh báo khẩn cấp (v6=1) nút đỏ báo nguy hiểm trên app
      sendSOS();
      currentState = WAIT_FOR_BUTTON;
    }
    break;
  }

  case IDLE:
  default:
    break;
  }
}

// ================= XỬ LÝ SỰ KIỆN TỪ BLYNK APP =================

// Nút "Tôi ổn" trong quá trình FALL_DETECTED (pin V0)
BLYNK_WRITE(V0) { blynk_v0_state = param.asInt(); }

// BƯỚC 7 phần 2: Nút "An toàn" trên blynk (pin V11)
BLYNK_WRITE(V11) {
  if (param.asInt() == 1 && currentState == LEVEL_2) {
    pauseApiRetrieval(); // Ngưng truy xuất api
    resetSystem();       // Reset hệ thống
  }
}

// BƯỚC 7 phần 3: Nút "SOS" (pin V12)
BLYNK_WRITE(V12) {
  if (param.asInt() == 1 && currentState == LEVEL_2) {
    sendSOS();     // Gửi thông tin y tế, người nhà
    resetSystem(); // Reset hệ thống
  }
}

// Huỷ trạng thái SOS/WAIT_FOR_BUTTON bằng nút V6 trên Blynk
BLYNK_WRITE(V6) {
  int v6_state = param.asInt();
  // Chỉ khi người nhà bấm lại nút V6 về 0 thì mới kích hoạt reset
  if (v6_state == 0 && currentState == WAIT_FOR_BUTTON) {
    Serial.println("[HỆ THỐNG] Người nhà đã xác nhận đã biết tin và xử lý từ "
                   "Blynk App (Nút V6 = 0).");
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
    
    // Cấu hình Blynk (KHÔNG BLOCK HỆ THỐNG nếu WiFi rớt)
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(); // Sẽ cố gắng connect, nếu không được vẫn thoát ra chay tiep
  } else {
    Serial.println("\n[LỖI] Không thể kết nối WiFi, vào mode offline theo dõi.");
  }
  
  Serial.println("Hệ thống khởi động hoàn tất!");
}

void loop() {
  Blynk.run();

  // BƯỚC 2: Kiểm tra cờ tạm dừng truy xuất API (15 phút)
  if (isApiPaused) {
    if (millis() - apiPauseStartTime > API_PAUSE_DURATION) {
      isApiPaused = false;
      Serial.println(
          "[HỆ THỐNG] Đã hết 15 phút tạm dừng API. Bắt đầu theo dõi lại.");
    }
  }

  // Kiểm tra API mỗi 2 giây nếu đang ở trạng thái IDLE và không bị tạm dừng
  static unsigned long lastApiCheck = 0;
  if (!isApiPaused && (millis() - lastApiCheck > 2000)) {
    lastApiCheck = millis();
    if (currentState == IDLE) {
      checkApiStatus();
    }
  }

  handleStateLogic();
}