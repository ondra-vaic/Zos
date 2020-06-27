#include <iostream>
#include <vector>
#include <sstream>

#include <map>
#include <functional>
#include <fstream>
#include "FileSystem.h"
#include "SuperBlock.h"
#include "Bitmap.h"

using namespace std;


int main(int argc, char** argv) {
    FileSystem fileSystem(argv[1]);
    fileSystem.Run();
    return 0;
}
