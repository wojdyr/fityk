// $Id$

// fastest_humlik.for and humdev.for - from Bob Wells Voigt Function Page
// http://www.atm.ox.ac.uk/user/wells/voigt.html
// Translated to C++ with f2c program and modified by M.W.
// It can be slower than original, I haven't compared the speed.

#include <math.h>
#include "voigt.h"

///     Calculates the Faddeeva function
///     and partial derivatives of the Voigt function for y>=0
///     (from http://www.atm.ox.ac.uk/user/wells/voigt.html)
/// arguments:
///  x, y - Faddeeva/Voigt function arguments
///  k - voigt              -- output
///  l - Imaginary part     -- output
///  dkdx - dVoigt/dx       -- output
///  dkdy - dVoigt/dy       -- output
void humdev(const float x, const float y,
            float &k, float &l, float &dkdx, float &dkdy)
{
    static const float c[6] = { 1.0117281f,     -0.75197147f,      0.012557727f,
                                0.010022008f,   -2.4206814e-4f,    5.0084806e-7f };
    static const float s[6] = { 1.393237f,       0.23115241f,     -0.15535147f,
                                0.0062183662f,   9.1908299e-5f,   -6.2752596e-7f };
    static const float t[6] = { 0.31424038f,     0.94778839f,      1.5976826f,
                                2.2795071f,      3.020637f,        3.8897249f };

    static const float rrtpi = 0.56418958f; // 1/SQRT(pi)
    static const double drtpi = 0.5641895835477563f; // 1/SQRT(pi)

    static float a0, b1, c0, c2, d0, d1, d2, e0, e2, e4, f1, f3, f5,
                 g0, g2, g4, g6, h0, h2, h4, h6, p0, p2, p4, p6, p8,
                 q1, q3, q5, q7, r0, r2, w0, w2, w4, z0, z2, z4, z6, z8,
                 mf[6], pf[6], mq[6], mt[6], pq[6], pt[6], xm[6], ym[6],
                 xp[6], yp[6];

    static float old_y = -1.f;

    static bool rgb, rgc, rgd;
    static float yq, xlima, xlimb, xlimc, xlim4;

    if (y != old_y) {
        old_y = y;
        rgb = true, rgc = true, rgd = true;
        yq = y * y;
        xlima = 146.7f - y;
        xlimb = 24.f - y;
        xlimc = 7.4f - y;
        xlim4 = y * 18.1f + 1.65f;
    }

    float abx = fabs(x);
    float xq = abx * abx;

    if (abx > xlima) {  //  Region A
        float d = (float)1. / (xq + yq);
        d1 = d * rrtpi;
        k = d1 * y;
        l = d1 * x;
        d1 *= d;
        dkdx = -d1 * (y + y) * x;
        dkdy = d1 * (xq - yq);
    }

    else if (abx > xlimb) { //  Region B
        if (rgb) {
            rgb = false;
            a0 = yq + .5f;
            b1 = yq - .5f;
            d0 = a0 * a0;
            d2 = b1 + b1;
            c0 = yq * (1.f - d2) + 1.5f;
            c2 = a0 + a0;
            r0 = yq * (.25f - yq * (yq + .5f)) + .125f;
            r2 = yq * (yq + 5.f) + .25f;
        }
        float d = 1.f / (d0 + xq * (d2 + xq));
        d1 = d * rrtpi;
        k = d1 * (a0 + xq) * y;
        l = d1 * (b1 + xq) * x;
        d1 *= d;
        dkdx = d1 * x * y * (c0 - (c2 + xq) * (xq + xq));
        dkdy = d1 * (r0 - xq * (r2 - xq * (b1 + xq)));
    }
    else {

        if (abx > xlimc) {  // Region C
            if (rgc) {
                rgc = false;
                h0 = yq * (yq * (yq * (yq + (float)6.) + (float)10.5)
                        + (float)4.5) + (float).5625;
                h2 = yq * (yq * (yq * (float)4. + (float)6.) + (float)9.)
                        - (float)4.5;
                h4 = (float)10.5 - yq * ((float)6. - yq * (float)6.);
                h6 = yq * (float)4. - (float)6.;
                w0 = yq * (yq * (yq * (float)7. + (float)27.5)
                        + (float) 24.25) + (float)1.875;
                w2 = yq * (yq * (float)15. + (float)3.) + (float)5.25;
                w4 = yq * (float)9. - (float)4.5;
                f1 = yq * (yq * (yq + (float)4.5) + (float)5.25)
                    - (float) 1.875;
                f3 = (float)8.25 - yq * ((float)1. - yq * (float)3.);
                f5 = yq * (float)3. - (float)5.5;
                e0 = y * (yq * (yq * (yq + (float)5.5) + (float)8.25)
                        + (float)1.875);
                e2 = y * (yq * (yq * (float)3. + (float)1.) + (float) 5.25);
                e4 = y * (float).75 * h6;
                g0 = y * (yq * (yq * (yq * (float)8. + (float)36.)
                            + (float)42.) + (float)9.);
                g2 = y * (yq * (yq * (float)24. + (float)24.) + (float)18.);
                g4 = y * (yq * (float)24. - (float)12.);
                g6 = y * (float)8.;
            }
            float u = e0 + xq * (e2 + xq * (e4 + xq * y));
            float d = (float)1. / (h0 + xq * (h2 + xq * (h4 + xq
                                                            * (h6 + xq))));
            k = d * rrtpi * u;
            l = d * rrtpi * x * (f1 + xq * (f3 + xq * (f5 + xq)));
            float dudy = w0 + xq * (w2 + xq * (w4 + xq));
            float dvdy = g0 + xq * (g2 + xq * (g4 + xq * g6));
            dkdy = d * rrtpi * (dudy - d * u * dvdy);
        }

        else if (abx < (float).85) {  // Region D
            if (rgd) {
                rgd = false;
                z0 = y * (y * (y * (y * (y * (y * (y * (y * (y *
                        (y + (float)13.3988) + (float)88.26741) + (float)
                        369.1989) + (float)1074.409) + (float)2256.981) +
                        (float)3447.629) + (float)3764.966) + (float)
                        2802.87) + (float)1280.829) + (float)272.1014;
                z2 = y * (y * (y * (y * (y * (y * (y * (y * (float)5.
                        + (float)53.59518) + (float)266.2987)
                        + (float)793.4273) + (float)1549.675) + (float)2037.31)
                        + (float)1758.336) + (float)902.3066) + (float)211.678;
                z4 = y * (y * (y * (y * (y * (y * (float)10. + (float)80.39278)
                        + (float)269.2916) + (float) 479.2576)
                        + (float)497.3014) + (float)308.1852) + (float)78.86585;
                z6 = y * (y * (y * (y * (float)10. + (float)53.59518)
                        + (float)92.75679) + (float)55.02933) + (float)22.03523;
                z8 = y * (y * (float)5. + (float)13.3988) + (float)1.49646;
                p0 = y * (y * (y * (y * (y * (y * (y * (y * (y *
                        (float).3183291 + (float)4.264678) + (float)
                        27.93941) + (float)115.3772) + (float)328.2151) +
                        (float)662.8097) + (float)946.897) + (float)
                        919.4955) + (float)549.3954) + (float)153.5168;
                p2 = y * (y * (y * (y * (y * (y * (y * (float)
                        1.2733163 + (float)12.79458) + (float)56.81652) +
                        (float)139.4665) + (float)189.773) + (float)
                        124.5975) - (float)1.322256) - (float)34.16955;
                p4 = y * (y * (y * (y * (y * (float)1.9099744 + (
                        float)12.79568) + (float)29.81482) + (float)
                        24.01655) + (float)10.46332) + (float)2.584042;
                p6 = y * (y * (y * (float)1.273316 + (float)4.266322)
                        + (float).9377051) - (float).07272979;
                p8 = y * (float).3183291 + (float)5.480304e-4;
                q1 = y * (y * (y * (y * (y * (y * (y * (y * (
                        float).3183291 + (float)4.26413) + (float)27.6294)
                         + (float)111.0528) + (float)301.3208) + (float)
                        557.5178) + (float)685.8378) + (float)508.2585) +
                        (float)173.2355;
                q3 = y * (y * (y * (y * (y * (y * (float)1.273316 +
                        (float)12.79239) + (float)55.8865) + (float)
                        130.8905) + (float)160.4013) + (float)100.7375) +
                        (float)18.97431;
                q5 = y * (y * (y * (y * (float)1.909974 + (float)
                        12.79239) + (float)28.8848) + (float)19.83766)
                        + (float)7.985877;
                q7 = y * (y * (float)1.273316 + (float)4.26413)
                        + (float).6276985;
            }
            float u = (p0 + xq * (p2 + xq * (p4 + xq * (p6 + xq * p8))))
                                                        * 1.7724538f;
            float d = 1.f / (z0 + xq * (z2 + xq * (z4 + xq
                                                * (z6 + xq * (z8 + xq)))));
            k = d * u;
            l = d * 1.7724538f * x * (q1 + xq * (q3 +
                    xq * (q5 + xq * (q7 + xq * .3183291f))));
            dkdy = 2 * ((double) x * (double) l
                                    + (double) y * (double) k - drtpi);
        }

        else {     // Use CPF12
            float ypy0 = y + 1.5f;
            float ypy0q = ypy0 * ypy0;
            k = (float)0.;
            l = (float)0.;
            for (int j = 0; j <= 5; ++j) {
                mt[j] = x - t[j];
                mq[j] = mt[j] * mt[j];
                mf[j] = (float)1. / (mq[j] + ypy0q);
                xm[j] = mf[j] * mt[j];
                ym[j] = mf[j] * ypy0;
                pt[j] = x + t[j];
                pq[j] = pt[j] * pt[j];
                pf[j] = (float)1. / (pq[j] + ypy0q);
                xp[j] = pf[j] * pt[j];
                yp[j] = pf[j] * ypy0;
                l += c[j] * (xm[j] + xp[j]) + s[j] * (ym[j] - yp[j]);
            }
            if (abx <= xlim4) {  // Humlicek CPF12 Region I
                float yf1 = ypy0 + ypy0;
                float yf2 = ypy0q + ypy0q;
                dkdy = (float)0.;
                for (int j = 0; j <= 5; ++j) {
                    float mfq = mf[j] * mf[j];
                    float pfq = pf[j] * pf[j];
                    k += c[j] * (ym[j] + yp[j]) - s[j] * (xm[j] - xp[j]);
                    dkdy += c[j] * (mf[j] + pf[j] - yf2 * (mfq + pfq))
                               + s[j] * yf1 * (mt[j] * mfq - pt[j] * pfq);
                }
            }
            else {               //  Humlicek CPF12 Region II
                float yp2y0 = y + (float)3.;
                for (int j = 0; j <= 5; ++j) {
                    k += (c[j] * (mq[j] * mf[j] - ym[j] * 1.5f)
                             + s[j] * yp2y0 * xm[j]) / (mq[j] + 2.25f)
                          + (c[j] * (pq[j] * pf[j] - yp[j] * 1.5f)
                            - s[j] * yp2y0 * xp[j]) / (pq[j] + 2.25f);
                }
                k = y * k + exp(-xq);
                dkdy = 2 * ((double) x * (double) l
                                  + (double) y * (double) k - drtpi);
            }
        }
        dkdx = 2 * ((double) y * (double) l
                                        - (double) x * (double) k);
    }
}


