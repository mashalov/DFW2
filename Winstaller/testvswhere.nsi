# define the name of the installer
Outfile "testvswhere.exe"

!include Library.nsh
!include nsDialogs.nsh
!include LogicLib.nsh
Unicode True

!define Trim "!insertmacro Trim"
!define VersionCheckNew "!insertmacro VersionCheckV5"
 
!macro Trim ResultVar String
  Push "${String}"
  Call Trim
  Pop "${ResultVar}"
!macroend

!macro VersionCheckV5 Ver1 Ver2 OutVar
 Push "${Ver1}"
 Push "${Ver2}"
  Call VersionCheckV5
 Pop "${OutVar}"
!macroend


 
# define the directory to install to, the desktop in this case as specified  
# by the predefined $DESKTOP variable
InstallDir $DESKTOP
 
# default section
Section
 
# define the output path for this file
SetOutPath $INSTDIR
 
# define what to install and place it in the output path
File testvswhere.nsi
	nsExec::ExecToStack /OEM '"$PROGRAMFILES32\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe"'
	Pop $R0 # return value/error/timeout
	Pop $R1 # printed text, up to ${NSIS_MAX_STRLEN}
	DetailPrint "MSBuild Exitcode: $R0"
	DetailPrint "MSBuild Result: $R1"
	${Trim} $R0 $R1
	DetailPrint "MSBuild Result trimmed: $R0"
	nsExec::ExecToStack /OEM '"$PROGRAMFILES32\Microsoft Visual Studio\Installer\vswhere.exe" -property catalog_productDisplayVersion'
	Pop $R0 # return value/error/timeout
	Pop $R1 # printed text, up to ${NSIS_MAX_STRLEN}
	DetailPrint "vswhere Exitcode: $R0"
	DetailPrint "vswhere Result: $R1"
	${Trim} $R2 $R1
	DetailPrint "vswhere Result trimmed: $R2"
	${VersionCheckNew} "17.6.5" $R2 $R0
	DetailPrint "Version Check 17.6.5: $R0"
	${VersionCheckNew} "17.0.0" $R2 $R0
	DetailPrint "Version Check 17.0.0: $R0"
	${VersionCheckNew} "17.7.0" $R2 $R0
	DetailPrint "Version Check 17.7.0: $R0"
SectionEnd

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
