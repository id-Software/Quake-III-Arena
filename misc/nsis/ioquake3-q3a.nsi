; NSIS description file for quake3 data file installer

SetCompressor lzma

!define NAME "Quake III Arena"
!define FSNAME "ioquake3-q3a"
!define VERSION "1.32"
!define RELEASE "1"

!define MULTIUSER_MUI
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "Software\ioquake3"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "Install_Mode"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "Software\ioquake3"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "Install_Dir"
!define MULTIUSER_INSTALLMODE_INSTDIR "ioquake3"
!include MultiUser.nsh

!include "FileFunc.nsh"
Var q3a_pak0
Var q3ta_pak0

!include "MUI2.nsh"
!define MUI_ICON "../quake3.ico"

; The name of the installer
Name "${NAME}-${VERSION} for ioquake3"

; The file to write
OutFile "${FSNAME}-${VERSION}-${RELEASE}.x86.exe"

;Interface Settings

!define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_LICENSE "id_patch_pk3s_Q3A_EULA.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

;--------------------------------
;Multiuser stuff
Function .onInit
  !insertmacro MULTIUSER_INIT
  StrCpy $q3a_pak0 "notfound"
  ReadRegStr $0 SHCTX "Software\ioquake3" ${MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME}
  IfErrors 0 oninitdone
    MessageBox MB_OK "You need to install the ioquake3 engine first"
    Abort
  oninitdone:
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

; The stuff to install
Section "${NAME}" sec_q3a

  SectionIn RO

  SetOutPath $INSTDIR
  File "id_patch_pk3s_Q3A_EULA.txt"

  SetOutPath "$INSTDIR\baseq3"
  File "baseq3/pak1.pk3"
  File "baseq3/pak2.pk3"
  File "baseq3/pak3.pk3"
  File "baseq3/pak4.pk3"
  File "baseq3/pak5.pk3"
  File "baseq3/pak6.pk3"
  File "baseq3/pak7.pk3"
  File "baseq3/pak8.pk3"

  ; Write the uninstall keys for Windows
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "DisplayName" "${NAME}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "UninstallString" '"$INSTDIR\uninstall-${FSNAME}.exe"'
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "NoModify" 1
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "NoRepair" 1
  WriteUninstaller "uninstall-${FSNAME}.exe"

SectionEnd

Section /o "${NAME} CDROM" sec_q3acd
	AddSize 468992
	q3apak0retry:
	ClearErrors
	StrCmp $q3a_pak0 "notfound" 0 q3apak0copy
		call findq3acd
	q3apak0copy:
	CopyFiles $q3a_pak0 "$INSTDIR\baseq3"
	IfErrors 0 q3apak0done
		MessageBox MB_RETRYCANCEL "Copying the Quake III Arena pak0.pk3 file failed. Make sure the correct CD is in the drive" IDRETRY q3apak0retry IDCANCEL q3apak0cancel
	goto q3apak0done
	q3apak0cancel:
		Abort
	q3apak0done:
SectionEnd

Function findq3acd
	StrCpy $q3a_pak0 "notfound"
	${GetDrives} "CDROM" "findq3acd_cb"
FunctionEnd

Function findq3acd_cb
	StrCpy $R1 "$9baseq3\pak0.pk3"
	IfFileExists $R1 q3acd_cb_found 0
	StrCpy $R1 "$9quake3\baseq3\pak0.pk3"
	IfFileExists $R1 0 q3acd_cb_done
q3acd_cb_found:
	StrCpy $q3a_pak0 $R1
	StrCpy $0 StopGetDrives

q3acd_cb_done:
	Push $0
FunctionEnd

Section "Quake III Team Arena" sec_q3ta

  SetOutPath "$INSTDIR\missionpack"

  File "missionpack/pak1.pk3"
  File "missionpack/pak2.pk3"
  File "missionpack/pak3.pk3"

  CreateShortCut "$SMPROGRAMS\ioquake3\Team Arena.lnk" "$INSTDIR\ioquake3.x86.exe" "+set fs_game missionpack" "$INSTDIR\ioquake3.x86.exe" 0 "" "" "Team Arena"

SectionEnd

