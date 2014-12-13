cd ..\..\..
@echo off
for /f "usebackq tokens=1*" %%f in (`reg query HKCR\Python.File\shell\open\command`) do (set _my_=%%f %%g)
goto next%errorlevel%

:next1
echo Building without Python ...
goto therest

:next0
echo Building with Python ...
set _res_=%_my_:*REG_SZ=%
set _end_=%_res_:*exe"=%
call set _python_=%%_res_:%_end_%=%%
call %_python_% modules\arch\x86\gen_x86_insn.py

:therest
@echo on
call :update %1 x86insn_nasm.gperf x86insn_nasm.c
call :update %1 x86insn_gas.gperf x86insn_gas.c
call :update %1 modules\arch\x86\x86cpu.gperf x86cpu.c
call :update %1 modules\arch\x86\x86regtmod.gperf x86regtmod.c
goto :eof

:update
%1 %2 tf
call mkfiles\vc10\out_copy_rename tf .\ %3
del tf
