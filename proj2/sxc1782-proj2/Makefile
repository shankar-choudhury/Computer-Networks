CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
TARGET = proj2

all: $(TARGET)

$(TARGET): proj2.cpp forwarding_table.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) proj2.cpp forwarding_table.cpp

clean:
	rm -f $(TARGET) *.o