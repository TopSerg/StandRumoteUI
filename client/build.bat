@echo off
rmdir /s /q build
rmdir /s /q dist
del app.spec
pyinstaller --onefile --noconsole app.py
pause