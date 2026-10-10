#include "XmlToCpp.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: XmlCompile <input.xml> <output.cpp>\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Failed to read " << argv[1] << "\n";
        return 1;
    }
    const std::string xml((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    try {
        Robo::XmlToCpp compiler;
        const std::string cpp = compiler.Convert(xml);
        std::ofstream output(argv[2], std::ios::binary);
        if (!output) {
            std::cerr << "Failed to write " << argv[2] << "\n";
            return 1;
        }
        output << cpp;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << "\n";
        return 1;
    }
    return 0;
}
