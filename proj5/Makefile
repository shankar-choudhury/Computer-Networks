CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
TARGETS = proj5 proj5d

all: $(TARGETS)

proj5: proj5.cpp bbp.hpp
	$(CXX) $(CXXFLAGS) -o proj5 proj5.cpp

proj5d: proj5d.cpp bbp.hpp
	$(CXX) $(CXXFLAGS) -o proj5d proj5d.cpp

clean:
	rm -f $(TARGETS) *.o
