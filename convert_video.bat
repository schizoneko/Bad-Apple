@echo off
REM Batch file to convert an MP4 video to 128x64 grayscale frames at 10 fps using FFmpeg and ImageMagick

REM Input video file (modify the path accordingly)
set "INPUT_VIDEO=Bad Apple!!.mp4"

REM Output directory for frames (creates if it doesn't exist)
set "OUTPUT_DIR=ESP32\OLed\frames"
set "CONVERTED_DIR=ESP32\OLed\converted_frames"

REM Create output directories if they don't exist
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

if not exist "%CONVERTED_DIR%" (
    mkdir "%CONVERTED_DIR%"
)

REM Convert video to frames using FFmpeg
ffmpeg -i "%INPUT_VIDEO%" -vf "scale=128:64,fps=10" "%OUTPUT_DIR%\frame_%%04d.png"

REM Check if FFmpeg was successful
if %errorlevel% neq 0 (
    echo FFmpeg conversion failed!
    pause
    exit /b
)

REM Convert each PNG frame to 128x64 monochrome BMP using ImageMagick
for %%f in ("%OUTPUT_DIR%\*.png") do (
    echo Converting %%f to monochrome BMP...
    magick "%%f" -resize 128x64 -monochrome "%CONVERTED_DIR%\%%~nf.bmp"
)

echo Conversion complete. Frames are saved in the "%CONVERTED_DIR%" directory.
pause
