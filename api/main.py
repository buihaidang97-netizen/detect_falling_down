from fastapi import FastAPI, File, UploadFile, Form
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel
from datetime import datetime
import shutil
import os
import time

app = FastAPI()

# Cấu hình lưu trữ ảnh
STORAGE_DIR = "api/storage"
if not os.path.exists(STORAGE_DIR):
    os.makedirs(STORAGE_DIR)

# Mount thư mục storage để có thể truy cập ảnh qua URL
app.mount("/evidence", StaticFiles(directory=STORAGE_DIR), name="evidence")

# Mount static files for Web Demo
app.mount("/static", StaticFiles(directory="api/static"), name="static")

@app.get("/")
async def root():
    return FileResponse("api/static/index.html")

from typing import Optional

# Trạng thái hiện tại
class FallStatus:
    def __init__(self):
        self.fall_detected = False
        self.fall_time = ""
        self.evidence_url = ""
        
        # Safe mode properties
        self.safe_mode: bool = False
        self.safe_start_time: float = 0.0
        self.safe_duration: float = 0.0  # in seconds internally
        self.last_action: Optional[str] = None

    def start_cooldown(self, duration_minutes: float):
        """ Kích hoạt độ an toàn (safe mode) """
        self.safe_mode = True
        self.safe_start_time = time.time()
        self.safe_duration = duration_minutes * 60.0
        self.fall_detected = False  # Reset cờ ngã khi vào mode an toàn
        print(f"[API] Đã kích hoạt Safe Mode trong {duration_minutes} phút.")

    def stop_cooldown(self):
        """ Hủy chế độ an toàn """
        self.safe_mode = False
        self.safe_start_time = 0.0
        self.safe_duration = 0.0
        print(f"[API] Đã tắt chế độ Safe Mode.")

    def is_safe_mode(self) -> bool:
        """ Kiểm tra xem hiện tại có đang trong thời gian an toàn không """
        if not self.safe_mode:
            return False
            
        elapsed = time.time() - self.safe_start_time
        if elapsed < self.safe_duration:
            return True
        else:
            # Hết thời gian an toàn, tự động tắt
            self.stop_cooldown()
            return False

    def get_remaining_cooldown(self) -> float:
        """ Lấy thời gian an toàn còn lại (giây) """
        if not self.is_safe_mode():
            return 0.0
        return max(0.0, self.safe_duration - (time.time() - self.safe_start_time))

current_status = FallStatus()

@app.post("/report")
async def report_fall(
    track_id: int = Form(...),
    image: UploadFile = File(...),
    action: Optional[str] = Form(None)
):
    """
    Endpoint dành cho AI báo cáo khi phát hiện ngã hoặc cập nhật action tĩnh.
    """
    current_status.last_action = action
    
    # Logic tự động tắt safe mode nếu ai đó đã khỏe và đứng dậy
    if current_status.is_safe_mode():
        if action and action not in ["Fall Down", "Lying Down", "Fall", "Lying"]:
            print(f"[API] Phát hiện người dùng đã dậy (Action: {action}). Tự động tắt Safe Mode.")
            current_status.stop_cooldown()
            current_status.fall_detected = False
            return {"status": "success", "message": "Safe mode disabled because user is up"}
        else:
            print(f"[API] Đang trong Safe Mode: Bỏ qua báo cáo ngã từ ID {track_id}")
            return {"status": "success", "message": "Ignored fall during safe mode"}

    # Lưu file ảnh
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"fall_{track_id}_{timestamp}.jpg"
    file_path = os.path.join(STORAGE_DIR, filename)
    
    with open(file_path, "wb") as buffer:
        shutil.copyfileobj(image.file, buffer)
    
    # Cập nhật trạng thái ngã
    current_status.fall_detected = True
    current_status.fall_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    current_status.evidence_url = f"/evidence/{filename}"
    
    print(f"[API] Đã nhận báo cáo ngã: ID {track_id}, Ảnh: {filename}, Action: {action}")
    
    return {"status": "success", "message": "Fall reported"}

@app.get("/gate")
async def get_gate_data():
    """
    Endpoint cổng chính dành cho ESP32 lấy dữ liệu.
    Dựa vào trạng thái cooldown sẽ trả về API tương ứng.
    """
    if current_status.is_safe_mode():
        # Đang trong thời gian an toàn
        remaining = current_status.get_remaining_cooldown()
        return {
            "type": "apiSendSafe",
            "safe": True,
            "safe_remaining_seconds": round(remaining, 1)
        }
    else:
        # Không trong thời gian an toàn, trả về trạng thái ngã trực tiếp (không tự gạt cờ)
        return {
            "type": "apiSendFall",
            "fall_detected": current_status.fall_detected,
            "fall_time": current_status.fall_time,
            "evidence": current_status.evidence_url
        }

class ResponseFromESP32(BaseModel):
    safe: bool
    safe_duration_minutes: float = 0.0

@app.post("/response-from-esp32")
async def handle_esp32_response(data: ResponseFromESP32):
    """
    Endpoint để ESP32 phản hồi trạng thái an toàn của người dùng.
    """
    if data.safe:
        current_status.start_cooldown(data.safe_duration_minutes)
        return {"status": "success", "message": f"Safe mode activated for {data.safe_duration_minutes} minutes"}
    else:
        current_status.stop_cooldown()
        return {"status": "success", "message": "Safe mode deactivated"}

@app.post("/reset")
async def reset_status():
    """
    Endpoint để reset trạng thái.
    """
    current_status.fall_detected = False
    current_status.fall_time = ""
    current_status.evidence_url = ""
    current_status.stop_cooldown()
    return {"status": "success"}

class LoginRequest(BaseModel):
    username: str
    password: str

@app.post("/api/login")
async def admin_login(req: LoginRequest):
    if req.username == "admin" and req.password == "admin123":
        return {"status": "success", "token": "admin-demo-token"}
    return {"status": "error"}

@app.get("/api/status")
async def get_current_status():
    return {
        "fall_detected": current_status.fall_detected,
        "evidence_url": current_status.evidence_url,
        "fall_time": current_status.fall_time
    }

@app.post("/api/mock-fall")
async def trigger_mock_fall():
    current_status.fall_detected = True
    current_status.fall_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    current_status.evidence_url = "https://upload.wikimedia.org/wikipedia/commons/3/3f/Placeholder_view_vector.svg"
    current_status.stop_cooldown()
    return {"status": "success"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
