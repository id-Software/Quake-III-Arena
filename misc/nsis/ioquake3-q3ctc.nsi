; sample NSIS description file for a ioquake3 mod installer

!define NAME "Catch the Chicken"
!define FSNAME "ioquake3-q3ctc"
!define VERSION "1.4"
!define RELEASE "1"
!define MODDIR "q3ctc"
!define PUBLISHER "The ioquake3 Team"
!define URL "http://ioquake3.org/"
; uncomment if the mod works without baseq3
;!define STANDALONE

!define MUI_ICON "../quake3.ico"

!macro FILES
  File ctc0.pk3
  File ctc1.pk3
  File description.txt
  File gfx/chicken.jpg
  File gfx/egggun.jpg
  File gfx/q3ctc.gif
  File q3ctc.config
  File q3ctcteam.config
  File readme.html
  File readmeFirst.txt
!macroend

; no need to edit below in theory

SetCompressor lzma

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

!include "MUI2.nsh"

; The name of the installer
Name "${NAME}-${VERSION} for ioquake3"

; The file to write
OutFile "${FSNAME}-${VERSION}-${RELEASE}.x86.exe"

;Interface Settings

!define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MULTIUSER_PAGE_INSTALLMODE
;!insertmacro MUI_PAGE_LICENSE "id_patch_pk3s_Q3A_EULA.txt"
!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

;--------------------------------
;Multiuser stuff
Function .onInit
  !insertmacro MULTIUSER_INIT
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
Section "${NAME}" sec_base

  SectionIn RO

  SetOutPath $INSTDIR

  SetOutPath "$INSTDIR\${MODDIR}"

  !insertmacro FILES

  ; Write the uninstall keys for Windows
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "DisplayName" "${NAME}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "DisplayVersion" "${VERSION}-${RELEASE}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "Publisher" "${PUBLISHER}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "URLInfoAbout" "${URL}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "UninstallString" '"$INSTDIR\uninstall-${FSNAME}.exe"'
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "NoModify" 1
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}" "NoRepair" 1

  WriteUninstaller "uninstall-${FSNAME}.exe"

!ifdef STANDALONE
!define ARGS "+set com_standalone 1 "
!else
!define ARGS ""
!endif
  CreateShortCut "$SMPROGRAMS\ioquake3\${NAME}.lnk" "$INSTDIR\ioquake3.x86.exe" "${ARGS}+set fs_game ${MODDIR}" "$INSTDIR\ioquake3.x86.exe" 0 "" "" "${NAME}"

SectionEnd

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FSNAME}"

  ; Remove files and uninstaller
  Delete $INSTDIR\${MODDIR}\*
  
  Delete $INSTDIR\uninstall-${FSNAME}.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\ioquake3\${NAME}.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\ioquake3"
  RMDir "$INSTDIR\${MODDIR}"
  RMDir "$INSTDIR"

SectionEnd
