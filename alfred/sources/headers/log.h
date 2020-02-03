/*
 * File:   log.h
 * Author: alainlg
 *
 * Created on 30 octobre 2013, 11:05
 */

#pragma once

#include <stdio.h>

#ifndef _WIN32
#include <syslog.h>
#else
    #define	LOG_EMERG	0	/* system is unusable */
    #define	LOG_ALERT	1	/* action must be taken immediately */
    #define	LOG_CRIT	2	/* critical conditions */
    #define	LOG_ERR		3	/* error conditions */
    #define	LOG_WARNING	4	/* warning conditions */
    #define	LOG_NOTICE	5	/* normal but significant condition */
    #define	LOG_INFO	6	/* informational */
    #define	LOG_DEBUG	7	/* debug-level messages */
#endif
    #define	LOG_WARN	LOG_WARNING	/* warning conditions */

	typedef struct
{
    const char *name;
    char *filename;
    int  filter_priority_level;
    unsigned int pid;
    unsigned int channels_mask;
    unsigned int _saved_channels_mask;
} logInfo_t;

#define STD_ERR_OUT     0x0001
#define FILE_LOG_OUT    0x0002
#define SYS_LOG_OUT     0x0004
#define ALL_LOG_OUT     0x0007

#define JSON_OUT        0x8000

void InitLog(const char *_name, const char *_filepath, int _priority_filter, int channels);
void PrintLog(int _priority, int _line, const char *_file, const char *_format, ... );


/*
 * Change les channels actifs.
 * _state == 1 => active
 * _state == 0 => désactive
 * _state == 2 => restaure le dernier masque (avant changement)
 */
void changeChannelLog(int _channel, int _state);

#ifdef LOG
#error "on utilise les deux systemes de log en meme temps"
#endif

// le ## indique au compilo qu'il ne doit pas mettre de "," si argv n'a aucun params
#if defined(_WIN32)
	#define LOG(_priority_level, _desc_str, ...) PrintLog(_priority_level, __LINE__, __FILE__, _desc_str, __VA_ARGS__)
#else
	#define LOG(_priority_level, _desc_str, _params...) PrintLog(_priority_level, __LINE__, __FILE__, _desc_str, ##_params)
#endif

