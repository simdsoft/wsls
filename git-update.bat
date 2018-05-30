@echo off
git pull
ping /n 3 127.0.1 >nul
goto :eof

