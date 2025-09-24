# 졸음 및 부정맥 감지 모니터링 시스템
이 프로젝트는 졸음과 부정맥을 감지하는 시스템입니다. 경량 환경에서도 사용할 수 있도록 엣지 디바이스에 배포되었습니다.

## Project Goal
이 프로젝트는 졸음이나 부정맥과 같은 상황에서 사전 예방적 대응을 제공하는 것을 목표로 합니다.

시스템은 졸음 운전을 감지하고, AI와의 대화를 통해 졸음을 완화할 수 있도록 돕습니다.

부정맥이 감지되면 응급 상황으로 분류하여 주변 지인과 지역 응급 시스템에 사용자의 위치와 상태를 전달합니다.

**built with**

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)

![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54)

![Java](https://img.shields.io/badge/java-%23ED8B00.svg?style=for-the-badge&logo=openjdk&logoColor=white)

## 물리적 환경

**졸음 감지: Jetson Nano B01**

**부정맥 감지: Raspberry Pi 5**

**서버 및 외부 전송: Raspberry Pi 3**

**PPG 센서, 카메라, 마이크, 스피커**

<img width="353" height="335" alt="Image" src="https://github.com/user-attachments/assets/f915e30d-eab1-4d76-8097-436eb2040182" />

## 주요 기능
* 카메라와 Transformer 기반 모델을 활용한 졸음 감지
* AI 어시스턴트(Llama + STT/TTS)와의 실시간 대화
* GPS 위치 기반 자동 응급 신고
* PPG 센서와 scikit-learn을 이용한 부정맥 감지
## 프레임워크 및 AI 모델
* **Mediapipe: 얼굴 랜드마크 감지**

* **scikit-learn: 부정맥 감지**
 [부정맥 감지 정리](https://scratched-vise-e9a.notion.site/learning-PPG-data-1d60a75193b480e19feed34ca9d18802?source=copy_link)

* **PyTorch: 졸음 감지**

* **Llama: 사용자와의 대화**

## API
**Google STT, TTS API**

## 설치 환경
* Mediapipe 0.8.5
* python 3.8.0
* bazel 3.7.2(C++ 빌드를 원하는 경우)
* Jetpack 4.6.6

## 개선 사항
기존의 Mediapipe 랜드마크 분석뿐 아니라 Transformer + 1D Convolution 모델을 적용하여 졸음 감지 정확도를 높였습니다.

## poster
<img width="680" height="917" alt="Image" src="https://github.com/user-attachments/assets/d800c5d3-c783-4ffa-a639-af57e582ff3d" />
