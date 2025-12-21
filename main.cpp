#include <future>
#include <spdlog/spdlog.h>

#include "cl/Program.hpp"

int RunOpenCL(int argc, char **argv);
int RunFreetype(int argc, char **argv);


int main(int argc, char **argv)
{
    if (argc < 1) {
        spdlog::error("Not enough arguments");
        return EXIT_FAILURE;
    }

    ProgramCache sha;

    try {
        auto task1 = std::async(std::launch::async, RunFreetype, argc, argv);
        auto task2 = std::async(std::launch::async, RunOpenCL, argc, argv);

        task1.get();
        task2.get();
    } catch (const std::exception &e) {
        spdlog::error("Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
