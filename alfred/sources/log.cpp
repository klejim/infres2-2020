#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#if !defined(_WIN32)
	#include <unistd.h>
#endif

#include "log.h"

logInfo_t g_log_info;

void InitLog(const char *_name, const char *_filepath, int _filter_priority_level, int channels)
{
    g_log_info.name = _name;
    g_log_info.filter_priority_level = _filter_priority_level;
	{
		int s = snprintf(g_log_info.filename, 0, "%s_%s_log", _filepath, _name) +1;
		g_log_info.filename = (char*) calloc(s, sizeof(char));
		snprintf(g_log_info.filename, s, "%s_%s_log", _filepath, _name);
	}
#if defined(_WIN32)
    g_log_info.pid = 0;
#else
    g_log_info.pid = getpid();
#endif
    g_log_info.channels_mask = channels;
    g_log_info._saved_channels_mask = channels;

    if (g_log_info.channels_mask & FILE_LOG_OUT)
    {
        FILE *fp = fopen(g_log_info.filename, "a");

#ifdef XXXXX_DEBUG
        fprintf(fp, "init log %s : mode debug\n", g_log_info.filename);
#else
        fprintf(fp, "init log %s : mode release\n", g_log_info.filename);
#endif

        if (fp)
            fclose(fp);
    }
}

void changeChannelLog(int _channel, int _state)
{
	assert( _state >= 0 && _state <= 2);
	assert( _state == 2 || _channel != 0 );

	if( _state == 1 )
	{
		g_log_info._saved_channels_mask = g_log_info.channels_mask;
		g_log_info.channels_mask |= _channel;
	}
	else if( _state == 0 )
	{
		g_log_info._saved_channels_mask = g_log_info.channels_mask;
		g_log_info.channels_mask ^= _channel;
	}
	else if( _state == 2 )
	{
		int swap = g_log_info.channels_mask;
		g_log_info.channels_mask = g_log_info._saved_channels_mask;
		g_log_info._saved_channels_mask = swap;
	}
}

void PrintLog(int _priority, int _line, const char *_file, const char *_format, ...)
{
    if (_priority > g_log_info.filter_priority_level)
        return;

    char *filename = (char*) strrchr(_file, '/');

    if (filename == NULL)
        filename = (char *) _file;
    else
        filename++;

    time_t mytime = time(0);
    char _time[64];
    strftime(_time, sizeof (_time), "+%Y-%m-%d %X", localtime(&mytime));

    const char *priority_str = "undefined";

    switch (_priority)
    {
        case LOG_INFO: priority_str = "INFO";
            break;
        case LOG_DEBUG: priority_str = "DEBUG";
            break;
        case LOG_NOTICE: priority_str = "NOTICE";
            break;
        case LOG_WARNING: priority_str = "WARNING";
            break;
        case LOG_ERR: priority_str = "ERROR";
            break;
        case LOG_CRIT: priority_str = "CRITICAL";
            break;
        case LOG_ALERT: priority_str = "ALERT";
            break;
        case LOG_EMERG: priority_str = "EMERGENCY";
            break;
    }

    char *str_msg = NULL;
    va_list ap1, ap2;
    va_start(ap1, _format);
	va_copy(ap2, ap1);
	size_t s = vsnprintf(str_msg, 0, _format, ap1) +1;
	str_msg = (char*) calloc(s, sizeof(char));
	vsnprintf(str_msg, s, _format, ap2);
    va_end(ap1);

    char *str = NULL;
    int str_size;

    if (g_log_info.channels_mask & JSON_OUT)
	{
		str_size = snprintf(str, 0, "{\"time\":\"%s\",\"module\":\"%s\",\"pid\":%u,\"filename\":\"%s\",\"line\":%u,\"criticity\":\"%s\",\"message\":\"%s\"}", _time, g_log_info.name, g_log_info.pid, filename, _line, priority_str, str_msg) +1;
		str = (char*) calloc(str_size, sizeof(char));
        snprintf(str, str_size, "{\"time\":\"%s\",\"module\":\"%s\",\"pid\":%u,\"filename\":\"%s\",\"line\":%u,\"criticity\":\"%s\",\"message\":\"%s\"}", _time, g_log_info.name, g_log_info.pid, filename, _line, priority_str, str_msg);
	}
    else
	{
        str_size = snprintf(str, 0, "%s [%s(%d) %s line:%d] %s > %s\n", _time, g_log_info.name, g_log_info.pid, filename, _line, priority_str, str_msg) +1;
		str = (char*) calloc(str_size, sizeof(char));
        snprintf(str, str_size, "%s [%s(%d) %s line:%d] %s > %s\n", _time, g_log_info.name, g_log_info.pid, filename, _line, priority_str, str_msg);
	}

    free(str_msg);

    if (g_log_info.channels_mask & STD_ERR_OUT)
        fprintf(stderr, "%s", str);

    if (g_log_info.channels_mask & FILE_LOG_OUT)
    {
        FILE *fp = fopen(g_log_info.filename, "a");
        if (fp)
        {
            fprintf(fp, "%s", str);
            fflush(fp);
            fclose(fp);
        }
    }

#ifndef _WIN32

    if (g_log_info.channels_mask & SYS_LOG_OUT)
    {
        openlog(g_log_info.name, 0, LOG_LOCAL7);
        syslog(_priority, "%s", str);
        closelog();
    }

#endif

    if (str_size >= 0)
        free(str);
}


