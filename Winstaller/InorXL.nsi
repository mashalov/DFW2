;InorXL Install script
SetCompressor /solid LZMA
;SetCompress off

!define ver "1.3.0.132"

!include "Library.nsh"
!include "UAC.nsh"
!include "x64.nsh"

!define MUI_CUSTOMFUNCTION_GUIINIT		CustomGuiInit
!define MUI_CUSTOMFUNCTION_UNGUIINIT	CustomGuiUnint
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "Inorheader.bmp"
;!define MUI_BGCOLOR "000000" 

!include "MUI2.nsh"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

LangString CopyrightInfo ${LANG_ENGLISH} "${ver} © Eugene Mashalov 2013"
LangString CopyrightInfo ${LANG_RUSSIAN} "${ver} © Евгений Машалов 2013"
LangString CheckSystemRqr ${LANG_ENGLISH} "Checking system requirements"
LangString CheckSystemRqr ${LANG_RUSSIAN} "Проверка системных требований"
LangString CheckSystemRqr2 ${LANG_ENGLISH} "Now installer will check your computer hardware and software capabilites"
LangString CheckSystemRqr2 ${LANG_RUSSIAN} "Будет выполнен сбор сведений о конфигурации компьютера и программного обеспечения"
LangString LicenseTitle ${LANG_ENGLISH} "License agreement"
LangString LicenseTitle ${LANG_RUSSIAN} "Лицензия на использование"
LangString LicensePrompt ${LANG_ENGLISH} "To continue using this program you must read and accept the following license agreement"
LangString LicensePrompt ${LANG_RUSSIAN} "Для того, чтобы использовать эту программу, Вы должны прочитать лицензионное соглашение и согласиться с ним"
LangString SSE3Support ${LANG_ENGLISH} "SSE3 support"
LangString SSE3Support ${LANG_RUSSIAN} "Поддержка SSE3"
LangString MSExcelVersion ${LANG_RUSSIAN} "Версия Microsoft Excel"
LangString MSExcelVersion ${LANG_ENGLISH} "Microsoft Excel version"
LangString RastrWinDB ${LANG_RUSSIAN} "Ядро Rastrwin (не обязательно)"
LangString RastrWinDB ${LANG_ENGLISH} "Rastrwin DB engine (optional)"
LangString InstallInProgress ${LANG_RUSSIAN} "Идет установка"
LangString InstallInProgress ${LANG_ENGLISH} "Install is in progress"
LangString WaitForInstallComplete ${LANG_RUSSIAN} "Дождитесь завершения процесса установки"
LangString WaitForInstallComplete ${LANG_ENGLISH} "Wait unitl install is complete"
LangString ThisProgramRqAdmin ${LANG_RUSSIAN} "Эта программа установки требует привилегий администратора !"
LangString ThisProgramRqAdmin ${LANG_ENGLISH} "This program requires administrator privileges !"
LangString CannotElevate ${LANG_RUSSIAN} "Невозможно поднять привилегии пользователя. Ошибка: $0"
LangString CannotElevate ${LANG_ENGLISH} "Cannot elevate user rights. Error: $0"
LicenseLangString LicenseFileName ${LANG_RUSSIAN} "License.txt"
LicenseLangString LicenseFileName ${LANG_ENGLISH} "LicenseEn.txt"
LangString ExecInstFailure ${LANG_RUSSIAN} "При установке возникли ошибки. Детальная информация доступна в протоколе установки"
LangString ExecInstFailure ${LANG_ENGLISH} "There are errors encountered while installation. See log for detailed information"



Var InstallX64

Name "InorXL" 
OutFile "InorXLSetup.exe"
InstallDir "$PROGRAMFILES\InorXL"
RequestExecutionLevel user
BrandingText $(CopyrightInfo)
Icon "install.ico"
SetOverwrite On
ShowInstDetails show

!include nsDialogs.nsh
!include LogicLib.nsh


