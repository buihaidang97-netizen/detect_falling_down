import os
import cv2
import time
import torch
import argparse
import numpy as np

from CameraLoader import CamLoader, CamLoader_Q
from Yolo26PoseLoader import YOLO26PoseEstimator
from fn import draw_single

from Track.Tracker import Detection, Tracker
from ActionsEstLoader import TSSTG
from fall_logic import FallLogicManager

#source = '../Data/test_video/test7.mp4'
#source = '../Data/falldata/Home/Videos/video (2).avi'  # hard detect
#source = '../Data/falldata/Home/Videos/video (1).avi'
source = 0


def kpt2bbox(kpt, ex=20):
    """Get bbox that hold on all of the keypoints (x,y)
    kpt: array of shape `(N, 2)`,
    ex: (int) expand bounding box,
    """
    return np.array((kpt[:, 0].min() - ex, kpt[:, 1].min() - ex,
                     kpt[:, 0].max() + ex, kpt[:, 1].max() + ex))


if __name__ == '__main__':
    par = argparse.ArgumentParser(description='Human Fall Detection Demo (YOLO26-Pose).')
    par.add_argument('-C', '--camera', default=source,
                        help='Source of camera or video file path.')
    par.add_argument('--yolo_model', type=str, default='yolo26n-pose.pt',
                        help='YOLO26-Pose model file name.')
    par.add_argument('--show_detected', default=False, action='store_true',
                        help='Show all bounding box from detection.')
    par.add_argument('--show_skeleton', default=True, action='store_true',
                        help='Show skeleton pose.')
    par.add_argument('--save_out', type=str, default='',
                        help='Save display to video file.')
    par.add_argument('--device', type=str, default='cuda',
                        help='Device to run model on cpu or cuda.')
    args = par.parse_args()

    device = args.device

    # YOLO26-POSE MODEL (unified detection + pose).
    pose_detector = YOLO26PoseEstimator(
        model_path=args.yolo_model,
        device=device,
        conf_thres=0.45
    )

    # Tracker.
    max_age = 30
    tracker = Tracker(max_age=max_age, n_init=3)

    # Actions Estimate (ST-GCN — kept unchanged).
    action_model = TSSTG(device=device)

    # Fall Logic Manager (unified states + 45s timer).
    fall_manager = FallLogicManager(persistence_threshold=10) #/////////////////////////////////////////////////////////////////////////////////////////////

    cam_source = args.camera
    if type(cam_source) is str and os.path.isfile(cam_source):
        # Use loader thread with Q for video file.
        cam = CamLoader_Q(cam_source, queue_size=1000).start()
    else:
        # Use normal thread loader for webcam.
        if isinstance(cam_source, int):
            cam = CamLoader(cam_source).start()
        else:
            cam = CamLoader(int(cam_source) if cam_source.isdigit() else cam_source).start()

    outvid = False
    if args.save_out != '':
        outvid = True
        codec = cv2.VideoWriter_fourcc(*'MJPG')
        writer = cv2.VideoWriter(args.save_out, codec, 30, (0, 0))  # Size set on first frame
        writer_initialized = False

    fps_time = time.time()
    f = 0
    while cam.grabbed():
        f += 1
        frame = cam.getitem()
        image = frame.copy()

        # Detect humans and estimate pose in one pass with YOLO26.
        # Each result has: keypoints (13,2), kp_score (13,1), bbox (4,), bbox_score
        pose_results = pose_detector.detect_and_pose(frame)

        # Predict each tracks bbox of current frame from previous frames information with Kalman filter.
        tracker.predict()

        # Create Detections object for tracking.
        detections = []
        for ps in pose_results:
            kpts = ps['keypoints']       # (13, 2)
            scores = ps['kp_score']      # (13, 1)
            bbox = ps['bbox']            # (4,)

            # Combine keypoints (x,y) and scores into (13, 3) format
            kpts_with_score = np.concatenate((kpts, scores), axis=1)  # (13, 3)

            det_bbox = kpt2bbox(kpts, ex=20)
            avg_score = scores.mean()

            detections.append(Detection(det_bbox, kpts_with_score, avg_score))

        # VISUALIZE detected bboxes.
        if args.show_detected:
            for ps in pose_results:
                bb = ps['bbox']
                frame = cv2.rectangle(frame, (int(bb[0]), int(bb[1])), (int(bb[2]), int(bb[3])), (0, 0, 255), 1)

        # Update tracks by matching each track information of current and previous frame or
        # create a new track if no matched.
        tracker.update(detections)

        # Predict Actions of each track.
        for i, track in enumerate(tracker.tracks):
            if not track.is_confirmed():
                continue

            track_id = track.track_id
            bbox = track.to_tlbr().astype(int)
            center = track.get_center().astype(int)

            action = 'pending..'
            clr = (0, 255, 0)
            # Use 30 frames time-steps to prediction.
            if len(track.keypoints_list) == 30:
                pts = np.array(track.keypoints_list, dtype=np.float32)
                out = action_model.predict(pts, frame.shape[:2])
                action_name = action_model.class_names[out[0].argmax()]
                
                # Apply unified fall logic.
                display_action, elapsed = fall_manager.update(track_id, action_name, frame)
                
                action = '{}: {:.2f}%'.format(display_action, out[0].max() * 100)
                if elapsed > 0:
                    action += ' ({:.1f}s)'.format(elapsed)
                
                if display_action == 'Fall':
                    clr = (255, 0, 0)
                elif action_name == 'Lying Down':
                    clr = (255, 200, 0)

            # VISUALIZE.
            if track.time_since_update == 0:
                if args.show_skeleton:
                    frame = draw_single(frame, track.keypoints_list[-1])
                frame = cv2.rectangle(frame, (bbox[0], bbox[1]), (bbox[2], bbox[3]), (0, 255, 0), 1)
                frame = cv2.putText(frame, str(track_id), (center[0], center[1]), cv2.FONT_HERSHEY_COMPLEX,
                                    0.4, (255, 0, 0), 2)
                frame = cv2.putText(frame, action, (bbox[0] + 5, bbox[1] + 15), cv2.FONT_HERSHEY_COMPLEX,
                                    0.4, clr, 1)

        # Cleanup inactive tracks from fall manager.
        active_ids = [t.track_id for t in tracker.tracks if t.is_confirmed()]
        fall_manager.cleanup_inactive_tracks(active_ids)

        # Show Frame.
        frame = cv2.resize(frame, (0, 0), fx=2., fy=2.)
        frame = cv2.putText(frame, '%d, FPS: %f' % (f, 1.0 / (time.time() - fps_time)),
                            (10, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        fps_time = time.time()

        if outvid:
            if not writer_initialized:
                h, w = frame.shape[:2]
                writer = cv2.VideoWriter(args.save_out, codec, 30, (w, h))
                writer_initialized = True
            writer.write(frame)

        cv2.imshow('frame', frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Clear resource.
    cam.stop()
    if outvid:
        writer.release()
    cv2.destroyAllWindows()
