/*
 * gcc-debug.h
 *
 * Copyright(C) 2013   MT
 *
 * Define various debugging and error handling routines.  
 *
 * This  program is free software: you can redistribute it and/or modify it
 * under  the terms of the GNU General Public License as published  by  the
 * Free  Software Foundation, either version 3 of the License, or (at  your
 * option) any later version.
 *
 * This  program  is distributed in the hope that it will  be  useful,  but
 * WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You  should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 17 Aug 13  0.1.0001  - Initial version - MT
 * 07 Dec 18            - Updated to allow debugging to be turned on or off
 *                        by setting  DEBUG true or false - MT
 * 22 Aug 21            - Prints Current filename and line number - MT
 *                      - Added verbose macro - MT
 * 03 Jan 22  0.2.0005  - Changed both macros to allow DEBUG and VERBOSE to
 *                        be defined from the command line - MT
 * 09 Jun 25  0.3.0006  - Changed to use stdout instead of stderr - MT
 * 25 Jul 25  0.4.0007  - Added profile and error macros - MT
 *                      - Made error handling ANSI C compatible - MT   
 *  1 Aug 25            - All macro definitions are now guarded by a single
 *                        conditional  - MT
 * 
 */
 
#ifndef _DEBUG_H /* Don't redefine functions if already defined. */
#define _DEBUG_H        1

#if defined DEBUG  /* Execute code if DEBUG is True */ 
#define debug(code) do {fprintf(stdout, "Debug\t: %s line : %d : ", \
            __FILE__, __LINE__); code; fprintf(stdout, "\n");} while(0)
#else
#define debug(code)
#endif

#if defined VERBOSE  /* Execute code if VERBOSE is True */ 
#define verbose(code) do {fprintf(stdout, "Verbose\t: %s line : %d : ", \
            __FILE__, __LINE__); code; fprintf(stdout, "\n");} while(0)
#else
#define verbose(code)
#endif

#if defined DEBUG  /* Execute code if DEBUG is True */ 
#include <time.h>
#define profile(__code) do { /* Time how long code takes to execute */  \
   struct timespec __t_start, __t_end; \
   double __d_elapsed; \
   clock_gettime(CLOCK_MONOTONIC, &__t_start); \
   __code \
   clock_gettime(CLOCK_MONOTONIC, &__t_end); \
   __d_elapsed = (__t_end.tv_sec - __t_start.tv_sec) + \
                    (__t_end.tv_nsec - __t_start.tv_nsec) / 1e9; \
   fprintf(stdout, "%0.9fs\n", __d_elapsed); \
} while(0)
#else
#define profile(__code) do { /* Just run it */ \
   __code \
} while(0)
#endif

#if defined NAME
void error(int i_errno, const char *s_format, ...)  /* Print formatted error message and exit returning errno */
{
   va_list t_args;
   if (!(i_errno)) i_errno = -1;  /* If errno not set return -1 */
   va_start(t_args, s_format);
   fprintf(stderr, "%s: ", NAME);
   vfprintf(stderr, s_format, t_args);
   fprintf(stderr, "\n");
   va_end(t_args);
   exit(i_errno);
}

void warning(int i_errno, const char *s_format, ...)  /* Print formatted warning message */
{
   va_list t_args;
   va_start(t_args, s_format);
   fprintf(stderr, "%s: ", NAME);
   vfprintf(stderr, s_format, t_args);
   fprintf(stderr, "\n");
   va_end(t_args);
}
#else
void error(int i_errno, const char *s_format, ...);
void warning(int i_errno, const char *s_format, ...);
#endif
#endif