Function CustomGuiInit
	InitPluginsDir
	File /oname=$PLUGINSDIR\ok.ico "Background\ok.ico"
	File /oname=$PLUGINSDIR\failed.ico "Background\failed.ico"
	File /oname=$PLUGINSDIR\x64.ico "Background\x64.ico"
	File /oname=$PLUGINSDIR\bg.jpg "Background\bg.jpg"
	File /oname=$PLUGINSDIR\bg2.jpg "Background\bg2.jpg"
	File /oname=$PLUGINSDIR\djpeg.exe "Background\djpeg.exe"
	File /oname=$PLUGINSDIR\jpeg62.dll "Background\jpeg62.dll"
	File /oname=$PLUGINSDIR\librle3.dll "Background\librle3.dll"
    File /oname=$PLUGINSDIR\pr.exe "Background\PhotoResize400.exe"
	System::Call "user32::GetSystemMetrics(i 0) i .r0"
	System::Call "user32::GetSystemMetrics(i 1) i .r1"
	
	nsExec::ExecToLog '"$PLUGINSDIR\pr.exe" -a$0x$1 "$PLUGINSDIR\bg.jpg"'
	nsExec::ExecToLog '"$PLUGINSDIR\djpeg.exe" -bmp -outfile "$PLUGINSDIR\bg.bmp" "$PLUGINSDIR\bg-A$0x$1.jpg"'
	
	newadvsplash::show 1500 300 300 -1 /NOCANCEL "$PLUGINSDIR\bg2.jpg"
	Sleep 500
 	BgImage::SetBg /GRADIENT 27 29 161 5 9 94 
	BgImage::SetReturn on
	BgImage::AddImage $PLUGINSDIR\bg.bmp 0 0
	BgImage::Redraw
FunctionEnd

Function CustomGuiUnint
 	BgImage::Destroy
FunctionEnd

Function .OnInit
   ; !insertmacro MUI_LANGDLL_DISPLAY
UAC_Elevate:
    UAC::RunElevated 
    StrCmp 1223 $0 UAC_ElevationAborted ; UAC dialog aborted by user?
    StrCmp 0 $0 0 UAC_Err ; Error?
    StrCmp 1 $1 0 UAC_Success ;Are we the real deal or just the wrapper?
    Quit
 
UAC_Err:
    MessageBox mb_iconstop $(CannotElevate)
    Abort
 
UAC_ElevationAborted:
    # elevation was aborted, run as normal?
    MessageBox mb_iconstop $(ThisProgramRqAdmin)
    Abort
 
UAC_Success:
    StrCmp 1 $3 +4 ;Admin?
    StrCmp 3 $1 0 UAC_ElevationAborted ;Try again?
    MessageBox mb_iconstop $(ThisProgramRqAdmin)
    goto UAC_Elevate 
	

     ; add your initialization code here
FunctionEnd

Function .OnInstFailed
    UAC::Unload
FunctionEnd
 
Function .OnInstSuccess
    UAC::Unload
FunctionEnd

Function un.OnInit
UAC_Elevate:
    UAC::RunElevated 
    StrCmp 1223 $0 UAC_ElevationAborted ; UAC dialog aborted by user?
    StrCmp 0 $0 0 UAC_Err ; Error?
    StrCmp 1 $1 0 UAC_Success ;Are we the real deal or just the wrapper?
    Quit
 
UAC_Err:
    MessageBox mb_iconstop $(CannotElevate)
    Abort
 
UAC_ElevationAborted:
    # elevation was aborted, run as normal?
    MessageBox mb_iconstop $(ThisProgramRqAdmin)
    Abort
 
UAC_Success:
    StrCmp 1 $3 +4 ;Admin?
    StrCmp 3 $1 0 UAC_ElevationAborted ;Try again?
    MessageBox mb_iconstop $(ThisProgramRqAdmin)
    goto UAC_Elevate 
FunctionEnd

Function un.OnUnInstFailed
    UAC::Unload
FunctionEnd
 
Function un.OnUnInstSuccess
    UAC::Unload
FunctionEnd

var SysRqDialog
var SSE3Label
var ExcelLabel
var RastrLabel
var FWLabel
var SSE3Bmp
var ExcelBmp
var RastrBmp
var FWBmp
var X64Bmp
var SSE3ImageHandle
var ExcelImageHandle
var RastrImageHandle
var FWImageHandle
var X64ImageHandle

!ifndef RDW_INVALIDATE
	!define RDW_INVALIDATE 0x0001
	!define RDW_INTERNALPAINT 0x0002
	!define RDW_ERASE 0x0004
	!define RDW_VALIDATE 0x0008
	!define RDW_NOINTERNALPAINT 0x0010
	!define RDW_NOERASE 0x0020
	!define RDW_NOCHILDREN 0x0040
	!define RDW_ALLCHILDREN 0x0080
	!define RDW_UPDATENOW 0x0100
	!define RDW_ERASENOW 0x0200
	!define RDW_FRAME 0x0400
	!define RDW_NOFRAME 0x0800
