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

// priklad - verze 2019-01
// jedná se o SIMULACI souborového systému
// první i-uzel bude odkaz na hlavní adresář (1. datový blok)
// počet přímých i nepřímých odkazů je v reálném systému větší
// adresář vždy obsahuje dvojici číslo i-uzlu - název souboru
// jde jen o příklad, vlastní datové struktury si můžete upravit


int main(int argc, char** argv) {

    //    std::ofstream output( "abc.jpg", std::ios::binary);
//    string inputLine;
//
//    while (getline(input, inputLine))
//    {
//        output << inputLine << std::endl;
//    }
//
    // 01100101000
//    Bitmap b(36);
//    b.bitmap[0] = false;
//    b.bitmap[1] = true;
//    b.bitmap[2] = true;
//    b.bitmap[3] = false;
//    b.bitmap[4] = false;
//    b.bitmap[5] = true;
//    b.bitmap[6] = false;
//    b.bitmap[7] = true;
//
//    b.bitmap[8] = true;
//    b.bitmap[9] = true;
//    b.bitmap[10] = false;
//    b.bitmap[11] = false;
//    b.bitmap[12] = true;
//    b.bitmap[13] = true;
//    b.bitmap[14] = true;
//    b.bitmap[15] = true;
//
//    b.Save("disk", 0);
//    Bitmap c("disk", 0, 4);
//
//
//    cout << "------test-----" << endl;
//    cout << c.bitmap[0] <<
//            c.bitmap[1] <<
//            c.bitmap[2] <<
//            c.bitmap[3] <<
//            c.bitmap[4] <<
//            c.bitmap[5] <<
//            c.bitmap[6] <<
//            c.bitmap[7] << endl;
//
//    cout << "num 2 " << endl;
//    cout << c.bitmap[8] <<
//            c.bitmap[9] <<
//            c.bitmap[10] <<
//            c.bitmap[11] <<
//            c.bitmap[12] <<
//            c.bitmap[13] <<
//            c.bitmap[14] <<
//            c.bitmap[15] <<endl;
//
//    cout << "------end-----" << endl;
//
//    c.Save("disk", 0);
//
//    Bitmap d("disk", 0, 2);
//
//    cout << "------test-----" << endl;
//    cout << d.bitmap[0] <<
//         d.bitmap[1] <<
//         d.bitmap[2] <<
//         d.bitmap[3] <<
//         d.bitmap[4] <<
//         d.bitmap[5] <<
//         d.bitmap[6] <<
//         d.bitmap[7] << endl;
//
//    cout << "num 2 " << endl;
//    cout << d.bitmap[8] <<
//         d.bitmap[9] <<
//         d.bitmap[10] <<
//         d.bitmap[11] <<endl;
//
//    cout << "------end-----" << endl;
//
//
//    return 0;


    FileSystem fileSystem(argv[1]);
    fileSystem.Run();
    return 0;
}
