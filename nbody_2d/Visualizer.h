#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <string>

#include "NBodySystem.h"

class Visualizer {
public:
    static bool dumpPositionsCSV(const NBodySystem& system, const std::string& output_file);
};

#endif