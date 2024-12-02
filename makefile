CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lcrypto -lz
TARGET = mygit

all: $(TARGET)

$(TARGET): mygit.cpp
	$(CXX) $(CXXFLAGS) mygit.cpp -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)



