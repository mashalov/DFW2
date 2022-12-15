!define ProductName "RaidenEMS"
!define InputFolder "..\x64\release\"

!include MUI2.nsh
!include Library.nsh
!include nsDialogs.nsh
!define LIBRARY_X64
Unicode True

!define UninstallerName $(^Name)Uninstall.exe
SetCompressor /SOLID /FINAL lzma
Name ${ProductName}

BrandingText $(CopyrightInfo)

!macro VersionCheckV5 Ver1 Ver2 OutVar
 Push "${Ver1}"
 Push "${Ver2}"
  Call VersionCheckV5
 Pop "${OutVar}"
!macroend


!define Trim "!insertmacro Trim"
 
!macro Trim ResultVar String
  Push "${String}"
  Call Trim
  Pop "${ResultVar}"
!macroend

!define VersionCheckNew "!insertmacro VersionCheckV5"


var SysRqDialog
var MSBuildLabel
var RastrWinLabel

var MSBuildInstallationCheckResult
var RastrWinInstallationCheckResult
var ComponentsPath

Function .onInit

FunctionEnd





Function SystemRequirementsPage
 	nsDialogs::Create 1018
 	Pop $SysRqDialog
 	${If} $SysRqDialog == error
		Abort
	${EndIf}
	GetDlgItem $1 $HWNDPARENT 1
	EnableWindow $1 0
	!insertmacro MUI_HEADER_TEXT $(CheckSystemRqr) $(CheckSystemRqr2)
	${NSD_CreateLabel} 5u 5u 100u 12u  $(MSBuildToolsSetup)
	Pop $MSBuildLabel
	${NSD_CreateLabel} 5u 25u 100u 12u $(RastrWinSetup)
	Pop $RastrWinLabel
	Push "2.7.1.6311"
	Call CheckRastrWin
	Call CheckMSBuild
	
	${NSD_CreateLabel} 110u 5u 150u 12u $MSBuildInstallationCheckResult
	${NSD_CreateLabel} 110u 25u 150u 12u $RastrWinInstallationCheckResult
	
	StrCmp $MSBuildInstallationCheckResult $(Installed) 0 DisableInstall
	StrCmp $MSBuildInstallationCheckResult $(Installed) 0 DisableInstall
	EnableWindow $1 1
	
DisableInstall:
	nsDialogs::Show
	
FunctionEnd

Function SystemRequirementsPageLeave
FunctionEnd

Function CheckRastrWin
	ClearErrors
	Pop $R3
	StrCpy $RastrWinInstallationCheckResult $(RastrWinRegistryFailed)
	ReadRegStr $R4 HKCU Software\RastrWin3 "InstallPath"
	ReadRegStr $R1 HKCU Software\RastrWin3 "Version"
	IfErrors FailedCheckRastrWin
	StrCpy $RastrWinInstallationCheckResult $(RastrWinAstraNotFound)
	StrCpy $R0 "$R4\astra.dll"
	IfFileExists $R0 0 FailedCheckRastrWin
	StrCpy $RastrWinInstallationCheckResult "$(OldVersion)$R1$(RequiredVersion)$R3"
	${VersionCheckNew} $R3 $R1 $R0
	IntCmp $R0 1 FailedCheckRastrWin
	StrCpy $RastrWinInstallationCheckResult $(Installed)
	StrCpy $ComponentsPath $R4
FailedCheckRastrWin:
FunctionEnd


Function CheckMSBuild
	ClearErrors
	StrCpy $MSBuildInstallationCheckResult $(MSBuildVsWhere)
	nsExec::ExecToStack /OEM '"$PROGRAMFILES32\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe"'
	Pop $R0 # return value/error/timeout
	Pop $R1 # printed text, up to ${NSIS_MAX_STRLEN}
	StrCmp $R0 "error" FailedCheckMSBuild 0
	IntCmp $R0 0 0 FailedCheckMSBuild FailedCheckMSBuild
	StrCpy $MSBuildInstallationCheckResult $(MSBuildNotFound)
	${Trim} $R0 $R1
	IfFileExists "$R0" 0 FailedCheckMSBuild
	StrCpy $MSBuildInstallationCheckResult $(Installed)
FailedCheckMSBuild:
FunctionEnd


RequestExecutionLevel admin
Page custom SystemRequirementsPage SystemRequirementsPageLeave
!insertmacro MUI_PAGE_LICENSE "licenseru.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
Section "Программные модули"
	ClearErrors
	SetOutPath $ComponentsPath
	File "${InputFolder}dfw2.dll"
	File "${InputFolder}umc.dll"
	!insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_PROTECTED "${InputFolder}ResultFile.dll" $OUTDIR\ResultFile2.dll $TEMP
	WriteUninstaller "${UninstallerName}"
	IfErrors 0 CopySectionOK
	MessageBox MB_ICONSTOP $(InstallationFailed)
CopySectionOK:	
SectionEnd

Section "Uninstall"
  Delete $INSTDIR\${UninstallerName}
  Delete $INSTDIR\dfw2.dll
  Delete $INSTDIR\umc.dll
  !insertmacro UninstallLib REGDLL NOTSHARED NOREBOOT_PROTECTED $INSTDIR\ResultFile.dll
  RMDir $INSTDIR
  DeleteRegKey HKLM SOFTWARE\myApp
SectionEnd

