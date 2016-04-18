//-----------------------------------------------------------------------------------
// Parts of the actual HDR loader & saver code came from this page:
// http://radsite.lbl.gov/radiance/refer/Notes/picture_format.html
// describing the HDR format and code as used in Greg Ward's Radiance render package.

#include "HDR_io.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

//---------------------------------------------------------------------------
// CONVERSION FUNCTIONS RGBE <-> FLOAT COLOR
// could use new rgbe_t class, but for now use the old code

//rgbe -> float color
void RGBE2FLOAT(RGBE rgbe, fCOLOR fcol)
{
	if (rgbe[EXP]==0) {
		fcol[RED] = fcol[GRN] = fcol[BLU] = 0;
	}
	else {
		CFLOAT f = ldexp(1.0, rgbe[EXP]-(COLXS+8));
		fcol[RED] = (rgbe[RED]+.5)*f;
		fcol[GRN] = (rgbe[GRN]+.5)*f;
		fcol[BLU] = (rgbe[BLU]+.5)*f;
	}
}

//---------------------------------------------------------------------------

//float color -> rgbe
void FLOAT2RGBE(fCOLOR fcol, RGBE rgbe)
{
	CFLOAT d = (fcol[RED]>fcol[GRN])?fcol[RED]:fcol[GRN];
	if (fcol[BLU]>d) d = fcol[BLU];
	if (d <= 1e-32f)
		rgbe[RED] = rgbe[GRN] = rgbe[BLU] = rgbe[EXP] = 0;
	else {
		int e;
		d = frexp(d, &e) * 256.f / d;
		rgbe[RED] = (unsigned char)(fcol[RED] * d);
		rgbe[GRN] = (unsigned char)(fcol[GRN] * d);
		rgbe[BLU] = (unsigned char)(fcol[BLU] * d);
		rgbe[EXP] = (unsigned char)(e + COLXS);
	}
}

//---------------------------------------------------------------------------
// HDR write file

class HDRwrite_t
{
public:
	HDRwrite_t(FILE* f, int wd, int ht)
	{
		file = f;
		width = wd;
		height = ht;
		rgbe_scan = new RGBE[width];
	}
	~HDRwrite_t()
	{
		if (rgbe_scan) delete[] rgbe_scan;
		rgbe_scan = NULL;
	}
	int fwritecolrs(float* fpscan);
private:
	FILE* file;
	int width, height;
	RGBE* rgbe_scan;
};

int HDRwrite_t::fwritecolrs(float* fpscan)
{
	int i, j, beg, c2, cnt=0;
	// convert scanline
	for (i=0;i<width;i++) {
		RGBE &hscan = rgbe_scan[i];
		float* scan = &fpscan[i<<2];
		CFLOAT d = std::max(scan[RED], std::max(scan[GRN], scan[BLU]));
		if (d <= 1e-32f)
			hscan[RED] = hscan[GRN] = hscan[BLU] = hscan[EXP] = 0;
		else {
			int e;
			d = frexp(d, &e) * 256.f / d;
			hscan[RED] = (unsigned char)(scan[RED] * d);
			hscan[GRN] = (unsigned char)(scan[GRN] * d);
			hscan[BLU] = (unsigned char)(scan[BLU] * d);
			hscan[EXP] = (unsigned char)(e + COLXS);
		}
	}
	if ((width < MINELEN) | (width > MAXELEN))	// OOBs, write out flat
					return (fwrite((char *)rgbe_scan, sizeof(RGBE), width, file) - width);
	// put magic header
	putc(2, file);
	putc(2, file);
	putc((unsigned char)(width>>8), file);
	putc((unsigned char)(width&255), file);
	// put components seperately
	for (i=0;i<4;i++) {
		for (j=0;j<width;j+=cnt) {	// find next run
			for (beg=j;beg<width;beg+=cnt) {
				for (cnt=1;(cnt<127) && ((beg+cnt)<width) && (rgbe_scan[beg+cnt][i] == rgbe_scan[beg][i]); cnt++);
				if (cnt>=MINRUN) break;   // long enough
			}
			if (((beg-j)>1) && ((beg-j) < MINRUN)) {
				c2 = j+1;
				while (rgbe_scan[c2++][i] == rgbe_scan[j][i])
					if (c2 == beg) {        // short run
						putc((unsigned char)(128+beg-j), file);
						putc((unsigned char)(rgbe_scan[j][i]), file);
						j = beg;
						break;
					}
			}
			while (j < beg) {     // write out non-run
				if ((c2 = beg-j) > 128) c2 = 128;
				putc((unsigned char)(c2), file);
				while (c2--) putc(rgbe_scan[j++][i], file);
			}
			if (cnt >= MINRUN) {      // write out run
				putc((unsigned char)(128+cnt), file);
				putc(rgbe_scan[beg][i], file);
			}
			else cnt = 0;
		}
	}
	return(ferror(file) ? -1 : 0);
}

