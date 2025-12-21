#include <future>
#include <spdlog/spdlog.h>

#include "cl/Program.hpp"

int RunOpenCL();
int RunFreetype(int argc, char **argv);


int main(int argc, char **argv)
{
    ProgramCache sha;

    try {
        auto task1 = std::async(std::launch::async, RunFreetype, argc, argv);
        auto task2 = std::async(std::launch::async, RunOpenCL);

        task1.get();
        task2.get();
    } catch (const std::exception &e) {
        spdlog::error("Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
