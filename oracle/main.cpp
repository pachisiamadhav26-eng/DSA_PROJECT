#include "replay.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    if (argc>2) { std::cerr<<"Usage: oracle [trace-file]\n"; return 2; }
    std::ifstream file;
    if (argc==2) { file.open(argv[1]); if (!file) {std::cerr<<"Cannot open trace\n";return 2;} }
    std::istream& in=argc==2 ? file : std::cin;
    oracle::Session session([](){return std::make_unique<oracle::SortedArray>();});
    std::string line;
    try { while (std::getline(in,line)) std::cout<<session.execute(oracle::parse(line))<<'\n'; }
    catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
