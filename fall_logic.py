import time
import cv2
import os
from iot_handler import IoTHandler

class FallLogicManager:
    def __init__(self, persistence_threshold=45):
        """
        Quản lý logic nhận diện ngã thống nhất.
        Gộp 'Lying Down' và 'Fall Down' thành một trạng thái 'Fall'.
        Chỉ kích hoạt cảnh báo nếu trạng thái 'Fall' duy trì > persistence_threshold giây.
        """
        # Lưu trạng thái của từng track: track_id -> {'start_time': timestamp, 'reported': bool}
        self.track_states = {}
        self.persistence_threshold = persistence_threshold
        
        # Handler xử lý gửi dữ liệu sang API/IoT
        self.iot_handler = IoTHandler(api_url="http://localhost:8000/report")
        
        # Thư mục lưu bằng chứng
        self.evidence_dir = 'evidence'
        if not os.path.exists(self.evidence_dir):
            os.makedirs(self.evidence_dir)

    def update(self, track_id, action_name, frame):
        """
        Cập nhật trạng thái cho một track.
        Trả về (Action Hiển thị, Thời gian đã duy trì)
        """
        current_time = time.time()
        
        # Thống nhất trạng thái: Nằm hoặc Ngã -> 'Ngã'
        is_current_fall = action_name in ['Lying Down', 'Fall Down']
        
        if is_current_fall:
            if track_id not in self.track_states:
                # Bắt đầu đếm thời gian khi phát hiện Ngã/Nằm
                self.track_states[track_id] = {'start_time': current_time, 'reported': False}
                elapsed_time = 0
            else:
                elapsed_time = current_time - self.track_states[track_id]['start_time']
            
            # Nếu vượt quá ngưỡng 45s và chưa báo cáo
            if elapsed_time >= self.persistence_threshold and not self.track_states[track_id]['reported']:
                self.trigger_alert(track_id, frame, elapsed_time, action_name)
                self.track_states[track_id]['reported'] = True
            
            return 'Fall', elapsed_time
        else:
            # Nếu chuyển sang tư thế khác (Đi, Đứng, Ngồi...), reset bộ đếm
            if track_id in self.track_states:
                del self.track_states[track_id]
            return action_name, 0

    def trigger_alert(self, track_id, frame, elapsed_time, action_name):
        """
        Kích hoạt cảnh báo và chụp ảnh.
        """
        print(f"[CẢNH BÁO] Người dùng ID {track_id} đã ngã trong {elapsed_time:.1f} giây!")
        
        # Lưu ảnh bằng chứng
        timestamp = int(time.time())
        filename = f"{self.evidence_dir}/fall_event_{track_id}_{timestamp}.jpg"
        cv2.imwrite(filename, frame)
        print(f"[INFO] Đã lưu ảnh bằng chứng: {filename}")
        
        # Gọi IoT Handler để báo cáo sang API server kèm hành động
        self.iot_handler.report_fall(track_id, filename, action=action_name)


    def cleanup_inactive_tracks(self, active_track_ids):
        """
        Dọn dẹp các track không còn xuất hiện trong khung hình để giải phóng bộ nhớ.
        """
        current_tracked_ids = list(self.track_states.keys())
        for tid in current_tracked_ids:
            if tid not in active_track_ids:
                del self.track_states[tid]

def get_current_summary(tracks_metadata):
    """
    Hàm tiện ích lấy tất cả trạng thái hiện tại hiển thị trên khung hình.
    """
    summary = []
    for tid, state in tracks_metadata.items():
        summary.append({
            'track_id': tid,
            'action': state['action'],
            'duration': state['duration']
        })
    return summary
