import pytest
from fastapi.testclient import TestClient
import time
from api.main import app, current_status
import io

client = TestClient(app)

@pytest.fixture(autouse=True)
def reset_status_before_test():
    """Reset trạng thái trước mỗi test."""
    current_status.stop_cooldown()
    current_status.fall_detected = False
    current_status.fall_time = ""
    current_status.evidence_url = ""
    current_status.last_action = None

def test_gate_default_returns_fall_data():
    """Mặc định /gate trả apiSendFall."""
    response = client.get("/gate")
    assert response.status_code == 200
    data = response.json()
    assert data["type"] == "apiSendFall"
    assert data["fall_detected"] is False

def test_report_fall_updates_status():
    """POST /report cập nhật trạng thái ngã."""
    # Mock image file
    file_content = b"fake image content"
    file = io.BytesIO(file_content)
    
    response = client.post(
        "/report",
        data={"track_id": 1, "action": "Fall Down"},
        files={"image": ("test.jpg", file, "image/jpeg")}
    )
    assert response.status_code == 200
    
    # Kiểm tra cổng get
    gate_res = client.get("/gate")
    data = gate_res.json()
    assert data["type"] == "apiSendFall"
    assert data["fall_detected"] is True
    assert "fall_1_" in data["evidence"]

def test_response_from_esp32_activates_safe_mode():
    """POST /response-from-esp32 kích hoạt safe mode."""
    # EPS gửi safe_duration 1 phút
    res = client.post("/response-from-esp32", json={"safe": True, "safe_duration_minutes": 1.0})
    assert res.status_code == 200
    
    assert current_status.is_safe_mode() is True
    
    # Lúc này /gate phải trả apiSendSafe
    gate_res = client.get("/gate")
    data = gate_res.json()
    assert data["type"] == "apiSendSafe"
    assert data["safe"] is True
    assert 59.0 <= data["safe_remaining_seconds"] <= 60.0

def test_action_change_cancels_safe_mode():
    """Khi AI gửi action khác Fall/Lying -> tự tắt safe mode."""
    # Kích hoạt safe mode
    client.post("/response-from-esp32", json={"safe": True, "safe_duration_minutes": 1.0})
    assert current_status.is_safe_mode() is True
    
    # AI báo cáo hành động mới: Standing
    file_content = b"fake image content"
    file = io.BytesIO(file_content)
    
    client.post(
        "/report",
        data={"track_id": 1, "action": "Standing"},
        files={"image": ("test.jpg", file, "image/jpeg")}
    )
    
    # Kiểm tra safe mode đã tự tắt
    assert current_status.is_safe_mode() is False
    
    # /gate phải trả lại apiSendFall
    gate_res = client.get("/gate")
    assert gate_res.json()["type"] == "apiSendFall"

def test_cooldown_expiry_returns_to_fall_mode():
    """Hết thời gian an toàn -> /gate trở lại apiSendFall."""
    # Chỉnh safe_duration nội bộ trực tiếp cho nhanh
    current_status.start_cooldown(1.0 / 60.0) # 1 giây = 1/60 phút
    assert current_status.is_safe_mode() is True
    
    time.sleep(1.1) # Chờ hơn 1s
    
    # Kiểm tra
    assert current_status.is_safe_mode() is False
    gate_res = client.get("/gate")
    assert gate_res.json()["type"] == "apiSendFall"

def test_reset_clears_all_state():
    """POST /reset xóa trạng thái và tắt safe mode."""
    client.post("/response-from-esp32", json={"safe": True, "safe_duration_minutes": 1.0})
    
    res = client.post("/reset")
    assert res.status_code == 200
    
    assert current_status.is_safe_mode() is False
    assert current_status.fall_detected is False
