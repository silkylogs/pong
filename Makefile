COMPILEFLAGS= -O1 -Wall -std=c++11
LINKFLAGS= -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows

all:
	g++ main.cpp -o game.exe $(COMPILEFLAGS) -I include/ -L lib/ $(LINKFLAGS)