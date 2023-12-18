 /**************************************************************************\
 *                                                                          *
 *   pxwc - wgcc's gcc to cl libc wrapper                                   *
 *   Copyright (C) 2006  Markus Duft <markus.duft@salomon.at>               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with this library; if not, write to the Free Software    *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,             *
 *   MA  02110-1301 USA                                                     *
 *                                                                          *
 \**************************************************************************/

#ifndef ARM

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#if defined(__PXWC__)
#error "You have another version of pxwc in your environment! Please remove it befor building pxwc!"
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

static int find_char(const char * str, char ch)
{
	while(*str != '\0')
	{
		if(*(++str) == ch)
			return 1;
	}
	return 0;
}

/* Returns NULL or a pointer to "Buffer" if successfull */
char * __pxwc_convert_binary(char * Buffer, int Size)
{
	/* Take Buffer (contains Path) and convert it, replace old value and return buffer */
	char * Output = (char*)malloc(Size);
	char * InterixDir = getenv("SFUDIR");
	char * command = (char*)malloc(strlen(InterixDir) + Size + 100);
	FILE * pipe;

	strcpy(command, InterixDir);
	strcat(command, "/bin/unixpath2win ");
	strcat(command, Buffer);
	strcat(command, " 2>__pxwc_env_tmp");

	pipe = _popen(command, "r");

	if(!fgets(Output, Size, pipe))
	{
		/* Don't return immediatly, because we need to clean up */
		/* But still let us know we've failed */
		free(Output);
		Output = NULL;
	}

	_pclose(pipe);
	free(command);
	unlink("__pxwc_env_tmp");

	if(!Output || strlen(Output) <= 0)
		return NULL;

	Output[strlen(Output) - 1] = '\0';	/* Strip the '\n' */

	strncpy(Buffer, Output, Size);

	free(Output);

	return Buffer;
}

#if defined(INTERIX_VERSION) && INTERIX_VERSION < 5
typedef char* (__cdecl * u2w_func_t)(const char*, int, char*, size_t);
#else
typedef int (__cdecl * u2w_func_t)(const char*, int, char*, size_t);
#endif

/* Returns NULL or a pointer to "Buffer" if successfull */
char * __pxwc_convert_dll(char * Buffer, int Size)
{
	/* we wrap this in a SEH __try/__except to catch LoadLibrary errors with invalid dll's */
	__try {

		/* Take Buffer (contains Path) and convert it, replace old value and return buffer */
		char * Output = (char*)malloc(Size * 2);
	#if defined(INTERIX_VERSION) && INTERIX_VERSION < 5
		char * InterixDll = "psxwcl32";
	#else
		char * InterixDll = "psxdll";
	#endif
		static HMODULE hLibrary = NULL;
		u2w_func_t pFunction = NULL;
		
		if(!hLibrary)
			hLibrary = LoadLibrary(InterixDll);

		if(!hLibrary)
		{
			free(Output);
			return NULL;
		}

	#if defined(INTERIX_VERSION) && INTERIX_VERSION < 5
		pFunction = (u2w_func_t)GetProcAddress(hLibrary, "WCLunixpath2win");
	#else
		pFunction = (u2w_func_t)GetProcAddress(hLibrary, "unixpath2win");
	#endif

		if(!pFunction)
		{
			free(Output);
			return NULL;
		}

	#if defined(INTERIX_VERSION) && INTERIX_VERSION < 5
		if(!pFunction(Buffer, 0, Output, Size))
	#else
		if(pFunction(Buffer, 0, Output, Size) != 0)
	#endif
		{
			free(Output);
			return NULL;
		}

		if(!Output || strlen(Output) <= 0)
			return NULL;

		strncpy(Buffer, Output, Size);
		free(Output);

		return Buffer;

	} __except(EXCEPTION_EXECUTE_HANDLER) {
		/* if we had a SEH exception, just return NULL, so the binary fallback
		 * takes over control ;o) */
		return NULL;
	}
}

char * __pxwc_convert(char* Buffer, int Size)
{
	char * tmp = NULL;

	if((tmp = __pxwc_convert_dll(Buffer, Size)) == NULL)
	{
		tmp = __pxwc_convert_binary(Buffer, Size);
	}

	return tmp;
}

