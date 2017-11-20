@echo off
set /a c=0

setlocal ENABLEDELAYEDEXPANSION

for /r %%x in (*.ecl) do (
  set /a c=c+1
  erase %%x
)

echo Znaleziono i skasowano !c! plikow ecl
endlocal