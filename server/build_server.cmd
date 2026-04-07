@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build MarathonWS and collect runtime files into dist\server
cd /d "%~dp0"

set "ROOT=%CD%"
set "VCPKG_ROOT=%ROOT%\vcpkg"
set "TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
set "BUILD_DIR=%ROOT%\out\build\x64-Release"
set "DIST_DIR=%ROOT%\dist\server"

echo [1/5] Checking vcpkg...
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
  if exist "%VCPKG_ROOT%\bootstrap-vcpkg.bat" (
    call "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
    if errorlevel 1 (
      echo ERROR: vcpkg bootstrap failed.
      exit /b 1
    )
  ) else (
    echo ERROR: vcpkg not found at "%VCPKG_ROOT%".
    exit /b 1
  )
)

if not exist "%TOOLCHAIN%" (
  echo ERROR: vcpkg toolchain not found: "%TOOLCHAIN%"
  exit /b 1
)

echo [2/6] Installing vcpkg dependencies...
"%VCPKG_ROOT%\vcpkg.exe" install boost-system boost-thread boost-asio boost-beast nlohmann-json --triplet x64-windows
if errorlevel 1 (
  echo ERROR: vcpkg dependency install failed.
  exit /b 1
)

echo [3/6] Configuring CMake...
cmake -S "%ROOT%" -B "%BUILD_DIR%" --fresh -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
if errorlevel 1 (
  echo ERROR: CMake configure failed.
  exit /b 1
)

echo [4/6] Building Release target...
cmake --build "%BUILD_DIR%" --config Release --target MarathonWS
if errorlevel 1 (
  echo ERROR: Build failed.
  exit /b 1
)

set "BIN_DIR="
if exist "%BUILD_DIR%\Release\MarathonWS.exe" set "BIN_DIR=%BUILD_DIR%\Release"
if not defined BIN_DIR if exist "%BUILD_DIR%\MarathonWS.exe" set "BIN_DIR=%BUILD_DIR%"

if not defined BIN_DIR (
  echo ERROR: MarathonWS.exe not found after build.
  exit /b 1
)

echo [5/6] Preparing dist folder...
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
del /q "%DIST_DIR%\MarathonWS.exe" 2>nul

copy /y "%BIN_DIR%\MarathonWS.exe" "%DIST_DIR%\MarathonWS.exe" >nul
if errorlevel 1 (
  echo ERROR: Could not copy MarathonWS.exe to dist.
  exit /b 1
)

echo [6/6] Copying runtime DLLs near exe...
for %%F in ("%BIN_DIR%\*.dll") do (
  if exist "%%~fF" copy /y "%%~fF" "%DIST_DIR%\" >nul
)

echo.
echo Build completed.
echo Output: "%DIST_DIR%\MarathonWS.exe"
echo DLLs copied from: "%BIN_DIR%"
exit /b 0

