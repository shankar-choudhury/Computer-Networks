CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
TARGETS = proj5 proj5d

CMDS_OBJS = bbp_commands_core.o \
            bbp_commands_items.o \
            bbp_commands_search.o \
            bbp_commands_links.o \
            bbp_commands_books.o \
            bbp_commands_usage.o

all: $(TARGETS)

proj5: proj5.cpp bbp.hpp
	$(CXX) $(CXXFLAGS) -o proj5 proj5.cpp

proj5d: proj5d.o $(CMDS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGETS) *.o
