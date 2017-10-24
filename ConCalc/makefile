CXX=g++
CXXFLAGS=-I. -std=c++11 -Wall -o2
LDFLAGS=-s
DEPS = concalc.h
OBJS=calc.o main.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

concalc: $(OBJS)
	$(CXX) $(LDFLAGS) -o concalc $^ $(CXXFLAGS)