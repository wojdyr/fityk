// @(#)root/spectrum:$Id$
// Author: Miroslav Morhac   27/05/99

/*************************************************************************
 * Copyright (C) 1995-2006, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

// Modified by Cristiano Fontana 17/11/2016
// Eliminated the dependency on ROOT

#ifndef __BACKGROUND_HPP__
#define __BACKGROUND_HPP__

namespace background {

    enum {
        kBackIncreasingWindow =0,
        kBackDecreasingWindow =1,
        kBackOrder2 =2,
        kBackOrder4 =4,
        kBackOrder6 =6,
        kBackOrder8 =8,
        kBackSmoothing3 =3,
        kBackSmoothing5 =5,
        kBackSmoothing7 =7,
        kBackSmoothing9 =9,
        kBackSmoothing11 =11,
        kBackSmoothing13 =13,
        kBackSmoothing15 =15
   };

    std::vector<double> background(std::vector<double> spectrum,
                                   int numberIterations,
                                   int direction,
                                   int filterOrder,
                                   bool smoothing,
                                   int smoothWindow,
                                   bool compton);
}

#endif
