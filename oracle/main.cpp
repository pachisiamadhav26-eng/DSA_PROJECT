#include "replay.hpp"

#include <fstream>
#include <iostream>

void print_answers(std::istream& input) {
    oracle::Session session(std::make_unique<oracle::SortedArray>());
    std::string line;
    while (std::getline(input, line)) {
        oracle::Command command = oracle::parse_command(line);
        std::string answer = session.execute(command);
        if (!answer.empty()) {
            std::cout << answer << '\n';
        }
    }
}

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: oracle [trace-file]\n";
        return 2;
    }

    try {
        if (argc == 1) {
            print_answers(std::cin);
        } else {
            std::ifstream trace_file(argv[1]);
            if (!trace_file) {
                std::cerr << "Cannot open trace\n";
                return 2;
            }
            print_answers(trace_file);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
