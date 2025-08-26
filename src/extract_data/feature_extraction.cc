#include "mediapipe/framework/calculator_graph.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/deps/status_macros.h"

// GPU 관련 헤더 추가
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "mediapipe/gpu/gpu_shared_data_internal.h"
#include "mediapipe/gpu/gl_context.h"

#include <opencv2/opencv.hpp>
#include <memory>

constexpr char kInputStream[] = "input_video";
constexpr char kOutputStream[] = "multi_face_landmarks";
const std::string graph_config_file = "/home/js/mediapipe/mediapipe/graphs/face_mesh/face_mesh_desktop_live_gpu.pbtxt";

absl::Status RunMPPGraph() {
    // 1. 그래프 설정 파일 읽기 및 파싱
    std::string calculator_graph_config_contents;
    MP_RETURN_IF_ERROR(mediapipe::file::GetContents(
        graph_config_file, &calculator_graph_config_contents));

    mediapipe::CalculatorGraphConfig config =
        mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
            calculator_graph_config_contents);

    // 2. GPU 리소스 초기화
    MP_ASSIGN_OR_RETURN(auto gpu_resources, mediapipe::GpuResources::Create());
    
    // 3. 그래프 초기화 및 GPU 리소스 설정
    mediapipe::CalculatorGraph graph;
    MP_RETURN_IF_ERROR(graph.SetGpuResources(std::move(gpu_resources)));
    MP_RETURN_IF_ERROR(graph.Initialize(config));

    // 4. GPU Helper 초기화 (수정된 부분 - void 반환)
    mediapipe::GlCalculatorHelper gpu_helper;
    gpu_helper.InitializeForTest(graph.GetGpuResources().get());

    // 5. 출력 스트림 폴러 생성
    MP_ASSIGN_OR_RETURN(auto poller, graph.AddOutputStreamPoller(kOutputStream));
    MP_RETURN_IF_ERROR(graph.StartRun({}));

    // 6. 비디오 캡처 초기화
    cv::VideoCapture capture("/home/js/python_source/DROZY/videos_i8/1-1.mp4");
    if (!capture.isOpened()) {
        return absl::UnavailableError("Failed to open video file.");
    }

    double fps = capture.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0;
    int64_t frame_timestamp_us = 0;
    int64_t timestamp_increment_us = static_cast<int64_t>(1e6 / fps);

    cv::Mat camera_frame;
    cv::Mat rgba_frame;

    while (capture.read(camera_frame)) {
        cv::cvtColor(camera_frame, rgba_frame, cv::COLOR_BGR2RGBA);

        auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
            mediapipe::ImageFormat::SRGBA, 
            rgba_frame.cols, 
            rgba_frame.rows,
            mediapipe::ImageFrame::kDefaultAlignmentBoundary);
        
        cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
        rgba_frame.copyTo(input_frame_mat);

        auto gpu_buffer = gpu_helper.CreateSourceTexture(*input_frame);

        // ConvertToGpuBuffer 제거, 바로 move로 넘김
        auto input_packet = mediapipe::MakePacket<mediapipe::GpuBuffer>(
            std::move(gpu_buffer)).At(mediapipe::Timestamp(frame_timestamp_us));

        MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(kInputStream, input_packet));

        mediapipe::Packet packet;
        if (poller.Next(&packet)) {
            const auto& multi_face_landmarks = 
                packet.Get<std::vector<mediapipe::NormalizedLandmarkList>>();
            
            cv::Mat display_frame = camera_frame.clone();
            for (const auto& landmarks : multi_face_landmarks) {
                for (const auto& landmark : landmarks.landmark()) {
                    int x = static_cast<int>(landmark.x() * display_frame.cols);
                    int y = static_cast<int>(landmark.y() * display_frame.rows);
                    cv::circle(display_frame, cv::Point(x, y), 1, cv::Scalar(0, 255, 0), -1);
                }
            }
            cv::imshow("MediaPipe Face Mesh GPU", display_frame);
        } else {
            cv::imshow("MediaPipe Face Mesh GPU", camera_frame);
        }

        if (cv::waitKey(1) & 0xFF == 27) break;
        frame_timestamp_us += timestamp_increment_us;
    }

    MP_RETURN_IF_ERROR(graph.CloseInputStream(kInputStream));
    return graph.WaitUntilDone();
}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    absl::Status run_status = RunMPPGraph();
    
    if (!run_status.ok()) {
        LOG(ERROR) << "Failed to run the graph: " << run_status.message();
        return EXIT_FAILURE;
    } else {
        LOG(INFO) << "Success!";
    }
    
    return EXIT_SUCCESS;
}

