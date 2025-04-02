import torch
import cv2
import dlib
import numpy as np
import time
from math import hypot

# yolo weight 설정
weight_path = "face_detection_yolov5s.pt"
model = torch.hub.load('ultralytics/yolov5', 'custom', path=weight_path)

# face detection 설정정
predictor_path = 'shape_predictor_68_face_landmarks.dat'
predictor = dlib.shape_predictor(predictor_path)

# --- 눈 깜빡임 감지 설정 ---
EAR_THRESHOLD = 0.19  # 눈 깜빡임 임계값
EAR_CONSEC_FRAMES = 2 # EAR이 임계값 이하로 유지되어야 하는 최소 연속 프레임 수

# -- 하품 감지 설정 --
MAR_THRESHOLD = 0.5
MAR_CONSEC_FRAME = 2

# 깜빡임 횟수 및 연속 프레임 카운터 초기화
close_consec_frames = 0
blink_count = 0

# 하품 횟수 및 연속 프레임 카운터 초기화
yawn_consec_frames = 0
yawn_count = 0

left_eyes = [36, 37, 38, 39, 40, 41]
right_eyes = [42, 43, 44, 45, 46, 47]
mouth = [48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63]

# 점 거리 계산
def euclidian_dist(p1, p2):
    return hypot((p1.x - p2.x), (p1.y - p2.y))
# EAR 계산
def calculate_ear(eye_points, facial_landmarks):
    A = euclidian_dist(facial_landmarks.part(eye_points[0]), facial_landmarks.part(eye_points[3]))
    B = euclidian_dist(facial_landmarks.part(eye_points[1]), facial_landmarks.part(eye_points[5]))
    C = euclidian_dist(facial_landmarks.part(eye_points[2]), facial_landmarks.part(eye_points[4]))

    ear = (B + C) / (2.0 * A) if A != 0 else 0 
    return ear
# MAR 계산
def calculate_mar(facial_landmarks):
    A = euclidian_dist(facial_landmarks.part(60), facial_landmarks.part(64))
    B = euclidian_dist(facial_landmarks.part(61), facial_landmarks.part(67))
    C = euclidian_dist(facial_landmarks.part(62), facial_landmarks.part(66))
    D = euclidian_dist(facial_landmarks.part(63), facial_landmarks.part(65))

    mar = (B + C + D) / (3.0 * A) if A != 0 else 0
    return mar

cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("cannot open Webcam")
    exit()

# --- FPS 계산을 위한 변수 초기화 ---
prev_time = 0 # 이전 프레임 처리 완료 시간

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to capture frame")
        break
    
    img_display = frame.copy() # 원본 복사
    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY) # gray scale frame

    # --- FPS 계산 ---
    current_time = time.time() 
    loop_time = current_time - prev_time
    fps = 1 / loop_time if loop_time > 0 else 0
    prev_time = current_time
    fps_text = f"FPS: {fps:.2f}"
    cv2.putText(img_display, fps_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2, cv2.LINE_AA)


    result = model(frame) 
    detections = result.xyxy[0]

    for *box, conf, cls in detections:
        #face box
        x1, y1, x2, y2 = map(int, box)
        # dlib용 사각형 객체 생성
        rect = dlib.rectangle(x1, y1, x2, y2)
        try:
            shape = predictor(gray, rect)

            left_ear = calculate_ear(left_eyes, shape)
            right_ear = calculate_ear(right_eyes, shape)
            mouth_mar = calculate_mar(shape)

            for i in mouth:
                x, y = shape.part(i).x, shape.part(i).y
                cv2.circle(img_display, (x,y), 1, (0,0,255), -1)

            if left_ear < EAR_THRESHOLD and right_ear < EAR_THRESHOLD:
                close_consec_frames += 1
            else:
                if close_consec_frames >= EAR_CONSEC_FRAMES: # 눈 감고 있던 프레임 횟수가 연속해서 2번이었다면 깜빡임으로
                    blink_count += 1
                close_consec_frames = 0

            # 화면에 깜빡임 횟수 표시
            cv2.putText(img_display, f"Blinks: {blink_count}", (x1, y2 + 40), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)

            if mouth_mar > MAR_THRESHOLD:
                yawn_consec_frames += 1
            else:
                if yawn_consec_frames >= MAR_CONSEC_FRAME:
                    yawn_count+=1
                yawn_consec_frames = 0
            # 화면에 하품 횟수 표시

            cv2.putText(img_display, f"Yawns: {yawn_count}", (x1, y2 + 70), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        except Exception as e:
            pass 

     # 결과 프레임 보여주기
    cv2.imshow("YOLOv5 Face Detection & Dlib Landmarks", img_display)
       # 'q' 키를 누르면 루프 종료
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# --- 종료 처리 ---
cap.release()
cv2.destroyAllWindows()

print("Webcam stopped.")