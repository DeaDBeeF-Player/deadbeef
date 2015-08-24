VGMPlay Build Instructions
--------------------------

Compile VGMPlay under Windows
-----------------------------

using MS Visual Studio 6.0:
1. Open VGMPlay.dsw.
2. Build the project.
3. Done.

using later versions of MS Visual Studio:
1. Open VGMPlay.dsw.
2. It will ask you to convert the project to the current project format. Click "Yes".
3. Build the project.
4. Done.

using MinGW/MSYS:
1. edit the Makefile and enable the line "WINDOWS = 1" (remove the #)
2. open MSYS and run the "make" command in VGMPlay's folder.
3. Done.

Note: You can compile it without MSYS, but you need to manually create the OBJDIRS paths (or make them use the backslash '\'), because mkdir fails at paths with a forward slash.



Compile VGMPlay under Linux
---------------------------
1. [optional step] If you have libao installed, you can edit the Makefile to make VGMPlay use libao instead of OSS.
2. run "make" in VGMPlay's folder
3. Done.