/* This has the same signature as the real getenv. */
/* In pxwc.h getenv is redefined to this one. This
	affects stdlib.h, which is fine here, because
	we then get this function by only including stdlib.h.
	No need to include win_paths.h anywhere for now! */


static char * __dummy_getenv(const char * EnvName)
{
	char * EnvContent = getenv(EnvName);

	/* Return null if nothing was found for that env */
	if(!EnvContent)
		return NULL;

	/* Return unconverted Env if it does not start with "/" or ":/" */
	/* The ":" may appear as first char in seperated path lists ... */
	if(!(EnvContent[0] == '/' || (strncmp(EnvContent, ":/", 2) == 0) || (strncmp(EnvContent, ".:/", 3) == 0)))
		return EnvContent;

	/* It seems here we can asume we have a path ... Just find out how many of them */
	/* Inside this "if" conversion takes place and the modified vars get written back to the Env */
	if(find_char(EnvContent, ':'))
	{
		/* Multiple Paths found  */
		char * Content	= strdup(EnvContent);
		char * Final	= (char*)malloc(strlen(EnvContent) * 2);
		char * Token	= strtok(Content, ":");

		Final[0] = '\0';	/* So we can use strcat */

		while(Token != NULL)
		{
			if(Token[0] == '.')
			{
				strcat(Final, Token);
				strcat(Final, ";");
			} else {
				char * Buffer = (char*)malloc(strlen(Token) * 2);
				strcpy(Buffer, Token);	/* Don't use Token directly, may corrupt Content */

				if(__pxwc_convert_dll(Buffer, strlen(Token) * 2))
				{
					strcat(Final, Buffer);
				} else if(__pxwc_convert_binary(Buffer, strlen(Token)*2)) {
					strcat(Final, Buffer);
				} else {
					strcat(Final, Token);	/* Still insert original path if conversion failed */
				}

				free(Buffer);
				strcat(Final, ";");
			}
			Token = strtok(NULL, ":");
		}

		if(Final && strlen(Final) >= 0)
		{
			char * PutEnv = (char*)malloc(strlen(EnvName) + strlen(Final) + 1 /* For the "=" */);

			Final[strlen(Final) - 1] = '\0'; /* Strip last ";" */

			strcpy(PutEnv, EnvName);
			strcat(PutEnv, "=");
			strcat(PutEnv, Final);

			if(_putenv(PutEnv) != 0)
			{
				free(Content);
				free(Final);
				free(PutEnv);
				return EnvContent;
			}

			free(PutEnv);
		}

		free(Content);
		free(Final);

	} else {
		/* One single Path found */
		char * Buffer = (char*)malloc(strlen(EnvContent) * 2);
		strcpy(Buffer, EnvContent);

		if(__pxwc_convert_dll(Buffer, strlen(EnvContent) * 2))
		{
			char * PutEnv = (char*)malloc(strlen(EnvName) + strlen(Buffer) + 1 /* for the "=" */ + 1 /* for the \0 */);
			strcpy(PutEnv, EnvName);
			strcat(PutEnv, "=");
			strcat(PutEnv, Buffer);

			if(_putenv(PutEnv) != 0)
			{
				free(PutEnv);
				free(Buffer);
				return EnvContent;
			}
			free(PutEnv);
		} else if(__pxwc_convert_binary(Buffer, strlen(EnvContent) * 2)) {
			char * PutEnv = (char*)malloc(strlen(EnvName) + strlen(Buffer) + 1 /* for the "=" */ + 1 /* for the \0 */);
			strcpy(PutEnv, EnvName);
			strcat(PutEnv, "=");
			strcat(PutEnv, Buffer);

			if(_putenv(PutEnv) != 0)
			{
				free(PutEnv);
				free(Buffer);
				return EnvContent;
			}
			free(PutEnv);
		} else {
			/* Buffer is untouched, so it's save to free() it */
			free(Buffer);
			return EnvContent;
		}

		free(Buffer);
	}

	/* If everything went well, we return an original getenv on the converted var... */
	return getenv(EnvName);
}

#endif /* ARM */

char * __cdecl __pxwc_getenv(const char * EnvName)
{
#ifdef ARM
	return 0;
#else
	return __dummy_getenv(EnvName);
#endif
}

#ifndef PIC
char * (*_imp____pxwc_getenv)(const char*) = __pxwc_getenv;
#endif
