//
// Created by me on 1/10/20.
//

#ifndef ZOS_UTILS_H
#define ZOS_UTILS_H

#include <vector>
#include <string>
#include <fstream>
using namespace std;

class Utils {
public:
    static vector<string> GetCommand();

    static bool GetBit(unsigned char num, unsigned char pos);
    static unsigned char SetBit(unsigned char num, unsigned char pos);
    static char* CreateIdentifier(string s);
    static void SetZeros(const string& fileSystemName, int32_t offset, int32_t size);
};


#endif //ZOS_UTILS_H
