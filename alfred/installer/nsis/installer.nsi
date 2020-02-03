; name of the file that will be generated.
OutFile "xxxxx_drp_installer.exe"

;--------------------------------
!include "MUI2.nsh"
!include "FileFunc.nsh"		; Functions like GetTime
!include 'LogicLib.nsh'
!include "x64.nsh"		; Macros for x64 machines
!include 'WinVer.nsh'
!include "nsProcess.nsh"
!insertmacro "GetParameters"
!insertmacro "GetOptions"

!define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MUI_PAGE_DIRECTORY

!define service_id "XxxxxDRP"
!define service_human_name "Xxxxx DRP agent"
!define service_description "Provides automatic failover on a network drive."
!define exec_name "xxxxx_drp.exe"

Var /GLOBAL UNC_PATH_MAIN_SHARE
Var /GLOBAL HANDLE_UNC_PATH_MAIN_SHARE
Var /GLOBAL SHARE_LOGIN
Var /GLOBAL HANDLE_SHARE_LOGIN
Var /GLOBAL SHARE_PASSWORD
Var /GLOBAL HANDLE_SHARE_PASSWORD
Var /GLOBAL UNC_PATH_FAILOVER_SHARE
Var /GLOBAL HANDLE_UNC_PATH_FAILOVER_SHARE
Var /GLOBAL POLL_PERIOD_IN_SECOND
Var /GLOBAL HANDLE_POLL_PERIOD_IN_SECOND
Var /GLOBAL DRIVE_LETTER
Var /GLOBAL HANDLE_DRIVE_LETTER
Var /GLOBAL USER_LOGIN
Var /GLOBAL HANDLE_USER_LOGIN
Page custom nsDialogsPage nsDialogsPageLeave

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
!insertmacro MUI_LANGUAGE "English"

; The name of the installer
Name "${service_human_name}"
Caption "${service_human_name}"
Icon "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"

;/LANG=${LANG_ENGLISH}
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${service_human_name}"
VIAddVersionKey "Comments" "www.xxxxxtec.com"
VIAddVersionKey "CompanyName" "Xxxxx Technologies"
VIAddVersionKey "LegalCopyright" "©Xxxxx Technologies"
VIAddVersionKey "FileDescription" "${service_human_name} installer"
VIAddVersionKey "ProductVersion" "${VERSION}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
InstallColors FF8080 000030
XPStyle on