!endif


Function SystemRequirementsPage
 	nsDialogs::Create 1018
 	Pop $SysRqDialog
 	${If} $SysRqDialog == error
		Abort
	${EndIf}
	GetDlgItem $1 $HWNDPARENT 1
	EnableWindow $1 0
!insertmacro MUI_HEADER_TEXT $(CheckSystemRqr) $(CheckSystemRqr2)
	${NSD_CreateLabel} 5u 5u 150u 12u  $(SSE3Support) 
	Pop $SSE3Label
	${NSD_CreateLabel} 5u 25u 150u 12u $(MSExcelVersion)
	Pop $ExcelLabel
	${NSD_CreateLabel} 5u 45u 150u 12u $(RastrWinDB)
	Pop $RastrLabel
	${NSD_CreateLabel} 5u 65u 150u 12u ".NET Framework 4.0"
	Pop $FWLabel
	
	cpuid::CheckSSE3
	pop $R1
	StrCmp $R1 "1" 0 +3
	StrCpy $R1 "ok.ico"
	goto +2
	StrCpy $R1 "failed.ico"
	
	StrCpy $InstallX64 "0"
	cpuid::CheckExcel
	pop $R5		; x64 - "1"
	pop $R2		; Excel present - "1"
	StrCmp $R2 "1" 0 +3
	StrCpy $R2 "ok.ico"
	goto +3
	StrCpy $R2 "failed.ico"
	StrCpy $R5 "0"
	
	StrCmp $R5 "0" 0 +6		; don't even check RastrWin in case Excel is x64
	cpuid::CheckRastr
	pop $R3
	StrCmp $R3 "1" 0 +3
	StrCpy $R3 "ok.ico"
	goto +2
	StrCpy $R3 "failed.ico"

	IfFileExists "$WINDIR\Microsoft.NET\Framework\v4.0.30319" 0 +3
	StrCpy $R4 "ok.ico"
	goto +2
	Strcpy $R4 "failed.ico"
	
	
	StrCpy $R0 "$PLUGINSDIR\"
		
	StrCpy $R1 "$R0$R1"
	StrCpy $R2 "$R0$R2"
	StrCpy $R3 "$R0$R3"
	StrCpy $R4 "$R0$R4"

			
	${NSD_CreateIcon} -25u 1u 12 12 ""
	Pop $SSE3Bmp
	${NSD_SetIcon} $SSE3Bmp $R1 $SSE3ImageHandle

	${NSD_CreateIcon} -25u 21u 12 12 ""
	Pop $ExcelBmp
	${NSD_SetIcon} $ExcelBmp $R2 $ExcelImageHandle

	${NSD_CreateIcon} -25u 41u 12 12 ""
	Pop $RastrBmp
	${NSD_SetIcon} $RastrBmp $R3 $RastrImageHandle

	${NSD_CreateIcon} -25u 61u 12 12 ""
	Pop $FWBmp
	${NSD_SetIcon} $FWBmp $R4 $FWImageHandle
	
	StrCpy $InstallX64 "0"
	StrCmp $R5 "1" 0 +6
	StrCpy $InstallX64 "1"
	${NSD_CreateIcon} -45u 21u 32 32 ""
	Pop $X64Bmp
	StrCpy $R5 "$R0x64.ico"
	${NSD_SetIcon} $X64Bmp $R5 $X64ImageHandle
	
	StrCmp $R1 "$PLUGINSDIR\failed.ico" DisableInstall
	StrCmp $R2 "$PLUGINSDIR\failed.ico" DisableInstall
	StrCmp $R4 "$PLUGINSDIR\failed.ico" DisableInstall

	EnableWindow $1 1

DisableInstall:

	nsDialogs::Show
	${NSD_FreeIcon} $SSE3ImageHandle
	${NSD_FreeIcon} $ExcelImageHandle
	${NSD_FreeIcon} $RastrImageHandle
	${NSD_FreeIcon} $FWImageHandle
	${NSD_FreeIcon} $X64ImageHandle
FunctionEnd

Function SystemRequirementsPageLeave
FunctionEnd

