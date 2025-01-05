#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    std::ofstream output_file("output.txt", std::ios::app);
    if (!output_file) {
        return 1;
    }

    output_file << "-----START-----" << std::endl;

    for (int i = 0; i < argc; ++i) {
        output_file << argv[i] << " ";
    }
    output_file << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        output_file << line << std::endl;
    }

    output_file << "-----END-----" << std::endl;

    output_file.close();
    return 0;
}