; Macro pour écrire des logs dans un fichier et/ou en visuel aux utilisateurs
!macro DetailPrintAndLog msg type
	${If} "${type}" != "DetailPrintOnly"
		${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
		nsislog::log "$INSTDIR\log_nsis.log" "LocalTime YYYY/MM/DD hh:mm:ss : $2/$1/$0 $4:$5:$6 :    ${msg}"
	${EndIf}

	${If} "${type}" != "LogOnly"
		DetailPrint "${msg}"
	${Endif}
!macroend

; Define for insertmacro
!define DetailPrintAndLog '!insertmacro DetailPrintAndLog'

; Macro qui permet de retourner le dossier parent d'un fichier / dossier.
!macro SHAREGETPARENT un
Function ${un}GetParent
	Exch $R0
	Push $R1
	Push $R2
	Push $R3
	StrCpy $R1 0
	StrLen $R2 $R0
	loop:
		IntOp $R1 $R1 + 1
		IntCmp $R1 $R2 get 0 get
		StrCpy $R3 $R0 1 -$R1
		StrCmp $R3 "\" get
		Goto loop
	get:
		StrCpy $R0 $R0 -$R1
		Pop $R3
		Pop $R2
		Pop $R1
		Exch $R0
FunctionEnd
!macroend

!insertmacro SHAREGETPARENT ""
!insertmacro SHAREGETPARENT "un."

; Macro pour remplacer un caractère/une chaine de caractère dans une chaine de caractère par un(e) autre
; Get from : nsis.sourceforge.net/StrRep
!define StrRep "!insertmacro StrRep"
!macro StrRep output string old new
    Push `${string}`
    Push `${old}`
    Push `${new}`
    !ifdef __UNINSTALL__
        Call un.StrRep
    !else
        Call StrRep
    !endif
    Pop ${output}
!macroend

!macro Func_StrRep un
    Function ${un}StrRep
        Exch $R2 ;new
        Exch 1
        Exch $R1 ;old
        Exch 2
        Exch $R0 ;string
        Push $R3
        Push $R4
        Push $R5
        Push $R6
        Push $R7
        Push $R8

        Push $R9

        StrCpy $R3 0
        StrLen $R4 $R1
        StrLen $R6 $R0
        StrLen $R9 $R2
        loop:
            StrCpy $R5 $R0 $R4 $R3
            StrCmp $R5 $R1 found
            StrCmp $R3 $R6 done
            IntOp $R3 $R3 + 1 ;move offset by 1 to check the next character
            Goto loop
        found:
            StrCpy $R5 $R0 $R3
            IntOp $R8 $R3 + $R4
            StrCpy $R7 $R0 "" $R8
            StrCpy $R0 $R5$R2$R7
            StrLen $R6 $R0
            IntOp $R3 $R3 + $R9 ;move offset by length of the replacement string
            Goto loop
        done:

        Pop $R9
        Pop $R8
        Pop $R7
        Pop $R6
        Pop $R5
        Pop $R4
        Pop $R3
        Push $R0
        Push $R1
        Pop $R0
        Pop $R1
        Pop $R0
        Pop $R2
        Exch $R1
    FunctionEnd
!macroend
!insertmacro Func_StrRep ""
!insertmacro Func_StrRep "un."

CheckBitmap "${NSISDIR}\Contrib\Graphics\Checks\classic-cross.bmp"
; Request application privileges
RequestExecutionLevel admin

;--------------------------------


; Pages
;Page directory
;Page instfiles

AutoCloseWindow false
ShowInstDetails show

;--------------------------------

Function .onInit
	InitPluginsDir
	IfSilent +3
	File /oname=$PLUGINSDIR\splash.bmp "logo_xxxxx_210.bmp"
	advsplash::show 800 800 400 -1 $PLUGINSDIR\splash

	${If} ${RunningX64}
		StrCmp ${ARCH} "x64" goodarch
	${Else}
		StrCmp ${ARCH} "x86" goodarch
	${EndIf}

	MessageBox MB_ICONINFORMATION|MB_OK "The architecture of the ${service_human_name} service (${ARCH} bits) is not the same as this operating system"
	Quit

goodarch:
	${IfNot} ${AtLeastWin2008}
		MessageBox MB_ICONINFORMATION|MB_OK "${service_human_name} supports only Windows 2008 or newer."
		Quit
	${EndIf}

	UserInfo::getAccountType
	Pop $0
	# compare the result with the string "Admin" to see if the user is admin.
    # If match, jump 3 lines down.
    StrCmp $0 "Admin" +3

    # if there is not a match, print message and return
    MessageBox MB_ICONINFORMATION|MB_OK "This program requires administrator privileges (you are: $0)"
    Quit

	; Remove before install if already installed
	ReadRegStr $R0 HKLM \
		"Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}" \
		"UninstallString"
	StrCmp $R0 "" done

	MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"${service_human_name} is already installed. $\n$\nClick `OK` to remove the \
		previous version or `Cancel` to cancel this upgrade." \
		IDOK uninst
	Abort

	;Run the uninstaller
	uninst:
	ClearErrors
	Exec "$R0"

	done:

	var /GLOBAL cmdLineParams
	${GetParameters} $cmdLineParams
	;
	; Get parameter INSTALL_DIR if exists or set to default value
	${GetOptions} $cmdLineParams "/INSTALL_DIR=" $INSTDIR
	${If} $INSTDIR == ""
		${If} ${RunningX64}
			StrCpy $INSTDIR "$programfiles64\Xxxxx-Technologies\${service_id}\"
		${Else}
			StrCpy $INSTDIR "$programfiles\Xxxxx-Technologies\${service_id}\"
		${EndIf}
	${EndIf}

	SetOutPath $PLUGINSDIR
	File conf.cfg
	nsJSON::Set /tree initconf /file "$PLUGINSDIR\conf.cfg"
	; Get parameter UNC_PATH_MAIN_SHARE if exists or set to default value
	${GetOptions} $cmdLineParams "/UNC_PATH_MAIN_SHARE=" $UNC_PATH_MAIN_SHARE
	${If} $UNC_PATH_MAIN_SHARE == ""
		ClearErrors
		nsJSON::Get /tree initconf `unc_path_main_share` /end 
		${IfNot} ${Errors}
			pop $UNC_PATH_MAIN_SHARE
		${Else}
			StrCpy $UNC_PATH_MAIN_SHARE "\\main.domain.com\share_name"
		${EndIf}
	${EndIf}

	; Get parameter UNC_PATH_FAILOVER_SHARE if exists or set to default value
	${GetOptions} $cmdLineParams "/UNC_PATH_FAILOVER_SHARE=" $UNC_PATH_FAILOVER_SHARE
	${If} $UNC_PATH_FAILOVER_SHARE == ""
		ClearErrors
		nsJSON::Get /tree initconf `unc_path_failover_share` /end 
		${IfNot} ${Errors}
			pop $UNC_PATH_FAILOVER_SHARE
		${Else}
			StrCpy $UNC_PATH_FAILOVER_SHARE "\\failover.domain.com\share_name"
		${EndIf}
	${EndIf}

	; Get parameter SHARE_LOGIN if exists or set to default value
	${GetOptions} $cmdLineParams "/SHARE_LOGIN=" $SHARE_LOGIN

	; Get parameter SHARE_PASSWORD if exists or set to default value
	${GetOptions} $cmdLineParams "/SHARE_PASSWORD=" $SHARE_PASSWORD

	; Get parameter POLL_PERIOD_IN_SECOND if exists or set to default value
	${GetOptions} $cmdLineParams "/POLL_PERIOD_IN_SECOND=" $POLL_PERIOD_IN_SECOND
	${If} $POLL_PERIOD_IN_SECOND == ""
		ClearErrors
		nsJSON::Get /tree initconf `poll_period_in_second` /end 
		${IfNot} ${Errors}
			pop $POLL_PERIOD_IN_SECOND
		${Else}
			StrCpy $POLL_PERIOD_IN_SECOND "10" ;
		${EndIf}
	${EndIf}

	; Get parameter DRIVE_LETTER if exists or set to default value
	${GetOptions} $cmdLineParams "/DRIVE_LETTER=" $DRIVE_LETTER
	${If} $DRIVE_LETTER == ""
		ClearErrors
		nsJSON::Get /tree initconf `mount_point` /end 
		${IfNot} ${Errors}
			pop $DRIVE_LETTER
		${Else}
			StrCpy $DRIVE_LETTER "Z"
		${EndIf}
	${EndIf}

	ClearErrors
	${GetOptions} $cmdLineParams "/SHARE_USER_LOGIN" $R0
	${IfNot} ${Errors}
		${NSD_Check} $USER_LOGIN
	${EndIf}
	ClearErrors

	; Language selection
	!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd


Function nsDialogsPage
	!insertmacro MUI_HEADER_TEXT "Configuration :" "Parameters for the ${service_human_name} service"
	nsDialogs::Create 1018

	${NSD_CreateLabel} 0 0 100% 24u "Please specify the following parameters. You will be able to update them later by editing conf.cfg in the installation directory."

	${NSD_CreateLabel}  0  25u 120u 12u "UNC path of the main share: "
	${NSD_CreateText} 121u 25u 150u 12u $UNC_PATH_MAIN_SHARE
	Pop $HANDLE_UNC_PATH_MAIN_SHARE

	${NSD_CreateLabel}  0  38u 120u 12u "UNC path of the failover share: "
	${NSD_CreateText} 121u 38u 150u 12u $UNC_PATH_FAILOVER_SHARE
	Pop $HANDLE_UNC_PATH_FAILOVER_SHARE

	${NSD_CreateCheckbox} 0 51u 100% 12u "&Mount with user login"
	Pop $HANDLE_USER_LOGIN

	${NSD_CreateLabel}  0  64u 120u 12u "Share login: "
	${NSD_CreateText} 121u 64u 150u 12u $SHARE_LOGIN
	Pop $HANDLE_SHARE_LOGIN

	${NSD_CreateLabel}  0  77u 120u 12u "Share password: "
	${NSD_CreateText} 121u 77u 150u 12u $SHARE_PASSWORD
	Pop $HANDLE_SHARE_PASSWORD

	${NSD_CreateLabel}  0  90u 120u 12u "Polling period in second : "
	${NSD_CreateNumber} 121u 90u 150u 12u $POLL_PERIOD_IN_SECOND
	Pop $HANDLE_POLL_PERIOD_IN_SECOND

	${NSD_CreateLabel}  0  103u 120u 12u 'Drive letter (eg "Z"): '
	${NSD_CreateText} 121u 103u 150u 12u $DRIVE_LETTER
	Pop $HANDLE_DRIVE_LETTER

	nsDialogs::Show
FunctionEnd

Function nsDialogsPageLeave
	; ${NSD_GetText} control_HWND output_variable
	${NSD_GetText} $HANDLE_UNC_PATH_MAIN_SHARE $UNC_PATH_MAIN_SHARE
	${NSD_GetText} $HANDLE_UNC_PATH_FAILOVER_SHARE $UNC_PATH_FAILOVER_SHARE
	${NSD_GetText} $HANDLE_SHARE_LOGIN $SHARE_LOGIN
	${NSD_GetText} $HANDLE_SHARE_PASSWORD $SHARE_PASSWORD
	${NSD_GetText} $HANDLE_POLL_PERIOD_IN_SECOND $POLL_PERIOD_IN_SECOND
	${NSD_GetText} $HANDLE_DRIVE_LETTER $DRIVE_LETTER
	${NSD_GetState} $HANDLE_USER_LOGIN $USER_LOGIN
FunctionEnd

; Entry point of installer.
Section "" ;No components page, name is not important
	;Creating install directory, must be created before any log
	CreateDirectory $INSTDIR

	${DetailPrintAndLog} "Installing ${service_human_name} :" ""

	${DetailPrintAndLog} "  - Checking if ${service_human_name} is already installed..." ""
	SimpleSC::GetServiceStatus "${service_id}"
	Pop $0
	Pop $1
	${If} $0 == 0
		IfSilent +2
		MessageBox MB_ICONINFORMATION|MB_OK "There is already a service with the name '${service_id}' installed."
      		Return
	${Endif}


	${DetailPrintAndLog} "  - Copying files in destination folder..." ""

	; Set output path to the installation directory.
	SetOutPath $INSTDIR
	; Put file there
	File /r "dll\"
	File ${exec_name}
	File conf.cfg

	nsJSON::Set /tree conf /file $INSTDIR\conf.cfg
	${StrRep} $UNC_PATH_MAIN_SHARE '$UNC_PATH_MAIN_SHARE' '\' '\\'
	nsJSON::Set /tree conf `unc_path_main_share` /value `"$UNC_PATH_MAIN_SHARE"`
	${StrRep} $UNC_PATH_FAILOVER_SHARE '$UNC_PATH_FAILOVER_SHARE' '\' '\\'
	nsJSON::Set /tree conf `unc_path_failover_share` /value `"$UNC_PATH_FAILOVER_SHARE"`
	${StrRep} $SHARE_LOGIN '$SHARE_LOGIN' '\' '\\'
	nsJSON::Set /tree conf `share_login` /value `"$SHARE_LOGIN"`
	nsJSON::Set /tree conf `share_password` /value `"$SHARE_PASSWORD"`
	nsJSON::Set /tree conf `poll_period_in_second` /value $POLL_PERIOD_IN_SECOND
	nsJSON::Set /tree conf `mount_point` /value `"$DRIVE_LETTER"`
	${If} $USER_LOGIN == ${BST_CHECKED}
		nsJSON::Set /tree conf `share_user_login` /value `"true`
	${Else}
		nsJSON::Set /tree conf `share_user_login` /value `false`
	${EndIf}
	nsJSON::Serialize /tree conf /format /file $INSTDIR\conf.cfg

	VAR /GLOBAL CONF_PATH
	${StrRep} $CONF_PATH '$INSTDIR\' '\' '\\'

	${DetailPrintAndLog} "  - Adding registry keys..." ""
	WriteRegStr HKLM SOFTWARE\Xxxxx-Technologies\${service_id} "Install_Dir" "$INSTDIR"

	; write uninstall strings
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}" "DisplayName" "${service_human_name}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}" "Publisher" "Xxxxx Technologies"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}" "UninstallString" '"$INSTDIR\uninstaller.exe"'

	${DetailPrintAndLog} "  - Writing uninstaller..." ""
	WriteUninstaller "uninstaller.exe"

	${DetailPrintAndLog} "  - Installing new service..." ""
	; Service installation and start
	; SimpleSC::InstallService [name_of_service] [display_name] [service_type] [start_type] [binary_path] [dependencies] [account] [password]
	; SimpleSC::SetServiceDescription [name_of_service] [service_description]
	; SimpleSC::SetServiceFailure [name_of_service] [reset_period] [reboot_message] [command] [action_type_1] [action_delay_1] [action_type_2] [action_delay_2] [action_type_3] [action_delay_3]
	SimpleSC::InstallService "${service_id}" "${service_human_name}" "16" "2" "$INSTDIR\${exec_name}" "" "" ""
	SimpleSC::SetServiceDescription "${service_id}" "${service_description}"
	SimpleSC::SetServiceFailure "${service_id}" "0" "" "" "1" "60000" "0" "0" "0" "0"

	${DetailPrintAndLog} "Parameters : $cmdLineParams" ""
	ClearErrors

	${GetOptions} $cmdLineParams "/NOSTART" $R0
	${If} ${Errors}
		${DetailPrintAndLog} "  - Starting new service..." ""
		; SimpleSC::StartService [name_of_service] [arguments] [timeout]
		SimpleSC::StartService ${service_id} "" "60"
	${EndIf}
	ClearErrors

	${DetailPrintAndLog} "Installation finished." ""
