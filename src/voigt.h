// $Id$

//     To calculate the Faddeeva function 
//     and partial derivatives of the Voigt function for y>=0  
//     (from http://www.atm.ox.ac.uk/user/wells/voigt.html)
void humdev(const float x, const float y, 
            float &k, float &l, float &dkdx, float &dkdy);
        // arguments:
        //  x, y - Faddeeva/Voigt function arguments 
        //  k - voigt              -- output 
        //  l - Imaginary part     -- output
        //  dkdx - dVoigt/dx       -- output
        //  dkdy - dVoigt/dy       -- output


//   To calculate the Faddeeva function with relative error less than 10^(-4).
//     (from http://www.atm.ox.ac.uk/user/wells/voigt.html)
float humlik(const float x, const float y);
        // arguments:
        //  x, y - Faddeeva/Voigt function arguments 
        //  return value -- voigt
