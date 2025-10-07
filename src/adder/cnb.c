#define WHISKER_NOPREFIX
#include "cnb.h"

#include <time.h>

int main(int argc, const char* argv[]) {
    bool rebuilt = getenv("CNB_REBUILT") != NULL;
    if (rebuilt) {
        unsetenv("CNB_REBUILT");
        printf("Build system was rebuilt, forcing full recompilation\n");
    } else {
        printf(COLOR_YELLOW "\nwhisker build system" COLOR_RESET "\n\n");
    }

    rebuild_build("cnb.c", argv);

    Whikser_Config config = {
        BUILD_SOURCES(
            "server/server.c",
        ),
        BUILD_FLAGS(
            "-O3",
            "-flto",
            "-march=native"
        ),
        .compiler = "clang",
        .build_dir = "build/",
        .output = "build/bin/server",
        .parallel_jobs = 12
    };

    Whikser_Config config2 = {
        BUILD_SOURCES(
            "client/client.c",
        ),
        BUILD_FLAGS(
            "-O3",
            "-flto",
            "-march=native"
        ),
        .compiler = "clang",
        .build_dir = "build/",
        .output = "build/bin/client",
        .parallel_jobs = 12
    };

    struct timespec t_start;
    struct timespec t_end;

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    bool result = build_project(&config, rebuilt);
    bool result2 = build_project(&config2, rebuilt);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    double start = t_start.tv_sec * 1000.0 + t_start.tv_nsec / 1000000.0;
    double end = t_end.tv_sec * 1000.0 + t_end.tv_nsec / 1000000.0;
    double elapsed = end  - start;

    if (result || result2) {
        printf("Build \033[1m" COLOR_GREEN "finished!\033[0m" COLOR_RESET "(took %.3fs)\n", elapsed / 1000.0);
        return 0;
    } else {
        return 1;
    }
}
