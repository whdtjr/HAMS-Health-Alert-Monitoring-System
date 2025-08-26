import cv2
import mediapipe as mp
import tensorflow as tf # TensorFlow가 GPU를 사용하는지 확인하기 위함
import os
import numpy as np
import re
from collections import deque
import pandas as pd

mp_face_mesh = mp.solutions.face_mesh
mp_drawing = mp.solutions.drawing_utils
mp_drawing_style = mp.solutions.drawing_styles
directory_path = './DROZY/videos_i8'

left_eye_landmarks_points = [133, 173, 157, 158, 159, 160, 161, 246, 33, 7, 163, 144, 145, 153, 154, 155]
right_eye_landmarks_points = [362, 398, 384, 385, 386, 387, 388, 466, 263, 249, 390, 373, 374, 380, 381, 382]
mouth_landmarks_points = [308, 415, 310, 311, 312, 13, 82, 81, 80, 191, 78, 95, 88, 178, 87, 14, 317, 402, 318, 324]

MAR_ratio_time = 60
PERCLOS_time = 30

frame_count = 0
left_EAR_baseline = 0.0
right_EAR_baseline = 0.0
MAR_baseline = 0.0
baseline_sample_limit = 60

EAR_THRESHOLD = 0
MAR_THRESHOLD = 0

is_blinking = False
is_yawning = False

sequence_data = []
feature_per_video = []


def PERCLOS_rate(fps: int, is_blinking: bool):
    window_size = PERCLOS_time * fps
    if not hasattr(PERCLOS_rate, "blink_window"):
        PERCLOS_rate.blink_window = deque(maxlen=window_size)

    PERCLOS_rate.blink_window.append(1 if is_blinking else 0)
    
    perclos_rate = sum(PERCLOS_rate.blink_window) / len(PERCLOS_rate.blink_window) if len(PERCLOS_rate.blink_window) > 0 else 0

    return perclos_rate

def Yawn_rate(fps : int, is_yawning : bool):
    window_size = MAR_ratio_time * fps
    if not hasattr(Yawn_rate, "yawn_window"):
        Yawn_rate.yawn_window = deque(maxlen=window_size)

    Yawn_rate.yawn_window.append(1 if is_yawning else 0)
    
    yawn_rate = sum(Yawn_rate.yawn_window)/ len(Yawn_rate.yawn_window) if len(Yawn_rate.yawn_window) > 0 else 0
    
    return yawn_rate
 
def _euclidian_distance(point1, point2):
    return np.sqrt((point1.x - point2.x)**2 + (point1.y - point2.y)**2)


def EAR_calculation(landmarks, eye_type = 'left'):
    if not landmarks:
        return 0.0
    
    if eye_type =='left':
        w1 = landmarks[33]
        w2 = landmarks[133]
        
        h1 = landmarks[157]
        h2 = landmarks[154]
        
        h3 = landmarks[159]
        h4 = landmarks[145]
        
        h5 = landmarks[161]
        h6 = landmarks[163]
        
    elif eye_type == 'right':
        w1 = landmarks[362]
        w2 = landmarks[263]
        
        h1 = landmarks[384]
        h2 = landmarks[381]
        
        h3 = landmarks[386]
        h4 = landmarks[374]
        
        h5 = landmarks[388]
        h6 = landmarks[390]
    else:
        return 0.0
    
    horizon_distance = _euclidian_distance(w1, w2)
    if horizon_distance == 0:
        return 0.0
    
    vertical_distance1 = _euclidian_distance(h1, h2)
    vertical_distance2 = _euclidian_distance(h3, h4)
    vertical_distance3 = _euclidian_distance(h5, h6)

    
    return (vertical_distance1 + vertical_distance2 + vertical_distance3) / (3*horizon_distance)
                
def MAR_calculation(landmarks):
    if landmarks == 0.0:
        return 0.0
    
    w1 = landmarks[78]
    w2 = landmarks[308]
    
    h1 = landmarks[81]
    h2 = landmarks[178]
    
    h3 = landmarks[13]
    h4 = landmarks[14]
    
    h5 = landmarks[311]
    h6 = landmarks[402]
    
    horizon_distance = _euclidian_distance(w1, w2)
    if horizon_distance == 0:
        return 0.0
    
    vertical_distance1 = _euclidian_distance(h1, h2)
    vertical_distance2 = _euclidian_distance(h3, h4)
    vertical_distance3 = _euclidian_distance(h5, h6)
    
    return (vertical_distance1 + vertical_distance2 + vertical_distance3) / (3*horizon_distance)


