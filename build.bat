@echo off
setlocal

rem Build mkfile.c using MSVC (run from Developer Command Prompt)

set SRC=mkfile.c
set OUT=mkfile.exe

if not exist "%SRC%" (
  echo ERROR: %SRC% not found.
  exit /b 1
)

del /q "%OUT%" 2>nul

rem Compile + link
cl /nologo ^
   /W4 ^
   /O2 ^
   /GS- ^
   /Zc:wchar_t ^
   "%SRC%" ^
   /link ^
   /nologo ^
   /incremental:no ^
   shell32.lib

if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

echo Built: %OUT%
exit /b 0
