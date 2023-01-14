/*
Command line parsing. Choice of public domain or MIT-0. See license statements at the end of this file.

David Reid - mackron@gmail.com
*/

/*
This is a very simple library for parsing an argv style command lines. Essentially all it does is
searches for strings within a list. If you're looking for something full-featured you'll need to
look elsewhere. More features may be added later.

Although there's barely any code here, I'm tired of having to copy this around from other projects
so I've decided to just put this into a library so I don't ever need to do that song and dance ever
again.

Use `argv_find()` to find the index in `argv` of the given string. If it exists, 0 will be
returned. Otherwise, a negative number will be returned.

Use `argv_get()` to retrieve the value of a switch. For example, to return "value" in the command
line `--key value`, do the following:

    const char* value = argv_get(argc, argv, "--key");

Note how it's just a simple string comparison for the key. There is no logic for handling prefixes
like "--", "+", etc.

All key lookups start from index 1, with the assumption that the name of the application is in the
first item in the array which is therefore skipped.
*/
#ifndef argv_h
#define argv_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* For size_t. */

#ifndef ARGV_API
#define ARGV_API extern
#endif

typedef struct
{
    void* pUserData;
    void* (* onMalloc)(size_t sz, void* pUserData);
    void* (* onRealloc)(void* p, size_t sz, void* pUserData);
    void  (* onFree)(void* p, void* pUserData);
} argv_allocation_callbacks;

ARGV_API void* argv_malloc(size_t sz, const argv_allocation_callbacks* pAllocationCallbacks);
ARGV_API void* argv_realloc(void* p, size_t sz, const argv_allocation_callbacks* pAllocationCallbacks);
ARGV_API void argv_free(void* p, const argv_allocation_callbacks* pAllocationCallbacks);

ARGV_API int argv_find(int argc, const char** argv, const char* key, int* index);
ARGV_API const char* argv_get(int argc, const char** argv, const char* key);
ARGV_API int argv_from_WinMain(const char* cmdline, int* pargc, char*** pargv, const argv_allocation_callbacks* pAllocationCallbacks);

#ifdef __cplusplus
}
#endif
#endif  /* argv_h */




#if defined(ARGV_IMPLEMENTATION)
#ifndef argv_c
#define argv_c

#ifndef ARGV_NULL
#define ARGV_NULL 0
#endif

#if !defined(ARGV_MALLOC) || !defined(ARGV_REALLOC) || !defined(ARGV_FREE)
    #include <stdlib.h>
    #define ARGV_MALLOC(sz)      malloc(sz)
    #define ARGV_REALLOC(p, sz)  realloc((p), (sz))
    #define ARGV_FREE(p)         free(p)
#endif

ARGV_API void* argv_malloc(size_t sz, const argv_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == ARGV_NULL) {
        return ARGV_MALLOC(sz);
    } else {
        if (pAllocationCallbacks->onMalloc != ARGV_NULL) {
            return pAllocationCallbacks->onMalloc(sz, pAllocationCallbacks->pUserData);
        } else {
            return ARGV_NULL;    /* No malloc() implementation. */
        }
    }
}

ARGV_API void* argv_realloc(void* p, size_t sz, const argv_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == ARGV_NULL) {
        return ARGV_REALLOC(p, sz);
    } else {
        if (pAllocationCallbacks->onRealloc != ARGV_NULL) {
            return pAllocationCallbacks->onRealloc(p, sz, pAllocationCallbacks->pUserData);
        } else {
            return ARGV_NULL;    /* No realloc() implementation. */
        }
    }
}

ARGV_API void argv_free(void* p, const argv_allocation_callbacks* pAllocationCallbacks)
{
    if (pAllocationCallbacks == ARGV_NULL) {
        ARGV_FREE(p);
    } else {
        if (pAllocationCallbacks->onFree != ARGV_NULL) {
            pAllocationCallbacks->onFree(p, pAllocationCallbacks->pUserData);
        }
    }
}


static void argv_copy_memory(void* dst, const void* src, size_t sz)
{
    size_t i;
    for (i = 0; i < sz; i += 1) {
        ((unsigned char*)dst)[i] = ((unsigned char*)src)[i];
    }
}


static size_t argv_strlen(const char* src)
{
    const char* end;

    end = src;
    while (end[0] != 0) {
        end += 1;
    }

    return end - src;
}

static int argv_strcmp(const char* a, const char* b)
{
    while (a[0] != 0 && a[0] == b[0]) {
        a += 1;
        b += 1;
    }

    return (unsigned char)a[0] - (unsigned char)b[0];
}

ARGV_API int argv_find(int argc, const char** argv, const char* key, int* index)
{
    int i;

    /* Always start at 1 because 0 is the name of the executable. */
    if (argc <= 1 || argv == ARGV_NULL || key == ARGV_NULL) {
        return -1;  /* Invalid args. */
    }

    for (i = 1; i < argc; ++i) {
        if (argv_strcmp(argv[i], key) == 0) {
            if (index != ARGV_NULL) {
                *index = i;
            }

            return 0;  /* Found. */
        }
    }

    /* Not found. */
    return -1;
}

ARGV_API const char* argv_get(int argc, const char** argv, const char* key)
{
    int ikey;

    if (argv_find(argc, argv, key, &ikey) != 0) {
        return ARGV_NULL; /* Key not found. */
    }

    if (ikey+1 == argc) {
        return ARGV_NULL; /* Nothing after the key. */
    }

    return argv[ikey+1];
}


static int argv_is_whitespace_ascii(char cp)
{
    switch (cp) {
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x20:
        case 0x85:
        case 0xA0:
            return 1;   /* Whitespace. */
                
        default:
            return 0;   /* Not whitespace. */
    }
}

