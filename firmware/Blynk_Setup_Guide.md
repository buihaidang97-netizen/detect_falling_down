# Hướng dẫn chi tiết thiết lập giao diện Blynk (Cập nhật 18/03/2026)

Tài liệu này hướng dẫn cách cấu hình **Blynk Console** và **Blynk Mobile App** để khớp 100% với logic code trong `firmware/src/main.cpp`.

---

## 1. Cấu hình Datastream (Trên giao diện Web)

Truy cập **Blynk Console -> Templates -> FallDetection -> Datastreams**, tạo các biến sau:

| Tên Datastream | Virtual Pin | Kiểu dữ liệu | Giá trị min/max | Đặc điểm quan trọng |
| :--- | :--- | :--- | :--- | :--- |
| **I_AM_OK** | `V0` | Integer | `0` to `1` | Dành cho nạn nhân (Level 1). Code tự bật V0=1 khi ngã. |
| **Terminal_CanhBao** | `V1` | String | - | Widget **Terminal**. Hiển thị lời nhắc và giờ ngã (Level 2). |
| **Hinh_Anh** | `V3` | Image | - | Widget **Image Gallery**. Code tự tải URL ảnh. |
| **SOS_Confirm** | `V6` | Integer | `0` to `1` | Dùng để xác nhận Đã Biết tin cấp cứu và Reset hệ thống. |
| **Nut_An_Toan_NN** | `V11` | Integer | `0` to `1` | Nút dành cho người nhà xác nhận nạn nhân ổn (Level 2). |
| **Nut_Goi_SOS** | `V12` | Integer | `0` to `1` | Nút chung để nạn nhân tự bấm hoặc người nhà bấm gọi SOS ở cả Level 1 và Level 2. |

---

## 2. Thiết lập Giao diện Mobile App (Widgets)

Sắp xếp màn hình theo 3 khu vực chức năng:

### 🟥 KHU VỰC 1: Dành cho Nạn nhân (Level 1)
*   **Button (V0 - I'm OK):** 
    - Cài đặt dạng **Switch**.
    - Màu: Cam/Đỏ (khi ON), Xanh (khi OFF).
    - Ý nghĩa: Khi thấy nút này hiện Đỏ, nạn nhân gạt về Xanh để báo "Tôi ổn".

### 🟧 KHU VỰC 2: Thông tin Bằng chứng (Level 2)
*   **Terminal (V1):** Hiển thị lời nhắc và mốc thời gian (gộp từ V1, V2 cũ).
*   **Image Gallery (V3):** Hiển thị ảnh chụp từ AI Camera.

### 🟩 KHU VỰC 3: Dành cho Người nhà (Xử lý & Reset)
*   **Button (V11 - An Toàn):** Dạng **Push**. Màu Xanh lá. (Dùng ở Level 2).
*   **Button (V12 - Gọi Cấp Cứu SOS):** Dạng **Push**. Màu Đỏ đậm. (Dùng xuyên suốt ở Level 1 & Level 2).
*   **Button (V6 - Xác nhận SOS):** 
    - Cài đặt dạng **Push**.
    - Ý nghĩa: Sau khi SOS đã nổ, người nhà bấm nút này để xác nhận đã biết và đưa hệ thống về trạng thái bình thường.

---

## 3. Quy trình vận hành thực tế

1.  **AI phát hiện ngã:** Hệ thống bật **V0** (I'm OK) và **V12** (Gọi cấp cứu SOS).
2.  **Trong Giai đoạn 1 (20 giây):** 
    - Nạn nhân gạt **V0** về 0 -> Hệ thống tự tắt, khôi phục an toàn.
    - Nạn nhân bấm **V12** -> Gọi SOS ngay.
3.  **Hết 20 giây:** Chuyển sang Level 2, hiện ảnh (**V3**), bật nút **V11** (nút **V12** vẫn tiếp tục hoạt động).
4.  **Trong 30 giây tiếp theo:**
    - Người nhà bấm **V11** -> Báo an toàn, reset tất cả.
    - Người nhà bấm **V12** -> Gọi SOS ngay.
5.  **Hết 3 phút:** Tự động gọi SOS, bật đèn **V6** nhấp nháy đỏ.
6.  **Kết thúc:** Bấm **V6** để dọn dẹp màn hình và quay lại chờ (IDLE).
