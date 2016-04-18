#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "color.h"

__BEGIN_YAFRAY

//YAFRAYCORE_EXPORT void wl2rgb_fromCIE(CFLOAT wl, color_t &col);
//YAFRAYCORE_EXPORT void approxSpectrumRGB(CFLOAT wl, color_t &col);
//YAFRAYCORE_EXPORT void fakeSpectrum(CFLOAT p, color_t &col);
YAFRAYCORE_EXPORT void CauchyCoefficients(PFLOAT IOR, PFLOAT disp_pw, PFLOAT &CauchyA, PFLOAT &CauchyB);
YAFRAYCORE_EXPORT PFLOAT getIORcolor(PFLOAT w, PFLOAT CauchyA, PFLOAT CauchyB, color_t &col);

__END_YAFRAY

#endif //__HRAY_SPECTRUM__
