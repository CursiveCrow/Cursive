@echo off
setlocal
for /f "usebackq tokens=1* delims=:" %%A in (`findstr /B /C:"cmd:" "C:\Dev\Cursive\HelloCursive\build\main\bin\main.linker.log"`) do call %%B
endlocal
