/*
 * skeleton.c - Include skeleton code in an output stream.
 *
 * Copyright (C) 2001  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "system.h"
#include "input.h"
#include "info.h"
#include "errors.h"
#if HAVE_UNISTD_H
	#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif
#if defined(WIN32) || defined(_WIN32)
	#include <windows.h>
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Check for a skeleton file in a specific directory.
 * Returns the malloc'ed path if found, or NULL otherwise.
 */
static char *CheckSkeleton(const char *dir, const char *skeleton)
{
	int len;
	char *path;

	/* Construct the full skeleton path */
	len = strlen(dir);
	if((path = (char *)malloc(len + strlen(skeleton) + 2)) == 0)
	{
		TreeCCOutOfMemory(0);
	}
	strcpy(path, dir);
#if (defined(WIN32) || defined(_WIN32)) && !defined(__CYGWIN__)
	path[len] = '\\';
#else
	path[len] = '/';
#endif
	strcpy(path + len + 1, skeleton);

	/* Is the file present? */
#if HAVE_ACCESS
	if(access(path, 0) == 0)
	{
		return path;
	}
#else
#if HAVE_STAT
	{
		struct stat st;
		if(stat(path, &st) == 0)
		{
			return path;
		}
	}
#else
	{
		/* Don't have "access" or "stat", so try to open it */
		FILE *file = fopen(path, "r");
		if(file)
		{
			fclose(file);
			return path;
		}
	}
#endif
#endif

	/* Could not find the skeleton file */
	free(path);
	return 0;
}

/*
 * Find a skeleton file along the standard search path.
 * Returns the malloc'ed path if found, or NULL otherwise.
 */
static char *FindSkeleton(TreeCCContext *context, const char *skeleton)
{
	char *path;

	/* Look in the user-supplied skeleton directory */
	if(context->skeletonDirectory)
	{
		if((path = CheckSkeleton(context->skeletonDirectory, skeleton)) != 0)
		{
			return path;
		}
	}

	/* Look in Windows-specific locations */
#if defined(WIN32) || defined(_WIN32)
	{
		char moduleName[1024];
		int len;

		if(GetModuleFileName(NULL, moduleName, sizeof(moduleName) - 8) != 0)
		{
			/* Trim the module name to the name of the directory */
			len = strlen(moduleName);
			while(len > 0 && moduleName[len - 1] != '\\' &&
				  moduleName[len - 1] != '/' &&
				  moduleName[len - 1] != ':')
			{
				--len;
			}
			if(len > 0 && moduleName[len - 1] != ':')
			{
				--len;
			}
			if(len > 0)
			{
				/* Look in the "etc" sub-directory underneath where the
				   executable was loaded from */
				strcpy(moduleName + len, "\\etc");
				if((path = CheckSkeleton(moduleName, skeleton)) != 0)
				{
					return path;
				}

				/* Look in the same directory as the executable */
				moduleName[len] = '\0';
				if((path = CheckSkeleton(moduleName, skeleton)) != 0)
				{
					return path;
				}
			}
		}
	}
#endif

#if !(defined(WIN32) || defined(_WIN32)) || defined(__CYGWIN__)

	/* Try looking in the compiled-in default directory */
#ifdef TREECC_DATA_DIR
	if((path = CheckSkeleton(TREECC_DATA_DIR, skeleton)) != 0)
	{
		return path;
	}
#endif

	/* Look in several standard places that it might be */
	if((path = CheckSkeleton("/usr/local/share/treecc", skeleton)) != 0)
	{
		return path;
	}
	if((path = CheckSkeleton("/opt/local/share/treecc", skeleton)) != 0)
	{
		return path;
	}
	if((path = CheckSkeleton("/usr/share/treecc", skeleton)) != 0)
	{
		return path;
	}
#endif

	/* Could not find the skeleton */
	TreeCCAbort(0, "could not locate the skeleton file \"%s\"\n", skeleton);

	/* Keep the compiler happy */
	return (char *)0;
}

void TreeCCIncludeSkeleton(TreeCCContext *context, TreeCCStream *stream,
						   const char *skeleton)
{
	char *path = FindSkeleton(context, skeleton);
	FILE *file = fopen(path, "r");
	char buffer[BUFSIZ];
	int posn, start;
	if(!file)
	{
		perror(path);
		exit(1);
	}
	TreeCCStreamPrint(stream, "#line 1 \"%s\"\n", path);
	while(fgets(buffer, BUFSIZ, file))
	{
	#if HAVE_STRCHR
		if(strchr(buffer, 'Y') != 0 || strchr(buffer, 'y') != 0)
	#endif
		{
			/* The line probably contains "YYNODESTATE" or "yy" */
			posn = 0;
			start = 0;
			while(buffer[posn] != '\0')
			{
				if(buffer[posn] == 'Y' &&
				   !strncmp(buffer + posn, "YYNODESTATE", 11))
				{
					buffer[posn] = '\0';
					if(start < posn)
					{
						TreeCCStreamCode(stream, buffer + start);
					}
					TreeCCStreamCode(stream, context->state_type);
					posn += 11;
					start = posn;
				}
				else if(buffer[posn] == 'y' && buffer[posn + 1] == 'y')
				{
					buffer[posn] = '\0';
					if(start < posn)
					{
						TreeCCStreamCode(stream, buffer + start);
					}
					TreeCCStreamCode(stream, context->yy_replacement);
					posn += 2;
					start = posn;
				}
				else
				{
					++posn;
				}
			}
			if(start < posn)
			{
				TreeCCStreamCode(stream, buffer + start);
			}
		}
	#if HAVE_STRCHR
		else
		{
			/* Ordinary line */
			TreeCCStreamCode(stream, buffer);
		}
	#endif
	}
	fclose(file);
	TreeCCStreamFixLine(stream);
	free(path);
}

#ifdef	__cplusplus
};
#endif
