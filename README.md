# Monitoring system

## Project Goal

## Project Environment

**Jetson Nano B01**

**Jetpack: 4.6.6**

**Python: 3.6.9** (python 3.6.9 is the last version to support Jetpack4.6.6)

**openCV: 4.5.4** (not pip install, build openCV)

**pytorch: 1.10.0 cp36**

**torchvision: 0.11.1** 

**detecting model: YOLOv5, 68 face feature landmark**

[jetson nano torch, torchvision download guide](https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048)

### how to set environment

python 3.6.9를 이용하여 venv 생성

```bash
sudo apt-get install python3-pip # for python3.x
virtualenv -p /usr/bin/python3 my_opencv_env
source my_opencv_env/bin/activate
```

opencv 설치를 위한 모든 프로그램 설치 
```bash
sudo sh -c "echo '/usr/local/cuda/lib64' >> /etc/ld.so.conf.d/nvidia-tegra.conf"
sudo ldconfig
sudo apt-get install build-essential cmake git unzip pkg-config
sudo apt-get install libjpeg-dev libpng-dev libtiff-dev
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev
sudo apt-get install libgtk2.0-dev libcanberra-gtk*
sudo apt-get install python3-dev python3-numpy python3-pip
sudo apt-get install libxvidcore-dev libx264-dev libgtk-3-dev
sudo apt-get install libtbb2 libtbb-dev libdc1394-22-dev
sudo apt-get install libv4l-dev v4l-utils
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install libavresample-dev libvorbis-dev libxine2-dev
sudo apt-get install libfaac-dev libmp3lame-dev libtheora-dev
sudo apt-get install libopencore-amrnb-dev libopencore-amrwb-dev
sudo apt-get install libopenblas-dev libatlas-base-dev libblas-dev
sudo apt-get install liblapack-dev libeigen3-dev gfortran
sudo apt-get install libhdf5-dev protobuf-compiler
sudo apt-get install libprotobuf-dev libgoogle-glog-dev libgflags-dev
```

opencv build


`build 하기 전에 jetson nano는 4gb이기 때문에 build시에 메모리 부족 crash가 날 수 있다. 
그러므로 swap file 수정을 통해 ram memory를 ssd에서 땡겨와서 이용하자`

```bash
# dphys-swapfile
sudo apt-get install dphys-swapfile

## 두 Swap파일의 값이 다음과 같도록 값을 수정
# CONF_SWAPSIZE=4096
# CONF_SWAPFACTOR=2
# CONF_MAXSWAP=4096

sudo vim /sbin/dphys-swapfile

sudo vim /etc/dphys-swapfile

# Jetson Nano 재부팅
sudo reboot
```
용량 확인
```bash
free -h # swap 5.9GB 라면 성공
```
```bash
# zip down
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.5.4.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.5.4.zip
# change name
mv opencv-4.5.4 opencv
mv opencv-4.5.4_contrib opencv_contrib

cd opencv
mkdir build
cd build
```
cmake 설정
```bash
cmake -D CMAKE_BUILD_TYPE=RELEASE \
-D CMAKE_INSTALL_PREFIX=/usr/local \
-D OPENCV_EXTRA_MODULES_PATH=~/opencv_contrib/modules \
-D EIGEN_INCLUDE_PATH=/usr/include/eigen3 \
-D WITH_OPENCL=OFF \
-D WITH_CUDA=ON \
-D CUDA_ARCH_BIN=5.3 \
-D CUDA_ARCH_PTX="" \
-D WITH_CUDNN=ON \
-D WITH_CUBLAS=ON \
-D ENABLE_FAST_MATH=ON \
-D CUDA_FAST_MATH=ON \
-D OPENCV_DNN_CUDA=ON \
-D ENABLE_NEON=ON \
-D WITH_QT=OFF \
-D WITH_OPENMP=ON \
-D WITH_OPENGL=ON \
-D BUILD_TIFF=ON \
-D WITH_FFMPEG=ON \
-D WITH_GSTREAMER=ON \
-D WITH_TBB=ON \
-D BUILD_TBB=ON \
-D BUILD_TESTS=OFF \
-D WITH_EIGEN=ON \
-D WITH_V4L=ON \
-D WITH_LIBV4L=ON \
-D OPENCV_ENABLE_NONFREE=ON \
-D INSTALL_C_EXAMPLES=OFF \
-D INSTALL_PYTHON_EXAMPLES=OFF \
-D BUILD_NEW_PYTHON_SUPPORT=ON \
-D BUILD_opencv_python3=TRUE \
-D OPENCV_GENERATE_PKGCONFIG=ON \
-D BUILD_EXAMPLES=OFF ..
```
build 실행
```bash
make -j4 # core number $(nproc --all) it takes about 2 hours 
```

설치 확인
```bash
sudo pip3 install jetson-stats
sudo reboot

jtop # 설치 확인 가능
```


## Dataset

## Algorithms

## Reference

