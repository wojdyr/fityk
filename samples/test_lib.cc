
#include <iostream>

#include <fityk.h>

using namespace std;
using namespace fityk;

int main()
{
    Fityk *f = new Fityk;
    cout << f->get_info("version", true) << endl;
    delete f;
    return 0;
}
