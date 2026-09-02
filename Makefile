CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

TARGET = shell

SRC = main.cpp prompt.cpp parser.cpp builtins.cpp redirection.cpp pipeline.cpp execution.cpp signals.cpp history.cpp autocomplete.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)