"""YOLO26-Pose Unified Detector and Pose Estimator.

Replaces both DetectorLoader.py (TinyYOLOv3) and PoseEstimateLoader.py (AlphaPose)
with a single YOLO26-Pose model that does detection + pose estimation in one pass.

The output keypoints are mapped from YOLO's 17-point COCO format to the 13-point
'coco_cut' format expected by the ST-GCN action recognition model.
"""

import numpy as np
from ultralytics import YOLO


# YOLO COCO 17 keypoints → ST-GCN coco_cut 13 keypoints mapping.
# We keep: Nose(0), LShoulder(5), RShoulder(6), LElbow(7), RElbow(8),
#           LWrist(9), RWrist(10), LHip(11), RHip(12), LKnee(13),
#           RKnee(14), LAnkle(15), RAnkle(16)
# We drop: LEye(1), REye(2), LEar(3), REar(4)
YOLO_TO_COCO_CUT = [0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]


class YOLO26PoseEstimator:
    """Unified person detection and pose estimation using YOLO26-Pose.

    Args:
        model_path: (str) Path to YOLO26-Pose model weights.
        device: (str) Device to run model on ('cpu' or 'cuda').
        conf_thres: (float) Minimum confidence threshold for detections.
    """

    def __init__(self, model_path='yolo26n-pose.pt', device='cuda', conf_thres=0.45):
        self.device = device
        self.conf_thres = conf_thres
        self.model = YOLO(model_path)
        # Warm up the model
        self.model.predict(
            source=np.zeros((64, 64, 3), dtype=np.uint8),
            device=self.device,
            verbose=False
        )

    def detect_and_pose(self, frame):
        """Run detection + pose estimation on a single frame.

        Args:
            frame: (numpy array) BGR image from OpenCV.

        Returns:
            list of dict, each containing:
                - 'keypoints': np.array of shape (13, 2) — x, y coordinates
                - 'kp_score': np.array of shape (13, 1) — confidence scores
                - 'bbox': np.array of shape (4,) — [x1, y1, x2, y2]
                - 'bbox_score': float — detection confidence
        """
        results = self.model.predict(
            source=frame,
            device=self.device,
            conf=self.conf_thres,
            verbose=False
        )

        detections = []
        if results and len(results) > 0:
            result = results[0]

            if result.keypoints is not None and result.boxes is not None:
                keypoints_data = result.keypoints.data.cpu().numpy()  # (N, 17, 3)
                boxes = result.boxes.xyxy.cpu().numpy()               # (N, 4)
                box_confs = result.boxes.conf.cpu().numpy()            # (N,)

                for i in range(len(boxes)):
                    kpts_17 = keypoints_data[i]  # (17, 3) — x, y, conf

                    # Map 17 → 13 keypoints (drop eyes and ears)
                    kpts_13 = kpts_17[YOLO_TO_COCO_CUT]  # (13, 3)

                    keypoints_xy = kpts_13[:, :2]   # (13, 2)
                    kp_scores = kpts_13[:, 2:3]     # (13, 1)

                    detections.append({
                        'keypoints': keypoints_xy,
                        'kp_score': kp_scores,
                        'bbox': boxes[i],
                        'bbox_score': float(box_confs[i])
                    })

        return detections