!define MUI_PAGE_HEADER_TEXT $(LicenseTitle)
!define MUI_PAGE_HEADER_SUBTEXT $(LicensePrompt)
!insertmacro MUI_PAGE_LICENSE $(LicenseFileName)
Page custom SystemRequirementsPage SystemRequirementsPageLeave
Page instfiles

Section "Install Modules"
   !insertmacro MUI_HEADER_TEXT $(InstallInProgress) $(WaitForInstallComplete)
   ClearErrors
   ${DisableX64FSRedirection}

   StrCpy $R4 "\InorXL"

   ${if} $InstallX64 == "1"
	StrCpy $INSTDIR "$PROGRAMFILES64$R4"
   ${Else}
	StrCpy $INSTDIR "$PROGRAMFILES$R4"
   ${EndIf}
   SetOutPath "$INSTDIR"

   !define LIBRARY_IGNORE_VERSION
   ${if} $InstallX64 == "1"
	   ClearErrors
	   !define LIBRARY_X64
	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\InorXL\x64\Release\InorXL.dll" $INSTDIR\inorXL.dll "$INSTDIR"
	   !insertmacro InstallLib TLB NOTSHARED NOREBOOT_NOTPROTECTED "..\InorXLTaskbars\bin\x64\Release\InorXLTaskbars.tlb" $INSTDIR\InorXLTaskbars.tlb "$INSTDIR"
	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\MiniCAD\x64\Release\MiniCAD.dll" $INSTDIR\MiniCAD.dll "$INSTDIR"
	   !insertmacro InstallLib TLB NOTSHARED NOREBOOT_NOTPROTECTED "..\MiniCADUI\bin\x64\Release\MiniCADUI.tlb" $INSTDIR\MiniCADUI.tlb "$INSTDIR"
	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\Chart\x64\Release\Chart.dll" $INSTDIR\Chart.dll "$INSTDIR"
	   File /r ..\InorXL\x64\Release\InorXLLib.dll
	   File /r ..\MiniCAD\x64\Release\AxMiniCADLib.dll
	   File /r ..\MiniCAD\x64\Release\MiniCADLib.dll
	   File /r ..\MiniCADUI\bin\x64\Release\MiniCADUI.dll
	   File /r ..\Chart\x64\Release\AxChartLib.dll
	   File /r ..\Chart\x64\Release\ChartLib.dll
	   File /r ..\MiniCAD\win32\Release\pts55f.ttf
	   File /r ..\MiniCAD\win32\Release\pts75f.ttf	   
 	   !undef LIBRARY_X64
	   Push "v4.0"
	   Call GetDotNetDir
	   Pop $R0 ; .net framework v4.0 installation directory
	   StrCmpS "" $R0 NetNotFound
	   File /r ..\InorXLTaskbars\bin\x64\Release\InorXLTaskbars.dll
	   SetOutPath "$INSTDIR\en"
	   File /r ..\InorXLTaskbars\bin\x64\Release\en\*.*
	   File /r ..\MiniCADUI\bin\x64\Release\en\*.*	   
	   SetOutPath "$INSTDIR\ru"
	   File /r ..\InorXLTaskbars\bin\x64\Release\ru\*.*
	   File /r ..\MiniCADUI\bin\x64\Release\ru\*.*
	   SetOutPath "$INSTDIR"
	   IfErrors Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLTaskbars.dll" /codebase'
	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADUI.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxMiniCADLib.dll" /codebase'
  	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxChartLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\ChartLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst64Error
	   goto Inst64OK
Inst64Error:
	   MessageBox mb_iconstop $(ExecInstFailure)
	   Abort
