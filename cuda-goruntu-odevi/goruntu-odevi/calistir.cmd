@echo off
setlocal
cd /d "%~dp0"
if not defined OpenCV_DIR set "OpenCV_DIR=%USERPROFILE%\Documents\Codex\2026-09-28\reis-g-r-nt-i-leme\outputs\opencv-cuda"
if not defined CUDA_PATH set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6"
set "PATH=%OpenCV_DIR%\x64\vc17\bin;%CUDA_PATH%\bin;%PATH%"
if not exist odev.exe call derle.cmd
if not exist odev.exe exit /b 1
odev.exe %*
set "sonuc=%errorlevel%"
if not defined ODEV_OTOMATIK pause
exit /b %sonuc%
