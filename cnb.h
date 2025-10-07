#ifndef CNB_H
#define CNB_H

#ifndef WHISKER_ASSERT
#include <assert.h>
#define WHISKER_ASSERT assert
#endif // WHISKER_ASSERT

#ifndef WHISKER_ALLOC
#include <stdlib.h>
#define WHISKER_ALLOC malloc
#endif // WHISKER_ALLOC

#ifndef WHISKER_FREE
#include <stdlib.h>
#define WHISKER_FREE free
#endif // WHISKER_FREE

#ifndef WHISKER_REALLOC
#include <stdlib.h>
#define WHISKER_REALLOC realloc 
#endif // WHISKER_REALLOC

#ifdef WHISKER_NOPREFIX
    #define Cmd Whisker_Cmd
    #define cmd_append whisker_cmd_append
    #define cmd_append_args whisker_cmd_append_args
    #define cmd_execute whisker_cmd_execute
    #define cmd_execute_async whisker_cmd_execute_async
    #define cmd_reset whisker_cmd_reset
    #define cmd_destroy whisker_cmd_destroy
    #define should_rebuild whisker_should_rebuild
    #define rebuild_build whisker_rebuild_build
    #define build_project whisker_build_project
    #define BUILD_SOURCES WHISKER_BUILD_SOURCES
    #define BUILD_FLAGS WHISKER_BUILD_FLAGS
    #define COLOR_RESET WHISKER_COLOR_RESET
    #define COLOR_GREEN WHISKER_COLOR_GREEN
    #define COLOR_YELLOW WHISKER_COLOR_YELLOW
    #define COLOR_RED WHISKER_COLOR_RED
#endif

#define WHISKER_COLOR_RESET   "\033[0m"
#define WHISKER_COLOR_GREEN   "\033[32m"
#define WHISKER_COLOR_YELLOW  "\033[33m"
#define WHISKER_COLOR_RED     "\033[31m"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define WHISKER_BUILD_SOURCES(...) \
    .sources = (const char*[]) { __VA_ARGS__ }, \
    .source_count = (sizeof((const char*[]) { __VA_ARGS__}) / sizeof(const char*))

#define WHISKER_BUILD_FLAGS(...) \
    .flags = (const char*[]) { __VA_ARGS__ }, \
    .flag_count = (sizeof((const char*[]) { __VA_ARGS__}) / sizeof(const char*))

typedef struct {
    const char* compiler;
    const char** sources;
    const size_t source_count;
    const char** flags;
    const size_t flag_count;
    const char* build_dir;
    const char* output_dir;
    const char* output;
    const size_t parallel_jobs;
} Whikser_Config;

#define WHISKER_CMD_DEFAULT_CAP 16

typedef struct {
    const char** items;
    size_t count;
    size_t capacity;
} Whisker_Cmd;

#define whisker_cmd_append_args(cmd, ...) \
    whisker_cmd_append_args_impl(cmd, \
            ((const char*[]){ __VA_ARGS__ }), \
            (sizeof((const char*[]){ __VA_ARGS__ }) / sizeof(const char*))) 

static void whisker_cmd_destroy(Whisker_Cmd* cmd) {
    WHISKER_ASSERT(cmd && "cmd cannot be null");

    WHISKER_FREE(cmd -> items);
    cmd -> items = NULL;
    cmd -> count = 0;
    cmd -> capacity= 0;
}

// static void whisker_cmd_reset(Whisker_Cmd* cmd) {
//     WHISKER_ASSERT(cmd && "cmd cannot be null");
//
//     cmd -> count = 0;
//     cmd -> capacity = WHISKER_CMD_DEFAULT_CAP;
//     WHISKER_FREE(cmd -> items);
//
//     cmd -> items = (const char**) WHISKER_ALLOC(sizeof(const char*) * cmd -> capacity);
//     WHISKER_ASSERT(cmd -> items && "alloc failed, buy more ram :p");
// }

static void whisker_cmd_append(Whisker_Cmd* cmd, const char* arg) {
    WHISKER_ASSERT(cmd && "cmd cannot be null");

    if (cmd -> count >= cmd -> capacity) {
        const size_t new_capacity = cmd -> capacity == 0 ? WHISKER_CMD_DEFAULT_CAP : cmd -> capacity * 2;
        WHISKER_ASSERT(new_capacity > cmd -> capacity || new_capacity < (SIZE_MAX / sizeof(const char*))); 

        const char** new_items = (const char**) WHISKER_REALLOC(cmd -> items, sizeof(const char*) * new_capacity);
        WHISKER_ASSERT(new_items && "alloc failed, buy more ram :p");

        cmd -> items = new_items;
        cmd -> capacity = new_capacity;
    }

    cmd -> items[cmd -> count++] = arg;
}

static void whisker_cmd_append_args_impl(Whisker_Cmd* cmd, const char** args, const size_t n) {
    WHISKER_ASSERT(cmd && "cmd cannot be null");
    WHISKER_ASSERT(args && "args cannot be null");

    for (size_t i = 0; i < n; i++) {
        whisker_cmd_append(cmd, args[i]);
    }
}  

static bool whisker_cmd_execute(Whisker_Cmd* cmd) {
    WHISKER_ASSERT(cmd && "cmd cannot be null");
    WHISKER_ASSERT(cmd -> count > 0 && "no arguments");
    
    if (cmd -> items[cmd -> count - 1] != NULL) {
        whisker_cmd_append(cmd, NULL);
    }

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        execvp(cmd -> items[0], (char* const*) cmd -> items);
        fprintf(stderr, "execvp failed! %s\n", cmd -> items[0]);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static pid_t whisker_cmd_execute_async(Whisker_Cmd* cmd) {
    WHISKER_ASSERT(cmd && "cmd cannot be null");
    WHISKER_ASSERT(cmd -> count > 0 && "no arguments");
    
    if (cmd -> items[cmd -> count - 1] != NULL) {
        whisker_cmd_append(cmd, NULL);
    }

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        execvp(cmd -> items[0], (char* const*) cmd -> items);
        fprintf(stderr, "execvp failed! %s\n", cmd -> items[0]);
        _exit(127);
    }

    return pid;
}

