#ifndef __HDR_IO_H
#define __HDR_IO_H

#include <stdio.h>
//#include <math.h>
#include "color.h"
#include "buffer.h"
#include "output.h"

__BEGIN_YAFRAY

#define MINELEN 8
#define MAXELEN 0x7fff
#define MINRUN	4	// minimum run length
#define RED 0
#define GRN 1
#define BLU 2
#define EXP 3
#define COLXS 128
typedef unsigned char RGBE[4];
typedef CFLOAT fCOLOR[3];
// copy source -> dest
#define copy_rgbe(c1, c2) (c2[RED]=c1[RED], c2[GRN]=c1[GRN], c2[BLU]=c1[BLU], c2[EXP]=c1[EXP])
#define copy_fcol(f1, f2) (f2[RED]=f1[RED], f2[GRN]=f1[GRN], f2[BLU]=f1[BLU])

class YAFRAYCORE_EXPORT HDRimage_t
{
public:
	enum formatType {HDR_FLOAT, HDR_RGBE};
	// ctor
	HDRimage_t()
	{
		fRGB = NULL;
		rgbe_scan = NULL;
		RGBE_img = NULL;
		EXPadjust = 0;
	}
	// dtor
	~HDRimage_t() { freeBuffers(); }
	// mtds
	bool LoadHDR(const char* filename, formatType ft);
	// EXPadjust now float multiply, same result as previous integer
	void setExposureAdjust(GFLOAT ex) { EXPadjust = pow((GFLOAT)2.0, ex); }
	color_t BilerpSample(GFLOAT u, GFLOAT v);
	RGBE* getRGBEData() const { return RGBE_img; }
	int getWidth() const { return xmax; }
	int getHeight() const { return ymax; }
protected:
	FILE *file;
	fCOLOR* fRGB;  //float rgb img.
	RGBE* rgbe_scan;  //scanline buffer for writing
	RGBE* RGBE_img;  //rgbe image
	int xmax, ymax;
	GFLOAT EXPadjust;
	void freeBuffers();
	bool CheckHDR();
	bool radiance2fp();
	bool radiance2rgbe();
	bool fp2radiance();
	bool freadcolrs(RGBE *scan);
	bool oldreadcolrs(RGBE *scan);
};

class YAFRAYCORE_EXPORT outHDR_t : public colorOutput_t
{
	public:
		outHDR_t(int resx, int resy, const char *fname)
		{
			sizex = resx;
			sizey = resy;
			filename = fname;
			fbuf = new fcBuffer_t(resx, resy);
		}
		virtual bool putPixel(int x, int y, const color_t &c, 
				CFLOAT alpha=0,PFLOAT depth=0)
		{
			(*fbuf)(x, y) << c;
			return true;
		}
		void flush() { saveHDR(); }
		virtual ~outHDR_t()
		{
			if (fbuf) delete fbuf;
			fbuf = NULL;
		}
	protected:
		outHDR_t(const outHDR_t &o) {}; //forbidden
		bool saveHDR();
		fcBuffer_t* fbuf;
		int sizex, sizey;
		const char* filename;
};

YAFRAYCORE_EXPORT fcBuffer_t* loadHDR(const char* filename);

__END_YAFRAY

#endif // __HDR_IO_H
