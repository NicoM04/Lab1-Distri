#include "CudaBuffer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

int main() {
    const std::vector<double> input{1.5, -2.0, 3.25, 4.5, -7.75};
    std::vector<double> output(input.size(), 0.0);

    CudaBuffer<double>::resetTransferStatistics();

    CudaBuffer<double> buffer(input.size());
    if (buffer.size() != input.size()) {
        std::cerr << "FAIL: size() no coincide con el tamano reservado\n";
        return EXIT_FAILURE;
    }

    buffer.copyToDevice(input.data(), input.size());
    buffer.copyToHost(output.data(), output.size());

    if (!std::equal(input.begin(), input.end(), output.begin())) {
        std::cerr << "FAIL: round-trip H2D/D2H no coincide\n";
        return EXIT_FAILURE;
    }

    if (CudaBuffer<double>::hostToDeviceCopyCount() != 1U) {
        std::cerr << "FAIL: se esperaba 1 copia H2D\n";
        return EXIT_FAILURE;
    }

    if (CudaBuffer<double>::deviceToHostCopyCount() != 1U) {
        std::cerr << "FAIL: se esperaba 1 copia D2H\n";
        return EXIT_FAILURE;
    }

    std::cout << "PASS: CudaBuffer round-trip test\n";
    return EXIT_SUCCESS;
}
