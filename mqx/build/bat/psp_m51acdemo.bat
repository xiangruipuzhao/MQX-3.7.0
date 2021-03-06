@echo off

rem make sure we are in target directory
call "%~dp0/verify_dir.bat" %1
if %ERRORLEVEL% NEQ 0 exit 1
rem  Common non-private header files

call "%~dp0\psp_mcf.bat" %1 %2

rem  Configuration header files

for %%F in (..\..\..\config\common\*.h)     do copy /Y /B ..\..\..\mqx\source\include\dontchg.h + %%F ..\%%~nF%%~xF
for %%F in (..\..\..\config\m51acdemo\*.h)  do copy /Y /B ..\..\..\mqx\source\include\dontchg.h + %%F ..\%%~nF%%~xF


rem  Processor-specific PSP files

copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51AC.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51AC_adc.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51AC_i2c.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51xx_ictl.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51xx_sci.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf5xxx_spi8.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf5xxx_spi16.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51xx_pmc.h  .
copy /Y ..\..\..\mqx\source\psp\coldfire\mcf51xx_ftsr.h  .
