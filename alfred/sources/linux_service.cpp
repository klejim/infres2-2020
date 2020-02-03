#include "main_loop.h"
#include "log.h"
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <libgen.h> // pour dirname()
#include <limits.h> // pour PATH_MAX

static bool stopMainLoop = false;

static void signal_handler(int)
{
	stopMainLoop = true;
}

void setSignalHandler(void)
{
	struct sigaction struct_sigaction;
	sigset_t sig_block;
	memset(&struct_sigaction, 0, sizeof ( struct_sigaction));
	struct_sigaction.sa_handler = signal_handler;
	sigemptyset(&struct_sigaction.sa_mask);
	sigaddset(&struct_sigaction.sa_mask, SIGINT);
	sigaddset(&struct_sigaction.sa_mask, SIGUSR1);
	sigaddset(&struct_sigaction.sa_mask, SIGTERM);
	sigaction(SIGINT, &struct_sigaction, nullptr);
	sigaction(SIGUSR1, &struct_sigaction, nullptr);
	sigaction(SIGTERM, &struct_sigaction, nullptr);
	sigemptyset(&sig_block);
	sigaddset(&sig_block, SIGINT);
	sigaddset(&sig_block, SIGUSR1);
	sigaddset(&sig_block, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &sig_block, nullptr);
}

bool is_stop_main_loop_requested(const unsigned int timeout_in_second) {
	LOG(LOG_DEBUG, "is_stop_main_loop_requested: start");
	if( not stopMainLoop ) sleep(timeout_in_second);
	LOG(LOG_DEBUG, "is_stop_main_loop_requested: end");
	return stopMainLoop;
}

const std::string getProgDirectory()
{
    int nsize = PATH_MAX + 1 ;
    char prog_abs_path[ nsize ] ;
    memset( prog_abs_path , 0 , sizeof( prog_abs_path ) ) ; // On s'assure d'avoir un tableau de caractère vide..
    int n = readlink( "/proc/self/exe" , prog_abs_path , nsize ) ; // On récupère le chemin absolu vers l'exécutable
    if ( n > 0 ) // Si l'opération s'est bien déroulé, on "formate" la sortie afin d'avoir une chaine de caractère convenable
    {
        prog_abs_path[ n ] = 0 ;
    }
    if ( n == 0 || n == nsize ) // S'il est impossible d'obtenir le chemin absolu de l'exécutable où que la taille du tableau est dépassé/atteint
    {
        return std::string() ;
    }
    char result_cstr[ nsize ] ;
    memset( result_cstr , 0 , sizeof( result_cstr ) ) ; // On s'assure d'avoir un tableau de caractère vide..
    char* prog_path = strdup( prog_abs_path ) ; // On recopie la chaine de caractère présente dans prog_abs_path..
    char* prog_dirname = strdup( dirname( prog_path ) ) ; // On récupère uniquement la partie "répertoire" du chemin
    snprintf( result_cstr , sizeof( result_cstr ) , "%s/" , prog_dirname ) ;
    free( prog_path ) ;
    free( prog_dirname ) ;
    return std::string( result_cstr ) ;
}
