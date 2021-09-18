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

#include <cmath>
#include <vector>

#include "fityk.h"
#include "background.hpp"

////////////////////////////////////////////////////////////////////////////////
/// This function calculates background spectrum from source spectrum.
/// The result is placed in the vector pointed by spectrum pointer.
/// The goal is to separate the useful information (peaks) from useless
/// information (background).
///
/// - method is based on Sensitive Nonlinear Iterative Peak (SNIP) clipping
///      algorithm.
/// - new value in the channel "i" is calculated
///
/// \f[
/// v_p(i) = min \left\{ v_{p-1}(i)^{\frac{\left[v_{p-1}(i+p)+v_{p-1}(i-p)\right]}{2}}   \right\}
/// \f]
///
/// where p = 1, 2, ..., numberIterations. In fact it represents second order
/// difference filter (-1,2,-1).
///
/// One can also change the
/// direction of the change of the clipping window, the order of the clipping
/// filter, to include smoothing, to set width of smoothing window and to include
/// the estimation of Compton edges. On successful completion it returns 0. On
/// error it returns pointer to the string describing error.
///
/// #### Parameters:
///
/// - spectrum: vector of source spectrum
/// - numberIterations: maximal width of clipping window,
/// - direction:  direction of change of clipping window.
///      Possible values: kBackIncreasingWindow, kBackDecreasingWindow
/// - filterOrder: order of clipping filter.
///      Possible values: kBackOrder2, kBackOrder4, kBackOrder6, kBackOrder8
/// - smoothing: logical variable whether the smoothing operation in the
///      estimation of background will be included.
///      Possible values: kFALSE, kTRUE
/// - smoothWindow: width of smoothing window.
///      Possible values: kBackSmoothing3, kBackSmoothing5, kBackSmoothing7,
///      kBackSmoothing9, kBackSmoothing11, kBackSmoothing13, kBackSmoothing15.
/// - compton: logical variable whether the estimation of Compton edge will be
///      included. Possible values: kFALSE, kTRUE.
/// 
/// #### Return:
///
/// A vector with the estimated background.
///
/// #### References:
///
///   1. C. G Ryan et al.: SNIP, a statistics-sensitive background treatment for the
/// quantitative analysis of PIXE spectra in geoscience applications. NIM, B34
/// (1988), 396-402.
///
///   2. M. Morhac;, J. Kliman, V. Matouoek, M. Veselsky, I. Turzo:
/// Background elimination methods for multidimensional gamma-ray spectra. NIM,
/// A401 (1997) 113-132.
///
///   3. D. D. Burgess, R. J. Tervo: Background estimation for gamma-ray
/// spectroscopy. NIM 214 (1983), 431-434.
///
/// ## Examples from the ROOT documentation
///
/// ### Example 1 script Background_incr.c:
///
/// \image html TSpectrum_Background_incr.jpg Fig. 1 Example of the estimation of background for number of iterations=6. Original spectrum is shown in black color, estimated background in red color.
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the background estimator (class TSpectrum).
/// // To execute this example, do
/// // root > .x Background_incr.C
///
/// #include <TSpectrum>
///
/// void Background_incr() {
///    Int_t i;
///    Double_t nbins = 256;
///    Double_t xmin  = 0;
///    Double_t xmax  = nbins;
///    Double_t * source = new Double_t[nbins];
///    TH1F *back = new TH1F("back","",nbins,xmin,xmax);
///    TH1F *d = new TH1F("d","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    back=(TH1F*) f->Get("back1;1");
///    TCanvas *Background = gROOT->GetListOfCanvases()->FindObject("Background");
///    if (!Background) Background =
///      new TCanvas("Background",
///                  "Estimation of background with increasing window",
///                  10,10,1000,700);
///    back->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=back->GetBinContent(i + 1);
///    s->Background(source,nbins,6,kBackIncreasingWindow,kBackOrder2,kFALSE,
///                  kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d->SetBinContent(i + 1,source[i]);
///    d->SetLineColor(kRed);
///    d->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 2 script Background_decr.c:
///
/// In Fig. 1. one can notice that at the edges of the peaks the estimated
/// background goes under the peaks. An alternative approach is to decrease the
/// clipping window from a given value numberIterations to the value of one, which
/// is presented in this example.
///
/// \image html TSpectrum_Background_decr.jpg Fig. 2 Example of the estimation of background for numberIterations=6 using decreasing clipping window algorithm. Original spectrum is shown in black color, estimated background in red color.
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the background estimator (class TSpectrum).
/// // To execute this example, do
/// // root > .x Background_decr.C
///
/// #include <TSpectrum>
///
/// void Background_decr() {
///    Int_t i;
///    Double_t nbins = 256;
///    Double_t xmin  = 0;
///    Double_t xmax  = nbins;
///    Double_t * source = new Double_t[nbins];
///    TH1F *back = new TH1F("back","",nbins,xmin,xmax);
///    TH1F *d = new TH1F("d","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    back=(TH1F*) f->Get("back1;1");
///    TCanvas *Background = gROOT->GetListOfCanvases()->FindObject("Background");
///    if (!Background) Background =
///      new TCanvas("Background","Estimation of background with decreasing window",
///                  10,10,1000,700);
///    back->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=back->GetBinContent(i + 1);
///    s->Background(source,nbins,6,kBackDecreasingWindow,kBackOrder2,kFALSE,
///                  kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d->SetBinContent(i + 1,source[i]);
///    d->SetLineColor(kRed);
///    d->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 3 script Background_width.c:
///
/// The question is how to choose the width of the clipping window, i.e.,
/// numberIterations parameter. The influence of this parameter on the estimated
/// background is illustrated in Fig. 3.
///
/// \image html TSpectrum_Background_width.jpg Fig. 3 Example of the influence of clipping window width on the estimated background for numberIterations=4 (red line), 6 (blue line) 8 (green line) using decreasing clipping window algorithm.
///
/// in general one should set this parameter so that the value
/// 2*numberIterations+1 was greater than the widths of preserved objects (peaks).
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the influence of the clipping window width on the
/// // estimated background. To execute this example, do:
/// // root > .x Background_width.C
///
/// #include <TSpectrum>
///
/// void Background_width() {
///    Int_t i;
///    Double_t nbins = 256;
///    Double_t xmin  = 0;
///    Double_t xmax  = nbins;
///    Double_t * source = new Double_t[nbins];
///    TH1F *h = new TH1F("h","",nbins,xmin,xmax);
///    TH1F *d1 = new TH1F("d1","",nbins,xmin,xmax);
///    TH1F *d2 = new TH1F("d2","",nbins,xmin,xmax);
///    TH1F *d3 = new TH1F("d3","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    h=(TH1F*) f->Get("back1;1");
///    TCanvas *background = gROOT->GetListOfCanvases()->FindObject("background");
///    if (!background) background = new TCanvas("background",
///    "Influence of clipping window width on the estimated background",
///    10,10,1000,700);
///    h->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,4,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d1->SetBinContent(i + 1,source[i]);
///    d1->SetLineColor(kRed);
///    d1->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,6,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d2->SetBinContent(i + 1,source[i]);
///    d2->SetLineColor(kBlue);
///    d2->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,8,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d3->SetBinContent(i + 1,source[i]);
///    d3->SetLineColor(kGreen);
///    d3->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 4 script Background_width2.c:
///
/// another example for very complex spectrum is given in Fig. 4.
///
/// \image html TSpectrum_Background_width2.jpg Fig. 4 Example of the influence of clipping window width on the estimated background for numberIterations=10 (red line), 20 (blue line), 30 (green line) and 40 (magenta line) using decreasing clipping window algorithm.
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the influence of the clipping window width on the
/// // estimated background. To execute this example, do:
/// // root > .x Background_width2.C
///
/// #include <TSpectrum>
///
/// void Background_width2() {
///    Int_t i;
///    Double_t nbins = 4096;
///    Double_t xmin  = 0;
///    Double_t xmax  = 4096;
///    Double_t * source = new Double_t[nbins];
///    TH1F *h = new TH1F("h","",nbins,xmin,xmax);
///    TH1F *d1 = new TH1F("d1","",nbins,xmin,xmax);
///    TH1F *d2 = new TH1F("d2","",nbins,xmin,xmax);
///    TH1F *d3 = new TH1F("d3","",nbins,xmin,xmax);
///    TH1F *d4 = new TH1F("d4","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    h=(TH1F*) f->Get("back2;1");
///    TCanvas *background = gROOT->GetListOfCanvases()->FindObject("background");
///    if (!background) background = new TCanvas("background",
///    "Influence of clipping window width on the estimated background",
///    10,10,1000,700);
///    h->SetAxisRange(0,1000);
///    h->SetMaximum(20000);
///    h->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,10,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d1->SetBinContent(i + 1,source[i]);
///    d1->SetLineColor(kRed);
///    d1->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,20,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d2->SetBinContent(i + 1,source[i]);
///    d2->SetLineColor(kBlue);
///    d2->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,30,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d3->SetBinContent(i + 1,source[i]);
///    d3->SetLineColor(kGreen);
///    d3->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,10,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d4->SetBinContent(i + 1,source[i]);
///    d4->SetLineColor(kMagenta);
///    d4->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 5 script Background_order.c:
///
/// Second order difference filter removes linear (quasi-linear) background and
/// preserves symmetrical peaks. However if the shape of the background is more
/// complex one can employ higher-order clipping filters (see example in Fig. 5)
///
/// \image html TSpectrum_Background_order.jpg Fig. 5 Example of the influence of clipping filter difference order on the estimated background for fNnumberIterations=40, 2-nd order red line, 4-th order blue line, 6-th order green line and 8-th order magenta line, and using decreasing clipping window algorithm.
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the influence of the clipping filter difference order
/// // on the estimated background. To execute this example, do
/// // root > .x Background_order.C
///
/// #include <TSpectrum>
///
/// void Background_order() {
///    Int_t i;
///    Double_t nbins = 4096;
///    Double_t xmin  = 0;
///    Double_t xmax  = 4096;
///    Double_t * source = new Double_t[nbins];
///    TH1F *h = new TH1F("h","",nbins,xmin,xmax);
///    TH1F *d1 = new TH1F("d1","",nbins,xmin,xmax);
///    TH1F *d2 = new TH1F("d2","",nbins,xmin,xmax);
///    TH1F *d3 = new TH1F("d3","",nbins,xmin,xmax);
///    TH1F *d4 = new TH1F("d4","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    h=(TH1F*) f->Get("back2;1");
///    TCanvas *background = gROOT->GetListOfCanvases()->FindObject("background");
///    if (!background) background = new TCanvas("background",
///    "Influence of clipping filter difference order on the estimated background",
///    10,10,1000,700);
///    h->SetAxisRange(1220,1460);
///    h->SetMaximum(11000);
///    h->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,40,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d1->SetBinContent(i + 1,source[i]);
///    d1->SetLineColor(kRed);
///    d1->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,40,kBackDecreasingWindow,kBackOrder4,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d2->SetBinContent(i + 1,source[i]);
///    d2->SetLineColor(kBlue);
///    d2->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,40,kBackDecreasingWindow,kBackOrder6,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d3->SetBinContent(i + 1,source[i]);
///    d3->SetLineColor(kGreen);
///    d3->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,40,kBackDecreasingWindow,kBackOrder8,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d4->SetBinContent(i + 1,source[i]);
///    d4->SetLineColor(kMagenta);
///    d4->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 6 script Background_smooth.c:
///
/// The estimate of the background can be influenced by noise present in the
/// spectrum.  We proposed the algorithm of the background estimate with
/// simultaneous smoothing.  In the original algorithm without smoothing, the
/// estimated background snatches the lower spikes in the noise. Consequently,
/// the areas of peaks are biased by this error.
///
/// \image html TSpectrum_Background_smooth1.jpg Fig. 7 Principle of background estimation algorithm with simultaneous smoothing.
/// \image html TSpectrum_Background_smooth2.jpg Fig. 8 Illustration of non-smoothing (red line) and smoothing algorithm of background estimation (blue line).
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the background estimator (class TSpectrum) including
/// // Compton edges. To execute this example, do:
/// // root > .x Background_smooth.C
///
/// #include <TSpectrum>
///
/// void Background_smooth() {
///    Int_t i;
///    Double_t nbins = 4096;
///    Double_t xmin  = 0;
///    Double_t xmax  = nbins;
///    Double_t * source = new Double_t[nbins];
///    TH1F *h = new TH1F("h","",nbins,xmin,xmax);
///    TH1F *d1 = new TH1F("d1","",nbins,xmin,xmax);
///    TH1F *d2 = new TH1F("d2","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    h=(TH1F*) f->Get("back4;1");
///    TCanvas *background = gROOT->GetListOfCanvases()->FindObject("background");
///    if (!background) background = new TCanvas("background",
///    "Estimation of background with noise",10,10,1000,700);
///    h->SetAxisRange(3460,3830);
///    h->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,6,kBackDecreasingWindow,kBackOrder2,kFALSE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d1->SetBinContent(i + 1,source[i]);
///    d1->SetLineColor(kRed);
///    d1->Draw("SAME L");
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,6,kBackDecreasingWindow,kBackOrder2,kTRUE,
///    kBackSmoothing3,kFALSE);
///    for (i = 0; i < nbins; i++) d2->SetBinContent(i + 1,source[i]);
///    d2->SetLineColor(kBlue);
///    d2->Draw("SAME L");
/// }
/// ~~~
///
/// ### Example 8 script Background_compton.c:
///
/// Sometimes it is necessary to include also the Compton edges into the estimate of
/// the background. In Fig. 8 we present the example of the synthetic spectrum
/// with Compton edges. The background was estimated using the 8-th order filter
/// with the estimation of the Compton edges using decreasing
/// clipping window algorithm (numberIterations=10) with smoothing
/// (smoothingWindow=5).
///
/// \image html TSpectrum_Background_compton.jpg Fig. 8 Example of the estimate of the background with Compton edges (red line) for numberIterations=10, 8-th order difference filter, using decreasing clipping window algorithm and smoothing (smoothingWindow=5).
///
/// #### Script:
///
/// ~~~ {.cpp}
/// // Example to illustrate the background estimator (class TSpectrum) including
/// // Compton edges. To execute this example, do:
/// // root > .x Background_compton.C
///
/// #include <TSpectrum>
///
/// void Background_compton() {
///    Int_t i;
///    Double_t nbins = 512;
///    Double_t xmin  = 0;
///    Double_t xmax  = nbins;
///    Double_t * source = new Double_t[nbins];
///    TH1F *h = new TH1F("h","",nbins,xmin,xmax);
///    TH1F *d1 = new TH1F("d1","",nbins,xmin,xmax);
///    TFile *f = new TFile("spectra/TSpectrum.root");
///    h=(TH1F*) f->Get("back3;1");
///    TCanvas *background = gROOT->GetListOfCanvases()->FindObject("background");
///    if (!background) background = new TCanvas("background",
///    "Estimation of background with Compton edges under peaks",10,10,1000,700);
///    h->Draw("L");
///    TSpectrum *s = new TSpectrum();
///    for (i = 0; i < nbins; i++) source[i]=h->GetBinContent(i + 1);
///    s->Background(source,nbins,10,kBackDecreasingWindow,kBackOrder8,kTRUE,
///    kBackSmoothing5,,kTRUE);
///    for (i = 0; i < nbins; i++) d1->SetBinContent(i + 1,source[i]);
///    d1->SetLineColor(kRed);
///    d1->Draw("SAME L");
/// }
/// ~~~

