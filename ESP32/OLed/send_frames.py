import serial
import time
import os

SERIAL_PORT = 'COM10'  # Adjust to match your ESP32's port
BAUD_RATE = 115200
FRAME_FOLDER = './converted_frames/'  # Path to your BMP frames

def send_frame(file_path, ser):
    with open(file_path, 'rb') as f:
        data = f.read()
        ser.write(data)
        print(f"Sent frame: {file_path}")

def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Give time for ESP32 to reset

    frame_files = sorted(os.listdir(FRAME_FOLDER))
    for frame_file in frame_files:
        if frame_file.endswith('.bmp'):
            frame_path = os.path.join(FRAME_FOLDER, frame_file)
            send_frame(frame_path, ser)
            time.sleep(0.1)  # Delay to match the display frame rate

    ser.close()

if __name__ == "__main__":
    main()