SectionEnd ; end the section

; Uninstaller

Function un.onInit
	VAR /GLOBAL DEFAULT_INSTDIR
	; Création du répertoire d'installation
	StrCpy $DEFAULT_INSTDIR "$programfiles\Xxxxx-Technologies\${service_id}"
	${If} ${RunningX64}
		StrCpy $DEFAULT_INSTDIR "$programfiles64\Xxxxx-Technologies\${service_id}"
	${EndIf}
FunctionEnd

UninstallText "This will uninstall ${service_human_name} service by Xxxxx Technologies. Hit next to continue."
UninstallIcon "${NSISDIR}\Contrib\Graphics\Icons\nsis1-uninstall.ico"

Section "Uninstall"
	${DetailPrintAndLog} "  - Stopping and removing service..." ""
	; Service stop and uninstall
	; SimpleSC::StopService [name_of_service] [wait_for_file_release] [timeout]
	; SimpleSC::RemoveService [name_of_service]
	SimpleSC::StopService "${service_id}" "1" "60"
	SimpleSC::RemoveService "${service_id}"

	${DetailPrintAndLog} "  - Deleting registry keys..." ""
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${service_id}"
	DeleteRegKey HKLM "SOFTWARE\Xxxxx-Technologies"

	${DetailPrintAndLog} "  - Deleting files installed in $INSTDIR..." ""
	RMDir /r /REBOOTOK "$INSTDIR\dll"
	Delete "$INSTDIR\uninstaller.exe"
	Delete "$INSTDIR\${exec_name}"
	Delete "$INSTDIR\conf.cfg"
	Delete "$INSTDIR\log_nsis.log"
	Delete "$INSTDIR\xxxxx_drp__log"
	Push "$INSTDIR"
	Call un.GetParent
	Pop $R0
	${If} $INSTDIR == $DEFAULT_INSTDIR
		RMDir /r /REBOOTOK "$INSTDIR"
		RMDir "$R0"
	${EndIf}
SectionEnd