OutFile ${ProductName}Install.exe

 
Function VersionCheckV5
 Exch $R0 ; second version number
 Exch
 Exch $R1 ; first version number
 Push $R2
 Push $R3
 Push $R4
 Push $R5 ; second version part
 Push $R6 ; first version part
 
  StrCpy $R1 $R1.
  StrCpy $R0 $R0.
 
 Next: StrCmp $R0$R1 "" 0 +3
  StrCpy $R0 0
  Goto Done
 
  StrCmp $R0 "" 0 +2
   StrCpy $R0 0.
  StrCmp $R1 "" 0 +2
   StrCpy $R1 0.
 
 StrCpy $R2 0
  IntOp $R2 $R2 + 1
  StrCpy $R4 $R1 1 $R2
  StrCmp $R4 . 0 -2
    StrCpy $R6 $R1 $R2
    IntOp $R2 $R2 + 1
    StrCpy $R1 $R1 "" $R2
 
 StrCpy $R2 0
  IntOp $R2 $R2 + 1
  StrCpy $R4 $R0 1 $R2
  StrCmp $R4 . 0 -2
    StrCpy $R5 $R0 $R2
    IntOp $R2 $R2 + 1
    StrCpy $R0 $R0 "" $R2
 
 IntCmp $R5 0 Compare
 IntCmp $R6 0 Compare
 
 StrCpy $R3 0
  StrCpy $R4 $R6 1 $R3
  IntOp $R3 $R3 + 1
  StrCmp $R4 0 -2
 
 StrCpy $R2 0
  StrCpy $R4 $R5 1 $R2
  IntOp $R2 $R2 + 1
  StrCmp $R4 0 -2
 
 IntCmp $R3 $R2 0 +2 +4
 Compare: IntCmp 1$R5 1$R6 Next 0 +3
 
  StrCpy $R0 1
  Goto Done
  StrCpy $R0 2
 
 Done:
 Pop $R6
 Pop $R5
 Pop $R4
 Pop $R3
 Pop $R2
 Pop $R1
 Exch $R0 ; output
FunctionEnd



; Trim
;   Removes leading & trailing whitespace from a string
; Usage:
;   Push 
;   Call Trim
;   Pop 
Function Trim
	Exch $R1 ; Original string
	Push $R2
 
Loop:
	StrCpy $R2 "$R1" 1
	StrCmp "$R2" " " TrimLeft
	StrCmp "$R2" "$\r" TrimLeft
	StrCmp "$R2" "$\n" TrimLeft
	StrCmp "$R2" "$\t" TrimLeft
	GoTo Loop2
TrimLeft:	
	StrCpy $R1 "$R1" "" 1
	Goto Loop
 
Loop2:
	StrCpy $R2 "$R1" 1 -1
	StrCmp "$R2" " " TrimRight
	StrCmp "$R2" "$\r" TrimRight
	StrCmp "$R2" "$\n" TrimRight
	StrCmp "$R2" "$\t" TrimRight
	GoTo Done
TrimRight:	
	StrCpy $R1 "$R1" -1
	Goto Loop2
 
Done:
	Pop $R2
	Exch $R1
FunctionEnd

!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

LangString CopyrightInfo ${LANG_ENGLISH} "${ProductName} © Eugene Mashalov 2017-2022"
LangString CopyrightInfo ${LANG_RUSSIAN} "${ProductName} © Евгений Машалов 2017-2022"
LangString CheckSystemRqr ${LANG_ENGLISH} "Checking system requirements"
LangString CheckSystemRqr ${LANG_RUSSIAN} "Проверка системных требований"
LangString CheckSystemRqr2 ${LANG_ENGLISH} "Now installer will check your computer for installed prerequisites"
LangString CheckSystemRqr2 ${LANG_RUSSIAN} "Программа установки проверяет наличие требуемого ПО в системе"
LangString MSBuildToolsSetup ${LANG_ENGLISH} "Microsoft Build Tools"
LangString MSBuildToolsSetup ${LANG_RUSSIAN} "Microsoft Build Tools"
LangString RastrWinSetup ${LANG_ENGLISH} "RastrWin"
LangString RastrWinSetup ${LANG_RUSSIAN} "RastrWin"
LangString MSBuildNotInstalled ${LANG_ENGLISH} "Not installed"
LangString MSBuildNotInstalled ${LANG_RUSSIAN} "Не установлен"
LangString RastrWinNotInstalled ${LANG_ENGLISH} "Not installed"
LangString RastrWinNotInstalled ${LANG_RUSSIAN} "Не установлен"
LangString RastrWinRegistryFailed ${LANG_ENGLISH} "No registry record found"
LangString RastrWinRegistryFailed ${LANG_RUSSIAN} "Не записи в реестре"
LangString RastrWinAstraNotFound ${LANG_ENGLISH} "astra.dll not found"
LangString RastrWinAstraNotFound ${LANG_RUSSIAN} "Не найдена astra.dll"
LangString OldVersion ${LANG_ENGLISH} "Outdated "
LangString OldVersion ${LANG_RUSSIAN} "Версия "
LangString RequiredVersion ${LANG_ENGLISH} ", required "
LangString RequiredVersion ${LANG_RUSSIAN} ", требуется "
LangString Installed ${LANG_ENGLISH} "Installed"
LangString Installed ${LANG_RUSSIAN} "Установлен"
LangString MSBuildVsWhere ${LANG_ENGLISH} "Vswhere failed"
LangString MSBuildVsWhere ${LANG_RUSSIAN} "Ошибка vswhere"
LangString MSBuildNotFound ${LANG_ENGLISH} "msbuild.exe not found"
LangString MSBuildNotFound ${LANG_RUSSIAN} "Не найден msbuild.exe"
LangString InstallationFailed ${LANG_ENGLISH} "Installation failed. See details"
LangString InstallationFailed ${LANG_RUSSIAN} "Установка не выполнена. Детали в протоколе"