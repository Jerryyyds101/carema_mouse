import cv2
import mediapipe as mp
import pyautogui
import math
import win32gui
import win32con

# 获取屏幕尺寸
screen_width, screen_height = pyautogui.size()

# 导入手部检测任务
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# 初始化手部检测模型
base_options = python.BaseOptions(model_asset_path='hand_landmarker.task')
options = vision.HandLandmarkerOptions(
    base_options=base_options,
    num_hands=1,
    min_hand_detection_confidence=0.8,
    min_hand_presence_confidence=0.8,
    min_tracking_confidence=0.8
)

# 创建手部检测器
detector = vision.HandLandmarker.create_from_options(options)

# 打开摄像头
cap = cv2.VideoCapture(0)

# 设置摄像头帧率
cap.set(cv2.CAP_PROP_FPS, 30)

# 设置摄像头分辨率为640x480，提高处理速度
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# 点击状态
clicking = False

# 拖动状态
dragging = False

# 用于平滑鼠标移动的变量
mouse_history = []
history_length = 5  # 保存最近5次的鼠标位置

# 用于平滑手部关键点的变量
hand_landmarks_history = []
hand_history_length = 3  # 保存最近3次的手部关键点

# 创建窗口
cv2.namedWindow('Hand Tracking', cv2.WINDOW_NORMAL)

# 计算窗口大小为屏幕的三分之一，减少绘制开销
window_width = int(screen_width * 0.33)
window_height = int(screen_height * 0.33)

# 调整窗口大小和位置
cv2.resizeWindow('Hand Tracking', window_width, window_height)
# 将窗口居中显示
window_x = (screen_width - window_width) // 2
window_y = (screen_height - window_height) // 2
cv2.moveWindow('Hand Tracking', window_x, window_y)

# 定义中心操作区域（窗口的三分之二）
operation_margin_x = window_width // 6
operation_margin_y = window_height // 6
operation_x = operation_margin_x
operation_y = operation_margin_y
operation_width = window_width * 2 // 3
operation_height = window_height * 2 // 3

# 获取窗口句柄并设置为顶层窗口
hwnd = win32gui.FindWindow(None, 'Hand Tracking')
win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, 0, 0, 0, 0, win32con.SWP_NOMOVE | win32con.SWP_NOSIZE)

