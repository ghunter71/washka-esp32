@echo off
echo ========================================
echo Push to GitHub
echo ========================================
echo.

REM Prompt for GitHub username
set /p USERNAME="Enter your GitHub username: "

REM Prompt for repository name
set /p REPONAME="Enter repository name (default: washka-esp32): "
if "%REPONAME%"=="" set REPONAME=washka-esp32

echo.
echo Adding remote: https://github.com/%USERNAME%/%REPONAME%.git
git remote add origin https://github.com/%USERNAME%/%REPONAME%.git

echo.
echo Setting branch to main...
git branch -M main

echo.
echo Pushing to GitHub...
git push -u origin main

if errorlevel 1 (
    echo.
    echo ERROR: Push failed!
    echo.
    echo Possible reasons:
    echo - Repository doesn't exist on GitHub
    echo - Authentication failed
    echo - Remote already exists (use: git remote set-url origin URL)
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Successfully pushed to GitHub!
echo ========================================
echo.
echo Repository URL: https://github.com/%USERNAME%/%REPONAME%
echo.
pause
