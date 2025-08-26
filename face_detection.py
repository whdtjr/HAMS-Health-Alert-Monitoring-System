#!/usr/bin/env python
# -*- coding: utf-8 -*-
import copy
import argparse

import cv2 as cv
import numpy as np
import mediapipe as mp
from collections import deque
from utils import CvFpsCalc
import socket

SERVER_HOST = '192.168.1.115'
SERVER_PORT = 1234
BUFFER_SIZE = 1024


left_eye_landmarks_points = [133, 173, 157, 158, 159, 160, 161, 246, 163, 144, 145, 153, 154, 155]
right_eye_landmarks_points = [362, 398, 384, 385, 386, 387, 388, 466, 390, 373, 374, 380, 381, 382]
mouth_landmarks_points = [308, 415, 310, 311, 312, 13, 82, 81, 80, 191, 78, 95, 88, 178, 87, 14, 317, 402, 318, 324]

#frame rate
FPS = 13

# 임계값 설정
EAR_THRESHOLD = 0.41  # 눈 깜빡임 임계값
MAR_THRESHOLD = 0.5   # 하품 임계값
    
#졸음 초기값
blink_count = 0
    
#yawn_rate weight value, perclos weight value
YAWN_WEIGHT = 0.3
PERCLOS_WEIGHT = 0.7
    
# GStreamer 파이프라인 문자열 (카메라 해상도 및 fps 설정 가능)
def gstreamer_pipeline(
    capture_width=960, capture_height=540,
    display_width=960, display_height=540,
    framerate=30, flip_method=0
):
    return (
        f"nvarguscamerasrc ! video/x-raw(memory:NVMM), "
        f"width=(int){capture_width}, height=(int){capture_height}, "
        f"format=(string)NV12, framerate=(fraction){framerate}/1 ! "
        f"nvvidconv flip-method={flip_method} ! "
        f"video/x-raw, width=(int){display_width}, height=(int){display_height}, format=(string)BGRx ! "
        f"videoconvert ! video/x-raw, format=(string)BGR ! appsink"
    )

def get_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("--device", type=int, default=0)
    parser.add_argument("--width", help='cap width', type=int, default=960)
    parser.add_argument("--height", help='cap height', type=int, default=540)

    parser.add_argument("--max_num_faces", type=int, default=1)
    parser.add_argument("--min_detection_confidence",
                        help='min_detection_confidence',
                        type=float,
                        default=0.7)
    parser.add_argument("--min_tracking_confidence",
                        help='min_tracking_confidence',
                        type=int,
                        default=0.5)

    parser.add_argument('--use_brect', action='store_true')

    args = parser.parse_args()

    return args
def send_to_berry(data):
    try:
# 소켓 생성 및 연결
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((SERVER_HOST, SERVER_PORT))
        
# 데이터 전송
        client_socket.send(data.encode('utf-8'))
        print(f"데이터 전송: {data}")
        
# 소켓 닫기
        client_socket.close()
    except Exception as e:
        print(f"전송 오류: {e}")
        
# EAR 계산 함수
def calculate_ear(eye_points, landmarks, image_width, image_height):
    # 눈의 6개 주요 랜드마크 포인트 사용
    p1 = np.array([int(landmarks[eye_points[0]].x * image_width), int(landmarks[eye_points[0]].y * image_height)])
    p2 = np.array([int(landmarks[eye_points[2]].x * image_width), int(landmarks[eye_points[2]].y * image_height)])
    p3 = np.array([int(landmarks[eye_points[4]].x * image_width), int(landmarks[eye_points[4]].y * image_height)])
    p4 = np.array([int(landmarks[eye_points[7]].x * image_width), int(landmarks[eye_points[7]].y * image_height)])
    p5 = np.array([int(landmarks[eye_points[8]].x * image_width), int(landmarks[eye_points[8]].y * image_height)])
    p6 = np.array([int(landmarks[eye_points[10]].x * image_width), int(landmarks[eye_points[10]].y * image_height)])

    # 유클리드 거리 계산
    vertical_1 = np.linalg.norm(p2 - p6)
    vertical_2 = np.linalg.norm(p3 - p5)
    horizontal = np.linalg.norm(p1 - p4)

    # EAR 계산
    ear = (vertical_1 + vertical_2) / (2.0 * horizontal)
    return ear
