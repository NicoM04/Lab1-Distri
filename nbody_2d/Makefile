CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

TARGET = nbody_2d
SOURCES = main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)