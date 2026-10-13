# Hướng Dẫn Vận Hành Hệ Thống Phát Hiện Ngã (Readme_To_Run_System)

Tài liệu này tổng hợp ngắn gọn các điều kiện mạng, lưu ý kỹ thuật và thứ tự các lệnh cần chạy để khởi động toàn bộ hệ thống từ Local AI Server đến Mạch phần cứng ESP32.

---

## 1. ĐIỀU KIỆN TIÊN QUYẾT (QUAN TRỌNG)

1. **Cùng Chung Mạng WiFi (Local Network):** 
   - Máy tính chạy AI (chạy `main.py`, `api/main.py`) và mạch ESP32 **BẮT BUỘC** phải kết nối chung vào 1 cục phát WiFi (Cùng 1 Router hoặc cùng 1 điện thoại phát 4G).
   - Nếu máy tính cắm dây mạng LAN tĩnh, hãy đảm bảo ESP32 đang bắt WiFi từ thiết bị phát sinh ra cái mạng LAN tĩnh đó.
2. **Cập Nhật IP Động Của Máy Chủ:**
   - Mỗi lần bạn đổi WiFi hoặc tắt mở lại máy, IP máy tính của bạn thường bị thay đổi (Kiểm tra bằng lệnh `ipconfig` trên Windows).
   - Bạn cần copy file `firmware/include/Config.example.h` thành `firmware/include/Config.h`.
   - Mở file `Config.h` và cập nhật lại biến `server_url`, `WIFI_SSID`, `WIFI_PASS` và `BLYNK_AUTH_TOKEN` theo đúng thông tin và IP mới của máy tính mình trước khi Nạp code xuống ESP32.
3. **Cài Đặt Đầy Đủ Thư Viện (Python Dependencies):**
   - Trước khi có thể khởi chạy AI Camera và API, bạn cần chắc chắn hệ thống đã cài các thư viện bắt buộc. Mở Terminal tại thư mục dự án và chạy:
   ```bash
   pip install ultralytics opencv-python fastapi uvicorn pydantic python-multipart
   ```
   *(Nhấn Enter và đợi quá trình tải về hoàn tất trong 1-2 phút).*

4. **Môi trường lập trình Terminal:**
   - Khi chạy qua các Terminal (VS Code hoặc Command Prompt/Powershell), hãy luôn chắc chắn đường dẫn cấp cha (Current Working Directory) đang trỏ về đúng thư mục gốc dự án: `d:\GitHub\Human-Falling-Detect-Tracks`

---

## 2. THỨ TỰ VÀ CÁC LỆNH KHỞI ĐỘNG HỆ THỐNG

Vui lòng khởi chạy theo đúng trình tự từ 1 đến 3 dưới đây:

### Bước 1: Khởi Động API Server Giao Tiếp (Làm Cầu Nối)
Mở một Terminal đầu tiên đang đứng ở thư mục gốc, gõ lệnh:
```bash
python api/main.py
```
*Dấu hiệu nhận biết thành công: Màn hình báo `Application startup complete.` và `Uvicorn running on http://0.0.0.0:8000`.*

### Bước 2: Khởi Động Camera AI (Theo Dõi Bệnh Nhân)
Tiếp tục mở thêm 1 dấu + (Terminal mới) thứ hai, vẫn đứng ở thư mục gốc, gõ lệnh chạy luồng AI chính. Tùy thuộc vào phần cứng Laptop/PC của bạn mà chọn 1 trong 2 lệnh sau:

+ **Đối với máy chạy bằng Card màn hình (GPU) - Khuyên dùng để tốc độ xử lý nhanh, mượt:**
```bash
python main.py --device gpu
```

+ **Đối với máy tính yếu hoặc chạy bằng Vi Xử Lý trung tâm (CPU):**
```bash
python main.py --device cpu
```

*Dấu hiệu nhận biết thành công: Camera hiện lên và AI bắt đầu vẽ khung hộp (Bounding Box) quét dáng người.*

### Bước 3: Nạp Và Chạy Mạch Hành Vi ESP32 (Firmware C++)
Mở Terminal thứ 3 di chuyển vào đúng thư mục code mạch (Lưu ý phải cd vào firmware):
```bash
cd firmware
```

- Để biên dịch và kiểm tra lỗi cú pháp (Build):
```bash
pio run
```

- Để nạp code cứng xuống trực tiếp dây cáp ESP32 (Upload):
```bash
pio run -t upload
```

- Tùy chọn (Bật Màn Hình Console để theo dõi Logs hoạt động của Còi/Mic trên ESP)
```bash
pio device monitor
```
*(Mẹo: Bạn hoàn toàn có thể dùng các nút mũi tên tương ứng ở góc dưới cùng thanh xanh-nước-biển của VSCode PlatformIO đối với 3 lệnh nạp mạch trên cho lẹ).*

### Bước 4: Chạy Giao Diện Web Demo và Chia Sẻ Qua Internet (Tuỳ Chọn)
Hệ thống đã được tích hợp sẵn một trang Web Demo cực kỳ đẹp mắt, cho phép người dùng thao tác tương tự như trên app Blynk.
Sau khi **Bước 1** đã chạy thành công, trang web tự động chạy ngầm ở địa chỉ `http://localhost:8000`. Bạn có thể đăng nhập bằng tài khoản `admin` / `admin123`.

Nếu bạn muốn chia sẻ trang Web này cho người khác hoặc Ban giám khảo truy cập từ điện thoại 4G:
1. Mở thêm 1 cửa sổ Terminal mới (Terminal thứ 4).
2. Chạy lệnh Ngrok với tên miền của bạn (ví dụ):
```bash
ngrok http --domain=liver-copier-fifth.ngrok-free.dev 8000
```
3. Gửi link `https://liver-copier-fifth.ngrok-free.dev` cho mọi người. 

---
**Chúc bạn triển khai Demo mượt mà và thành công tốt đẹp! 🎉**
