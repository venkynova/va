@echo off
title Venky Agent v1 - Local
cd /d "%~dp0"
echo.
echo ================================
echo   Venky Agent v1 - Local AI
echo ================================
echo.
echo Make sure Ollama is running with gemma3:4b.
echo Starting local web server...
echo.
start "Venky Agent" http://127.0.0.1:8000/
python server.py
pause
