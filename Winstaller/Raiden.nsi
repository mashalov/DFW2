!define ProductName "RaidenEMS"
!define Version "1.0.1.123"
!define RastrWinX64VersionRequired "2.7.1.6388"
!define RastrWinX86VersionRequired "2.7.0.6387"

!getdllversion "..\release dll\dfw2.dll" DllVer

!define InputFolderX64 "..\x64\release\"
!define InputFolderX86 "..\release\"
!define ModelReferencePath "${ProductName}\Reference"
!define TestStuffPath "${ProductName}\Test"


!define VisualStudioLink "https://visualstudio.microsoft.com/ru/downloads/"
!define MSBuildToolsLink "https://aka.ms/vs/17/release/vs_BuildTools.exe"


!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP header.bmp
;!define MUI_WELCOMEFINISHPAGE_BITMAP header.bmp
;!define MUI_BGCOLOR 000000
;!define MUI_TEXTCOLOR FFFFFF

!include MUI2.nsh
!include Library.nsh
!include nsDialogs.nsh
!include LogicLib.nsh
Unicode True
ShowInstDetails show
ShowUninstDetails show

!define RastrWin3RegKey "Software\RastrWin3"
!define ProductRegKey "${RastrWin3RegKey}\${ProductName}"
!define VersionVerb "Version"
!define InstallPathVerb "InstallPath"
!define UninstallerName $(^Name)Uninstall.exe
!define ReadmeFileName "RaidenEMS.txt"
SetCompressor /SOLID /FINAL lzma
Name ${ProductName}

BrandingText $(CopyrightInfo)
!define MUI_ICON "installer.ico"

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
var RastrWinX64Label
var RastrWinX86Label

var MSBuildInstallationCheckResult
var RastrWinX64InstallationCheckResult
var RastrWinX86InstallationCheckResult
var RastrWinX64ComponentsPath
var RastrWinX86ComponentsPath

Function .onInit
	SectionSetText 0 $(ComponentsX64)
	SectionSetText 1 $(ComponentsX86)
	InitPluginsDir
FunctionEnd

 Function .onInstSuccess
    MessageBox MB_YESNO|MB_ICONQUESTION $(ShowReleaseNotes) IDNO NoReadme
      Exec "notepad.exe $TEMP\${ReadmeFileName}"
    NoReadme:
  FunctionEnd

Function onVSLink
	Pop $0
	ExecShell "open" ${VisualStudioLink}
FunctionEnd


Function onMSBuildLink
	Pop $0
	ExecShell "open" ${MSBuildToolsLink}
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
	${NSD_CreateLabel} 5u 25u 100u 12u $(RastrWinSetupX64)
	Pop $RastrWinX64Label
	${NSD_CreateLabel} 5u 45u 100u 12u $(RastrWinSetupX86)
	Pop $RastrWinX86Label
	Push ${RastrWinX64VersionRequired}
	Call CheckRastrWinX64
	Push ${RastrWinX86VersionRequired}
	Call CheckRastrWinX86
	Call CheckMSBuild
	
	${NSD_CreateLabel} 110u 5u 150u 12u $MSBuildInstallationCheckResult
	${NSD_CreateLabel} 110u 25u 150u 12u $RastrWinX64InstallationCheckResult
	${NSD_CreateLabel} 110u 45u 150u 12u $RastrWinX86InstallationCheckResult
	
	StrCmp $MSBuildInstallationCheckResult $(Installed) 0 MSBuildNotInstalled
	StrCmp $RastrWinX64InstallationCheckResult $(Installed) 0 RastrX64NotInstalled
	SectionSetFlags 0 1
	StrCmp $RastrWinX86InstallationCheckResult $(Installed) 0 RastrX86NotInstalled
	Goto EnableInstall
	
MSBuildNotInstalled:
	${NSD_CreateLabel} 5u 65u 250u 25u $(MSBuildInstallationInstructions)
	${NSD_CreateLink} 5u 95u 250u 12u ${VisualStudioLink}
	Pop $1
	${NSD_CreateLink} 5u 105u 250u 12u ${MSBuildToolsLink}
	Pop $0
	${NSD_OnClick} $1 onVSLink
	${NSD_OnClick} $0 onMSBuildLink
	Goto DisableInstall

RastrX86NotInstalled:	
	SectionSetFlags 1 16
	Goto EnableInstall
	
RastrX64NotInstalled:
	SectionSetFlags 0 16
	StrCmp $RastrWinX86InstallationCheckResult $(Installed) 0 DisableInstall
	SectionSetFlags 1 1

EnableInstall:	
	EnableWindow $1 1
	
DisableInstall:
	nsDialogs::Show
	
FunctionEnd

Function SystemRequirementsPageLeave
FunctionEnd

