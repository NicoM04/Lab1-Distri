CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -fopenmp

TARGET = nbody_2d
SOURCES = main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TEST_TARGET = tests/test_acceleration
TEST_WEEK2_TARGET = tests/test_parallel_vs_serial
TEST_WEEK3_TARGET = tests/test_integration
TEST_SOURCES = tests/test_acceleration.cpp Particle.cpp NBodySystem.cpp
TEST_WEEK2_SOURCES = tests/test_parallel_vs_serial.cpp Particle.cpp NBodySystem.cpp
TEST_WEEK3_SOURCES = tests/test_integration.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_WEEK2_TARGET): $(TEST_WEEK2_SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_WEEK3_TARGET): $(TEST_WEEK3_SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_TARGET) $(TEST_WEEK2_TARGET) $(TEST_WEEK3_TARGET)
	./$(TEST_TARGET)
	./$(TEST_WEEK2_TARGET)
	./$(TEST_WEEK3_TARGET)
clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_TARGET) $(TEST_WEEK2_TARGET)