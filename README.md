# portabat
Portable Batch environment..

Yet another fork of 7zip SFX module, this time focus on command line support.  
This is continuation of http://opensourcepack.blogspot.de/p/bat.html..

# features
- Your batch files always executed regardless Windows policy to disable command prompt  
- Your other executables / batch files in the archive will readily available in PATH lookup  
- Batch execution always reause existing console  
- Executed at the "correct" working directory  
- Bundled other useful tools that vanilla Windows severely lacking  
- Simple editing, just open it with 7zip and add your batch files there, that's all  
- Can handle inf files as well  

# notes
- Your `echo %%~nx0` is `echo %SFX%`  
- Your SFX path is `%SFXPATH%`  
- Your extraction path is `%~dp0`  
- Be sure to always reference other files/folder in archive with `%~dp0` prefix  
- Batch filename execution order (prefer .bat over .cmd) :
1. setup
2. install
3. run
4. start
- Other extension order after batch files (4-7 use default shellexecute) :
1. inf
2. exe
4. msi
5. hta
6. vbs
7. wsh

# credits
This project based on LZMA 17.00 SDK  
This project bundle the very compatible cmd.exe, reg.exe and cacls.exe from ReactOS (reg.exe rebuilt to avoid advapi32_vista.dll)  