Inst64OK:
   ${Else}
	   ClearErrors 
   	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\InorXL\win32\Release\InorXL.dll" $INSTDIR\inorXL.dll "$INSTDIR"
	   !insertmacro InstallLib TLB NOTSHARED NOREBOOT_NOTPROTECTED "..\InorXLTaskbars\bin\x86\Release\InorXLTaskbars.tlb" $INSTDIR\InorXLTaskbars.tlb "$INSTDIR"
   	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\MiniCAD\win32\Release\MiniCAD.dll" $INSTDIR\MiniCAD.dll "$INSTDIR"
	   !insertmacro InstallLib TLB NOTSHARED NOREBOOT_NOTPROTECTED "..\MiniCADUI\bin\x86\Release\MiniCADUI.tlb" $INSTDIR\MiniCADUI.tlb "$INSTDIR"
	   !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED "..\Chart\win32\Release\Chart.dll" $INSTDIR\Chart.dll "$INSTDIR"
	   File /r ..\InorXL\win32\Release\InorXLLib.dll
	   File /r ..\MiniCAD\win32\Release\AxMiniCADLib.dll
	   File /r ..\MiniCAD\win32\Release\MiniCADLib.dll
	   File /r ..\MiniCADUI\bin\x86\Release\MiniCADUI.dll
	   File /r ..\Chart\win32\Release\AxChartLib.dll
	   File /r ..\Chart\win32\Release\ChartLib.dll
   	   File /r ..\MiniCAD\win32\Release\pts55f.ttf
	   File /r ..\MiniCAD\win32\Release\pts75f.ttf	   
   	   Push "v4.0"
	   Call GetDotNetDir
	   Pop $R0 ; .net framework v4.0 installation directory
	   StrCmpS "" $R0 NetNotFound
	   File /r ..\InorXLTaskbars\bin\x86\Release\InorXLTaskbars.dll
	   SetOutPath "$INSTDIR\en"
	   File /r ..\InorXLTaskbars\bin\x86\Release\en\*.*
	   File /r ..\MiniCADUI\bin\x86\Release\en\*.*
	   SetOutPath "$INSTDIR\ru"
	   File /r ..\InorXLTaskbars\bin\x86\Release\ru\*.*
	   File /r ..\MiniCADUI\bin\x86\Release\ru\*.*
	   SetOutPath "$INSTDIR"
	   IfErrors Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLTaskbars.dll" /codebase'
	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
   	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADUI.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxMiniCADLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxChartLib.dll" /codebase'
       Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\ChartLib.dll" /codebase'
   	   Pop $0
	   IntCmp $0 0 0 0 Inst32Error
	   goto Inst32OK
Inst32Error:
	   MessageBox mb_iconstop $(ExecInstFailure)
	   Abort
Inst32OK:
   ${EndIf}
   WriteUninstaller "$INSTDIR\Uninstall.exe"
   SetShellVarContext all
   SetOutPath $INSTDIR\Docs
   File /r  Docs\*.*
   SetOutPath $DOCUMENTS\InorXL\Tests
   File /r  Tests\*.*
;   ReadRegStr  $0 HKCU "Software\InorXL\MiniCAD" "InitialDir"
;   StrCmp $0 "" 0 MiniCADInitialDirPresent
;   WriteRegStr HKCU "Software\InorXL\MiniCAD" "InitialDir" $DOCUMENTS\InorXL\Tests
;MiniCADInitialDirPresent:
;   ReadRegStr $0 HKCU "Software\InorXL\" "LastRgImportDir"
;   StrCmp $0 "" 0 Rg2ImportPresent
;   WriteRegStr HKCU "Software\InorXL\" "LastRgImportDir" $DOCUMENTS\InorXL\Tests
;Rg2ImportPresent:
;   ReadRegStr  $0 HKCU "Software\InorXL\CPF" "InitialDir"
;   StrCmp $0 "" 0 CPFDirPresent
;   WriteRegStr HKCU "Software\InorXL\CPF" "InitialDir" $DOCUMENTS\InorXL\Tests\CPF
;CPFDirPresent:
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL" "DisplayName" "InorXL"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL" "DisplayIcon" "$\"$INSTDIR\uninstall.exe$\""
   StrCpy $0 "x64"
   StrCmp $InstallX64 "1" RegUninstBitness
   StrCpy $0 "x86"
RegUninstBitness:   
   SetRegView 32
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL" "UninstallerBittness" "$0"
   goto NetRegDone
NetNotFound:
   Abort ".Net framework не найден"
NetRegDone:
SectionEnd

Section "Uninstall"
   SetRegView 32
   ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL" "UninstallerBittness"
   StrCpy $InstallX64 "1"
   StrCmp $0 "x64" UninstallCorrect
   StrCpy $InstallX64 "0"
