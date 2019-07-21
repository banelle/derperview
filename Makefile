CC=g++
CPPFLAGS=-std=c++14 -lavcodec -lavformat -lavutil

derperview:
	$(CC) $(CPPFLAGS) -o derperview src/*.cpp

clean:
	rm derperview