Function CheckRastrWinX64
	ClearErrors
	Pop $R3
	StrCpy $RastrWinX64InstallationCheckResult $(RastrWinRegistryFailed)
	SetRegView 64
	ReadRegStr $R4 HKLM ${RastrWin3RegKey} ${InstallPathVerb}
	ReadRegStr $R1 HKLM ${RastrWin3RegKey} ${VersionVerb}
	IfErrors FailedCheckRastrWinX64
	StrCpy $RastrWinX64InstallationCheckResult $(RastrWinAstraNotFound)
	StrCpy $R0 "$R4\astra.dll"
	IfFileExists $R0 0 FailedCheckRastrWinX64
	StrCpy $RastrWinX64InstallationCheckResult "$(OldVersion)$R1$(RequiredVersion)$R3"
	${VersionCheckNew} $R3 $R1 $R0
	IntCmp $R0 1 FailedCheckRastrWinX64
	StrCpy $RastrWinX64InstallationCheckResult $(Installed)
	StrCpy $RastrWinX64ComponentsPath $R4
FailedCheckRastrWinX64:
FunctionEnd

Function CheckRastrWinX86
	ClearErrors
	Pop $R3
	StrCpy $RastrWinX86InstallationCheckResult $(RastrWinRegistryFailed)
	SetRegView 32
	ReadRegStr $R4 HKLM ${RastrWin3RegKey} ${InstallPathVerb}
	ReadRegStr $R1 HKLM ${RastrWin3RegKey} ${VersionVerb}
	IfErrors FailedCheckRastrWinX86
	StrCpy $RastrWinX86InstallationCheckResult $(RastrWinAstraNotFound)
	StrCpy $R0 "$R4\astra.dll"
	IfFileExists $R0 0 FailedCheckRastrWinX86
	StrCpy $RastrWinX86InstallationCheckResult "$(OldVersion)$R1$(RequiredVersion)$R3"
	${VersionCheckNew} $R3 $R1 $R0
	IntCmp $R0 1 FailedCheckRastrWinX86
	StrCpy $RastrWinX86InstallationCheckResult $(Installed)
	StrCpy $RastrWinX86ComponentsPath $R4
FailedCheckRastrWinX86:
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


Section InstallX64 0
	ClearErrors
	DetailPrint '$(Installing) "$(ComponentsX64)"'
	SetRegView 64
	SetOutPath $RastrWinX64ComponentsPath
	File "${InputFolderX64}..\release dll\dfw2.dll"
	File "${InputFolderX64}umc.dll"
	!define LIBRARY_X64
	WriteRegStr HKLM ${ProductRegKey} ${VersionVerb} ${Version}
	SetOutPath $RastrWinX64ComponentsPath\${ModelReferencePath}
	File /r /x ".vs" "${InputFolderX64}\..\..\ReferenceCustomModel\*.*"
	SetOutPath $RastrWinX64ComponentsPath\${TestStuffPath}
	File "${InputFolderX64}BatchTest.exe"
	File "${InputFolderX64}Scn2Dfw.rbs"
	File "${InputFolderX64}ModelCorrect.rbs"
	File "${InputFolderX64}config.json"
	File "${InputFolderX64}Тестовая утилита.pdf"
	SetOutPath $RastrWinX64ComponentsPath\PDF\Raiden
	File "${InputFolderX64}Руководство администратора RaidenEMS.pdf"
	File "${InputFolderX64}Руководство пользователя RaidenEMS.pdf"
	File /oname=$TEMP\${ReadmeFileName} ${ReadmeFileName} 
	WriteUninstaller "$RastrWinX64ComponentsPath\${UninstallerName}"
	IfErrors 0 InstallX64OK
	MessageBox MB_ICONSTOP $(InstallationFailed)
InstallX64OK:	
SectionEnd

Section InstallX86 1
	ClearErrors
	DetailPrint '$(Installing) "$(ComponentsX86)"'
	SetRegView 32
	SetOutPath $RastrWinX86ComponentsPath
	File "${InputFolderX86}..\release dll\dfw2.dll"
	File "${InputFolderX86}umc.dll"
	!undef LIBRARY_X64
	WriteRegStr HKLM ${ProductRegKey} ${VersionVerb} ${Version}
	SetOutPath $RastrWinX86ComponentsPath\${ModelReferencePath}
	File /r /x ".vs" "${InputFolderX86}\..\ReferenceCustomModel\*.*"
	SetOutPath $RastrWinX64ComponentsPath\${TestStuffPath}
	File "${InputFolderX86}BatchTest.exe"
	File "${InputFolderX64}Scn2Dfw.rbs"
	File "${InputFolderX64}ModelCorrect.rbs"
	File "${InputFolderX64}config.json"
	File "${InputFolderX64}Тестовая утилита.pdf"
	SetOutPath $RastrWinX86ComponentsPath\PDF\Raiden
	File "${InputFolderX64}Руководство администратора RaidenEMS.pdf"
	File "${InputFolderX64}Руководство пользователя RaidenEMS.pdf"
	File /oname=$TEMP\${ReadmeFileName} ${ReadmeFileName} 
	WriteUninstaller "$RastrWinX86ComponentsPath\${UninstallerName}"
	IfErrors 0 InstallX86OK
	MessageBox MB_ICONSTOP $(InstallationFailed)
