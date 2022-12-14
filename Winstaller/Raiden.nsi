!define ProductName "RaidenEMS"
!define InputFolder "..\x64\release\"

!include MUI2.nsh
!include Library.nsh
!define LIBRARY_X64
Unicode True

!define UninstallerName $(^Name)Uninstall.exe
SetCompressor /SOLID /FINAL lzma
Name ${ProductName}

BrandingText $(CopyrightInfo)

function .onInit
StrCpy $INSTDIR "c:\program files\$(^Name)"
functionend

RequestExecutionLevel admin
!insertmacro MUI_PAGE_LICENSE "licenseru.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
Section "Программные модули"
	SetOutPath "$INSTDIR"
	File "${InputFolder}dfw2.dll"
	File "${InputFolder}umc.dll"
	!insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_PROTECTED "${InputFolder}ResultFile.dll" $OUTDIR\ResultFile.dll $TEMP
	WriteUninstaller "${UninstallerName}"
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

!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

LangString CopyrightInfo ${LANG_ENGLISH} "${ProductName} © Eugene Mashalov 2017-2022"
LangString CopyrightInfo ${LANG_RUSSIAN} "${ProductName} © Евгений Машалов 2017-2022"



