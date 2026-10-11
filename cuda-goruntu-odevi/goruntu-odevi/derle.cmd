@echo off
setlocal
call C:\BuildTools2022\Common7\Tools\VsDevCmd.bat -arch=x64 -host_arch=x64 -vcvars_ver=14.38
if errorlevel 1 exit /b 1
if not defined OpenCV_DIR set "OpenCV_DIR=%USERPROFILE%\Documents\Codex\2026-09-28\reis-g-r-nt-i-leme\outputs\opencv-cuda"
if not defined CUDA_PATH set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6"
set "PATH=%CUDA_PATH%\bin;C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_ARCHITECTURES=89 -DOpenCV_DIR="%OpenCV_DIR%"
if errorlevel 1 exit /b 1
cmake --build "%~dp0build" --parallel 4
if errorlevel 1 exit /b 1
copy /y "%~dp0build\odev.exe" "%~dp0odev.exe" >nul
exit /b %errorlevel%
