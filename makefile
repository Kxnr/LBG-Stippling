CXX=g++
CFLAGS=-g -Werror -Wpedantic -std=c++17 

ODIR=obj
IDIR=include
SDIR=src

IFLAGS=-Iinclude -Ilib -I$(VULKAN_SDK)/include
LFLAGS=-L/usr/X11R6/lib -L$(VULKAN_SDK)/lib -lvulkan -lm -lpthread -lX11

_OBJ=main.o stipples.o voronoi.o gpuVoronoi.o headlessVulkan.o pdf.o metrics.o
_DEPS=CImg.h vec3.h utils.h voronoi.h stipples.h gpuVoronoi.h headlessVulkan.h pdf.h metrics.h
_SRC=main.cpp stipples.cpp voronoi.cpp gpuVoronoi.cpp headlessVulkan.cpp pdf.cpp metrics.cpp

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

export VULKAN_SDK=/home/kxnr/src/vulkan/1.2.135.0/x86_64
export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d

all: shaders main 

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(IFLAGS) -c -o $@ $<

main: $(OBJ)
	$(CXX) $(CFLAGS) $(IFLAGS) -o $@.out $^ $(LFLAGS)

shaders: $(SDIR)/shaders/shader.frag $(SDIR)/shaders/shader.vert
	$(VULKAN_SDK)/bin/glslc $(SDIR)/shaders/shader.frag -o resources/shaders/frag.spv
	$(VULKAN_SDK)/bin/glslc $(SDIR)/shaders/shader.vert -o resources/shaders/vert.spv

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o 
	rm -f resources/shaders/*.spv
