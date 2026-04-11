#include "Visualizer.h"

#include <fstream>

bool Visualizer::dumpPositionsCSV(const NBodySystem& system, const std::string& output_file) {
    std::ofstream out(output_file);
    if (!out.is_open()) {
        return false;
    }

    out << "id,x,y,vx,vy,ax,ay\n";
    const std::vector<Particle>& bodies = system.bodies();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        out << i << ',' << p.getX() << ',' << p.getY() << ',' << p.getVx() << ',' << p.getVy() << ','
            << p.getAx() << ',' << p.getAy() << '\n';
    }

    return true;
}