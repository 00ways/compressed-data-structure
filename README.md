# compressed-data-structure
Format for local projects data containment

# Build
## Requirements
- CMake >=3.10
- GCC/Clang/MSVC compiler C11(I think) compatible
```sh
$ mkdir build
$ cd build
$ cmake ..
$ cd ..
$ cmake --build build
```
## Run
```sh
$ build/CDSpacker
```
## Use in libraries
Install:
```sh
$ mkdir deps
$ git submodule add --name "cds" https://github.com/00ways/compressed-data-structure deps/cds
```
```cmake
cmake_minimum_required(VERSION 3.10)
add_subdirectory(deps/cds)
project(proj LANGUAGES C)
add_executable(${PROJECT_NAME} "main.c")
target_link_libraries(${PROJECT_NAME} PUBLIC CDS)
```
# Usage example
```C
#include <stdio.h>
#include <00ways/cds.h>

int main(int argc, char* const argv[]) {
  cdsmap* map = cdsMapFile("file.cds");
  char* texta = cdsMapReadFile(map, "helloa.txt");
  char* textb = cdsMapReadFile(map, "hellob.txt");
  printf("texta: %s");
  printf("textb: %s");
  free(texta);
  free(textb);
  cdsCloseMap(map);
  return 0;
}
```
