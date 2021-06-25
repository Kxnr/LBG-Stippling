# Linde Buzo Gray Stippling

## Description

A replication of [http://graphics.uni-konstanz.de/publikationen/Deussen2017LindeBuzoGray/WeightedLindeBuzoGrayStippling_authorversion.pdf](this) 
paper, demonstrating a method of producing stippled versions of images. This is, effectively, my fourth implementation of this method. My first
approach used Processing, which is a great development environment but somewhat limited in performance. After porting over to C++, I have written
several iterations in search of greater performance. At this point, the heaviest calculation is done by a compute shader that was my excuse to
start learning Vulkan.


## Setup

### Dependencies

* vulkan sdk
* armadillo
* C++11 compatible compiler
* make

### Installation

* Export VULKAN_SDK environment variable
* Check armadillo link instructions
* Build program with `make all`

### Executing program

```
./main.out <input filename> <output filename>
```

## Authors

* [Connor Keane](kxnr.me)

## License

This project is licensed under the GPL3 License - see the LICENSE.md file for details

## Acknowledgments

* Check out the original work, it's pretty great
* Sascha Willems has some great tutorials and starter code for using Vulkan, many of which I used or referred to while working on this project

