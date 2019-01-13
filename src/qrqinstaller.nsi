!define src "."
!define startmenu "$SMPROGRAMS\qrq"

Name "qrq"
Caption "qrq _VERSION_ - High Speed CW Trainer"
OutFile "qrq-_VERSION_.exe"
Icon "qrq.ico"
 
InstallDir "$PROGRAMFILES\qrq"
DirText "Setup will install qrq _VERSION_ in the following folder. To install in a different folder, click Browse and select another folder. Click Install to start the installation. Note that if qrq is already installed the old installation is overwritten, except for the files 'qrqrc' and 'toplist'."
 
LicenseText "License of High Speed CW trainer 'qrq' (https://fkurz.net/ham/qrq.html) by Fabian Kurz, DJ1YFK"
LicenseData "COPYING"
 
Page license
Page directory
Page instfiles
 
UninstPage uninstConfirm
UninstPage instfiles
 
AutoCloseWindow false
ShowInstDetails show

Section
	SetOutPath $INSTDIR
	File "qrq.exe"
	File /oname=COPYING.txt "COPYING"
	File /oname=AUTHORS.txt "AUTHORS"
	File "callbase.qcb"
	File /oname=ChangeLog.txt "ChangeLog"
	File "english.qcb"
	File /oname=README.txt "README"
  	SetOverwrite off
	File "qrqrc"
	File "toplist"
  	SetOverwrite on
	WriteUninstaller "qrquninstall.exe"
SectionEnd
 
Section
  CreateDirectory "${startmenu}"
  SetOutPath $INSTDIR
  CreateShortCut "${startmenu}\qrq.lnk" "$INSTDIR\qrq.exe"
  CreateShortCut "$QUICKLAUNCH\qrq.lnk" "$INSTDIR\qrq.exe"
  CreateShortCut "$DESKTOP\qrq.lnk" "$INSTDIR\qrq.exe"
  CreateShortCut "${startmenu}\uninstall_qrq.lnk" "$INSTDIR\qrquninstall.exe"
  WriteINIStr "${startmenu}\qrq.url" "InternetShortcut" "URL" "https://fkurz.net/ham/qrq.html"
SectionEnd
 
; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller
 
UninstallText "This will completely uninstall qrq."
 
Section "Uninstall"
  Delete "${startmenu}\*.*"
  RMDir "${startmenu}"
  Delete "$QUICKLAUNCH\qrq.lnk"
  Delete "$DESKTOP\qrq.lnk"
  Delete "$INSTDIR\*.*"
  RMDir $INSTDIR
SectionEnd

