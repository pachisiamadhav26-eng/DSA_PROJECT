#include "replay.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "usage: oracle [trace-file]\n";
        return 2;
    }

    std::ifstream file;
    if (argc == 2) {
        std::error_code error;
        if (std::filesystem::is_directory(argv[1], error)) {
            std::cerr << "cannot read trace: path is a directory\n";
            return 2;
        }
        file.open(argv[1]);
        if (!file) {
            std::cerr << "cannot open trace file\n";
            return 2;
        }
    }

    std::istream& in = argc == 2 ? file : std::cin;
    std::string line;
    try {
        oracle::Session session([] { return std::make_unique<oracle::SortedArray>(); });
        while (std::getline(in, line))
            std::cout << session.execute(oracle::parse(line)) << '\n';
        if (in.bad() || !in.eof()) throw std::runtime_error("cannot read trace");
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("cannot write oracle output");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
