import cv2
from PIL import Image
import numpy as np
import os
import shutil

ASCII_CHARS = "@%#*+=-:. "

# Get the size of the terminal window
def get_terminal_size():
    columns, rows = shutil.get_terminal_size(fallback=(80, 24))
    return columns, rows

def resize_image(image, new_width):
    (original_width, original_height) = image.size
    aspect_ratio = original_height / original_width
    new_height = int(aspect_ratio * new_width * 0.55)  # Adjust for character height (0.55 is an approximation)
    return image.resize((new_width, new_height))

def grayify(image):
    return image.convert("L")  # Convert to grayscale

def pixels_to_ascii(image):
    pixels = image.getdata()
    ascii_str = ''.join(ASCII_CHARS[pixel * (len(ASCII_CHARS) - 1) // 255] for pixel in pixels)
    return ascii_str

def frame_to_ascii(frame, new_width):
    image = Image.fromarray(frame)
    image = resize_image(image, new_width)
    image = grayify(image)
    
    ascii_str = pixels_to_ascii(image)
    img_width = image.width
    ascii_str_len = len(ascii_str)
    ascii_img = '\n'.join(ascii_str[i:i + img_width] for i in range(0, ascii_str_len, img_width))

    return ascii_img

def video_to_ascii(video_path):
    # Get the size of the terminal window
    terminal_width, terminal_height = get_terminal_size()
    
    # Set the width of the ASCII video based on terminal size
    new_width = terminal_width - 2  # Subtracting 2 to avoid edge overflow
    
    # Open the video file
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break  # Exit loop if no frames are left
        
        # Convert frame from BGR to RGB
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        ascii_art = frame_to_ascii(frame, new_width)
        print(ascii_art)
        print("\n" + "="*40 + "\n")  # Separator between frames
        
        # Optional: Pause between frames to slow down output
        cv2.waitKey(2)  # Adjust wait time (2 ms here)

    cap.release()  # Release the video capture object

# Example usage
video_path = "E:\\code\\TaoTe\\Bad Apple!!.mp4"  # Replace with your video file path
video_to_ascii(video_path)