static const char* argv_ltrim(const char* str)
{
    if (str == ARGV_NULL) {
        return ARGV_NULL;
    }

    /* For now we're just assuming ASCII, but we should add support for Unicode whitespace. */
    while (str[0] != 0) {
        if (!argv_is_whitespace_ascii(str[0])) {
            break;
        }

        /* Getting here means the character is whitespace. Move on to the next character. */
        str += 1;
    }

    return str;
}

static int argv_from_WinMain_internal(const char* cmdline, int* pargc, const char** pargvbeg, const char** pargvend)
{
    int count = 0;

    if (pargc == ARGV_NULL) {
        return -1;  /* Invalid args. */
    }

    /* We're just splitting on whitespaces, except for strings that are wrapped in double quotes. */
    {
        int quoted = 0;
        const char* segbeg;
        const char* segend;
        int cap = *pargc;

        /* For each segment... */
        while ((pargvbeg == ARGV_NULL || pargvend == ARGV_NULL) || count < cap) {
            cmdline = argv_ltrim(cmdline);  /* First do a trim. */
            if (cmdline[0] == '\0') {
                break;  /* Reached the end. */
            }

            /* If the next character is a double quote we need to handle it differently. */
            if (cmdline[0] == '\"') {
                quoted = 1;
                segbeg = cmdline + 1;
            } else {
                quoted = 0;
                segbeg = cmdline + 0;
            }

            segend = segbeg + 1;

            /* For each character in the segment... */
            for (;;) {
                if (segend[0] == '\0') {
                    break;  /* End of the command line. */
                }

                /* If it's quoted, an unescaped double quote marks the end. Otherwise a whitespace marks the end. */
                if (quoted) {
                    /* A double quote marks the end. */
                    if (segend[0] == '\"' && segend[-1] != '\\') {
                        break;
                    }
                } else {
                    /* A whitespace marks the end. */
                    if (argv_is_whitespace_ascii(segend[0])) {
                        break;  /* Found a whitespace character. */
                    }
                }

                segend += 1;
            }

            /* Getting here means we've found the end of the segment. */
            if (segend > segbeg) {
                if (pargvbeg != ARGV_NULL && pargvend != ARGV_NULL) {
                    pargvbeg[count] = segbeg;
                    pargvend[count] = segend;
                }

                count += 1;
            }

            cmdline = segend;

            /*
            We need to go one extra character if it's quoted or else the next iteration will think we're
            starting a quoted item, when in fact it's just the end of the one we just processed.
            */
            if (quoted && segend[0] == '\"') {
                cmdline += 1;
            }
        }
    }

    *pargc = count;

    return 0;
}

#if defined(_WIN32)
#include <windows.h>    /* For GetModuleFileName(). */
#endif

ARGV_API int argv_from_WinMain(const char* cmdline, int* pargc, char*** pargv, const argv_allocation_callbacks* pAllocationCallbacks)
{
    int result;
    int argc = 0;
    char** argv = ARGV_NULL;
    char** argvbeg = ARGV_NULL;
    char** argvend = ARGV_NULL;
    size_t sz;
    char* str;
    int iarg;
    char executableName[260];
    size_t executableNameLen;

    if (cmdline == ARGV_NULL || pargc == ARGV_NULL || pargv == ARGV_NULL) {
        return -1;  /* Invalid args. */
    }

    /* We'll need an executable name to put into argv[0] since it won't be included by Windows. */
#if defined(_WIN32)
    GetModuleFileNameA(ARGV_NULL, executableName, sizeof(executableName));
#else
    executableName[0] = '\0';
#endif
    executableNameLen = argv_strlen(executableName);

    /* First step is the count the number of items. */
    result = argv_from_WinMain_internal(cmdline, &argc, ARGV_NULL, ARGV_NULL);
    if (result != 0) {
        return result;  /* Failed to count the number of items. */
    }

    /*
    The size of our data allocation will be based on the number of items in the array and the
    total length of every string in the argument list. The count is explicitly returned from
    argv_from_WinMain_internal() and the total length can be determined from the length of the
    input string. The reason this works is that the delimiters will just be replaced with null
    terminators.
    */
    sz  = sizeof(char*) * (argc+1) * 2; /* x2 so we can store the end pointers. */
    sz += executableNameLen + 1 + argv_strlen(cmdline) + 1;     /* +1 for null terminator. */
    
    argv = (char**)argv_malloc(sz, pAllocationCallbacks);
    if (argv == ARGV_NULL) {
        return -1;  /* Out of memory. */
    }

    argvbeg = argv + 1; /* +1 because the first slot will be reserved for the executable name. */
    argvend = argvbeg + argc;

    result = argv_from_WinMain_internal(cmdline, &argc, (const char**)argvbeg, (const char**)argvend);
    if (result != 0) {
        argv_free(argv, pAllocationCallbacks);
        return result;
    }

    /*
    We have the begin and end pointers for each item relative to the input string and now we need
    to copy that into our output buffer.
    */
    str = ((char*)argv) + sizeof(char*) * (argc+1) * 2; /* This is the beginning of our strings. */

    /* The executable name needs to be inserted. */
    argv_copy_memory(str, executableName, executableNameLen);
    str[executableNameLen] = '\0';

    argv[0] = str;
    str += executableNameLen + 1;

    for (iarg = 0; iarg < argc; iarg += 1) {
        size_t len = (argvend[iarg] - argvbeg[iarg]);

        argv_copy_memory(str, argvbeg[iarg], len);
        str[len] = '\0';

        argvbeg[iarg] = str;
        str += len + 1;
    }

    /* Done. */
    *pargc = argc + 1;  /* +1 for the executable name. */
    *pargv = argv;

    return 0;
}

#endif  /* argv_c */
#endif  /* ARGV_IMPLEMENTATION */

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2022 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
