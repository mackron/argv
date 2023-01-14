#define ARGV_IMPLEMENTATION
#include "../argv.h"

#include <stdio.h>

const char* g_Test0[] = {
    "appname",
    "key"
};

const char* g_Test1[] = {
    "appname",
    "--key",
    "value"
};

const char* g_Test2[] = {
    "appname",
    "--key",
    "the value"
};
const char* g_Test2_WinMain = "\"--key\" \"the value\"";    /* Intentionally leaving off "appname" because that'll be filled by argv_from_WinMain(). */

int main(int argc, char** argv)
{
    int result = 0;

    /* argv_find(). */
    printf("argv_find()\n");
    {
        int index;

        /* Should be found at index 1. */
        printf("    Key Found Test:       ");
        if (argv_find(sizeof(g_Test0) / sizeof(g_Test0[0]), g_Test0, "key", &index) == 0) {
            if (index == 1) {
                printf("PASSED.\n");
            } else {
                printf("FAILED. Key found at incorrect index %d\n", index);
                result = -1;
            }
        } else {
            printf("FAILED. Not found.\n");
            result = -1;
        }

        printf("    Key Not Found Test:   ");
        if (argv_find(sizeof(g_Test0) / sizeof(g_Test0[0]), g_Test0, "nokey", &index) == 0) {
            printf("FAILED. Key found at index %d\n", index);
            result = -1;
        } else {
            printf("PASSED.\n");
        }
    }
    printf("\n");

    /* argv_get(). */
    printf("argv_get()\n");
    {
        const char* value;

        printf("    Value Found Test:     ");
        value = argv_get(sizeof(g_Test1) / sizeof(g_Test1[0]), g_Test1, "--key");
        if (value != NULL) {
            if (argv_strcmp(value, "value") == 0) {
                printf("PASSED.\n");
            } else {
                printf("FAILED. Incorrect value found: \"%s\"\n", value);
                result = -1;
            }
        } else {
            printf("FAILED. Key not found.\n");
            result = -1;
        }

        printf("    Value Not Found Test: ");
        value = argv_get(sizeof(g_Test1) / sizeof(g_Test1[0]), g_Test1, "--nokey");
        if (value != NULL) {
            printf("FAILED. Unexpected value found.\n");
        } else {
            printf("PASSED.\n");
        }
    }
    printf("\n");

    /* argv_from_WinMain() */
    printf("argv_from_WinMain()\n");
    {
        int argc;
        char** argv;

        printf("    Test 1:               ");
        if (argv_from_WinMain(g_Test2_WinMain, &argc, &argv, NULL) == 0) {
            int expectedCount = sizeof(g_Test2) / sizeof(g_Test2[0]);
            if (argc != sizeof(g_Test2) / sizeof(g_Test2[0])) {
                printf("FAILED. Incorrect count. Expecting %d; parsed %d\n", expectedCount, argc);
            } else {
                int passed = 1;
                int iarg;
                for (iarg = 1; iarg < expectedCount; iarg += 1) {   /* Start from the second parameter because argv[0] is always the application name. */
                    if (argv_strcmp(argv[iarg], g_Test2[iarg]) != 0) {
                        passed = 0;
                        break;
                    }
                }

                if (passed) {
                    printf("PASSED.\n");
                } else {
                    printf("FAILED.\n");
                }
            }

            argv_free(argv, NULL);
        } else {
            printf("FAILED. Failed to parse.\n");
        }
    }

    return result;
}


#if defined(_WIN32)
#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int result;
    int argc;
    char** argv;

    (void)hInstance;
    (void)hPrevInstance;
    (void)nCmdShow;

    printf("WinMain lpCmdLine = %s\n", lpCmdLine);

    argv_from_WinMain(lpCmdLine, &argc, &argv, NULL);
    {
        result = main(argc, argv);
    }
    argv_free(argv, NULL);

    return result;
}
#endif