while True:
    # 读取摄像头画面
    ret, frame = cap.read()
    if not ret:
        break
    
    # 水平翻转画面，使操作更直观
    frame = cv2.flip(frame, 1)
    
    # 调整帧大小以匹配窗口大小
    frame = cv2.resize(frame, (window_width, window_height))
    
    # 创建一个透明背景的图像
    overlay = frame.copy()
    overlay = cv2.cvtColor(overlay, cv2.COLOR_BGR2BGRA)
    overlay[:, :, 3] = 0  # 设置alpha通道为0，完全透明
    
    # 获取画面尺寸
    frame_height, frame_width, _ = frame.shape
    
    # 将BGR转换为RGB
    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    
    # 创建MediaPipe图像对象
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
    
    # 检测手部
    results = detector.detect(mp_image)
    
    if results.hand_landmarks:
        for hand_landmarks in results.hand_landmarks:
            # 保存手部关键点到历史记录
            hand_landmarks_history.append(hand_landmarks)
            
            # 保持历史记录长度
            if len(hand_landmarks_history) > hand_history_length:
                hand_landmarks_history.pop(0)
            
            # 计算平滑后的手部关键点
            if len(hand_landmarks_history) > 1:
                # 创建一个新的列表来存储平滑后的关键点
                smoothed_landmarks = []
                for i in range(21):  # 21个关键点
                    # 计算所有历史记录中该关键点的平均值
                    avg_x = sum(lm[i].x for lm in hand_landmarks_history) / len(hand_landmarks_history)
                    avg_y = sum(lm[i].y for lm in hand_landmarks_history) / len(hand_landmarks_history)
                    avg_z = sum(lm[i].z for lm in hand_landmarks_history) / len(hand_landmarks_history)
                    
                    # 创建一个新的Landmark对象（这里使用一个简单的类来模拟）
                    class SmoothedLandmark:
                        def __init__(self, x, y, z):
                            self.x = x
                            self.y = y
                            self.z = z
                    
                    smoothed_landmarks.append(SmoothedLandmark(avg_x, avg_y, avg_z))
            else:
                # 如果历史记录不足，使用原始关键点
                smoothed_landmarks = hand_landmarks
            
            # 绘制手部连接
            connections = [
                (0, 1), (1, 2), (2, 3), (3, 4),  # 拇指
                (0, 5), (5, 6), (6, 7), (7, 8),  # 食指
                (0, 9), (9, 10), (10, 11), (11, 12),  # 中指
                (0, 13), (13, 14), (14, 15), (15, 16),  # 无名指
                (0, 17), (17, 18), (18, 19), (19, 20)   # 小指
            ]
            for connection in connections:
                start_idx, end_idx = connection
                start_point = smoothed_landmarks[start_idx]
                end_point = smoothed_landmarks[end_idx]
                # 计算相对于窗口的坐标
                start_x = int(start_point.x * window_width)
                start_y = int(start_point.y * window_height)
                end_x = int(end_point.x * window_width)
                end_y = int(end_point.y * window_height)
                # 绘制手部连接线，使用白色线条，更清晰
                cv2.line(overlay, (start_x, start_y), (end_x, end_y), (255, 255, 255, 255), 3)
            
            # 绘制手部关键点
            for landmark in smoothed_landmarks:
                # 计算相对于窗口的坐标
                x = int(landmark.x * window_width)
                y = int(landmark.y * window_height)
                # 绘制关键点，使用白色圆点
                cv2.circle(overlay, (x, y), 4, (255, 255, 255, 255), -1)
            
            # 获取平滑后的食指指尖坐标 (索引8)
            index_finger_tip = smoothed_landmarks[8]
            # 获取平滑后的拇指指尖坐标 (索引4)
            thumb_tip = smoothed_landmarks[4]
            # 获取平滑后的中指指尖坐标 (索引12)
            middle_finger_tip = smoothed_landmarks[12]
            
            # 计算食指在窗口中的坐标
            index_x = int(index_finger_tip.x * window_width)
            index_y = int(index_finger_tip.y * window_height)
            
            # 检查食指是否在中心操作区域内
            if operation_x <= index_x <= operation_x + operation_width and operation_y <= index_y <= operation_y + operation_height:
                # 将操作区域内的坐标映射到整个屏幕范围
                # 计算食指在操作区域内的相对位置 (0-1)
                rel_x = (index_x - operation_x) / operation_width
                rel_y = (index_y - operation_y) / operation_height
                
                # 应用轻微的非线性映射，提高中心区域的精度
                # 使用二次函数映射，使中心区域的移动更加精确
                rel_x = rel_x * rel_x * 2 if rel_x < 0.5 else 1 - (1 - rel_x) * (1 - rel_x) * 2
                rel_y = rel_y * rel_y * 2 if rel_y < 0.5 else 1 - (1 - rel_y) * (1 - rel_y) * 2
                
                # 将相对位置映射到整个屏幕
                screen_x = int(rel_x * screen_width)
                screen_y = int(rel_y * screen_height)
                
                # 添加到历史记录
                mouse_history.append((screen_x, screen_y))
                
                # 保持历史记录长度
                if len(mouse_history) > history_length:
                    mouse_history.pop(0)
                
                # 计算移动平均值，使用加权平均提高响应速度
                if mouse_history:
                    # 为最近的位置赋予更高的权重
                    weights = [i + 1 for i in range(len(mouse_history))]
                    total_weight = sum(weights)
                    
                    avg_x = sum(x * w for (x, y), w in zip(mouse_history, weights)) // total_weight
                    avg_y = sum(y * w for (x, y), w in zip(mouse_history, weights)) // total_weight
                    
                    # 检查是否处于拖动状态
                    if dragging:
                        # 拖动状态下，按住鼠标左键移动
                        pyautogui.dragTo(avg_x, avg_y, button='left', duration=0.05)
                    else:
                        # 正常状态下，移动鼠标
                        pyautogui.moveTo(avg_x, avg_y, duration=0.05)
                
                # 计算食指和拇指之间的距离（用于点击）
                index_thumb_distance = math.sqrt(
                    (index_finger_tip.x - thumb_tip.x) ** 2 +
                    (index_finger_tip.y - thumb_tip.y) ** 2 +
                    (index_finger_tip.z - thumb_tip.z) ** 2
                )
                
                # 计算拇指和中指之间的距离（用于拖动）
                thumb_middle_distance = math.sqrt(
                    (thumb_tip.x - middle_finger_tip.x) ** 2 +
                    (thumb_tip.y - middle_finger_tip.y) ** 2 +
                    (thumb_tip.z - middle_finger_tip.z) ** 2
                )
                
                # 当食指和拇指距离小于阈值时执行点击
                if index_thumb_distance < 0.05:
                    if not clicking:
                        pyautogui.click()
                        clicking = True
                else:
                    clicking = False
                
                # 当拇指和中指距离小于阈值时进入拖动状态
                if thumb_middle_distance < 0.05:
                    if not dragging:
                        dragging = True
                else:
                    dragging = False
            else:
                # 食指不在操作区域内，重置状态
                clicking = False
                dragging = False
    
    # 绘制中心操作区域的线框
    cv2.rectangle(overlay, (operation_x, operation_y), (operation_x + operation_width, operation_y + operation_height), (255, 255, 255, 200), 2)
    
    # 显示透明的手部追踪画面
    cv2.imshow('Hand Tracking', overlay)
    
    # 按退格键退出
    if cv2.waitKey(1) & 0xFF == 8:
        break

# 释放资源
cap.release()
cv2.destroyAllWindows()
detector.close()