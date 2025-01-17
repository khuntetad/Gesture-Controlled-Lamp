# use cv2, mediapipe and serial to be able to communicate, load stream, and draw handmarks
import cv2
import mediapipe as mp
from serial import Serial
import time

# use the esp stream link from iPhone (379)
ESP32_STREAM_URL = "http://172.20.10.6/stream"

# utilize the same port as displayed in arduino
# use 101
arduino_port = '/dev/cu.usbmodem101'

# baud rate of 1012500 too high use 9600 on BOTH ARDUINO AND PYTHON
baud_rate = 9600

# try to connect
try:
    print("Attempting to connect to Arduino...")

    # connect by using serial port
    arduino = Serial(arduino_port, baud_rate, timeout=1)
    time.sleep(2)
    print("Successfully connected to Arduino")

    # now we want to establish communication between arduino and python: sleep and do this
    time.sleep(1)
    while arduino.in_waiting > 0:
        init_msg = arduino.readline().decode().strip()
        print("From Arduino:", init_msg)
except Exception as e:
    print(f"Error connecting to Arduino: {e}")
    arduino = None

# after research mediapipe hands was the best choice in terms of speed after preprocessing
mp_hands = mp.solutions.hands

# only need one hand
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.9,
    min_tracking_confidence=0.9
)

# use the solutions functions to actually be able to draw the correct styles on the stream
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles

def count_fingers(hand_landmarks):
    """
    We need to use the indices of the hand landmarks to calculate how many fingers are up
    MediaPipe can do this by using the tip of the finger as well as the middle joint
    - index: 8, pip: 6
    - middle: 12, pip: 10
    - ring: 16, pip: 14
    - pinky: 20, pip: 18
    - thumb: 4, pip: 3

    Important note: thumb goes int he x axis, not the y axis
    """
    # Finger landmark indices
    thumb_tip = 4
    thumb_ip = 3
    index_tip = 8
    index_pip = 6
    middle_tip = 12
    middle_pip = 10
    ring_tip = 16
    ring_pip = 14
    pinky_tip = 20
    pinky_pip = 18

    # use the hand_landmarks from mediapipe to be able to landmark these locations
    t_tip = hand_landmarks.landmark[thumb_tip]
    t_ip = hand_landmarks.landmark[thumb_ip]

    i_tip = hand_landmarks.landmark[index_tip]
    i_pip = hand_landmarks.landmark[index_pip]

    m_tip = hand_landmarks.landmark[middle_tip]
    m_pip = hand_landmarks.landmark[middle_pip]

    r_tip = hand_landmarks.landmark[ring_tip]
    r_pip = hand_landmarks.landmark[ring_pip]

    p_tip = hand_landmarks.landmark[pinky_tip]
    p_pip = hand_landmarks.landmark[pinky_pip]

    # set the initial number of fingers up and some threshold that we need to exceed to count a finger
    threshold = 0.02
    fingers_up = 0

    # thumb measurement based on the x axis
    if t_tip.x < t_ip.x - threshold:
        fingers_up += 1

    # other fingers use the y axis and increment it
    # note for future tests: should we bump up the value of the threshold to reduce false positives
    if i_tip.y < i_pip.y - threshold:
        fingers_up += 1
    if m_tip.y < m_pip.y - threshold:
        fingers_up += 1
    if r_tip.y < r_pip.y - threshold:
        fingers_up += 1
    if p_tip.y < p_pip.y - threshold:
        fingers_up += 1

    return fingers_up

def main():
    # now we need to do some simple preprocessing of the stream with cv2 to open it up from python
    print("Attempting to connect to the ESP32 stream at:", ESP32_STREAM_URL)
    cap = cv2.VideoCapture(ESP32_STREAM_URL)

    if not cap.isOpened():
        print("Error: Unable to connect to ESP32 stream. Check the URL or your network.")
        return
    else:
        print("Successfully connected to ESP32 stream!")

    last_finger_count = None

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Failed to get frame from the ESP32 stream.")
            break

        # read the frame and convert to rgb
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb_frame)

        # process the frame and then go through each of the landmarks that we got from the results
        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                finger_count = count_fingers(hand_landmarks)

                # we only want to send arduino the new values if they have changed
                if finger_count != last_finger_count:
                    print(f"Fingers detected: {finger_count}")

                    # if we can successfully connect lets encode the total finger count to debug
                    if arduino is not None:
                        arduino.write((str(finger_count) + '\n').encode())
                    last_finger_count = finger_count

                # draw the landmarks
                # takes in the frame in question, the landmarks and connections
                mp_drawing.draw_landmarks(
                    frame,
                    hand_landmarks,
                    mp_hands.HAND_CONNECTIONS,
                    mp_drawing_styles.get_default_hand_landmarks_style(),
                    mp_drawing_styles.get_default_hand_connections_style()
                )

        cv2.imshow("ESP32 Live Stream", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("User requested to quit the stream.")
            break

    # quit the stream after to as to not hog resources or mess with the serial ports
    cap.release()

    # release and destroy windows
    cv2.destroyAllWindows()
    print("closing streams")

    if arduino is not None:
        arduino.close()
        print("no arduino connection")

if __name__ == "__main__":
    main()
