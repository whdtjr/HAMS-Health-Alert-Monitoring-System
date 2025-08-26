# Monitoring system to detect Drowsiness and Arrhythmia
This project contains system detecting drawsiness and arrhythmia. we deployed on an edge device so you can even use this system on the limited environment

## Project Goal
This project provides a solution for proactive prevention in situations such as drowsiness or arrhythmia.
The system detects drowsy driving and helps alleviate drowsiness through conversations between the user and AI.
In cases where arrhythmia is detected, it classifies the situation as an emergency and contacts nearby acquaintances and the local emergency system, providing them with the user’s location and condition

**built with**

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)

![Python](https://img.shields.io/badge/python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54)

![Java](https://img.shields.io/badge/java-%23ED8B00.svg?style=for-the-badge&logo=openjdk&logoColor=white)

## physical environment

**Jetson nano B01 for detecting drowsiness**

**Raspberry Pi 5 for detecting arrhythmia**

**Raspberry Pi 3 for the server and for broadcasting the situation to the outside**

**PPG sensor,Camera,Mike,speaker**

<img width="353" height="335" alt="Image" src="https://github.com/user-attachments/assets/f915e30d-eab1-4d76-8097-436eb2040182" />

## Features
* Drowsiness detection using camera and Transformer-based model
* real-time communication with AI-assistant(Llama + STT/TTS)
* Automatic emergency reporting with GPS location
* Arrhythmia detection using PPG sensor and scikit-learn
## Framework and AI model
* **Mediapipe for face landmark**

* **scikit-learn for arrhymia detecting**

* **pytorch for detecting Drowsiness**

* **Llama to communicate with user**

## API
**Google STT, TTS API**

## Usage

## Improvement
we started to detect drowsiness with not only mediapipe landmark analysis but also Transformer + 1D convolution model to raise detecting accuarcy


