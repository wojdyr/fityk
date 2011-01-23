
#include <iostream>
#include <cmath>

#include <fityk.h>

using namespace std;
using namespace fityk;

int main()
{
    Fityk *f = new Fityk;
    cout << f->get_info("version", true) << endl;
    cout << "ln(2) = " << f->calculate_expr("ln(2)") << endl;
    const double mu = 12.345;
    for (int i = 0; i != 500; ++i) {
        double x = i / 100. + 10;
        double y = ceil(100 * exp(-(x-mu)*(x-mu)/2));
        f->add_point(x, y, sqrt(y));
    }
    f->execute("Y = randnormal(y, s)");
    f->execute("guess %gauss = Gaussian");
    f->execute("fit");
    cout << "peak center: " << f->calculate_expr("%gauss.Center") << endl;
    delete f;
    return 0;
}
