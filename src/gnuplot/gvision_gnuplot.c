/**
 * Copyright (c) 2017 Atanas Filipov <it.feel.filipov@gmail.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gnuplot/gvision_gnuplot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

/** Maximal size of a gnuplot command */
#define GP_CMD_SIZE     	2048
/** Maximal size of a plot title */
#define GP_TITLE_SIZE   	80
/** Maximal size for an equation */
#define GP_EQ_SIZE      	512

/* int checkifexecutable(const char *filename)
 *
 * Return non-zero if the name is an executable file, and
 * zero if it is not executable, or if it does not exist.
 */
static int checkifexecutable(const char *filename)
{assert(filename);

     int result;
     struct stat statinfo;

     result = stat(filename, &statinfo);
     if (result < 0) {
        return 0;
     }

     if (!S_ISREG(statinfo.st_mode)) {
        return 0;
     }

     if (statinfo.st_uid == geteuid()) {
        return statinfo.st_mode & S_IXUSR;
     }

     if (statinfo.st_gid == getegid()) {
        return statinfo.st_mode & S_IXGRP;
     }

     return statinfo.st_mode & S_IXOTH;
}

/* int findpathof(char *pth, const char *exe)
 *
 * Find executable by searching the PATH environment variable.
 *
 * const char *exe - executable name to search for.
 *       char *pth - the path found is stored here, space
 *                   needs to be available.
 *
 * If a path is found, returns non-zero, and the path is stored
 * in pth.  If exe is not found returns 0, with pth undefined.
 */
static int findpathof(char *pth, const char *exe)
{assert(pth && exe);

     char *searchpath;
     char *beg, *end;
     int stop, found;
     int len;

     if (strchr(exe, '/') != NULL) {
	  if (realpath(exe, pth) == NULL) return 0;
	  return  checkifexecutable(pth);
     }

     searchpath = getenv("PATH");
     if (searchpath == NULL) return 0;
     if (strlen(searchpath) <= 0) return 0;

     beg = searchpath;
     stop = 0; found = 0;
     do {
	  end = strchr(beg, ':');
	  if (end == NULL) {
	       stop = 1;
	       strncpy(pth, beg, PATH_MAX);
	       len = strlen(pth);
	  } else {
	       strncpy(pth, beg, end - beg);
	       pth[end - beg] = '\0';
	       len = end - beg;
	  }
	  if (pth[len - 1] != '/') strncat(pth, "/", 1);
	  strncat(pth, exe, PATH_MAX - len);
	  found = checkifexecutable(pth);
	  if (!stop) beg = end + 1;
     } while (!stop && !found);

     return found;
}

FILE * gnuplot_init(void)
{
    FILE *handle = NULL;
    char path[PATH_MAX+1];

    if (getenv("DISPLAY") == NULL) {
        fprintf(stderr, "cannot find DISPLAY variable: is it set?\n");
    }

    if (findpathof(path, "gnuplot")) {
        handle = popen("gnuplot", "w");
        if (handle == NULL) {
            fprintf(stderr, "error starting gnuplot\n");
            return NULL ;
        }
    } else {
        fprintf(stderr, "cannot find gnuplot\n");
        return NULL ;
    }

    return handle;
}

void gnuplot_close(FILE* const handle)
{assert(handle);

    if (pclose(handle) == -1) {
        fprintf(stderr, "problem closing communication to gnuplot\n");
    }
}

void gnuplot_cmd(FILE* const handle, const char *cmd, ...)
{assert(handle && cmd);

    va_list ap ;
    char    local_cmd[GP_CMD_SIZE];

    va_start(ap, cmd);
    vsprintf(local_cmd, cmd, ap);
    va_end(ap);

    strcat(local_cmd, "\n");

    fputs(local_cmd, handle) ;
    fflush(handle);
}

void gnuplot_set_xlabel(FILE* const handle, const char *label)
{assert(handle && label);

    char    cmd[GP_CMD_SIZE] ;

    sprintf(cmd, "set xlabel \"%s\"", label) ;
    gnuplot_cmd(handle, cmd) ;
}

void gnuplot_set_ylabel(FILE* const handle, const char *label)
{assert(handle && label);

    char    cmd[GP_CMD_SIZE] ;

    sprintf(cmd, "set ylabel \"%s\"", label) ;
    gnuplot_cmd(handle, cmd) ;
}