# 권장 다운스케일링 범위
RECOMMENDED_SCALES = {
    "high_accuracy": 0.8,    # 80% - 높은 정확도 유지
    "balanced": 0.6,         # 60% - 성능과 정확도 균형
    "performance": 0.5       # 50% - 성능 우선 (주의 필요)
}

def optimal_resize(frame: np.ndarray, target_scale : float):
    height, width = frame.shape[:2]
    new_width = int(width * target_scale)
    new_height = int(height * target_scale)
    return cv2.resize(frame, (new_width, new_height), interpolation=cv2.INTER_AREA)
#  최고 품질 (느림)
#cv2.INTER_CUBIC      # 부드러운 결과, 정확한 랜드마크

# 균형잡힌 선택 (권장)
#cv2.INTER_AREA       # 다운스케일링에 최적화

# 최고 성능 (빠름, 정확도 저하)
#cv2.INTER_NEAREST    # 빠르지만 랜드마크 부정확할 수 있음

def list_files_directory(directory_path : str):
    files = os.listdir(directory_path)
    return files

def get_videos_files(filePath : str, files : list[str]):
    video_extensions = ['.mp4', '.avi']
    video_files= []
    
    for file in files:
        if any(file.lower().endswith(extension) for extension in video_extensions):
            video_files.append(os.path.join(filePath, file))
    def natural_sort_key(s):
        # 숫자는 int로 변환, 나머지는 그대로 두고 리스트로 반환
        return [int(text) if text.isdigit() else text.lower() for text in re.split('(\d+)', s)]
    
    sorted_files = sorted(video_files, key=natural_sort_key)      
    return sorted_files

all_files = list_files_directory(directory_path)
videos_path = get_videos_files(directory_path, all_files)
if not os.path.exists('sequence'):
    os.mkdir('sequence')

