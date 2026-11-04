@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.29 >nul
cd /d "%~dp0\..\.."
cl /nologo /EHsc /MD /I External\include docs\screenshots\glfwtest.cpp /link /LIBPATH:lib glfw3.lib opengl32.lib user32.lib gdi32.lib shell32.lib legacy_stdio_definitions.lib /OUT:docs\screenshots\glfwtest.exe