static bool whisker_cmd_wait(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool whisker_should_rebuild(const char* src, const char* bin) {
    struct stat src_stat;
    struct stat bin_stat;
    if (stat(src, &src_stat) < 0) return false;
    if (stat(bin, &bin_stat) < 0) return true;
    return src_stat.st_mtime > bin_stat.st_mtime;
}

static bool whisker_rebuild_build(const char* path, const char* argv[]) {
    if (!whisker_should_rebuild(path, argv[0])) return false;

    Whisker_Cmd cmd = {0};
    whisker_cmd_append(&cmd, "clang");
    whisker_cmd_append(&cmd, "-Wall");
    whisker_cmd_append(&cmd, "-O2");
    whisker_cmd_append(&cmd, "-flto");
    whisker_cmd_append(&cmd, "-march=native");
    whisker_cmd_append(&cmd, path);
    whisker_cmd_append(&cmd, "-o");
    whisker_cmd_append(&cmd, argv[0]);

    if (!whisker_cmd_execute(&cmd)) {
        whisker_cmd_destroy(&cmd);
        fprintf(stderr, "Build failed\n");
        exit(1);
    }

    whisker_cmd_destroy(&cmd);

    setenv("CNB_REBUILT", "1", 1);

    execvp(argv[0], (char* const*) argv);
    fprintf(stderr, "Failed to reload\n");
    exit(1);
}

static void whisker_make_object_path(char* obj_path, const char* dir, const char* src) {
    const char* name = strrchr(src, '/');
    name = name ? name + 1 : src;
    
    const char* ext = strrchr(name, '.');
    const size_t dir_len = strlen(dir);
    const size_t name_len = (size_t)(ext - name);

    strncpy(obj_path, dir, dir_len);
    strncpy(obj_path + dir_len, name, name_len);
    strcpy(obj_path + dir_len + name_len, ".o");
}

static bool whisker_build_project(Whikser_Config* cfg, bool rebuilt) {
    WHISKER_ASSERT(cfg && "cfg is null");
    WHISKER_ASSERT(cfg -> sources && "sources are null, consider adding sources");

    Whisker_Cmd link_cmd = {0};
    whisker_cmd_append(&link_cmd, cfg -> compiler);
    whisker_cmd_append_args_impl(&link_cmd, cfg -> flags, cfg -> flag_count);

    size_t job_count = cfg -> parallel_jobs == 0 ? cfg -> source_count : cfg -> parallel_jobs;
    job_count = job_count > cfg -> source_count ? cfg -> source_count : job_count;
    pid_t pids[job_count];

    size_t compiled = 0;
    size_t unchanged = 0;
    size_t index = 0;

    char** obj_paths = (char**) WHISKER_ALLOC(sizeof(char*) * cfg -> source_count);

    while (index < cfg -> source_count) {
        size_t batch_size = (cfg -> source_count - index > job_count) ? job_count : (cfg -> source_count - index); 
        size_t active = 0;

        for (size_t i = 0; i < batch_size; i++) {
            const char* source = cfg -> sources[index];
            obj_paths[index] = (char*) WHISKER_ALLOC(256);
            whisker_make_object_path(obj_paths[index], cfg -> build_dir, source);

            if (!whisker_should_rebuild(source, obj_paths[index]) && !rebuilt) {
                whisker_cmd_append(&link_cmd, obj_paths[index]);

                index++;
                compiled++;
                unchanged++;
                continue;
            }

            Whisker_Cmd cmd = {0};
            whisker_cmd_append(&cmd, cfg -> compiler); 
            whisker_cmd_append_args_impl(&cmd, cfg -> flags, cfg -> flag_count); 
            whisker_cmd_append(&cmd, "-c");
            whisker_cmd_append(&cmd, source);
            whisker_cmd_append(&cmd, "-o");
            whisker_cmd_append(&cmd, obj_paths[index]);

            pids[active] = whisker_cmd_execute_async(&cmd);
            whisker_cmd_destroy(&cmd);
            whisker_cmd_append(&link_cmd, obj_paths[index]);

            index++;
            active++;
        }

        for (size_t k = 0; k < active; k++) {
            if (!whisker_cmd_wait(pids[k])) {
                fprintf(stderr, "\nBuild" WHISKER_COLOR_RED " \033[1mfailed" WHISKER_COLOR_RESET"\n");
                return false;
            }

            compiled++;
        }
    }

    if (unchanged == compiled) {
        for (size_t i = 0; i < cfg -> source_count; i++) {
            WHISKER_FREE(obj_paths[i]);
        }
        WHISKER_FREE(obj_paths);

        whisker_cmd_destroy(&link_cmd);
        printf("Nothing to do, sorry if it was header, .h support is coming :/\n");
        exit(0);
    }

    whisker_cmd_append(&link_cmd, "-o");
    whisker_cmd_append(&link_cmd, cfg -> output);

    bool result = whisker_cmd_execute(&link_cmd);

    for (size_t i = 0; i < cfg -> source_count; i++) {
        WHISKER_FREE(obj_paths[i]);
    }
    WHISKER_FREE(obj_paths);

    whisker_cmd_destroy(&link_cmd);
    return result;
}

#endif /* ifndef CNB_H */