for video in videos_path:
    # 초기화할 값들 및 이용할 값
    frame_count = 0 # frame으로 baseline 초기설정에 이용되고, blink, yawn 지속 시간 측정에도 이용됨 
    left_EAR_baseline = 0.0
    right_EAR_baseline = 0.0
    MAR_baseline = 0.0
    # 깜빡임 지속 시간 기록
    current_blink_duration_frame = 0
    current_yawn_duration_frame = 0
    
    print(f"play ....... {video}") 
    
    cap = cv2.VideoCapture(video)
    if not cap.isOpened():
        print(f"video open error: {video}")
        continue
    with mp_face_mesh.FaceMesh(
        max_num_faces=1,
        refine_landmarks=True,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5
    )as face_mesh:
        
        while True:
            ret, frame = cap.read()
            fps = int(cap.get(cv2.CAP_PROP_FPS))
            wait_time = int(1000/fps)
            frame_count+=1
            if not ret:
                break
            # image processing
            # 프레임의 너비와 높이
            image_height, image_width, _ = frame.shape
            # read만 하는 처리를 통해 최적화
            frame.flags.writeable = False
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB) # opencv use bgr so we need to get rbg
            results = face_mesh.process(frame)
            
            # write image 
            frame.flags.writeable = True
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            
            if results.multi_face_landmarks:
                for face_landmarks in results.multi_face_landmarks:
                    # 시간 feature
                    time_sec = frame_count / fps
                    # 왼쪽 눈, EAR 평가
                    left_eye_ear = EAR_calculation(face_landmarks.landmark, 'left')
                    
                    #오른쪽 눈, EAR 평가
                    right_eye_ear = EAR_calculation(face_landmarks.landmark, 'right')
                    
                    # 입 mar 평가
                    mouth_mar = MAR_calculation(face_landmarks.landmark)
                    
                    # baseline 계산
                    if frame_count < baseline_sample_limit:
                        left_EAR_baseline += left_eye_ear
                        right_EAR_baseline += right_eye_ear
                        MAR_baseline += mouth_mar
                        avg_left = left_EAR_baseline / frame_count
                        avg_right = right_EAR_baseline / frame_count
                        baseline_avg_mar = MAR_baseline / frame_count
                        baseline_avg_ear = (avg_left + avg_right) / 2
                        
                        cv2.putText(frame, f"Calibrate EAR and MAR... {frame_count} / {baseline_sample_limit}", (10, 120),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,0,255), 2, cv2.LINE_AA)
                    else: #계산 완료
                        baseline_avg_ear = (left_EAR_baseline / baseline_sample_limit + right_EAR_baseline / baseline_sample_limit) / 2
                        ear = (left_eye_ear + right_eye_ear)/ 2
                        baseline_avg_mar = MAR_baseline / baseline_sample_limit
                        
                        EAR_THRESHOLD = baseline_avg_ear * 0.50
                        MAR_THRESHOLD = max(baseline_avg_mar * 2.0, 0.5)
                        
                        cv2.putText(frame, f"AVG_EAR : {baseline_avg_ear : .2f}", (10, 180), cv2.FONT_HERSHEY_SIMPLEX, 0.7, 
                                    (0, 255, 0), 2, cv2.LINE_AA)
                        cv2.putText(frame, f"AVG_MAR : {baseline_avg_mar : .2f}", (10, 210), cv2.FONT_HERSHEY_SIMPLEX, 0.7, 
                                    (0, 255, 0), 2, cv2.LINE_AA)
                        
                        if ear < EAR_THRESHOLD :
                            cv2.putText(frame, f"Blink detected, EAR threshold:{EAR_THRESHOLD:.2f}", 
                                        (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,0,255), 2, cv2.LINE_AA)
                            if not is_blinking:
                                blink_start_frame = frame_count
                                is_blinking = True
                            current_blink_duration_frame = frame_count - blink_start_frame
                        else:
                            if is_blinking:
                                blink_end_frame = frame_count
                                blink_duration = (blink_end_frame - blink_start_frame) / fps # 졸음 지속 시간 feature
                                #print(f"blink duration :{blink_duration}")
                                is_blinking = False # 각 인원 별 blink data 저장
                            current_blink_duration_frame = 0
                            
                            
                        perclos_30s_rate = PERCLOS_rate(fps, is_blinking) # feature perclos         
                        
                        if mouth_mar > MAR_THRESHOLD:
                            cv2.putText(frame, f"Yawn detected MAR threshold:{MAR_THRESHOLD:.2f}", 
                                        (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,0,255), 2, cv2.LINE_AA)
                            if not is_yawning:
                                yawn_start_frame = frame_count
                                is_yawning = True
                            current_yawn_duration_frame = frame_count - yawn_start_frame
                        else:
                            if is_yawning:
                                yawn_end_frame = frame_count
                                yawn_duration = (yawn_end_frame - yawn_start_frame) / fps  # 하품 지속 시간 feature
                                #print(f"yawn duration :{yawn_duration}")
                                is_yawning = False
                            current_yawn_duration_frame = 0
                        
                        
                        mar_60s_rate = Yawn_rate(fps, is_yawning) # feature mar rate
                        # 매 frame 마다 기록, 시계열 반영
                        sequence_data.append({
                            "time_sec": time_sec,
                            "current_yawn_duration_sec": current_yawn_duration_frame / fps,
                            "current_blink_duration_sec": current_blink_duration_frame / fps,
                            "left_ear": left_eye_ear,
                            "right_ear" : right_eye_ear,
                            "mouth_mar": mouth_mar,
                            "mar_60s_rate" : mar_60s_rate,
                            "perclos_30s_rate" : perclos_30s_rate
                        })
                    
                    cv2.putText(frame, f"left_EAR {left_eye_ear:.2f}", (10,30), cv2.FONT_HERSHEY_SIMPLEX, 
                                0.7, (0, 255, 0), 2, cv2.LINE_AA)
                    cv2.putText(frame, f"right_EAR {right_eye_ear:.2f}", (10,60), cv2.FONT_HERSHEY_SIMPLEX,
                                0.7, (0, 255, 0), 2, cv2.LINE_AA)
                    cv2.putText(frame, f"MAR {mouth_mar:.2f}", (10,90), cv2.FONT_HERSHEY_SIMPLEX,
                                0.7, (0, 255, 0), 2, cv2.LINE_AA)
            
            
            cv2.imshow('mediapipe frame', frame)
            if cv2.waitKey(wait_time) & 0xFF == ord('q'):
                break

        video_id = os.path.basename(video).split('.')[0]
        df_sequence = pd.DataFrame(sequence_data)
        df_sequence.to_csv(f'sequence/{video_id}_sequence.csv', index=False)
        
        cap.release()
        cv2.destroyAllWindows()