// ========================================================================



///   Calculates the Faddeeva function with relative error less than 10^(-4).
///     (from http://www.atm.ox.ac.uk/user/wells/voigt.html)
/// arguments:
///  x, y - Faddeeva/Voigt function arguments
/// return value -- voigt
float humlik(const float x, const float y)
{

    static const float c[6] = { 1.0117281f,     -0.75197147f,      0.012557727f,
                                0.010022008f,   -2.4206814e-4f,    5.0084806e-7f };
    static const float s[6] = { 1.393237f,       0.23115241f,     -0.15535147f,
                                0.0062183662f,   9.1908299e-5f,   -6.2752596e-7f };
    static const float t[6] = { 0.31424038f,     0.94778839f,      1.5976826f,
                                2.2795071f,      3.020637f,        3.8897249f };

    const float rrtpi = 0.56418958f; // 1/SQRT(pi)

    static float a0, d0, d2, e0, e2, e4, h0, h2, h4, h6,
                 p0, p2, p4, p6, p8, z0, z2, z4, z6, z8;
    static float mf[6], pf[6], mq[6], pq[6], xm[6], ym[6], xp[6], yp[6];
    static float old_y = -1.f;
    static bool rg1, rg2, rg3;
    static float xlim0, xlim1, xlim2, xlim3, xlim4;
    static float yq, yrrtpi;
    if (y != old_y) {
        old_y = y;
        yq = y * y;
        yrrtpi = y * rrtpi;
        rg1 = true, rg2 = true, rg3 = true;
        if (y < 70.55) {
            xlim0 = sqrt(y * (40.f - y * 3.6f) + 15100.f);
            xlim1 = (y >= 8.425 ?  0.f : sqrt(164.f - y * (y * 1.8f + 4.3f)));
            xlim2 = 6.8f - y;
            xlim3 = y * 2.4f;
            xlim4 = y * 18.1f + 1.65f;
            if (y <= 1e-6)
                xlim2 = xlim1 = xlim0;
        }
    }

    float abx = fabs(x);
    float xq = abx * abx;

    if (abx >= xlim0 || y >= 70.55)         // Region 0 algorithm
        return yrrtpi / (xq + yq);

    else if (abx >= xlim1) {            //  Humlicek W4 Region 1
        if (rg1) {  // First point in Region 1
            rg1 = false;
            a0 = yq + 0.5f;  //Region 1 y-dependents
            d0 = a0 * a0;
            d2 = yq + yq - 1.f;
        }
        return rrtpi / (d0 + xq * (d2 + xq)) * y * (a0 + xq);
    }

    else if (abx > xlim2) {  // Humlicek W4 Region 2
        if (rg2) {  //First point in Region 2
            rg2 = false;
            // Region 2 y-dependents
            h0 = yq * (yq * (yq * (yq + 6.f) + 10.5f) + 4.5f) + 0.5625f;
            h2 = yq * (yq * (yq * 4.f + 6.f) + 9.f) - 4.5f;
            h4 = 10.5f - yq * (6.f - yq * 6.f);
            h6 = yq * 4.f - 6.f;
            e0 = yq * (yq * (yq + 5.5f) + 8.25f) + 1.875f;
            e2 = yq * (yq * 3.f + 1.f) + 5.25f;
            e4 = h6 * 0.75f;
        }
        return rrtpi / (h0 + xq * (h2 + xq * (h4 + xq * (h6 + xq))))
                 * y * (e0 + xq * (e2 + xq * (e4 + xq)));
    }

    else if (abx < xlim3) { // Humlicek W4 Region 3
        if (rg3) {  // First point in Region 3
            rg3 = false;
            //Region 3 y-dependents
            z0 = y * (y * (y * (y * (y * (y * (y * (y * (y * (y
                    + 13.3988f) + 88.26741f) + 369.1989f) + 1074.409f)
                    + 2256.981f) + 3447.629f) + 3764.966f) + 2802.87f)
                    + 1280.829f) + 272.1014f;
            z2 = y * (y * (y * (y * (y * (y * (y * (y * 5.f  + 53.59518f)
                    + 266.2987f) + 793.4273f) + 1549.675f) + 2037.31f)
                    + 1758.336f) + 902.3066f) + 211.678f;
            z4 = y * (y * (y * (y * (y * (y * 10.f + 80.39278f) + 269.2916f)
                    + 479.2576f) + 497.3014f) + 308.1852f) + 78.86585f;
            z6 = y * (y * (y * (y * 10.f + 53.59518f) + 92.75679f)
                    + 55.02933f) + 22.03523f;
            z8 = y * (y * 5.f + 13.3988f) + 1.49646f;
            p0 = y * (y * (y * (y * (y * (y * (y * (y * (y * 0.3183291f
                    + 4.264678f) + 27.93941f) + 115.3772f) + 328.2151f) +
                    662.8097f) + 946.897f) + 919.4955f) + 549.3954f)
                    + 153.5168f;
            p2 = y * (y * (y * (y * (y * (y * (y * 1.2733163f + 12.79458f)
                    + 56.81652f) + 139.4665f) + 189.773f) + 124.5975f)
                    - 1.322256f) - 34.16955f;
            p4 = y * (y * (y * (y * (y * 1.9099744f + 12.79568f)
                    + 29.81482f) + 24.01655f) + 10.46332f) + 2.584042f;
            p6 = y * (y * (y * 1.273316f + 4.266322f) + 0.9377051f)
                    - 0.07272979f;
            p8 = y * .3183291f + 5.480304e-4f;
        }
        return 1.7724538f / (z0 + xq * (z2 + xq * (z4 + xq * (z6 +
                xq * (z8 + xq)))))
                  * (p0 + xq * (p2 + xq * (p4 + xq * (p6 + xq * p8))));
    }

    else {  //  Humlicek CPF12 algorithm
        float ypy0 = y + 1.5f;
        float ypy0q = ypy0 * ypy0;
        for (int j = 0; j <= 5; ++j) {
            float d = x - t[j];
            mq[j] = d * d;
            mf[j] = 1.f / (mq[j] + ypy0q);
            xm[j] = mf[j] * d;
            ym[j] = mf[j] * ypy0;
            d = x + t[j];
            pq[j] = d * d;
            pf[j] = 1.f / (pq[j] + ypy0q);
            xp[j] = pf[j] * d;
            yp[j] = pf[j] * ypy0;
        }
        float k = 0.f;
        if (abx <= xlim4) // Humlicek CPF12 Region I
            for (int j = 0; j <= 5; ++j)
                k += c[j] * (ym[j]+yp[j]) - s[j] * (xm[j]-xp[j]);
        else {           // Humlicek CPF12 Region II
            float yf = y + 3.f;
            for (int j = 0; j <= 5; ++j)
                k += (c[j] * (mq[j] * mf[j] - ym[j] * 1.5f)
                         + s[j] * yf * xm[j]) / (mq[j] + 2.25f)
                        + (c[j] * (pq[j] * pf[j] - yp[j] * 1.5f)
                           - s[j] * yf * xp[j]) / (pq[j] + 2.25f);
            k = y * k + exp(-xq);
        }
        return k;
    }
}


