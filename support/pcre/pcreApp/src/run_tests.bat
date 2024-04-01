setlocal
set "srcdir=%~dp0..\..\pcre-src"
set "pcretest=%~dp0..\..\bin\%EPICS_HOST_ARCH%\pcretest.exe"
call %srcdir%\RunTest.bat
if %errorlevel% neq 0 exit /b %errorlevel%
%~dp0..\..\bin\%EPICS_HOST_ARCH%\pcre_jit_test.exe
if %errorlevel% neq 0 exit /b %errorlevel%
%~dp0..\..\bin\%EPICS_HOST_ARCH%\pcrecpp_unittest.exe
if %errorlevel% neq 0 exit /b %errorlevel%
%~dp0..\..\bin\%EPICS_HOST_ARCH%\pcre_scanner_unittest.exe
if %errorlevel% neq 0 exit /b %errorlevel%
%~dp0..\..\bin\%EPICS_HOST_ARCH%\pcre_stringpiece_unittest.exe
if %errorlevel% neq 0 exit /b %errorlevel%
@echo done
