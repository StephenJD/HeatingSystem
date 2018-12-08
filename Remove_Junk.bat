@echo off
set dir= "%~dp0"
echo %dir%
CD %dir%

REM /D = directories only
REM /R = Recursive
REM FOR dir %%Var IN (match) DO command command_options // match is the filter used to return results
REM IF /i = ignore case
REM %%~nxX = expand %%X variable to name and extension only

REM for is required because we don't know where the directory might be.
FOR /D /R %dir% %%X IN (ipch) DO if exist %%X RMDIR /S /Q "%%X"
echo Deleted ipch VS files

FOR /D /R %dir% %%X IN (*.tlog) DO if exist %%X RMDIR /S /Q "%%X"
echo Deleted .tlog VS files

FOR /D /R %dir% %%X IN (Debug) DO if exist %%X call :process_debug "%%X"
echo Deleted Debug files

FOR /D /R %dir% %%X IN (Release) DO if exist %%X RMDIR /S /Q "%%X"
echo Deleted Release files

del /S *.sdf
echo Deleted .sdf VS files

del /S *.pdom
echo Deleted .pdom eclipse indexing files

del /S *.db
echo Deleted .db

FOR /D /R %dir% %%X IN (org.eclipse.epp.logging.aeri.ui) DO if exist %%X RMDIR /S /Q "%%X"
echo Deleted org.eclipse.epp.logging.aeri.ui

del .metadata\.plugins\org.eclipse.core.resources\.safetable\org.eclipse.core.resources
echo Deleted org.eclipse.core.resources

pause
exit
goto :EOF

:process_debug
   set CTD=%1
   for /R %CTD% %%f in (*.*) do (
     if /i not "%%~xf"==".mk" if not "%%~nf"=="makefile" del "%%f"
   )
   goto :EOF