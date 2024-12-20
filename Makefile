CXX = g++
CXXFLAGS = -std=c++11
LDFLAGS = -lcurl

TARGET = app

SRC = app.cpp
HEADER = credentials.h  # Assuming this is your header file

all: $(TARGET) run

$(TARGET): $(SRC) $(HEADER)  # Add header as dependency
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