std::vector<fityk::Point> ROOT::background(const std::vector<fityk::Point> spectrum,
                                           int numberIterations,
                                           int direction,
                                           int filterOrder,
                                           bool smoothing,
                                           int smoothWindow,
                                           bool compton)
{
    int i, j, w, bw, b1, b2, priz;
    double a, b, c, d, e, yb1, yb2, ai, av, men, b4, c4, d4, e4, b6, c6, d6, e6, f6, g6, b8, c8, d8, e8, f8, g8, h8, i8;

    const int ssize = (int) spectrum.size();

    if (ssize <= 0)
        throw fityk::ExecuteError("Wrong vector size");

    if (numberIterations < 1)
        throw fityk::ExecuteError("Width of Clipping Window Must Be Positive");

    if (ssize < 2 * numberIterations + 1)
        throw fityk::ExecuteError("Too Large Clipping Window");

    if (filterOrder != kBackOrder2 && filterOrder != kBackOrder4 && filterOrder != kBackOrder6 && filterOrder != kBackOrder8)
        throw fityk::ExecuteError("Incorrect order of clipping filter, possible values are: 2, 4, 6 or 8");

    if (smoothing == true && smoothWindow != kBackSmoothing3 && smoothWindow != kBackSmoothing5 && smoothWindow != kBackSmoothing7 && smoothWindow != kBackSmoothing9 && smoothWindow != kBackSmoothing11 && smoothWindow != kBackSmoothing13 && smoothWindow != kBackSmoothing15)
        throw fityk::ExecuteError("Incorrect width of smoothing window");

   std::vector<fityk::Point> working_space(2 * ssize);

   for (i = 0; i < ssize; i++)
    {
      working_space[i] = spectrum[i];
      working_space[i + ssize] = spectrum[i];
   }

   bw=(smoothWindow-1)/2;

   if (direction == kBackIncreasingWindow)
      i = 1;
   else if(direction == kBackDecreasingWindow)
      i = numberIterations;

   if (filterOrder == kBackOrder2) {
      do{
         for (j = i; j < ssize - i; j++) {
            if (smoothing == false){
               a = working_space[ssize + j].y;
               b = (working_space[ssize + j - i].y + working_space[ssize + j + i].y) / 2.0;
               if (b < a)
                  a = b;
               working_space[j].y = a;
            }

            else if (smoothing == true){
               a = working_space[ssize + j].y;
               av = 0;
               men = 0;
               for (w = j - bw; w <= j + bw; w++){
                  if ( w >= 0 && w < ssize){
                     av += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               av = av / men;
               b = 0;
               men = 0;
               for (w = j - i - bw; w <= j - i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     b += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b = b / men;
               c = 0;
               men = 0;
               for (w = j + i - bw; w <= j + i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     c += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c = c / men;
               b = (b + c) / 2;
               if (b < a)
                  av = b;
               working_space[j].y=av;
            }
         }
         for (j = i; j < ssize - i; j++)
            working_space[ssize + j].y = working_space[j].y;
         if (direction == kBackIncreasingWindow)
            i+=1;
         else if(direction == kBackDecreasingWindow)
            i-=1;
      }while((direction == kBackIncreasingWindow && i <= numberIterations) || (direction == kBackDecreasingWindow && i >= 1));
   }

   else if (filterOrder == kBackOrder4) {
      do{
         for (j = i; j < ssize - i; j++) {
            if (smoothing == false){
               a = working_space[ssize + j].y;
               b = (working_space[ssize + j - i].y + working_space[ssize + j + i].y) / 2.0;
               c = 0;
               ai = i / 2;
               c -= working_space[ssize + j - (int) (2 * ai)].y / 6;
               c += 4 * working_space[ssize + j - (int) ai].y / 6;
               c += 4 * working_space[ssize + j + (int) ai].y / 6;
               c -= working_space[ssize + j + (int) (2 * ai)].y / 6;
               if (b < c)
                  b = c;
               if (b < a)
                  a = b;
               working_space[j].y = a;
            }

            else if (smoothing == true){
               a = working_space[ssize + j].y;
               av = 0;
               men = 0;
               for (w = j - bw; w <= j + bw; w++){
                  if ( w >= 0 && w < ssize){
                     av += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               av = av / men;
               b = 0;
               men = 0;
               for (w = j - i - bw; w <= j - i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     b += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b = b / men;
               c = 0;
               men = 0;
               for (w = j + i - bw; w <= j + i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     c += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c = c / men;
               b = (b + c) / 2;
               ai = i / 2;
               b4 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b4 = b4 / men;
               c4 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     c4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c4 = c4 / men;
               d4 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     d4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d4 = d4 / men;
               e4 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     e4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e4 = e4 / men;
               b4 = (-b4 + 4 * c4 + 4 * d4 - e4) / 6;
               if (b < b4)
                  b = b4;
               if (b < a)
                  av = b;
               working_space[j].y=av;
            }
         }
         for (j = i; j < ssize - i; j++)
            working_space[ssize + j].y = working_space[j].y;
         if (direction == kBackIncreasingWindow)
            i+=1;
         else if(direction == kBackDecreasingWindow)
            i-=1;
      }while((direction == kBackIncreasingWindow && i <= numberIterations) || (direction == kBackDecreasingWindow && i >= 1));
   }

   else if (filterOrder == kBackOrder6) {
      do{
         for (j = i; j < ssize - i; j++) {
            if (smoothing == false){
               a = working_space[ssize + j].y;
               b = (working_space[ssize + j - i].y + working_space[ssize + j + i].y) / 2.0;
               c = 0;
               ai = i / 2;
               c -= working_space[ssize + j - (int) (2 * ai)].y / 6;
               c += 4 * working_space[ssize + j - (int) ai].y / 6;
               c += 4 * working_space[ssize + j + (int) ai].y / 6;
               c -= working_space[ssize + j + (int) (2 * ai)].y / 6;
               d = 0;
               ai = i / 3;
               d += working_space[ssize + j - (int) (3 * ai)].y / 20;
               d -= 6 * working_space[ssize + j - (int) (2 * ai)].y / 20;
               d += 15 * working_space[ssize + j - (int) ai].y / 20;
               d += 15 * working_space[ssize + j + (int) ai].y / 20;
               d -= 6 * working_space[ssize + j + (int) (2 * ai)].y / 20;
               d += working_space[ssize + j + (int) (3 * ai)].y / 20;
               if (b < d)
                  b = d;
               if (b < c)
                  b = c;
               if (b < a)
                  a = b;
               working_space[j].y = a;
            }

            else if (smoothing == true){
               a = working_space[ssize + j].y;
               av = 0;
               men = 0;
               for (w = j - bw; w <= j + bw; w++){
                  if ( w >= 0 && w < ssize){
                     av += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               av = av / men;
               b = 0;
               men = 0;
               for (w = j - i - bw; w <= j - i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     b += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b = b / men;
               c = 0;
               men = 0;
               for (w = j + i - bw; w <= j + i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     c += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c = c / men;
               b = (b + c) / 2;
               ai = i / 2;
               b4 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b4 = b4 / men;
               c4 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     c4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c4 = c4 / men;
               d4 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     d4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d4 = d4 / men;
               e4 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     e4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e4 = e4 / men;
               b4 = (-b4 + 4 * c4 + 4 * d4 - e4) / 6;
               ai = i / 3;
               b6 = 0, men = 0;
               for (w = j - (int)(3 * ai) - bw; w <= j - (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b6 = b6 / men;
               c6 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     c6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c6 = c6 / men;
               d6 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     d6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d6 = d6 / men;
               e6 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     e6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e6 = e6 / men;
               f6 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     f6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               f6 = f6 / men;
               g6 = 0, men = 0;
               for (w = j + (int)(3 * ai) - bw; w <= j + (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     g6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               g6 = g6 / men;
               b6 = (b6 - 6 * c6 + 15 * d6 + 15 * e6 - 6 * f6 + g6) / 20;
               if (b < b6)
                  b = b6;
               if (b < b4)
                  b = b4;
               if (b < a)
                  av = b;
               working_space[j].y=av;
            }
         }
         for (j = i; j < ssize - i; j++)
            working_space[ssize + j].y = working_space[j].y;
         if (direction == kBackIncreasingWindow)
            i+=1;
         else if(direction == kBackDecreasingWindow)
            i-=1;
      }while((direction == kBackIncreasingWindow && i <= numberIterations) || (direction == kBackDecreasingWindow && i >= 1));
   }

   else if (filterOrder == kBackOrder8) {
      do{
         for (j = i; j < ssize - i; j++) {
            if (smoothing == false){
               a = working_space[ssize + j].y;
               b = (working_space[ssize + j - i].y + working_space[ssize + j + i].y) / 2.0;
               c = 0;
               ai = i / 2;
               c -= working_space[ssize + j - (int) (2 * ai)].y / 6;
               c += 4 * working_space[ssize + j - (int) ai].y / 6;
               c += 4 * working_space[ssize + j + (int) ai].y / 6;
               c -= working_space[ssize + j + (int) (2 * ai)].y / 6;
               d = 0;
               ai = i / 3;
               d += working_space[ssize + j - (int) (3 * ai)].y / 20;
               d -= 6 * working_space[ssize + j - (int) (2 * ai)].y / 20;
               d += 15 * working_space[ssize + j - (int) ai].y / 20;
               d += 15 * working_space[ssize + j + (int) ai].y / 20;
               d -= 6 * working_space[ssize + j + (int) (2 * ai)].y / 20;
               d += working_space[ssize + j + (int) (3 * ai)].y / 20;
               e = 0;
               ai = i / 4;
               e -= working_space[ssize + j - (int) (4 * ai)].y / 70;
               e += 8 * working_space[ssize + j - (int) (3 * ai)].y / 70;
               e -= 28 * working_space[ssize + j - (int) (2 * ai)].y / 70;
               e += 56 * working_space[ssize + j - (int) ai].y / 70;
               e += 56 * working_space[ssize + j + (int) ai].y / 70;
               e -= 28 * working_space[ssize + j + (int) (2 * ai)].y / 70;
               e += 8 * working_space[ssize + j + (int) (3 * ai)].y / 70;
               e -= working_space[ssize + j + (int) (4 * ai)].y / 70;
               if (b < e)
                  b = e;
               if (b < d)
                  b = d;
               if (b < c)
                  b = c;
               if (b < a)
                  a = b;
               working_space[j].y = a;
            }

            else if (smoothing == true){
               a = working_space[ssize + j].y;
               av = 0;
               men = 0;
               for (w = j - bw; w <= j + bw; w++){
                  if ( w >= 0 && w < ssize){
                     av += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               av = av / men;
               b = 0;
               men = 0;
               for (w = j - i - bw; w <= j - i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     b += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b = b / men;
               c = 0;
               men = 0;
               for (w = j + i - bw; w <= j + i + bw; w++){
                  if ( w >= 0 && w < ssize){
                     c += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c = c / men;
               b = (b + c) / 2;
               ai = i / 2;
               b4 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b4 = b4 / men;
               c4 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     c4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c4 = c4 / men;
               d4 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     d4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d4 = d4 / men;
               e4 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     e4 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e4 = e4 / men;
               b4 = (-b4 + 4 * c4 + 4 * d4 - e4) / 6;
               ai = i / 3;
               b6 = 0, men = 0;
               for (w = j - (int)(3 * ai) - bw; w <= j - (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b6 = b6 / men;
               c6 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     c6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c6 = c6 / men;
               d6 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     d6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d6 = d6 / men;
               e6 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     e6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e6 = e6 / men;
               f6 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     f6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               f6 = f6 / men;
               g6 = 0, men = 0;
               for (w = j + (int)(3 * ai) - bw; w <= j + (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     g6 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               g6 = g6 / men;
               b6 = (b6 - 6 * c6 + 15 * d6 + 15 * e6 - 6 * f6 + g6) / 20;
               ai = i / 4;
               b8 = 0, men = 0;
               for (w = j - (int)(4 * ai) - bw; w <= j - (int)(4 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     b8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               b8 = b8 / men;
               c8 = 0, men = 0;
               for (w = j - (int)(3 * ai) - bw; w <= j - (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     c8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               c8 = c8 / men;
               d8 = 0, men = 0;
               for (w = j - (int)(2 * ai) - bw; w <= j - (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     d8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               d8 = d8 / men;
               e8 = 0, men = 0;
               for (w = j - (int)ai - bw; w <= j - (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     e8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               e8 = e8 / men;
               f8 = 0, men = 0;
               for (w = j + (int)ai - bw; w <= j + (int)ai + bw; w++){
                  if (w >= 0 && w < ssize){
                     f8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               f8 = f8 / men;
               g8 = 0, men = 0;
               for (w = j + (int)(2 * ai) - bw; w <= j + (int)(2 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     g8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               g8 = g8 / men;
               h8 = 0, men = 0;
               for (w = j + (int)(3 * ai) - bw; w <= j + (int)(3 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     h8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               h8 = h8 / men;
               i8 = 0, men = 0;
               for (w = j + (int)(4 * ai) - bw; w <= j + (int)(4 * ai) + bw; w++){
                  if (w >= 0 && w < ssize){
                     i8 += working_space[ssize + w].y;
                     men +=1;
                  }
               }
               i8 = i8 / men;
               b8 = ( -b8 + 8 * c8 - 28 * d8 + 56 * e8 - 56 * f8 - 28 * g8 + 8 * h8 - i8)/70;
               if (b < b8)
                  b = b8;
               if (b < b6)
                  b = b6;
               if (b < b4)
                  b = b4;
               if (b < a)
                  av = b;
               working_space[j].y = av;
            }
         }
         for (j = i; j < ssize - i; j++)
            working_space[ssize + j].y = working_space[j].y;
         if (direction == kBackIncreasingWindow)
            i += 1;
         else if(direction == kBackDecreasingWindow)
            i -= 1;
      }while((direction == kBackIncreasingWindow && i <= numberIterations) || (direction == kBackDecreasingWindow && i >= 1));
   }

   if (compton == true) {
      for (i = 0, b2 = 0; i < ssize; i++){
         b1 = b2;
         a = working_space[i].y, b = spectrum[i].y;
         j = i;
         if (fabs(a - b) >= 1) {
            b1 = i - 1;
            if (b1 < 0)
               b1 = 0;
            yb1 = working_space[b1].y;
            for (b2 = b1 + 1, c = 0, priz = 0; priz == 0 && b2 < ssize; b2++){
               a = working_space[b2].y, b = spectrum[b2].y;
               c = c + b - yb1;
               if (fabs(a - b) < 1) {
                  priz = 1;
                  yb2 = b;
               }
            }
            if (b2 == ssize)
               b2 -= 1;
            yb2 = working_space[b2].y;
            if (yb1 <= yb2){
               for (j = b1, c = 0; j <= b2; j++){
                  b = spectrum[j].y;
                  c = c + b - yb1;
               }
               if (c > 1){
                  c = (yb2 - yb1) / c;
                  for (j = b1, d = 0; j <= b2 && j < ssize; j++){
                     b = spectrum[j].y;
                     d = d + b - yb1;
                     a = c * d + yb1;
                     working_space[ssize + j].y = a;
                  }
               }
            }

            else{
               for (j = b2, c = 0; j >= b1; j--){
                  b = spectrum[j].y;
                  c = c + b - yb2;
               }
               if (c > 1){
                  c = (yb1 - yb2) / c;
                  for (j = b2, d = 0;j >= b1 && j >= 0; j--){
                     b = spectrum[j].y;
                     d = d + b - yb2;
                     a = c * d + yb2;
                     working_space[ssize + j].y = a;
                  }
               }
            }
            i=b2;
         }
      }
   }

    working_space.resize(ssize);

    return working_space;
}
