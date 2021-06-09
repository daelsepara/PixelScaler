GPP=nvcc
CPP=clang++
INCLUDES=-I/usr/local/include -I/usr/include -I/usr/include/opencv2 -I/usr/local/include/opencv-3.4.2/ -I/usr/local/include/opencv4
LIB=-L/usr/local/lib -L/usr/lib -lopencv_core -lopencv_highgui -lopencv_imgcodecs
UNAME_s := $(shell uname -s)
MSG=VERSION

ifeq ($(UNAME_s),Darwin)
	LIB += -lopencv_imgcodecs
	MSG += OSX
else
	MSG += OTHER
endif

all: all-cpu

all-cpu: cpu

cpu:
	$(info $(MSG))
	$(CPP) PixelScaler.cpp lodepng.cpp xbrz.cpp -o PixelScaler.exe -O3 $(INCLUDES) $(LIB) -D_FORCE_INLINES -std=c++11

clean-cpu:
	rm -f *.exe

clean: clean-cpu
