#include "hash.h"    
#include <iostream>   

using std::cout;
using std::endl;

int main() {
    cout << "H(\"\")        = " << hashFunction("") << endl;
    cout << "H(\"lietuva\") = " << hashFunction("lietuva") << endl;
    return 0;
}