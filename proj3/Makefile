CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
TARGET = proj3

all: $(TARGET)

$(TARGET): proj3.cpp Packet.cpp FlowTracker.cpp RTTTracker.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) proj3.cpp Packet.cpp FlowTracker.cpp RTTTracker.cpp

clean:
	rm -f $(TARGET) *.o