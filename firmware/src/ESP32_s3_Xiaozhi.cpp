#include <Arduino.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>

// ==========================================
// THÔNG SỐ KẾT NỐI VỚI HỆ THỐNG TRUNG TÂM
// ==========================================
const char *CENTRAL_ESP_IP = "192.168.1.100"; // ĐỔI ĐỊA CHỈ NÀY SAU

WebServer xiaozhiListener(80);

// ==========================================
// 1. NHẬN LỆNH KÍCH HOẠT TỪ TRUNG TÂM
// ==========================================
void sendGETtoCentral(String endpoint);

void handleAlertFromCentral()
{
  Serial.println("[XIAOZHI_API] HTTP GET: Xảy ra rơi ngã. Hệ thống yêu cầu "
                 "kiểm tra nạn nhân!");
  xiaozhiListener.send(200, "text/plain", "OK"); // Báo lại là "Tôi đã nhận"

  // --- [MÃ NGUỒN XIAOZHI CỦA BẠN SẼ GẮN Ở ĐÂY] ---
  // 1. Dừng phát nhạc / xóa bộ đệm.
  // 2. Chạy hàm TTS: playTTS("Bạn có ổn không?...");
  // 3. Khởi động Micro cưỡng bức để chờ người dùng nói phản hồi.
}

// Bắt buộc gọi khởi tạo này trong hàm setup() của Xiaozhi
void setupXiaozhiListener()
{
  xiaozhiListener.on("/alert", HTTP_GET, handleAlertFromCentral);
  xiaozhiListener.begin();
  Serial.println("[XIAOZHI_API] Lắng nghe báo động ở cổng 80...");
}

// ==========================================
// 2. XỬ LÝ TEXT VÀ BẮN TRẢ MODULE TRUNG TÂM
// ==========================================
// Hàm này mô phỏng phần Xử lý sau khi AI/STT trả lượng chữ (text) về
void processIntentAndSendToCentral(String userText)
{
  userText.toLowerCase();

  // Nạn nhân nói: tổi ổn, mình ổn, không sao đâu
  if (userText.indexOf("ổn") != -1 || userText.indexOf("không sao") != -1)
  {
    Serial.println("[XIAOZHI_API] KHÔNG NGUY HIỂM => Gọi /safe");
    sendGETtoCentral("/safe");
  }
  // Nạn nhân nói: cấp cứu, cứu rôi với, đau quá, sos
  else if (userText.indexOf("cấp cứu") != -1 || userText.indexOf("cứu") != -1 ||
           userText.indexOf("đau") != -1 || userText.indexOf("sos") != -1)
  {
    Serial.println("[XIAOZHI_API] YÊU CẦU TRỢ GIÚP => Gọi /sos");
    sendGETtoCentral("/sos");
  }
}

// Hàm hạ tầng để bắn HTTP GET chéo qua mạng LAN
void sendGETtoCentral(String endpoint)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(CENTRAL_ESP_IP) + endpoint;
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("[XIAOZHI_API] Gửi HTTP Endpoint %s - Code: %d\n",
                  endpoint.c_str(), httpCode);
    http.end();
  }
  else
  {
    Serial.println("[XIAOZHI_API] Lỗi: Không có kết nối WiFi WiFi");
  }
}

// ==========================================
// BẠN VUI LÒNG DÁN MÃ NGUỒN XIAOZHI HIỆN TẠI VÀO BÊN DƯỚI
// ĐỂ CHÚNG TA CÙNG LÊN KẾ HOẠCH GẮN VÀO NHA!
// ==========================================