# MAR 계산 함수
def calculate_mar(mouth_points, landmarks, image_width, image_height):
    # 입의 주요 랜드마크 포인트 사용
    p1 = np.array([int(landmarks[mouth_points[0]].x * image_width), int(landmarks[mouth_points[0]].y * image_height)])  # 오른쪽 끝
    p2 = np.array([int(landmarks[mouth_points[5]].x * image_width), int(landmarks[mouth_points[5]].y * image_height)])  # 아래 중앙
    p3 = np.array([int(landmarks[mouth_points[10]].x * image_width), int(landmarks[mouth_points[10]].y * image_height)]) # 왼쪽 끝
    p4 = np.array([int(landmarks[mouth_points[15]].x * image_width), int(landmarks[mouth_points[15]].y * image_height)]) # 위 중앙

    # 유클리드 거리 계산
    vertical = np.linalg.norm(p4 - p2)
    horizontal = np.linalg.norm(p1 - p3)

    # MAR 계산
    mar = vertical / horizontal
    return mar

def PERCLOS_rate(eye_closed):
    
    WINDOW_SEC=60
    window_size = WINDOW_SEC *FPS
    
    if not hasattr(PERCLOS_rate, "eye_closed_queue"):
        PERCLOS_rate.eye_closed_queue = deque(maxlen=window_size)
    
    PERCLOS_rate.eye_closed_queue.append(1 if eye_closed else 0)
    perclos = sum(PERCLOS_rate.eye_closed_queue) / len(PERCLOS_rate.eye_closed_queue)
    return perclos

def Yawn_rate(mouth_opened):
    WINDOW_SEC= 60*10   
    window_size = WINDOW_SEC * FPS
    
    if not hasattr(Yawn_rate, "mouth_opened_queue"):
        Yawn_rate.mouth_opened_queue = deque(maxlen=window_size)
    
    Yawn_rate.mouth_opened_queue.append(1 if mouth_opened else 0)
    yawn_score = sum(Yawn_rate.mouth_opened_queue) / len(Yawn_rate.mouth_opened_queue)
    return yawn_score

def drowsiness_detector(perclos, yawn_rate):
    SCORE_WINDOW = 10 * FPS # 10초간 확인 smoothing 
    score = perclos * PERCLOS_WEIGHT + yawn_rate * YAWN_WEIGHT
# Smooth the score
    if not hasattr(drowsiness_detector, "score_queue"):
        drowsiness_detector.score_queue = deque(maxlen=SCORE_WINDOW)
    drowsiness_detector.score_queue.append(score)
    smoothed_score = sum(drowsiness_detector.score_queue) / len(drowsiness_detector.score_queue)
    
    if smoothed_score >= 0.25:
        send_to_berry("drowsy")
        
    
