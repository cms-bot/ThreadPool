CXX = g++
CFLAGS = -std=c++17 -o2 -Wall -g 
TARGET = target
OBJS = ../code/*.cpp ../code/FinalPool/*.h  
all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS)  -fsanitize=address -fsanitize=leak -fsanitize=undefined  -o ../bin/$(TARGET) 
clean:
	rm -rf ./bin/*.o $(TARGET)
#-fsanitize=address -fsanitize=leak -fsanitize=undefined