Section /o "Quake III Team Arena CDROM" sec_q3tacd
	AddSize 344064
	q3tapak0retry:
	ClearErrors
	StrCmp $q3ta_pak0 "notfound" 0 q3tapak0copy
		call findq3tacd
	q3tapak0copy:
	CopyFiles $q3ta_pak0 "$INSTDIR\missionpack"
	IfErrors 0 q3tapak0done
		MessageBox MB_RETRYCANCEL "Copying the Quake III TeamArena pak0.pk3 file failed. Make sure the correct CD is in the drive" IDRETRY q3tapak0retry IDCANCEL q3tapak0cancel
	goto q3tapak0done
	q3tapak0cancel:
		Abort
	q3tapak0done:
SectionEnd

Function findq3tacd
	StrCpy $q3ta_pak0 "notfound"
	${GetDrives} "CDROM" "findq3tacd_cb"
FunctionEnd

Function findq3tacd_cb
	StrCpy $R1 "$9Setup\missionpack\pak0.pk3"
	IfFileExists $R1 0 q3tacd_cb_done
	StrCpy $q3ta_pak0 $R1
	StrCpy $0 StopGetDrives
q3tacd_cb_done:
	Push $0
FunctionEnd

;--------------------------------

; Uninstaller

Section "un.Quake III Arena and Team Arena" sec_un_q3a

  SectionIn RO

  ; Remove registry keys
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}"

  ; Remove files and uninstaller
  Delete $INSTDIR\baseq3\pak1.pk3
  Delete $INSTDIR\baseq3\pak2.pk3
  Delete $INSTDIR\baseq3\pak3.pk3
  Delete $INSTDIR\baseq3\pak4.pk3
  Delete $INSTDIR\baseq3\pak5.pk3
  Delete $INSTDIR\baseq3\pak6.pk3
  Delete $INSTDIR\baseq3\pak7.pk3
  Delete $INSTDIR\baseq3\pak8.pk3
  
  Delete $INSTDIR\missionpack\pak1.pk3
  Delete $INSTDIR\missionpack\pak2.pk3
  Delete $INSTDIR\missionpack\pak3.pk3

  Delete $INSTDIR\uninstall-${FSNAME}.exe

  Delete "$INSTDIR\id_patch_pk3s_Q3A_EULA.txt"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\ioquake3\Team Arena.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\ioquake3"
  RMDir "$INSTDIR\baseq3"
  RMDir "$INSTDIR\missionpack"
  RMDir "$INSTDIR"

SectionEnd

Section "un.CDROM Data" sec_un_q3a_cd
  Delete $INSTDIR\baseq3\pak0.pk3
  Delete $INSTDIR\missionpack\pak0.pk3
  RMDir "$INSTDIR\baseq3"
  RMDir "$INSTDIR\missionpack"
  RMDir "$INSTDIR"
SectionEnd

Function .onSelChange
	${If} ${SectionIsSelected} ${sec_q3acd}
		Call findq3acd
		StrCmp $q3a_pak0 "notfound" 0 +2
			MessageBox MB_OK "Quake III Arena CD not found. Make sure it is in the drive otherwise installation will fail"
	${EndIf}
	${If} ${SectionIsSelected} ${sec_q3tacd}
		Call findq3tacd
		StrCmp $q3ta_pak0 "notfound" 0 +2
			MessageBox MB_OK "Quake III TeamArena CD not found. Make sure it is in the drive otherwise installation will fail"
	${EndIf}
FunctionEnd


LangString DESC_q3a ${LANG_ENGLISH} "Install official Quake III Arena Point Release 1.32 data files. Note that the data files alone are useless. You need to also install the Quake III Arena base assets (pak0.pk3) from the game's CD-ROM."
LangString DESC_q3acd ${LANG_ENGLISH} "Install the Quake III Arena base assets (pak0.pk3) from the game's CD-ROM."
LangString DESC_q3ta ${LANG_ENGLISH} "Install official Quake III Team Arena Point Release 1.32 data files. Note that the data files alone are useless. You need to also install the Quake III Team Arena base assets (pak0.pk3) from the game's CD-ROM."
LangString DESC_q3tacd ${LANG_ENGLISH} "Install the Quake III Team Arena base assets (pak0.pk3) from the game's CD-ROM."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_q3a} $(DESC_q3a)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_q3acd} $(DESC_q3acd)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_q3ta} $(DESC_q3ta)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_q3tacd} $(DESC_q3tacd)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
