#ifndef __EXR_IO_H
#define __EXR_IO_H

#include "color.h"
#include "buffer.h"
#include "output.h"

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT outEXR_t : public colorOutput_t
{
	public:
		outEXR_t(int resx, int resy, const char *fname, const std::string &exr_flags)
		{
			sizex = resx;
			sizey = resy;
			filename = fname;
			fbuf = new fcBuffer_t(resx, resy);
			zbuf = NULL;
			out_flags = exr_flags;
			// zbuf handled here, other flags are handled in saveEXR() in EXR_io.cc
			if (int(exr_flags.find("zbuf"))!=-1) zbuf = new gBuf_t<float, 1>(resx, resy);
		}
		virtual bool putPixel(int x, int y, const color_t &c, 
				CFLOAT alpha=0, PFLOAT depth=0)
		{
			(*fbuf)(x, y) << colorA_t(c, alpha);
			if (zbuf) *(*zbuf)(x, y) = depth;
			return true;
		}
		void flush() { SaveEXR(); }
		virtual ~outEXR_t()
		{
			if (zbuf) delete zbuf;
			zbuf = NULL;
			if (fbuf) delete fbuf;
			fbuf = NULL;
		}
	protected:
		outEXR_t(const outEXR_t &o) {}; //forbidden
		bool SaveEXR();
		fcBuffer_t* fbuf;
		gBuf_t<float, 1>* zbuf;
		int sizex, sizey;
		const char* filename;
		std::string out_flags;
};

YAFRAYCORE_EXPORT fcBuffer_t* loadEXR(const char* fname);

__END_YAFRAY

#endif // __EXR_OUT_H
