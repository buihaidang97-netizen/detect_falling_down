import requests
import os

class IoTHandler:
    def __init__(self, api_url="http://localhost:8000/report"):
        self.api_url = api_url

    def report_fall(self, track_id, image_path, action=None):
        """
        Gửi yêu cầu POST lên API server kèm theo ảnh và trạng thái hiện tại.
        """
        try:
            with open(image_path, 'rb') as f:
                files = {'image': (os.path.basename(image_path), f, 'image/jpeg')}
                data = {'track_id': track_id}
                if action:
                    data['action'] = action
                response = requests.post(self.api_url, data=data, files=files)
                if response.status_code == 200:
                    print(f"[IoT SEVICE] Đã gửi báo cáo thành công cho ID {track_id}")
                    return True
                else:
                    print(f"[ERROR] API trả về lỗi: {response.status_code}")
                    return False
        except Exception as e:
            print(f"[ERROR] Không thể gửi báo cáo API qua IoT Handler: {e}")
            return False