InstallX86OK:	
SectionEnd

Section "Uninstall"
	SetRegView 64
	ReadRegStr $R0 HKLM ${RastrWin3RegKey} ${InstallPathVerb}
	SetRegView 32
	ReadRegStr $R1 HKLM ${RastrWin3RegKey} ${InstallPathVerb}
	StrCmp $R0 $INSTDIR 0 UninstallX86
	SetRegView 64
	!define LIBRARY_X64
	DetailPrint '$(UnInstalling) "$(ComponentsX64)"'
	goto UninstallCommon
UninstallX86:
DetailPrint '$(UnInstalling) "$(ComponentsX86)"'
	goto UninstallCommon
UninstallCommon:
	Delete $INSTDIR\${UninstallerName}
	Delete $INSTDIR\dfw2.dll
	Delete $INSTDIR\umc.dll
	RmDir /r $INSTDIR\PDF\Raiden
	DeleteRegKey HKLM ${ProductRegKey}
	RmDir /r $INSTDIR\${ProductName}
SectionEnd

OutFile "${ProductName}Install ${Version}.exe"
 
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

LangString CopyrightInfo ${LANG_ENGLISH} "${ProductName} ${Version} © Eugene Mashalov 2017-2023"
LangString CopyrightInfo ${LANG_RUSSIAN} "${ProductName} ${Version} © Евгений Машалов 2017-2023"
LangString CheckSystemRqr ${LANG_ENGLISH} "Checking system requirements"
LangString CheckSystemRqr ${LANG_RUSSIAN} "Проверка системных требований"
LangString CheckSystemRqr2 ${LANG_ENGLISH} "Now installer will check your computer for installed prerequisites"
LangString CheckSystemRqr2 ${LANG_RUSSIAN} "Программа установки проверяет наличие требуемого ПО в системе"
LangString MSBuildToolsSetup ${LANG_ENGLISH} "Microsoft Build Tools"
LangString MSBuildToolsSetup ${LANG_RUSSIAN} "Microsoft Build Tools"
LangString RastrWinSetupX64 ${LANG_ENGLISH} "RastrWin x64"
LangString RastrWinSetupX64 ${LANG_RUSSIAN} "RastrWin x64"
LangString RastrWinSetupX86 ${LANG_ENGLISH} "RastrWin x86"
LangString RastrWinSetupX86 ${LANG_RUSSIAN} "RastrWin x86"
LangString MSBuildNotInstalled ${LANG_ENGLISH} "Not installed"
LangString MSBuildNotInstalled ${LANG_RUSSIAN} "Не установлен"
LangString RastrWinNotInstalled ${LANG_ENGLISH} "Not installed"
LangString RastrWinNotInstalled ${LANG_RUSSIAN} "Не установлен"
LangString RastrWinRegistryFailed ${LANG_ENGLISH} "No registry record found"
LangString RastrWinRegistryFailed ${LANG_RUSSIAN} "Не записи в реестре"
LangString RastrWinAstraNotFound ${LANG_ENGLISH} "astra.dll not found"
LangString RastrWinAstraNotFound ${LANG_RUSSIAN} "Не найдена astra.dll"
LangString ShowReleaseNotes ${LANG_ENGLISH} "Show release notes ?"
LangString ShowReleaseNotes ${LANG_RUSSIAN} "Показать сведения об изменениях ?"
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
LangString ComponentsX64 ${LANG_ENGLISH} "x64 components"
LangString ComponentsX64 ${LANG_RUSSIAN} "Компоненты x64"
LangString ComponentsX86 ${LANG_ENGLISH} "x86 components"
LangString ComponentsX86 ${LANG_RUSSIAN} "Компоненты x86"
LangString MSBuildInstallationInstructions ${LANG_ENGLISH} "This program requires Microsoft Visual Studio 2022 or Microsoft Build Tools to be installed with C++ workload"
LangString MSBuildInstallationInstructions ${LANG_RUSSIAN} "Для работы данного ПО необходима установка Microsoft Visual Studio 2022 или Microsoft Build Tools с рабочей нагрузкой C++"
LangString Installing ${LANG_ENGLISH} "Installing"
LangString Installing ${LANG_RUSSIAN} "Инсталляция"
LangString UnInstalling ${LANG_ENGLISH} "Uninstalling"
LangString UnInstalling ${LANG_RUSSIAN} "Деинсталляция"

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" ${ProductName}
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© Eugene Mashalov"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${ProductName} installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" ${Version}
VIAddVersionKey /LANG=${LANG_RUSSIAN} "ProductName" ${ProductName}
VIAddVersionKey /LANG=${LANG_RUSSIAN} "LegalCopyright" "© Евгений Машалов"
VIAddVersionKey /LANG=${LANG_RUSSIAN} "FileDescription" "Программа установки ${ProductName}"
VIAddVersionKey /LANG=${LANG_RUSSIAN} "FileVersion" ${Version}
VIProductVersion ${Version}
