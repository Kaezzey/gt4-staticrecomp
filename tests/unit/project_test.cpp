#include "gt4recomp/project.hpp"

#include <iostream>

int main() {
    if (gt4recomp::project_name() != "GT4Recomp") {
        std::cerr << "Unexpected project identity\n";
        return 1;
    }
    return 0;
}