bool outHDR_t::saveHDR()
{
	if (fbuf==NULL) return false;
	int width = fbuf->resx();
	int height = fbuf->resy();
	FILE* file = fopen(filename, "wb");
	fprintf(file, "#?RADIANCE");
	fputc(10, file);
	fprintf(file, "# %s", "Created with YafRay");
	fputc(10, file);
	fprintf(file, "FORMAT=32-bit_rle_rgbe");
	fputc(10, file);
	fprintf(file, "EXPOSURE=%25.13f", 1.0);
	fputc(10, file);
	fputc(10, file);
	fprintf(file, "-Y %d +X %d", height, width);
	fputc(10, file);
	HDRwrite_t hdrout(file, width, height);
	for (int y=0;y<height;y++) {
		if (hdrout.fwritecolrs((*fbuf)(0,y)) < 0) {        // error
			fclose(file);
			return false;
		}
	}
	fclose(file);
	return true;
}

//---------------------------------------------------------------------------
// START OF HDR LOADER

// loads HDR, converts to float image or stores directly as RGBE image
bool HDRimage_t::LoadHDR(const char* filename, formatType ft)
{
	file = fopen(filename, "rb");
	if (file==NULL) return false;
	if (!CheckHDR()) {
		fclose(file);
		return false;
	}
	bool ok;
	if (ft==HDR_FLOAT)
		ok = radiance2fp();
	else
		ok = radiance2rgbe();
	fclose(file);
	EXPadjust = 0;
	return ok;
}


// check header to see if this is really a HDR file
// if so get the resolution information
bool HDRimage_t::CheckHDR()
{
	int wd, ht;
	char cs[256], st1[80], st2[80];
	bool resok = false, HDRok1 = false, HDRok2 = false;
	while (!feof(file) && !resok) {
		fgets(cs, 255, file);
		if (strstr(cs, "#?RADIANCE")) HDRok1 = true;
		if (strstr(cs, "32-bit_rle_rgbe")) HDRok2 = true;
		if (HDRok1 && HDRok2) {
			// stupid me... better only continue looking for file info when we are sure it's an hdr file...
			if (strcmp(cs, "\n") == 0) {
				// empty line found, next is resolution info, format: -Y N +X N
				// directly followed by data
				fgets(cs, 255, file);
				if (sscanf(cs, "%s %d %s %d", st1, &ht, st2, &wd) != 4) {
					// scan error, not all args or eof
					HDRok1 = HDRok2 = false;
					break;
				}
				// test correct format, also don't accept negative x/y value's
				if (((st1[0]!='-') && (st1[0]!='+')) ||
						((st2[0]!='-') && (st2[0]!='+')) ||
						((st1[1]!='X') && (st1[1]!='Y')) ||
						((st2[1]!='X') && (st2[1]!='Y')) ||
						(wd<0) || (ht<0))
				{
					HDRok1 = HDRok2 = false;
					break;
				}
				// the only(?) errors still possible at this point
				// would either be out of range XY values (can throw bad_alloc, terminates),
				// and/or a corrupt file which is handled.
				resok = true;
				xmax = wd;
				ymax = ht;
				break;
			}
		}
	}
	return (HDRok1 && HDRok2 && resok);
}

// free any allocated images/buffers before allocating new memory
void HDRimage_t::freeBuffers()
{
	if (fRGB) {
		delete[] fRGB;
		fRGB = NULL;
	}
	if (rgbe_scan) {
	delete[] rgbe_scan;
	rgbe_scan = NULL;
	}
	if (RGBE_img) {
		delete[] RGBE_img;
		RGBE_img = NULL;
	}
}

// convert radiance hdr to float image
bool HDRimage_t::radiance2fp()
{
	RGBE *sline;
	int x,y,yx;
	freeBuffers();
	sline = new RGBE[xmax];
	//ouch!, fRGB can be HUGE! (stpeters=1500x1500 fRGB = 25.75MB!)
	fRGB = new fCOLOR[xmax*ymax];
	for (y=ymax-1;y>=0;y--) {
		yx = y*xmax;
		if (!freadcolrs(sline)) {
			std::cout << "Error while reading file\n";
			return false;
		}
		for (x=0;x<xmax;x++)
			RGBE2FLOAT(sline[x], fRGB[x+yx]);
	}
	delete[] sline;
	return true;
}


// directly reads in the image as RGBE, only decodes if RLE used
bool HDRimage_t::radiance2rgbe()
{
	freeBuffers();
	RGBE_img = new RGBE[xmax*ymax];
	for (int y=ymax-1;y>=0;y--) {
		if (!freadcolrs(&RGBE_img[y*xmax])) {
			std::cout << "Error while reading file\n";
			return false;
		}
	}
	return true;
}


