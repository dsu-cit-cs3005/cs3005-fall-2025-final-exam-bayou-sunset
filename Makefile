CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic
LDFLAGS = -ldl

# Main target
all: RobotWarz

# Compile RobotBase
RobotBase.o: RobotBase.cpp RobotBase.h RadarObj.h
	$(CXX) $(CXXFLAGS) -fPIC -c RobotBase.cpp

# Compile Arena
Arena.o: Arena.cpp Arena.h RobotBase.h RadarObj.h
	$(CXX) $(CXXFLAGS) -c Arena.cpp

# Link everything
RobotWarz: main.cpp Arena.o RobotBase.o
	$(CXX) $(CXXFLAGS) main.cpp Arena.o RobotBase.o $(LDFLAGS) -o RobotWarz

# Test robot program
test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o $(LDFLAGS) -o test_robot

clean:
	rm -f *.o RobotWarz test_robot *.so lib*.so
	