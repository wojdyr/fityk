
///     calculates the Faddeeva function
///     and partial derivatives of the Voigt function for y>=0
///     Based on (now only in Wayback Machine):
///     http://web.archive.org/web/20100503005358/http://www.atm.ox.ac.uk/user/wells/voigt.html
void humdev(const float x, const float y,
            float &k, float &l, float &dkdx, float &dkdy);
        // arguments:
        //  x, y - Faddeeva/Voigt function arguments
        //  k - voigt              -- output
        //  l - Imaginary part     -- output
        //  dkdx - dVoigt/dx       -- output
        //  dkdy - dVoigt/dy       -- output


///     calculates the Faddeeva function with relative error less than 10^(-4).
float humlik(const float x, const float y);
        // arguments:
        //  x, y - Faddeeva/Voigt function arguments
        //  return value -- voigt


/// wrapper around humdev(), return dk/dx. Can be slow.
inline float humdev_dkdx(const float x, const float y)
{
    float k, l, dkdx, dkdy;
    humdev(x, y, k, l, dkdx, dkdy);
    return dkdx;
}

/// wrapper around humdev(), return dk/dy. Can be slow.
inline float humdev_dkdy(const float x, const float y)
{
    float k, l, dkdx, dkdy;
    humdev(x, y, k, l, dkdx, dkdy);
    return dkdy;
}

