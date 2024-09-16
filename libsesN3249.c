#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define __USE_XOPEN
#include <sys/stat.h>
#include <sys/types.h>

#define __USE_XOPEN_EXTENDED
#include <ftw.h>

#include "plugin_api.h"

/*
0 - equal(eq)
1 - not equal(ne)
2 - greater than(gt)
3 - less than(lt)
4 - greater or equal(ge)
5 - less or equal(le)
*/

// #define LAB1DEBUG

char* lib_name = "lines-count";

int to_int(char* str, int* err) {
    int ptr = 0, ans = 0;
    while(str[ptr] != '\0') {
        if(str[ptr] >= '0' && str[ptr] <= '9') {
            ans *= 10;
            ans += (str[ptr] - '0');
        } else {
            *err = 1;
            return 0;
        }
        ptr++;
    }
    *err = 0;
    return ans;
}

struct plugin_option opt_arr[2] = {
    {
        {"lines-count", required_argument, NULL, 0},
        "--lines-count [value] to find files with exact [value] of lines",
    },
    {
        {"lines-count-comp", required_argument, NULL, 0},
        "--lines-count-comp option lets you compare number of lines in file with some [value] using comparator code",
    },
};

int plugin_get_info(struct plugin_info* ppi) {
    if(!ppi) {
        fprintf(stderr, "ERROR: plugin_get_info: invalid argument\n");
        return -1;
    }

    ppi->sup_opts = opt_arr;
    ppi->sup_opts_len = 2;
    ppi->plugin_author = "Stanbekov Sultan Ernisovich, N3249";
    ppi->plugin_purpose = "This plugin lets you find files in directory tree with exact number of lines or, alternatively, you can compare it to some value";
    return 0;   
}

int plugin_process_file(const char *fname, struct option in_opts[], size_t in_opts_len) {
    if(in_opts_len <= 0 || in_opts == NULL || strlen(fname) == 0 || !fname) {
        fprintf(stderr, "ERROR: plugin_process_file: invalid argument\n");
        return -1;
    }

    int value = -1;
    int cmp = 0;
    int err = 1;
    
    value = to_int((char*)in_opts[0].flag, &err);
    if(err == 1) {
        fprintf(stderr, "%s: plugin_process_file: invalid value", lib_name);
        return -1;
    }

    if(in_opts_len == 2) {
        if(!strcmp((char*)in_opts[1].flag, "eq")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- ==\n");
            #endif
            cmp = 0;
        } else if(!strcmp((char*)in_opts[1].flag, "ne")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- !=\n");
            #endif
            cmp = 1;
        } else if(!strcmp((char*)in_opts[1].flag, "gt")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- >\n");
            #endif
            cmp = 2;
        } else if(!strcmp((char*)in_opts[1].flag, "lt")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- <\n");
            #endif
            cmp = 3;
        } else if(!strcmp((char*)in_opts[1].flag, "ge")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- >=\n");
            #endif
            cmp = 4;
        } else if(!strcmp((char*)in_opts[1].flag, "le")) {
            #ifdef LAB1DEBUG
            printf("cmp opt -- <=\n");
            #endif
            cmp = 5;
        } else {
            cmp = -1;
        }
    }

    if(cmp == -1) {
        fprintf(stderr, "ERROR: %s: invalid comparison arg\n", lib_name);
        return -1;
    }

    if(value == -1) {
        fprintf(stderr, "ERROR: invalid usage, use -h for help");
    }

    int lines_count = 0;

    FILE* fp = fopen(fname, "r");

    if(!fp) {
        fprintf(stderr, "ERROR: plugin_get_info: couldn't open file %s: %s\n", fname, strerror(errno)); 
        return -1;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
        lines_count++;
    }

    if(line) free(line);

    fclose(fp);

    #ifdef LAB1DEBUG
    printf("%s has %d line(-s)\n", fname, lines_count);
    #endif
    [[maybe_unused]] char* string = "0";
    int buf = 0;

    switch (cmp)
    {
    case 0:
        string = (lines_count == value) ? "accepted\n" : "not accepted\n";
        buf = (lines_count == value);
        break;
    case 1:
        string = (value != lines_count) ? "accepted\n" : "not accepted\n";
        buf = (value != lines_count);
        break;
    case 2:
        string = (lines_count > value) ? "accepted\n" : "not accepted\n";
        buf = (lines_count > value);
        break;
    case 3:
        string = (lines_count < value) ? "accepted\n" : "not accepted\n";
        buf = (lines_count < value);
        break;
    case 4:
        string = (lines_count >= value) ? "accepted\n" : "not accepted\n";
        buf = (lines_count >= value);
        break;
    case 5:
        string = (lines_count <= value) ? "accepted\n" : "not accepted\n";
        buf = (lines_count <= value);
        break;
    
    default:
        break;
    }

    #ifdef LAB1DEBUG
    printf("%s\n", string);
    #endif

    if(string[0] == '0') {
        fprintf(stderr, "ERROR: something went wrong\n"); 
        return -1;
    }

    return !buf;
}