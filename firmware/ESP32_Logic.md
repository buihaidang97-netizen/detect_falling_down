# Logic hoạt động ESP32-S3 — Hệ thống phát hiện ngã
## Tổng quan
ESP32-S3 đóng vai trò **bộ điều khiển trung tâm**, liên tục truy vấn server AI để nhận kết quả phát hiện ngã, sau đó xử lý phản hồi theo **3 cấp độ cảnh báo leo thang**.
## Luồng hoạt động chính
```
IDLE → [Phát hiện ngã] → LEVEL 1 → [Hết 2 phút] → LEVEL 2 → [Hết 3 phút] → SOS
```
### 🟢 IDLE — Trạng thái chờ
- Gọi API server mỗi **2 giây** để kiểm tra trạng thái ngã
- Khi server báo `fall_detected = true` → chuyển sang **LEVEL 1**
### 🟡 LEVEL 1 — Cảnh báo nạn nhân (2 phút)
- **Phát âm thanh** cảnh báo từ thẻ SD qua loa (file `level1.mp3`)
- **Lắng nghe mic** 3 giây sau mỗi lần phát → nếu phát hiện âm thanh lớn → xác nhận **an toàn**
- Hiển thị nút **"I'm OK"** và **"Gọi Cấp Cứu"** trên app Blynk
- Nếu nạn nhân xác nhận ổn (giọng nói hoặc app) → **Reset về IDLE**, tạm dừng API 15 phút
- Nếu **hết 2 phút** không phản hồi → chuyển sang **LEVEL 2**
### 🟠 LEVEL 2 — Thông báo người nhà (3 phút)
- Gửi cảnh báo lên Blynk: thời gian ngã + ảnh bằng chứng từ camera
- Hiển thị nút **"An Toàn"** và **"SOS"** cho người nhà
- Người nhà bấm An Toàn → Reset về IDLE
- Người nhà bấm SOS hoặc **hết 3 phút** → tự động kích hoạt **SOS**
### 🔴 SOS — Gọi cấp cứu
- Gửi thông báo khẩn cấp đến danh sách số điện thoại đã cài sẵn
- Bật **đèn cảnh báo đỏ** liên tục trên app Blynk
- Chờ người nhà xác nhận **"Đã biết"** → Reset về IDLE
## Sơ đồ phần cứng liên quan






















| Chức năng | Giao tiếp | Chi tiết |
|---|---|---|
| **Loa** | I2S (NUM_0) | Phát MP3 qua BCLK, LRCK, DOUT |
| **Mic** | I2S (NUM_1) | Ghi âm qua DIN, 16kHz mono |
| **Thẻ SD** | SDIO 4-bit | Lưu file âm thanh level1/level2.mp3 |
| **App Blynk** | WiFi | Điều khiển & nhận thông báo từ xa |
| **Server AI** | HTTP REST | FastAPI trả kết quả phát hiện ngã |
