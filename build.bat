

cl.exe /I. /nologo /Fomain.c.obj /EHsc /Zc:preprocessor /WX /MT /std:c11 /analyze /DMain_TU /c main.c
cl.exe /I. /nologo /Fodx.cpp.obj /EHsc /Zc:preprocessor /WX /MT /analyze /c dx.cpp
cl.exe /nologo /Femain.exe /MT /EHsc main.c.obj dx.cpp.obj


