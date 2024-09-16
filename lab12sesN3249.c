#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>

#define __USE_XOPEN = 500
#include <sys/stat.h>
#include <sys/types.h>

#define __USE_XOPEN_EXTENDED
#include <ftw.h>

#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <dirent.h>

#include "plugin_api.h"

// #define LAB1DEBUG

// булевы операторы во флагах
int operator[3] = {1, 0, 0};
// количество полученных плагинов
int number_of_plugins = 0;
// количество подключенных операторов из плагинов (всех)
int number_of_opts = 0;
// массив количеств испольщованных операторов (для каждого плагина)
int* number_of_opts_per_plugin = NULL;

char* plugin_dir = NULL;

// массив указателей на process_file() из плагинов
int(**process_funcs)(const char *fname,
        struct option in_opts[],
        size_t in_opts_len) = NULL;

// массив plugin_info
struct plugin_info* plugin_info_arr = NULL;
// двумерный массив использованных операторов (ряд - номер плагина)
struct option** used_opts = NULL;
// все подключенные операторы
struct option* opts = NULL;

struct option base_opts[] = {
    {"and", no_argument, NULL, 'A'},
    {"or", no_argument, NULL, 'O'},
    {"not", no_argument, NULL, 'N'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {"P", required_argument, NULL, 'P'}
};

void** libs = NULL;

int hv[2] = {0, 0};

const int BASE_OPTS_NUM = 6;

struct pair {
    int first;
    int second;
};

struct pair* opts_index_range = NULL;

struct pair reserved_for_base_opts;

void print_help() {
    printf("Usage:\n");
    printf("./lab12sesN3249 [opts] [search_directory]\n");
    printf("---------------OPTIONS---------------\n");
    printf("default opts:\n");
    printf("-A --and: \t logical and for plugins\n");
    printf("-N --not: \t logical not for plugins\n");
    printf("-O --or: \t logical or for plugins\n");
    printf("-h --help: \t open this menu\n");
    printf("-v --version: \t version info\n");
    printf("-P [dir]: \t set plugin source to [dir]\n");
    printf("---------------PLUGINS---------------\n");

    printf("connected plugins and their options:\n");
    for(int i = 0; i < number_of_plugins; i++) {
        printf("plugin purpose: %s\n", plugin_info_arr[i].plugin_purpose);
        for(int j = 0; j < (int)plugin_info_arr[i].sup_opts_len; j++) {
            printf("--%s: %s\n", plugin_info_arr[i].sup_opts[j].opt.name, plugin_info_arr[i].sup_opts[j].opt_descr);
        }
        printf("plugin was made by %s\n", plugin_info_arr[i].plugin_author);
        if(i + 1 < number_of_plugins) {
            printf("------------------------------------------\n\n");
        }
    }
}

void print_ver() {
    printf("Version 1.0\nAuthor: Stanbekov Sultan Ernisovich\nGroup: N3249\nVariant: 10\n"); 
}

int is_valid_file_path(const char *path) {
    if (access(path, F_OK) != -1) {
        return 1;
    } else {
        return 0;
    }
}

int search_opt(struct pair* array, int idx) {
    for(int i = 0; i < number_of_plugins; i++) {
        if(idx >= array[i].first && idx <= array[i].second) {
            return i;
        }
    }
    return -1;
}

void free_mem() {
    if(libs) {
        for(int i = 0; i < number_of_plugins; i++) {
            dlclose(libs[i]);
        }
        free(libs);
    }
    if(used_opts) {
        for(int i = 0; i < number_of_plugins; i++) {
            free(used_opts[i]);
        }
        free(used_opts);
    }
    if(plugin_info_arr) free(plugin_info_arr);
    if(process_funcs) free(process_funcs);
    if(opts) free(opts);
    if(opts_index_range) free(opts_index_range);
    if(number_of_opts_per_plugin) free(number_of_opts_per_plugin);
    // if(plugin_dir) free(plugin_dir);
}

bool is_shared_object(char* fpath) {
    int len = strlen(fpath);
    if(len < 4) return false;
    return (fpath[len - 1] == 'o' && fpath[len - 2] == 's' && fpath[len - 3] == '.');
}

void get_plugin_dir(int argc, char** argv) {
    plugin_dir = malloc(256);
    char* ret = getcwd(plugin_dir, 256);
    if(!ret) {
        fprintf(stderr, "get_plugin_dir: getcwd error\n");
        free_mem();
        exit(-1);
    }

    for(int i = 1; i < argc; i++) {
        if(!strcmp("-P", argv[i])) {
            if(!argv[i + 1]) {
                fprintf(stderr, "Usage of -P: ./lab1sesN3249 -P [dir] [catalogue]\n");
                free_mem();
                exit(-1);
            }
            memcpy(plugin_dir, argv[i + 1], 256);
            break;
        }
    } 
}

void connect_libs() {
    struct dirent *dir;
    DIR* d = opendir(plugin_dir);

    if(!d) {
        fprintf(stderr, "Error opening dir %s\n", plugin_dir);
        free(plugin_dir);
        free_mem();
        exit(-1);
    }

    int ptr = 0;

    while ((dir = readdir(d)) != NULL) {
        if(dir->d_type != DT_REG) continue;

        char* buf = malloc(256);
        memcpy(buf, plugin_dir, 256);
        strcat(buf, "/");
        strcat(buf, dir->d_name);
        char* extension = strrchr(buf, '.');

        if(extension && !strcmp(extension, ".so")) {
            libs = realloc(libs, (number_of_plugins + 1) * sizeof(void*));
            libs[number_of_plugins] = dlopen(buf, RTLD_NOW);
            number_of_plugins++;

            #ifdef LAB1DEBUG
            printf("connected %s (plugin#%d)\n", buf, ptr);
            #endif

            ptr++;
        }

        free(buf);
    }
    #ifdef LAB1DEBUG
    printf("\n");
    #endif

    closedir(d);

    if(number_of_plugins == 0) {
        free_mem();
        free(plugin_dir);
        printf("0 plugins found in %s\nEXIT\n", plugin_dir);
        exit(0);
    }
    free(plugin_dir);
}

void read_plugin_funcs() {
    process_funcs = malloc(number_of_plugins * sizeof(void*));
    plugin_info_arr = malloc(number_of_plugins * sizeof(struct plugin_info));
    number_of_opts_per_plugin = calloc(number_of_plugins, sizeof(int));
    used_opts = malloc(number_of_plugins * sizeof(struct option*));

    for(int i = 0; i < number_of_plugins; i++) {
        int (*process)(const char *fname, struct option in_opts[], size_t in_opts_len) = dlsym(libs[i], "plugin_process_file");
        if(!process) {
            fprintf(stderr, "dlsym error\n");
            free_mem();
            exit(-1);
        }

        process_funcs[i] = process;

        int (*info)(struct plugin_info* ppi) = dlsym(libs[i], "plugin_get_info");
        if(!info) {
            fprintf(stderr, "dlsym error\n");
            free_mem();
            exit(-1);
        }

        struct plugin_info* ppi = malloc(sizeof(struct plugin_info));
        int ret = info(ppi);

        if(ret == -1) {
            free_mem();
            exit(-1);
        }

        number_of_opts += ppi->sup_opts_len;
        plugin_info_arr[i] = *ppi;

        #ifdef LAB1DEBUG
        printf("plugin#%d funcs are successfully connected\n\n", i);
        #endif

        free(ppi);
    }
    if(number_of_opts == 0) {
        fprintf(stderr, "plugins have 0 opts\nError\n");
        free_mem();
        exit(-1);
    }
}

void get_opts() {
    opts = malloc(number_of_opts * sizeof(struct option));
    opts_index_range = malloc(number_of_plugins * sizeof(struct pair));

    int ptr = 0;

    for(int i = 0; i < number_of_plugins; i++) {
        if(plugin_info_arr[i].sup_opts_len == 0) continue;

        for(int j = 0; j < (int)plugin_info_arr[i].sup_opts_len; j++) {
            opts[ptr] = plugin_info_arr[i].sup_opts[j].opt;
            opts[ptr].val = 0;

            #ifdef LAB1DEBUG
            printf("opt %s is connected\n", opts[ptr].name);
            #endif

            ptr++;
        }

        opts_index_range[i].first = ptr - plugin_info_arr[i].sup_opts_len;
        opts_index_range[i].second = ptr - 1;

        #ifdef LAB1DEBUG
        printf("plugin#%d opts indices are in [%d, %d]\n\n", i, opts_index_range[i].first, opts_index_range[i].second);
        #endif
    }
}

void add_base_opts() {
    if(BASE_OPTS_NUM == 0) return;

    opts = realloc(opts, (number_of_opts + BASE_OPTS_NUM) * sizeof(struct option));

    memcpy(opts + number_of_opts, base_opts, BASE_OPTS_NUM * sizeof(struct option));
    number_of_opts += BASE_OPTS_NUM;

    reserved_for_base_opts.first = number_of_opts - BASE_OPTS_NUM;
    reserved_for_base_opts.second = number_of_opts - 1;
}

void allocate_opts_matrix(int argc, char** argv) {
    optind = 1;
    int opt_idx = -1, longind = 1;

    while((opt_idx = getopt_long_only(argc, argv, "AONP:hv", opts, &longind)) != -1) {
        if(opt_idx == 'A' || opt_idx == 'P' || opt_idx == 'N' || opt_idx == 'O' || opt_idx == 'h' || opt_idx == 'v') {
            continue;
        }
        if(longind >= reserved_for_base_opts.first && longind <= reserved_for_base_opts.second) continue;

        int plugin_idx = search_opt(opts_index_range, longind);
        if(plugin_idx == -1) {
            fprintf(stderr, "Unknown opt\n");
            free_mem();
            exit(-1);
        }
        number_of_opts_per_plugin[plugin_idx]++;
    }

    for(int i = 0; i < number_of_plugins; i++) {
        used_opts[i] = malloc(number_of_opts_per_plugin[i] * sizeof(struct option));
    }
}

void process_opts(int argc, char** argv) {
    optind = 1;
    int opt = -1, longind = 1, process_and = 0;
    int* used_opts_ptr = calloc(number_of_plugins, sizeof(int));
    int base_opt = 0;

    while((opt = getopt_long_only(argc, argv, "AONP:hv", opts, &longind)) != -1) {
        switch (opt)
        {
        case 'P':
            base_opt = 1;
            break;

        case 'A':
            #ifdef LAB1DEBUG
            printf("processing -A opt\n");
            #endif
            operator[0] = 1;
            if(operator[1]) {
                fprintf(stderr, "Conflicting opts -A and -O\n");
                free(used_opts_ptr);
                free_mem();
                exit(-1);
            }
            process_and = 1;
            base_opt = 1;
            break;

        case 'O':
            #ifdef LAB1DEBUG
            printf("processing -O opt\n");
            #endif
            operator[1] = 1;
            if(process_and) {
                fprintf(stderr, "Conflicting opts -A and -O\n");
                free(used_opts_ptr);
                free_mem();
                exit(-1);
            }
            operator[0] = 0;
            base_opt = 1;
            break;

        case 'N':
            #ifdef LAB1DEBUG
            printf("processing -N opt\n");
            #endif
            operator[2] = 1;
            base_opt = 1;
            break;

        case 'h':
            #ifdef LAB1DEBUG
            printf("processing -h opt\n");
            #endif
            print_help();
            hv[0] = 1;
            base_opt = 1;
            break;

        case 'v':
            #ifdef LAB1DEBUG
            printf("processing -v opt\n\n");
            #endif
            print_ver();
            hv[1] = 1;
            base_opt = 1;
            break;
        
        default:
            break;
        }

        if((longind >= reserved_for_base_opts.first && longind <= reserved_for_base_opts.second) || base_opt) {
            base_opt = 0;
            continue;
        }

        int plugin_idx = search_opt(opts_index_range, longind);
        if(plugin_idx == -1) {
            fprintf(stderr, "Unknown opt\n");
            free(used_opts_ptr);
            free_mem();
            exit(-1);
        }

        struct option buf = opts[longind] = opts[longind];

        if(opts[longind].has_arg == required_argument) {
            if(optarg == NULL) {
                fprintf(stderr, "Missing argument: %s\n", opts[longind].name);
                free(used_opts_ptr);
                free_mem();
                exit(-1);
            }
            buf.flag = (int*)optarg;
        }
        if(opts[longind].has_arg == optional_argument) {
            if(optarg == NULL) continue;
            buf.flag = (int*)optarg;
        }

        used_opts[plugin_idx][used_opts_ptr[plugin_idx]] = buf;
        used_opts_ptr[plugin_idx]++;
    }

    free(used_opts_ptr);
}

int process_file(const char* fpath) {
    int ans = 1;
    int* rets = calloc(number_of_plugins, sizeof(int));
    int buf = -1;

    for(int i = 0; i < number_of_plugins; i++) {
        if(number_of_opts_per_plugin[i] > 0) {
            buf = process_funcs[i](fpath, used_opts[i], (size_t)number_of_opts_per_plugin[i]);
        } else {
            rets[i] = 1;
            continue;
        }
        if(buf == -1) {
            free_mem();
            exit(-1);
        }
        rets[i] = !buf;
    }

    #ifdef LAB1DEBUG
    printf("%s, ret value == %d\n", fpath, buf);
    #endif

    if(operator[0]) {
        for(int i = 0; i < number_of_plugins; i++) {
            ans &= rets[i];
        }
    }
    if(operator[1]) {
        for(int i = 0; i < number_of_plugins; i++) {
            ans |= rets[i];
        }
    }
    if(operator[2]) {
        ans = !ans;
    }
    free(rets);
    return ans;
}

int ftw_func(const char *fpath, const struct stat *sb, int typeflag, __attribute__((unused)) struct FTW *ftwbuf) {
    if (typeflag == FTW_F && ((sb->st_mode & S_IRWXU) & S_IRUSR) && ((sb->st_mode & S_IFMT) & S_IFREG)
        && ((sb->st_mode & S_IRWXG) & S_IRGRP) && ((sb->st_mode & S_IRWXO) & S_IROTH)) {
        int ans = process_file(fpath);
        if(ans) {
            printf("found %s\n", fpath);
        }
    } else {
        #ifdef LAB1DEBUG
        printf("skip %s\n", fpath);
        #endif
    }
    return 0;   
}

int main(int argc, char** argv) {
    if(argc < 2) {
        fprintf(stderr, "missing arguments. Use -h for help\n");
        free_mem();
        exit(-1);
        return -1;
    }
    get_plugin_dir(argc, argv);

    connect_libs();
    if(number_of_plugins > 0) {
        read_plugin_funcs();

        get_opts();

        add_base_opts();

        allocate_opts_matrix(argc, argv);
    }

    process_opts(argc, argv);

    if(!is_valid_file_path(argv[argc - 1])) {
        if(hv[0] || hv[1]) {
            free_mem();
            exit(0);
            return 0;
        }
        fprintf(stderr, "expected directory. Use -h for help\n");
        free_mem();
        exit(-1);
        return -1;
    }

    nftw(argv[argc - 1], ftw_func, 20, FTW_DEPTH || FTW_PHYS);
    
    free_mem();
    return 0;
}