UninstallCorrect:
   ${if} $InstallX64 == "1"
		DetailPrint "x64 mode"
        !define LIBRARY_X64
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\inorxl.dll"
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\MiniCAD.dll"
		!insertmacro UnInstallLib TLB NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\InorXLTaskbars.tlb"
		!insertmacro UnInstallLib TLB NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\MiniCADUI.tlb"
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\Chart.dll"
        !undef LIBRARY_X64
   ${Else}
		DetailPrint "x86 mode"
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\inorxl.dll"
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\MiniCAD.dll"
		!insertmacro UnInstallLib TLB NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\InorXLTaskbars.tlb"
		!insertmacro UnInstallLib TLB NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\MiniCADUI.tlb"
		!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\Chart.dll"
   ${Endif}
   Push "v4.0"
   Call un.GetDotNetDir
   Pop $R0 ; .net framework v4.0 installation directory
   StrCmpS "" $R0 NetNotFoundUninstall
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLTaskbars.dll" /u' 
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADUI.dll" /u' 
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxMiniCADLib.dll" /u' 
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\MiniCADLib.dll" /u'
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\InorXLLib.dll" /u'  
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\AxChartLib.dll" /u'
   nsExec::ExecToLog /OEM '"$R0\RegAsm.exe" "$INSTDIR\ChartLib.dll" /u'   
   SetRegView 64
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL"
   SetRegView 32
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\InorXL"
   RMDir /r "$INSTDIR"
   
NetNotFoundUninstall:
SectionEnd

Function un.GetDotNetDir
	ClearErrors
	Exch $R0 ; Set R0 to .net version major
	Push $R1
	Push $R2

	${If} $InstallX64 == "1"
		SetRegView 64
	${Endif}
 
	; set R1 to minor version number of the installed .NET runtime
	EnumRegValue $R1 HKLM \
		"Software\Microsoft\.NetFramework\policy\$R0" 0
	IfErrors getdotnetdir_err
 
	; set R2 to .NET install dir root
	ReadRegStr $R2 HKLM \
		"Software\Microsoft\.NetFramework" "InstallRoot"
	IfErrors getdotnetdir_err
 
	; set R0 to the .NET install dir full
	StrCpy $R0 "$R2$R0.$R1"
 
getdotnetdir_end:
	Pop $R2
	Pop $R1
	Exch $R0 ; return .net install dir full
	Return

getdotnetdir_err:
	StrCpy $R0 ""
	Goto getdotnetdir_end
 
FunctionEnd


Function GetDotNetDir
        ClearErrors
	Exch $R0 ; Set R0 to .net version major
	Push $R1
	Push $R2

	${If} $InstallX64 == "1"
		SetRegView 64
	${Endif}
	

	; set R1 to minor version number of the installed .NET runtime
	EnumRegValue $R1 HKLM \
		"Software\Microsoft\.NetFramework\policy\$R0" 0

	IfErrors getdotnetdir_err


	; set R2 to .NET install dir root
	ReadRegStr $R2 HKLM \
		"Software\Microsoft\.NetFramework" "InstallRoot"

	IfErrors getdotnetdir_err

 
	; set R0 to the .NET install dir full
	StrCpy $R0 "$R2$R0.$R1"
 
getdotnetdir_end:
	Pop $R2
	Pop $R1
	Exch $R0 ; return .net install dir full
	Return

getdotnetdir_err:
	StrCpy $R0 ""
	Goto getdotnetdir_end
 
FunctionEnd

;Section "-Install VCRedist"
;!insertmacro MUI_HEADER_TEXT "Идет установка" "Дождитесь завершения процесса установки"
;  SectionIn 1 2
;   ClearErrors
;   DetailPrint "Установка Microsoft Visual C++ 2008 Redistributable"
;   SetDetailsPrint listonly
;   File /oname=$PLUGINSDIR\vcredist_x86.exe vcredist_x86.exe
;   ExecWait '"$PLUGINSDIR\vcredist_x86.exe" /Q'$0
;   IfErrors VCR_Error
;   IntCmp $0 0 VCR_Done
;VCR_Error:
;   MessageBox MB_ICONSTOP "Ошибка при установке Visual C++ 2008 Redistributable"
;VCR_Done:
;   SetDetailsPrint both
;SectionEnd

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "InorXL Application"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© Eugene Mashalov"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "InorXL Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" ${ver}
VIAddVersionKey /LANG=${LANG_RUSSIAN} "ProductName" "Программа InorXL"
VIAddVersionKey /LANG=${LANG_RUSSIAN} "LegalCopyright" "© Евгений Машалов"
VIAddVersionKey /LANG=${LANG_RUSSIAN} "FileDescription" "Программа установки InorXL"
VIAddVersionKey /LANG=${LANG_RUSSIAN} "FileVersion" ${ver}
VIProductVersion ${ver}



