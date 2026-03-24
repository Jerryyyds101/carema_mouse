#include <opencv2/opencv.hpp>
#include <mediapipe/framework/calculator_framework.h>
#include <mediapipe/framework/formats/image_frame.h>
#include <mediapipe/framework/formats/image_frame_opencv.h>
#include <mediapipe/solutions/hands/hands.h>
#include <mediapipe/solutions/hands/hands_options.pb.h>
#include <Windows.h>
#include <iostream>

using namespace cv;
using namespace mediapipe;
using namespace std;

// 全局变量
bool clicking = false;
bool dragging = false;

// 鼠标移动历史记录
vector<POINT> mouseHistory;
const int historyLength = 5;

// 手部关键点历史记录
vector<vector<NormalizedLandmark>> handLandmarksHistory;
const int handHistoryLength = 3;

// 屏幕尺寸
int screenWidth, screenHeight;

// 窗口尺寸
int windowWidth, windowHeight;

// 操作区域
Rect operationRegion;

// 计算两点之间的距离
float calculateDistance(const NormalizedLandmark& a, const NormalizedLandmark& b) {
    return sqrt(pow(a.x() - b.x(), 2) + pow(a.y() - b.y(), 2) + pow(a.z() - b.z(), 2));
}

// 平滑鼠标移动
POINT smoothMouseMove(const POINT& newPoint) {
    mouseHistory.push_back(newPoint);
    if (mouseHistory.size() > historyLength) {
        mouseHistory.erase(mouseHistory.begin());
    }

    if (mouseHistory.empty()) {
        return newPoint;
    }

    // 加权平均
    POINT avgPoint = { 0, 0 };
    int totalWeight = 0;
    for (int i = 0; i < mouseHistory.size(); i++) {
        int weight = i + 1;
        avgPoint.x += mouseHistory[i].x * weight;
        avgPoint.y += mouseHistory[i].y * weight;
        totalWeight += weight;
    }
    avgPoint.x /= totalWeight;
    avgPoint.y /= totalWeight;

    return avgPoint;
}

// 平滑手部关键点
vector<NormalizedLandmark> smoothHandLandmarks(const vector<NormalizedLandmark>& newLandmarks) {
    handLandmarksHistory.push_back(newLandmarks);
    if (handLandmarksHistory.size() > handHistoryLength) {
        handLandmarksHistory.erase(handLandmarksHistory.begin());
    }

    if (handLandmarksHistory.size() <= 1) {
        return newLandmarks;
    }

    vector<NormalizedLandmark> smoothedLandmarks;
    for (int i = 0; i < newLandmarks.size(); i++) {
        NormalizedLandmark landmark;
        float avgX = 0, avgY = 0, avgZ = 0;
        for (const auto& history : handLandmarksHistory) {
            avgX += history[i].x();
            avgY += history[i].y();
            avgZ += history[i].z();
        }
        avgX /= handLandmarksHistory.size();
        avgY /= handLandmarksHistory.size();
        avgZ /= handLandmarksHistory.size();
        landmark.set_x(avgX);
        landmark.set_y(avgY);
        landmark.set_z(avgZ);
        smoothedLandmarks.push_back(landmark);
    }

    return smoothedLandmarks;
}

// 映射坐标到屏幕
POINT mapToScreen(const Point& point) {
    // 计算相对位置
    float relX = static_cast<float>(point.x - operationRegion.x) / operationRegion.width;
    float relY = static_cast<float>(point.y - operationRegion.y) / operationRegion.height;

    // 非线性映射
    if (relX < 0.5) {
        relX = relX * relX * 2;
    } else {
        relX = 1 - pow(1 - relX, 2) * 2;
    }
    if (relY < 0.5) {
        relY = relY * relY * 2;
    } else {
        relY = 1 - pow(1 - relY, 2) * 2;
    }

    // 映射到屏幕
    POINT screenPoint;
    screenPoint.x = static_cast<int>(relX * screenWidth);
    screenPoint.y = static_cast<int>(relY * screenHeight);

    return screenPoint;
}

// 控制鼠标
void controlMouse(const POINT& point, bool isDragging) {
    if (isDragging) {
        // 拖动模式
        mouse_event(MOUSEEVENTF_LEFTDOWN, point.x, point.y, 0, 0);
        SetCursorPos(point.x, point.y);
        mouse_event(MOUSEEVENTF_LEFTUP, point.x, point.y, 0, 0);
    } else {
        // 正常模式
        SetCursorPos(point.x, point.y);
    }
}

