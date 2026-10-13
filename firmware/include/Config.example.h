#ifndef CONFIG_H
#define CONFIG_H

// --- Blynk Credentials ---
#define BLYNK_TEMPLATE_ID "TMPL6nXDjldOr"
#define BLYNK_TEMPLATE_NAME "FallDetection"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"

// --- WiFi Credentials ---
const char *ssid = "YOUR_WIFI_SSID";
const char *pass = "YOUR_WIFI_PASSWORD";

// --- API Server Settings ---
// LƯU Ý: Thay đổi IP này thành IP máy tính của bạn (chạy FastAPI)
// Ví dụ: "http://192.168.1.10:8000"
const char *server_url = "http://YOUR_API_IP:8000/gate";
const char *reset_url = "http://YOUR_API_IP:8000/reset";

// --- Notification Settings ---
#define NOTIFICATION_INTERVAL 10000 // 20 giây một lần ở Level 2
#define LEVEL_1_TIMEOUT 20000       // 2 phút - 20s demo
#define LEVEL_2_TIMEOUT 30000       // 5 phút - 30s dome

// --- Telegram & Xiaozhi AI Settings ---
#define TELEGRAM_BOT_TOKEN "8271771967:AAHmku0WWhAZnFm-GALfTEhB80FSmkwLKXQ"
#define TELEGRAM_CHAT_ID "5348036208"
#define DIA_CHI_NHA "Nhà số 123, Đường ABC, Quận XYZ, TP. Hà Nội"
#define XIAOZHI_ALERT_URL "http://192.168.1.100/alert" // Thay IP Xiaozhi

#endif
