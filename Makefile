CXXFLAGS = -g -Wall -std=c++17 -O3
LDFLAGS = -L.
LDLIBS = -ldhnetsdk

all: dhstream

dhstream: dhstream.o
	$(CXX) $(LDFLAGS) -o $@ $< $(LDLIBS)

dhstream.o: dhstream.cpp dhstream.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o dhstream
