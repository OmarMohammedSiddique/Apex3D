@echo off
:: Apex3D – Checkpoint 2: Animation Timeline – Build Script
:: Usage: build.bat
:: Assumes g++ from C:\msys64\ucrt64\bin is in PATH or found at the fixed path.

set GPP=C:\msys64\ucrt64\bin\g++.exe
set ROOT=%~dp0
set OUTDIR=%ROOT%output

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

echo.
echo ============================================================
echo  Building: tests.cpp
echo ============================================================
"%GPP%" -std=c++20 -Wall -Wextra -Wpedantic ^
    "%ROOT%tests.cpp" ^
    -o "%ROOT%timeline_tests.exe"
if errorlevel 1 (
    echo FAILED: tests.cpp
    exit /b 1
)
echo OK

echo.
echo ============================================================
echo  Building: main.cpp  (timeline demo)
echo ============================================================
"%GPP%" -std=c++20 -Wall -Wextra -Wpedantic ^
    -I"%ROOT%..\..\transformations\include" ^
    "%ROOT%main.cpp" ^
    -o "%ROOT%timeline_demo.exe"
if errorlevel 1 (
    echo FAILED: main.cpp
    exit /b 1
)
echo OK

echo.
echo ============================================================
echo  Building: contact_sheet.cpp
echo ============================================================
"%GPP%" -std=c++20 -Wall -Wextra -Wpedantic ^
    "%ROOT%contact_sheet.cpp" ^
    -o "%ROOT%contact_sheet.exe"
if errorlevel 1 (
    echo FAILED: contact_sheet.cpp
    exit /b 1
)
echo OK

echo.
echo ============================================================
echo  Running tests
echo ============================================================
"%ROOT%timeline_tests.exe"
if errorlevel 1 ( echo TESTS FAILED; exit /b 1 )

echo.
echo ============================================================
echo  Running demo  (writes PPM frames to %OUTDIR%)
echo ============================================================
"%ROOT%timeline_demo.exe" "%OUTDIR%"
if errorlevel 1 ( echo DEMO FAILED; exit /b 1 )

echo.
echo ============================================================
echo  Building contact sheet
echo ============================================================
"%ROOT%contact_sheet.exe" "%OUTDIR%"
if errorlevel 1 ( echo CONTACT SHEET FAILED; exit /b 1 )

echo.
echo All done.
