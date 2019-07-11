CC=g++
CPPFLAGS=-std=c++14 -lavcodec -lavformat -lavutil

derperview: src/%.c
	$(CC) $(CPPFLAGS) -o derperview src/*.cpp

clean:
	rm derperview