def main():
    # args setting
    args = get_args()

    max_num_faces = args.max_num_faces
    min_detection_confidence = args.min_detection_confidence
    min_tracking_confidence = args.min_tracking_confidence

    use_brect = args.use_brect

    cap = cv.VideoCapture(gstreamer_pipeline(), cv.CAP_GSTREAMER)

    mp_face_mesh = mp.solutions.face_mesh
    face_mesh = mp_face_mesh.FaceMesh(
        max_num_faces=max_num_faces,
        min_detection_confidence=min_detection_confidence,
        min_tracking_confidence=min_tracking_confidence,
    )
    
    
    #FPS 
    cvFpsCalc = CvFpsCalc(buffer_len=10)

    while True:
        display_fps = cvFpsCalc.get()


        ret, image = cap.read()
        if not ret:
            break
        image = cv.flip(image, 1) # mirror effect
        debug_image = copy.deepcopy(image)

        image = cv.cvtColor(image, cv.COLOR_BGR2RGB)
        results = face_mesh.process(image)

        if results.multi_face_landmarks is not None:
            for face_landmarks in results.multi_face_landmarks:

               # brect = calc_bounding_rect(debug_image, face_landmarks)
                left_ear = calculate_ear(left_eye_landmarks_points, face_landmarks.landmark, image.shape[1], image.shape[0])
                light_ear = calculate_ear(right_eye_landmarks_points, face_landmarks.landmark, image.shape[1], image.shape[0])
                ear = (left_ear + light_ear) / 2.0
                
                mar = calculate_mar(mouth_landmarks_points, face_landmarks.landmark, image.shape[1], image.shape[0])
                
                # eye closed rate
                perclos_val = PERCLOS_rate(ear< EAR_THRESHOLD)
                yawn_val = Yawn_rate(mar > MAR_THRESHOLD)
                
                drowsiness_detector(yawn_val, perclos_val)
                #debug_image = draw_landmarks(debug_image, face_landmarks)
                
                
                # 결과 표시
                cv.putText(debug_image, f"PERCLOS: {perclos_val:.2f}", (10, 60),
                          cv.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv.putText(debug_image, f"YAWN: {yawn_val:.2f}", (10, 90),
                          cv.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
               # debug_image = draw_bounding_rect(use_brect, debug_image, brect)

        cv.putText(debug_image, "FPS:" + str(display_fps), (10, 30),
                   cv.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2, cv.LINE_AA)

        # esc to exit
        key = cv.waitKey(1)
        if key == 27:  # ESC
            break

        cv.imshow('MediaPipe Face Mesh Demo', debug_image)

    cap.release()
    cv.destroyAllWindows()


def calc_bounding_rect(image, landmarks):
    image_width, image_height = image.shape[1], image.shape[0]

    landmark_array = np.empty((0, 2), int)

    for _, landmark in enumerate(landmarks.landmark):
        landmark_x = min(int(landmark.x * image_width), image_width - 1)
        landmark_y = min(int(landmark.y * image_height), image_height - 1)

        landmark_point = [np.array((landmark_x, landmark_y))]

        landmark_array = np.append(landmark_array, landmark_point, axis=0)

    x, y, w, h = cv.boundingRect(landmark_array)

    return [x, y, x + w, y + h]


def draw_landmarks(image, landmarks):
    image_width, image_height = image.shape[1], image.shape[0]

    landmark_point = []

    for index, landmark in enumerate(landmarks.landmark):
        if landmark.visibility < 0 or landmark.presence < 0:
            continue

        landmark_x = min(int(landmark.x * image_width), image_width - 1)
        landmark_y = min(int(landmark.y * image_height), image_height - 1)

        landmark_point.append((landmark_x, landmark_y))

        # cv.circle(image, (landmark_x, landmark_y), 1, (0, 255, 0), 1)

    if len(landmark_point) > 0:


        cv.line(image, landmark_point[55], landmark_point[65], (0, 255, 0), 2)
        cv.line(image, landmark_point[65], landmark_point[52], (0, 255, 0), 2)
        cv.line(image, landmark_point[52], landmark_point[53], (0, 255, 0), 2)
        cv.line(image, landmark_point[53], landmark_point[46], (0, 255, 0), 2)

        cv.line(image, landmark_point[285], landmark_point[295], (0, 255, 0),
                2)
        cv.line(image, landmark_point[295], landmark_point[282], (0, 255, 0),
                2)
        cv.line(image, landmark_point[282], landmark_point[283], (0, 255, 0),
                2)
        cv.line(image, landmark_point[283], landmark_point[276], (0, 255, 0),
                2)

        cv.circle(image, landmark_point[133], 2, (0, 255, 0), 2) 
        cv.circle(image, landmark_point[173], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[157], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[158], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[159], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[160], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[161], 2, (0, 255, 0), 2)

        cv.circle(image, landmark_point[246], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[163], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[144], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[145], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[153], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[154], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[155], 2, (0, 255, 0), 2)

        cv.circle(image, landmark_point[362], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[398], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[384], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[385], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[386], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[387], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[388], 2, (0, 255, 0), 2)

        cv.circle(image, landmark_point[466], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[390], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[373], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[374], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[380], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[381], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[382], 2, (0, 255, 0), 2)
        
        cv.circle(image, landmark_point[308], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[415], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[310], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[311], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[312], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[13], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[82], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[81], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[80], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[191], 2, (0, 255, 0), 2)

        cv.circle(image, landmark_point[78], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[95], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[88], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[178], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[87], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[14], 2, (0, 255, 0), 2)
        cv.circle(image, landmark_point[317], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[402], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[318], 2, (0, 255, 0),
                2)
        cv.circle(image, landmark_point[324], 2, (0, 255, 0),
                2)

    return image


def draw_bounding_rect(use_brect, image, brect):
    if use_brect:
        cv.rectangle(image, (brect[0], brect[1]), (brect[2], brect[3]),
                     (0, 255, 0), 2)

    return image


if __name__ == '__main__':
    main()
