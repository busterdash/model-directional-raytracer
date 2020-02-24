OBJECTS = sun/main.o sun/raytracer.o sun/smd_model_reader.o sun/geometry.o lodepng/lodepng.o
TARGET = sun
CPP = g++
FLAGS = -std=c++11

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CPP) -o $(TARGET) $(OBJECTS)

main.o: sun/main.cpp
	$(CPP) $(FLAGS) -c -o sun/main.o sun/main.cpp

raytracer.o: sun/raytracer.cpp
	$(CPP) $(FLAGS) -c -o sun/raytracer.o sun/raytracer.cpp

smd_model_reader.o: sun/smd_model_reader.cpp
	$(CPP) $(FLAGS) -c -o sun/smd_model_reader.o sun/smd_model_reader.cpp

geometry.o: sun/geometry.cpp
	$(CPP) $(FLAGS) -c -o sun/geometry.o sun/geometry.cpp

lodepng.o: lodepng/lodepng.cpp
	$(CPP) -c -o lodepng/lodepng.o lodepng/lodepng.cpp -Wall -Wextra -pedantic -ansi -O3

.PHONY: clean
clean:
	rm -f sun/*.o *.exe lodepng/*.o
