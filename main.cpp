#include <future>
#include <spdlog/spdlog.h>

#include "cl/Program.hpp"
#include "font/text.hpp"

int RunOpenCL(int argc, char **argv);

int main(int argc, char **argv) {
  if (argc < 1) {
    spdlog::error("Not enough arguments");
    return EXIT_FAILURE;
  }

  ProgramCache sha;

  try {
    const char *text = (argc >= 3) ? argv[2] : "Hello beautiful, BGL!";
    const auto path = std::filesystem::path(argv[0]).parent_path() / "out.pgm";

    auto task1 = std::async(std::launch::async, RunFreetype, path, text);
    auto task2 = std::async(std::launch::async, RunOpenCL, argc, argv);

    task1.get();
    task2.get();
  } catch (const std::exception &e) {
    spdlog::error("Exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
