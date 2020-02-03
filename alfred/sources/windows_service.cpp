//windows
#include <windows.h>
#include <Winsvc.h>
#include "main_loop.h"
#include "windows_service.h"
#include "log.h"
#include "mountpoint.h" // For firstMount()
#include "configuration.h"

HANDLE stop_service_event;

SERVICE_TABLE_ENTRY service_table[2];

SERVICE_STATUS service_status;
SERVICE_STATUS_HANDLE h_status;

HANDLE impersonatedToken;

void start_win_service(void)
{
	service_table[0].lpServiceName = (TCHAR*) calloc(strlen(APP_NAME) + 1, sizeof (TCHAR));
	if (service_table[0].lpServiceName == NULL) {
		return;
	}
	strcpy(service_table[0].lpServiceName, APP_NAME);
	service_table[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION) main_loop;
	service_table[1].lpServiceName = NULL;
	service_table[1].lpServiceProc = NULL;
	StartServiceCtrlDispatcher(service_table);
}

DWORD control_handler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (dwControl)
	{
		case SERVICE_CONTROL_INTERROGATE:
			break;
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			LOG(LOG_DEBUG, "control_handler: received dwControl to stop");
			service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			service_status.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(h_status, &service_status);
			service_status.dwWin32ExitCode = 0;
			service_status.dwWaitHint = 0;
			service_status.dwCheckPoint = 0;
			SetEvent(stop_service_event);
			SetServiceStatus(h_status, &service_status);
			LOG(LOG_DEBUG, "control_handler: end");
			break;

		case SERVICE_CONTROL_PAUSE:
			break;

		case SERVICE_CONTROL_CONTINUE:
			break;

		case SERVICE_CONTROL_SESSIONCHANGE:
			switch(dwEventType)
			{
				case(WTS_SESSION_LOGON):
					PWTSSESSION_NOTIFICATION session = (PWTSSESSION_NOTIFICATION) lpEventData;
					LOG(LOG_DEBUG, "New logon session detected: %d", session->dwSessionId);
					{
						// todo Il vaudrait mieux créer un thread pour lancer cette fonction, on est censé rendre la main le plus vite possible car l'appel est bloquant au logon (30 secondes maxi sinon error)
						Context& ctx = Context::get_instance();
						std::lock_guard<std::mutex> lock(ctx.mtx);
						firstMount(session->dwSessionId);
					}
					break;
			}
			break;
		default:
			if (dwControl >= 128 && dwControl <= 255)
				// user defined control code
				break;
			else
				// unrecognised control code
				break;
	}
	return NO_ERROR;
}

bool impersonateDefault(void) {
	HANDLE hToken = NULL;
	LOG(LOG_DEBUG, "impersonateDefault");
	if( LogonUser("NETWORK SERVICE", "NT AUTHORITY", NULL, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken ) != 0) {
		bool res = false;
		res = impersonateToken(hToken);
		CloseHandle(hToken);
		return res;
	} else {
		return false;
	}
}

void revertToSelf() {
	RevertToSelf();
	if( impersonatedToken ) {
		CloseHandle(impersonatedToken);
		impersonatedToken = NULL;
	}
	LOG(LOG_DEBUG, "impersonateToken STOP");
}

bool impersonateToken(HANDLE hToken) {
	impersonatedToken = NULL;
	LOG(LOG_DEBUG, "impersonateToken START");
	if(hToken == NULL) {
		LOG(LOG_DEBUG, "hToken is null, do not impersonate");
		return true;
	}
	if(! DuplicateToken(hToken, SecurityImpersonation, &impersonatedToken) ) {
		LOG(LOG_DEBUG, "Cannot duplicate the token (%d)", GetLastError());
		return false;
	}
	if( ImpersonateLoggedOnUser(impersonatedToken) == 0) {
		LOG(LOG_DEBUG, "logon user failed %d", GetLastError());
		return false;
	}
	LOG(LOG_DEBUG, "impersonateToken OK");
	return true;
}