// read and possibly RLE decode a rgbe scanline
bool HDRimage_t::freadcolrs(RGBE *scan)
{
	int i,j,code,val;
	if ((xmax < MINELEN) | (xmax > MAXELEN)) return oldreadcolrs(scan);
	if ((i = getc(file)) == EOF) return false;
	if (i != 2) {
		ungetc(i, file);
		return oldreadcolrs(scan);
	}
	scan[0][GRN] = (unsigned char)getc(file);
	scan[0][BLU] = (unsigned char)getc(file);
	if ((i = getc(file)) == EOF) return false;
	if (((scan[0][BLU] << 8) | i) != xmax) return false;
	for (i=0;i<4;i++)
		for (j=0;j<xmax;) {
			if ((code = getc(file)) == EOF) return false;
			if (code > 128) {
				code &= 127;
				val = getc(file);
				while (code--)
					scan[j++][i] = (unsigned char)val;
			}
			else
				while (code--)
					scan[j++][i] = (unsigned char)getc(file);
		}
	return feof(file) ? false : true;
}


// old format
bool HDRimage_t::oldreadcolrs(RGBE *scan)
{
	int i, rshift = 0, len = xmax;
	while (len > 0) {
		scan[0][RED] = (unsigned char)getc(file);
		scan[0][GRN] = (unsigned char)getc(file);
		scan[0][BLU] = (unsigned char)getc(file);
		scan[0][EXP] = (unsigned char)getc(file);
		if (feof(file) || ferror(file)) return false;
		if (scan[0][RED] == 1 && scan[0][GRN] == 1 && scan[0][BLU] == 1) {
			for (i=scan[0][EXP]<<rshift;i>0;i--) {
				copy_rgbe(scan[-1], scan[0]);
				scan++;
				len--;
			}
			rshift += 8;
		}
		else {
			scan++;
			len--;
			rshift = 0;
		}
	}
	return true;
}

// END OF HDR LOADER

//---------------------------------------------------------------------------
// bilinear interpolation of float HDR image at coords (u, v)

color_t HDRimage_t::BilerpSample(GFLOAT u, GFLOAT v)
{
	GFLOAT xf = u * GFLOAT(xmax-1);
	GFLOAT yf = v * GFLOAT(ymax-1);
	GFLOAT dx=xf-floor(xf), dy=yf-floor(yf);
	GFLOAT w0=(1-dx)*(1-dy), w1=(1-dx)*dy, w2=dx*(1-dy), w3=dx*dy;
	int x2, y2, x1 = int(xf), y1 = int(yf);
	// reject outside, return black
	if ((x1<0) || (x1>=xmax) || (y1<0) || (y1>=ymax)) return color_t(0.0);
	if ((x2 = x1+1) >= xmax) x2 = xmax-1;
	if ((y2 = y1+1) >= ymax) y2 = ymax-1;
	fCOLOR k1, k2, k3, k4;
	if (RGBE_img) {
		RGBE2FLOAT(RGBE_img[x1 + y1*xmax], k1);
		RGBE2FLOAT(RGBE_img[x2 + y1*xmax], k2);
		RGBE2FLOAT(RGBE_img[x1 + y2*xmax], k3);
		RGBE2FLOAT(RGBE_img[x2 + y2*xmax], k4);
	}
	else {
		copy_fcol(fRGB[x1 + y1*xmax], k1);
		copy_fcol(fRGB[x2 + y1*xmax], k2);
		copy_fcol(fRGB[x1 + y2*xmax], k3);
		copy_fcol(fRGB[x2 + y2*xmax], k4);
	}
	return EXPadjust * color_t(w0*k1[RED] + w1*k3[RED] + w2*k2[RED] + w3*k4[RED],
														w0*k1[GRN] + w1*k3[GRN] + w2*k2[GRN] + w3*k4[GRN],
														w0*k1[BLU] + w1*k3[BLU] + w2*k2[BLU] + w3*k4[BLU]);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// for HDR as texture, returns float buffer
fcBuffer_t* loadHDR(const char* filename)
{
	HDRimage_t hdrimg;
	if (hdrimg.LoadHDR(filename, HDRimage_t::HDR_RGBE)) {
		// copy to float buffer, upside down
		int mx=hdrimg.getWidth(), my=hdrimg.getHeight();
		fcBuffer_t* fbuf = new fcBuffer_t(mx, my);
		float* fbufp = (*fbuf)(0, 0);
		RGBE* rgbep = hdrimg.getRGBEData();
		fCOLOR fcol;
		for (int y=0;y<my;y++) {
			for (int x=0;x<mx;x++) {
				RGBE2FLOAT(rgbep[((my-1)-y)*mx + x], fcol);
				*fbufp++ = fcol[RED];
				*fbufp++ = fcol[GRN];
				*fbufp++ = fcol[BLU];
				*fbufp++ = 1.f;	// alpha not in hdr
			}
		}
		return fbuf;
	}
	return NULL;
}

__END_YAFRAY
