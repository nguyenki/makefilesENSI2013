#ifndef __SDMAKE_H

#define __SDMAKE_H

#include <cstdlib>

#include <cstring>



#include <mpi.h>

#define BAD_FILE -1
using namespace std;

void parse(String &nameInputFile);

void parseCommand(int argc, char* argv[]);

