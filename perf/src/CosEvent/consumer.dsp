# Microsoft Developer Studio Project File - Name="consumer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=consumer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "consumer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "consumer.mak" CFG="consumer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "consumer - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "consumer - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "consumer - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "Debug\consumer"
# PROP Target_Dir ""
MTL=midl.exe
# ADD MTL /D "_DEBUG" /nologo /mktyplib203 /win32
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Gy /I "$(ACE_ROOT)" /I "$(TAO_ROOT)" /I "." /I "$(TAO_ROOT)\orbsvcs" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /i "$(ACE_ROOT)" /i "$(TAO_ROOT)" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 TAO_AnyTypeCoded.lib TAO_Messagingd.lib TAO_PortableServerd.lib TAO_ValueTyped.lib TAOd.lib ACEd.lib advapi32.lib user32.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /out:".\server.exe" /libpath:"$(TAO_ROOT)\..\lib" /libpath:"$(TAO_ROOT)\tao\PortableServer" /libpath:"$(ACE_ROOT)\ace" /libpath:"$(TAO_ROOT)\tao" /libpath:"$(TAO_ROOT)\tao\Strategies" /libpath:"$(TAO_ROOT)\tao\Messaging" /libpath:"$(TAO_ROOT)\tao\ValueType"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "consumer - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD MTL /D "NDEBUG" /nologo /mktyplib203 /win32
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "$(ACE_ROOT)" /I "$(TAO_ROOT)" /I "." /I "$(TAO_ROOT)\orbsvcs" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /i "$(ACE_ROOT)" /i "$(TAO_ROOT)" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 TAO_AnyTypeCode.lib TAO_CosEvent.lib TAO_Messaging.lib TAO_PortableServer.lib TAO_ValueType.lib TAO.lib ACE.lib advapi32.lib user32.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"server.exe" /libpath:"$(TAO_ROOT)\..\lib" /libpath:"$(TAO_ROOT)\tao\PortableServer" /libpath:"$(ACE_ROOT)\ace" /libpath:"$(TAO_ROOT)\tao" /libpath:"$(TAO_ROOT)\tao\Strategies" /libpath:"$(TAO_ROOT)\tao\Messaging" /libpath:"$(TAO_ROOT)\tao\ValueType"

!ENDIF 

# Begin Target

# Name "consumer - Win32 Debug"
# Name "consumer - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;cxx;c"
# Begin Source File

SOURCE="PerfC.cpp"
# End Source File
# Begin Source File

SOURCE="PerfS.cpp"
# End Source File
# Begin Source File

SOURCE="Consumer.cpp"
# End Source File
# Begin Source File

SOURCE=.\SyncC.cpp
# End Source File
# Begin Source File

SOURCE=.\SyncS.cpp
# End Source File
# Begin Source File

SOURCE=.\WorkerThread.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hh"
# Begin Source File

SOURCE="PerfC.h"
# End Source File
# Begin Source File

SOURCE="PerfS.h"
# End Source File
# Begin Source File

SOURCE="PerfS_T.h"
# End Source File
# Begin Source File

SOURCE=.\SyncS.h
# End Source File
# Begin Source File

SOURCE="WorkerThread.h"
# End Source File
# End Group
# Begin Group "Inline Files"

# PROP Default_Filter "i;inl"
# Begin Source File

SOURCE="PerfC.inl"
# End Source File
# Begin Source File

SOURCE="PerfS.inl"
# End Source File
# Begin Source File

SOURCE="PerfS_T.inl"
# End Source File
# Begin Source File

SOURCE="SyncC.inl"
# End Source File
# Begin Source File

SOURCE="SyncS.inl"
# End Source File
# Begin Source File

SOURCE="SyncS_T.inl"
# End Source File
# End Group
# Begin Group "Template Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE="SyncS_T.cpp"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE="PerfS_T.cpp"
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Idl Files"

# PROP Default_Filter "idl"
# Begin Source File

SOURCE="Perf.idl"

!IF  "$(CFG)" == "consumer - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__PERF_="$(ACE_ROOT)\bin\tao_idl.exe"	
# Begin Custom Build - Invoking $(ACE_ROOT)\bin\tao_idl on $(InputPath)
InputPath="Perf.idl"

BuildCmds= \
	PATH=%PATH%;$(ACE_ROOT)\lib \
	$(ACE_ROOT)\bin\tao_idl -Ge 1 -GC -I$(TAO_ROOT) $(InputPath) \
	

"PerfC.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfC.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfC.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "consumer - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__PERF_="$(ACE_ROOT)\bin\tao_idl.exe"	
# Begin Custom Build - Invoking $(ACE_ROOT)\bin\tao_idl on $(InputPath)
InputPath="Perf.idl"

BuildCmds= \
	PATH=%PATH%;$(ACE_ROOT)\lib \
	$(ACE_ROOT)\bin\tao_idl -Ge 1 -GC -I$(TAO_ROOT) $(InputPath) \
	

"PerfC.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfC.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfC.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"PerfS_T.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="Sync.idl"

!IF  "$(CFG)" == "consumer - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__SYNC_="$(ACE_ROOT)\bin\tao_idl.exe"	
# Begin Custom Build - Invoking $(ACE_ROOT)\bin\tao_idl on $(InputPath)
InputPath="Sync.idl"

BuildCmds= \
	PATH=%PATH%;$(ACE_ROOT)\lib \
	$(ACE_ROOT)\bin\tao_idl -Ge 1 -GC -I$(TAO_ROOT) $(InputPath) \
	

"SyncC.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncC.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncC.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "consumer - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__SYNC_="$(ACE_ROOT)\bin\tao_idl.exe"	
# Begin Custom Build - Invoking $(ACE_ROOT)\bin\tao_idl on $(InputPath)
InputPath="Sync.idl"

BuildCmds= \
	PATH=%PATH%;$(ACE_ROOT)\lib \
	$(ACE_ROOT)\bin\tao_idl -Ge 1 -GC -I$(TAO_ROOT) $(InputPath) \
	

"SyncC.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.inl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncC.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncC.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"SyncS_T.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
