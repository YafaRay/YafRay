//---------------------------------------------------------------------------
// Targa image loader
//---------------------------------------------------------------------------
#ifndef __TARGAIO_H
#define __TARGAIO_H

#include <stdio.h>
#include <string>
#include "color.h"
#include "buffer.h"
#include "output.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT targaImg_t
{
public:
	targaImg_t();
	~targaImg_t();
	cBuffer_t* Load(const char *fname, bool verify=false);
	unsigned short getWidth() const { return width; }
	unsigned short getHeight() const { return height; }
	//color_t operator() (int x, int y) const;
	std::string getErrorString() { return err_str; }
private:
	targaImg_t(const targaImg_t &t) {}; //forbidden
	FILE *fp;
	std::string err_str;
	unsigned char alpha_bits, byte_per_pix;
	bool has_alpha, isgray, IS_CMAP;
	unsigned short width, height;
	unsigned char *COLMAP; //possible colormap
	unsigned char R, G, B, A;
	enum {TGA_NO_DATA=0, TGA_UNC_CMAP, TGA_UNC_TRUE, TGA_UNC_GRAY,
				TGA_RLE_CMAP=9, TGA_RLE_TRUE, TGA_RLE_GRAY} TGA_TYPE;
	void getColor(unsigned char* scan=NULL );
};

//---------------------------------------------------------------------------
// buffer save
class YAFRAYCORE_EXPORT outTga_t : public colorOutput_t
{
	public:
		outTga_t(int resx, int resy, const char *fname, bool sv_alpha=false);
		virtual bool putPixel(int x, int y, const color_t &c, 
				CFLOAT alpha=0,PFLOAT depth=0);
		void flush() { savetga(outfile.c_str()); }
		virtual ~outTga_t();
	protected:
		outTga_t(const outTga_t &o) {}; //forbidden
		bool savetga(const char* filename);
		bool save_alpha;
		unsigned char *data;
		unsigned char *alpha_buf;
		int sizex, sizey;
		std::string outfile;
};

YAFRAYCORE_EXPORT cBuffer_t* loadTGA(const char* filename, bool verify);

__END_YAFRAY

//---------------------------------------------------------------------------
#endif //__TARGAIO_H
