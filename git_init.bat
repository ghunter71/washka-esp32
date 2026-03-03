@echo off
echo ========================================
echo Git Repository Initialization
echo ========================================
echo.

REM Check if git is installed
git --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Git is not installed!
    echo Please install Git from: https://git-scm.com/download/win
    pause
    exit /b 1
)

echo Step 1: Initialize Git repository...
git init

echo.
echo Step 2: Add all files...
git add .

echo.
echo Step 3: Create initial commit...
git commit -m "Initial commit: ESP32 Washka control system with FilesystemOTA"

echo.
echo ========================================
echo Git repository initialized successfully!
echo ========================================
echo.
echo Next steps:
echo 1. Create repository on GitHub: https://github.com/new
echo 2. Run: git remote add origin https://github.com/YOUR_USERNAME/washka-esp32.git
echo 3. Run: git branch -M main
echo 4. Run: git push -u origin main
echo.
pause