int win_service_postinit(void) {
	if ((stop_service_event = CreateEvent(0, FALSE, FALSE, 0)) == NULL) {
		LOG(LOG_ERR, "win_service_postinit: CreateEvent failed");
		return 1;
	}

	//Win 32 service
	service_status.dwServiceType = SERVICE_WIN32;
	service_status.dwCurrentState = SERVICE_START_PENDING;
	service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
	service_status.dwWin32ExitCode = 0;
	service_status.dwServiceSpecificExitCode = 0;
	service_status.dwCheckPoint = 0;
	service_status.dwWaitHint = 0;

	if ((h_status = RegisterServiceCtrlHandlerEx(APP_NAME, (LPHANDLER_FUNCTION_EX) control_handler, NULL)) == (SERVICE_STATUS_HANDLE) 0) {
		LOG(LOG_ERR, "win_service_postinit: RegisterServiceCtrlHandlerEx failed");
		return 1;
	}
	return 0;
}

void notify_run_win_service(void) {
	service_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(h_status, &service_status);
}

void close_win_handle(void) {
	CloseHandle(stop_service_event);
	stop_service_event = 0;
}

void finish_win_service(void) {
	RevertToSelf();
	service_status.dwWin32ExitCode = 0;
	service_status.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(h_status, &service_status);

	close_win_handle();

	service_status.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	service_status.dwCurrentState = SERVICE_STOPPED;
	service_status.dwWin32ExitCode = 0;
	service_status.dwWaitHint = 0;
	service_status.dwCheckPoint = 0;
	SetServiceStatus(h_status, &service_status);

	free(service_table[0].lpServiceName);
}

bool is_stop_main_loop_requested(const unsigned int timeout_in_second) {
	LOG(LOG_DEBUG, "is_stop_main_loop_requested: start");
	//Check if the service was stopped or the end of grab interval was reached.
	DWORD ret = (DWORD) WaitForSingleObject(((HANDLE) stop_service_event), timeout_in_second * 1000.0); // *1000 because windows uses millisecond and not second as this function takes.
	LOG(LOG_DEBUG, "is_stop_main_loop_requested: end");
	return (ret != WAIT_TIMEOUT);
}

const std::string getProgDirectory(void)
{
	std::string result ;
    result.clear() ; // On s'assure d'avoir une chaine de caractère vide..

    int max_size = MAX_PATH + 1 ;
    char progDirectory[ MAX_PATH + 1 ] ;
    memset( progDirectory , 0 , sizeof( progDirectory ) ) ;
    int nbr_char = GetModuleFileName( nullptr , progDirectory , max_size ) ; // On récupère le chemin absolu de l'exécutable de l'agent Discovery
    if ( nbr_char == 0 || nbr_char == max_size ) // Si la récupération échoue où qu'il y a un buffer overflow, on renvoit une chaine vide
    {
        return result ;
    }

    char dir[ _MAX_DIR ] ;
    char drive[ _MAX_DRIVE ] ;
    memset( dir , 0 , sizeof( dir ) ) ;
    memset( drive , 0 , sizeof( drive ) ) ;

    // On sépare le chemin absolu de l'exécutable en sa partie "Lecteur et "Dossier"
    if ( _splitpath_s( progDirectory , drive , sizeof( drive ) , dir , sizeof( dir ) , nullptr , 0 , nullptr , 0 ) == 0 )
    {
        char myDirectory[ _MAX_DRIVE + _MAX_DIR ] ;
        memset( myDirectory , 0 , sizeof( myDirectory ) ) ;
        // On construit ensuite une chaine de caractère composée de la suite logique "Lecteur" + "Dossier"
        if ( _makepath_s( myDirectory , sizeof( myDirectory ) , drive , dir , 0 , 0 ) == 0 )
        {
            result = std::string( myDirectory ) ;
        }
    }

    return result ;
}
