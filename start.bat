@echo off
title AI Interviewer - Live Environment
echo ========================================================
echo   Starting AI Interviewer Platform...
echo ========================================================
python run.py
if errorlevel 1 (
    echo.
    echo Python was not detected on PATH.
    echo Launching directly in your default browser...
    start index.html
)
pause
