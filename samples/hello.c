/* Example of using libfityk from ANSI C. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <fityk/fityk.h>


int main()
{
    int i;
    double x[500], y[500], sigma[500];
    const double mu = 12.345;
    double d;

    Fityk *f = fityk_create();

    char *s = fityk_get_info(f, "version", 0);
    printf("%s\n", s);
    free(s);
    d = fityk_calculate_expr(f, "ln(2)", 0);
    printf("ln(2) = %g\n", d);

    for (i = 0; i != 500; ++i) {
        x[i] = i / 100. + 10;
        y[i] = ceil(100 * exp(-(x[i]-mu)*(x[i]-mu)/2));
        sigma[i] = sqrt(y[i]);
    }
    fityk_load_data(f, 0, x, y, sigma, 500, "noisy gaussian");

    fityk_execute(f, "Y = randnormal(y, s)");
    fityk_execute(f, "guess %gauss = Gaussian");
    fityk_execute(f, "fit");
    d = fityk_calculate_expr(f, "%gauss.Center", 0);
    printf("peak center: %g\n", d);
    fityk_delete(f);
    return 0;
}