// 执行点击
void performClick() {
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

int main() {
    // 获取屏幕尺寸
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    // 计算窗口尺寸（屏幕的三分之一）
    windowWidth = screenWidth / 3;
    windowHeight = screenHeight / 3;

    // 计算操作区域（窗口的三分之二）
    int marginX = windowWidth / 6;
    int marginY = windowHeight / 6;
    operationRegion = Rect(marginX, marginY, windowWidth * 2 / 3, windowHeight * 2 / 3);

    // 初始化MediaPipe Hands
    HandsOptions handsOptions;
    handsOptions.set_max_num_hands(1);
    handsOptions.set_min_detection_confidence(0.8);
    handsOptions.set_min_tracking_confidence(0.8);
    unique_ptr<Hands> hands = Hands::Create(handsOptions);

    // 打开摄像头
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "无法打开摄像头" << endl;
        return -1;
    }

    // 设置摄像头参数
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(CAP_PROP_FPS, 30);

    // 创建窗口
    namedWindow("Hand Tracking", WINDOW_NORMAL);
    resizeWindow("Hand Tracking", windowWidth, windowHeight);
    moveWindow("Hand Tracking", (screenWidth - windowWidth) / 2, (screenHeight - windowHeight) / 2);

    // 设置窗口为顶层
    HWND hWnd = FindWindowA(NULL, "Hand Tracking");
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    Mat frame;
    while (true) {
        // 读取摄像头画面
        cap >> frame;
        if (frame.empty()) {
            break;
        }

        // 水平翻转画面
        flip(frame, frame, 1);

        // 调整帧大小
        resize(frame, frame, Size(windowWidth, windowHeight));

        // 创建透明背景
        Mat overlay = Mat::zeros(frame.size(), CV_8UC4);

        // 转换为RGB
        Mat rgbFrame;
        cvtColor(frame, rgbFrame, COLOR_BGR2RGB);

        // 创建MediaPipe图像
        ImageFrame imageFrame(ImageFormat::SRGB, rgbFrame.cols, rgbFrame.rows, ImageFrame::kDefaultAlignmentBoundary);
        memcpy(imageFrame.MutablePixelData(), rgbFrame.data, rgbFrame.total() * rgbFrame.elemSize());

        // 检测手部
        auto results = hands->Process(imageFrame);

        if (results.multi_hand_landmarks().size() > 0) {
            const auto& handLandmarks = results.multi_hand_landmarks(0);
            
            // 平滑手部关键点
            auto smoothedLandmarks = smoothHandLandmarks(handLandmarks.landmark());

            // 绘制手部连接
            vector<pair<int, int>> connections = {
                {0, 1}, {1, 2}, {2, 3}, {3, 4},  // 拇指
                {0, 5}, {5, 6}, {6, 7}, {7, 8},  // 食指
                {0, 9}, {9, 10}, {10, 11}, {11, 12},  // 中指
                {0, 13}, {13, 14}, {14, 15}, {15, 16},  // 无名指
                {0, 17}, {17, 18}, {18, 19}, {19, 20}   // 小指
            };
            for (const auto& connection : connections) {
                int startIdx = connection.first;
                int endIdx = connection.second;
                const auto& startPoint = smoothedLandmarks[startIdx];
                const auto& endPoint = smoothedLandmarks[endIdx];
                Point start(int(startPoint.x() * windowWidth), int(startPoint.y() * windowHeight));
                Point end(int(endPoint.x() * windowWidth), int(endPoint.y() * windowHeight));
                line(overlay, start, end, Scalar(255, 255, 255, 255), 3);
            }

            // 绘制手部关键点
            for (const auto& landmark : smoothedLandmarks) {
                Point point(int(landmark.x() * windowWidth), int(landmark.y() * windowHeight));
                circle(overlay, point, 4, Scalar(255, 255, 255, 255), -1);
            }

            // 获取指尖坐标
            const auto& indexFingerTip = smoothedLandmarks[8];  // 食指
            const auto& thumbTip = smoothedLandmarks[4];       // 拇指
            const auto& middleFingerTip = smoothedLandmarks[12];  // 中指

            // 计算食指在窗口中的坐标
            Point indexPoint(int(indexFingerTip.x() * windowWidth), int(indexFingerTip.y() * windowHeight));

            // 检查食指是否在操作区域内
            if (operationRegion.contains(indexPoint)) {
                // 映射到屏幕坐标
                POINT screenPoint = mapToScreen(indexPoint);
                
                // 平滑鼠标移动
                POINT smoothedPoint = smoothMouseMove(screenPoint);
                
                // 控制鼠标
                controlMouse(smoothedPoint, dragging);

                // 计算距离
                float indexThumbDistance = calculateDistance(indexFingerTip, thumbTip);
                float thumbMiddleDistance = calculateDistance(thumbTip, middleFingerTip);

                // 点击检测
                if (indexThumbDistance < 0.05) {
                    if (!clicking) {
                        performClick();
                        clicking = true;
                    }
                } else {
                    clicking = false;
                }

                // 拖动检测
                if (thumbMiddleDistance < 0.05) {
                    dragging = true;
                } else {
                    dragging = false;
                }
            } else {
                // 重置状态
                clicking = false;
                dragging = false;
            }
        }

        // 绘制操作区域
        rectangle(overlay, operationRegion, Scalar(255, 255, 255, 200), 2);

        // 显示画面
        imshow("Hand Tracking", overlay);

        // 按退格键退出
        if (waitKey(1) == 8) {
            break;
        }
    }

    // 释放资源
    cap.release();
    destroyAllWindows();

    return 0;
}