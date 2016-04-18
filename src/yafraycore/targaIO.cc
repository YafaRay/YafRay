//--------------------------------------------------------------------------------
// Targa image loader, loads colormap, 8 (grayscale), 15/16, 24, or 32 bit images.
//--------------------------------------------------------------------------------
#include "targaIO.h"
#include "vector3d.h"

//--------------------------------------------------------------------------------
// Save uncompressed 24 bit targa

using namespace std;

__BEGIN_YAFRAY

const unsigned char TGAHDR[12] = {0, 0, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0};

targaImg_t::targaImg_t()
{
	alpha_bits=0;  byte_per_pix=0;
	has_alpha=false;  isgray=false;
	IS_CMAP = false;
	COLMAP = NULL;
	fp = NULL;
}

targaImg_t::~targaImg_t()
{
	if (COLMAP) {
		delete[] COLMAP;
		COLMAP = NULL;
	}
	if (fp) fclose(fp);
}

//--------------------------------------------------------------------------------
// Output buffer

outTga_t::outTga_t(int resx, int resy, const char *fname, bool sv_alpha)
{
	unsigned int tsz = resx*resy;
	data = new unsigned char[tsz*3];
	if (data==NULL) {
		std::cout <<"Fatal error allocating memory in outTga_t\n";
		exit(1);
	}
	sizex = resx;
	sizey = resy;
	outfile = fname;

	alpha_buf = NULL;
	save_alpha = sv_alpha;
	if (save_alpha) {
		alpha_buf = new unsigned char[tsz];
		if (!alpha_buf) {
			std::cout << "Could not allocate memory for alpha buffer in outTga_t\n";
			exit(1);
		}
	}
}

bool outTga_t::putPixel(int x, int y, const color_t &c, 
		CFLOAT alpha,PFLOAT depth)
{
	unsigned int yx = sizex*y + x;
	(data+yx*3) << c;
	if (save_alpha)
		alpha_buf[yx] = (unsigned char)(255.0*((alpha<0)?0:((alpha>1)?1:alpha)));
	return true;
}

outTga_t::~outTga_t()
{
	if (data) {
		delete[] data;
		data = NULL;
	}
	if (alpha_buf) {
		delete[] alpha_buf;
		alpha_buf = NULL;
	}
}

bool outTga_t::savetga(const char* filename)
{
	// name is assigned by default
	cout << "Saving Targa file as " << filename << endl;

	FILE* fp;
	unsigned short w, h, x, y;
	unsigned char* yscan;
	unsigned char btsdesc[2];
	unsigned int dto;
	if (save_alpha) {
		btsdesc[0] = 0x20; // 32 bits
		btsdesc[1] = 0x28; // topleft / 8 bit alpha
	}
	else {
		btsdesc[0] = 0x18; // 24 bits
		btsdesc[1] = 0x20; // topleft / no alpha
	}
	w = (unsigned short)sizex;
	h = (unsigned short)sizey;
	fp = fopen(filename, "wb");
	if (fp == NULL)
		return false;
	else 
	{
		fwrite(&TGAHDR, 12, 1, fp);
		fputc(w, fp);
		fputc(w>>8, fp);
		fputc(h, fp);
		fputc(h>>8, fp);
		fwrite(&btsdesc, 2, 1, fp);
		for (y=0; y<h; y++) 
		{
			// swap R & B channels
			dto = y*w;
			yscan = &data[dto*3];
			for (x=0; x<w; x++, yscan+=3) 
			{
				fputc(*(yscan+2), fp);
				fputc(*(yscan+1), fp);
				fputc(*yscan,  fp);
				if (save_alpha) fputc(alpha_buf[dto+x], fp);
			}
		}
		fclose(fp);
		cout<<"OK"<<endl;
		return true;
	}
}

//--------------------------------------------------------------------------------
// Read targa

struct tga_footer_t
{
	unsigned int extensionAreaOffset;
	unsigned int developerDirectoryOffset;
	char signature[16];
	char dot;
	char null;
};

#define TGA_SIGNATURE "TRUEVISION-XFILE"

cBuffer_t* targaImg_t::Load(const char *fname, bool verify)
{
	// The disadvantage of standard targa images is that they have no ID
	// this means that basically any file could be accepted as tga.

	unsigned char header[18];
	unsigned char img_ori, IS_RLE;
	unsigned short x, y;
	unsigned int idlen, pt, scan_pt;

	unsigned short cmap_len; // colormap length
	unsigned char cmap_bits;

	fp = fopen(fname, "rb");
	if (fp==NULL) {
		err_str = "Cannot open file";
		return NULL;
	}
	else {
		if(verify) // check footer to ensure TGA v2.0 format, v1.0 cannot be identified unfortunately!
		{
			tga_footer_t footer;
			//check footer if is tga v2.0 format, otherwise have to reject file, since it could be anything...
			if ( fseek(fp, 0L - 18L, SEEK_END) || fread(&footer.signature[0], 18, 1, fp) != 1 )
			{
				err_str = "could not seek file footer!";
				fclose(fp);
				fp = NULL;
				return false;
			}
			if(strncmp(&footer.signature[0], "TRUEVISION-XFILE", 16) != 0)
			{
//				footer.signature[16] = '\0';
//				std::cout << &footer.signature[0] << "\n";
				err_str = "File is no v2.0 Targa file!";
				fclose(fp);
				fp = NULL;
				return false;
			}
			rewind(fp);
		}
			
		// read in header
		fread(&header, 1, 18, fp);

		IS_CMAP = ((header[2]==TGA_UNC_CMAP) || (header[2]==TGA_RLE_CMAP));
		if ((IS_CMAP) && (header[1]==0)) {
			err_str = "Colormap image without colormap??";
			fclose(fp);
			fp = NULL;
			return false;
		}

		cmap_bits = header[7];
		if ((IS_CMAP) && (cmap_bits!=15) && (cmap_bits!=16) &&
										(cmap_bits!=24) && (cmap_bits!=32))
		{
			err_str = "Unsupported colormap bitformat";
			fclose(fp);
			fp = NULL;
			return false;
		}

		if ((header[2]!=TGA_UNC_TRUE) && (header[2]!=TGA_UNC_GRAY) &&
				(header[2]!=TGA_RLE_TRUE) && (header[2]!=TGA_RLE_GRAY) && (!IS_CMAP))
		{
			// 'no-image-data' not supported, possibly other unknown type
			err_str = "Targa type not supported";
			fclose(fp);
			fp = NULL;
			return false;
		}

		isgray = ((header[2]==TGA_UNC_GRAY) | (header[2]==TGA_RLE_GRAY));
		IS_RLE = ((header[2]==TGA_RLE_TRUE) || (header[2]==TGA_RLE_GRAY) ||
							(header[2]==TGA_RLE_CMAP));

		// width & height (or any non-char) read as little endian
		width = (unsigned short)(header[12] + (header[13]<<8));
		height = (unsigned short)(header[14] + (header[15]<<8));
		byte_per_pix = (unsigned char)(header[16]>>3);
		alpha_bits = (unsigned char)(header[17] & 15);
		has_alpha = (alpha_bits!=0);
		img_ori = (unsigned char)((header[17] & 48)>>4);

		if (isgray) {
			if ((byte_per_pix!=1) && (byte_per_pix!=2)) {
				err_str = "Unsupported grayscale image format";
				fclose(fp);
				fp = NULL;
				return false;
			}
		}
		else if (IS_CMAP) {
			if (byte_per_pix>2) {
				// maybe this is actually correct, but ignore for now...
				err_str = "24/32 bit colormap index???";
				fclose(fp);
				fp = NULL;
				return false;
			}
		}
		else {
			if ((byte_per_pix<2) && (byte_per_pix>4)) {
				err_str = "Unsupported pixelformat, only accept 15/16-, 24- or 32-bit";
				fclose(fp);
				fp = NULL;
				return false;
			}
		}

		if (has_alpha) {
			if ((!((byte_per_pix==1) && (alpha_bits==8))) &&
					(!((byte_per_pix==2) && (alpha_bits==1))) &&
					(!((byte_per_pix==4) && (alpha_bits==8)))) {
				err_str = "Unsupported alpha format";
				fclose(fp);
				fp = NULL;
				return false;
			}
		}

		// read past any id
		idlen = (unsigned int)header[0];
		if (idlen) fseek(fp, idlen, SEEK_CUR);

		// read in colormap if needed
		if (IS_CMAP) {
			cmap_len = (unsigned short)(header[5] + (header[6]<<8));
			if (COLMAP) delete[] COLMAP;
			COLMAP = new unsigned char[cmap_len<<2]; //always expanded to 32 bit
			unsigned char * cpt = COLMAP;
			if (cmap_bits<=16) {
				// 15/16 bit
				for (x=0;x<cmap_len;x++) {
					unsigned char c1 = (unsigned char)fgetc(fp);
					unsigned char c2 = (unsigned char)fgetc(fp);
					// RGBA in order
					*cpt++ = (unsigned char)(((c2>>2)*255)/31);
					*cpt++ = (unsigned char)(((((c1 & 0xe0)>>5) + ((c2 & 3)<<3))*255)/31);
					*cpt++ = (unsigned char)(((c1 & 31)*255)/31);
					if (cmap_bits==16)
						*cpt++ = (unsigned char)((c2 & 128)*255);
					else
						*cpt++ = 0;
				}
			}
			else {
				// 24/32 bit
				for (x=0;x<cmap_len;x++) {
					*(cpt+2) = (unsigned char)fgetc(fp);
					*(cpt+1) = (unsigned char)fgetc(fp);
					*(cpt) = (unsigned char)fgetc(fp);
					if (cmap_bits==32)
						*(cpt+3) = (unsigned char)fgetc(fp);
					else
						*(cpt+3) = 0;
					cpt += 4;
				}
			}
		}

		unsigned int totsize = width*height*4;	//4, alpha not used (yet)
		cBuffer_t* imgdata = new cBuffer_t(width, height);
		unsigned char* datap = (*imgdata)(0, 0);

		pt = 0;	// image data pointer
		R = G = B = 0;
		A = 255;	// default alpha value (not used)
		if (IS_RLE) {
			// RUN LENGTH ENCODED
			while (pt<totsize) {
				if (feof(fp)) {
					fclose(fp);
					fp = NULL;
					err_str = "Corrupted or unexpected end of file";
					delete[] imgdata;
					imgdata = NULL;
					return NULL;
				}
				unsigned char repcnt = (unsigned char)fgetc(fp);
				bool rle_packet = ((repcnt & 0x80)!=0);
				repcnt = (unsigned char)((repcnt & 0x7f)+1);
				if (rle_packet) {
					getColor();
					for (x=0;x<(unsigned short)repcnt;x++) {
						datap[pt++] = R;
						datap[pt++] = G;
						datap[pt++] = B;
						datap[pt++] = A;
					}
				}
				else { // raw packet
					for (x=0;x<(unsigned short)repcnt;x++) {
						getColor();
						datap[pt++] = R;
						datap[pt++] = G;
						datap[pt++] = B;
						datap[pt++] = A;
					}
				}
			}
		}
		else {
			// UNCOMPRESSED IMAGE
			unsigned int scan_size = width*byte_per_pix;
			unsigned char* scanline = new unsigned char[scan_size];
			for (y=0;y<height;y++) {
				unsigned int realsize = fread(scanline, 1, scan_size, fp);
				if ((realsize!=scan_size) || (feof(fp))) {
					delete[] scanline;  scanline=NULL;
					delete[] imgdata;  imgdata=NULL;
					fclose(fp);
					fp = NULL;
					err_str = "Corrupted or unexpected end of file";
					return NULL;
				}
				// convert, swap R & B
				for (x=0, scan_pt=0;x<width;x++, scan_pt+=byte_per_pix) {
					getColor(&scanline[scan_pt]);
					datap[pt++] = R;
					datap[pt++] = G;
					datap[pt++] = B;
					datap[pt++] = A;
				}
			}
			delete[] scanline;
		}

		fclose(fp);
		fp = NULL;

		// flip data if needed
		const int imgbp = 4;	//cbuffer byte per pix.
		unsigned short ofy = (unsigned short)(width*imgbp);	// row byte offset
		if ((img_ori & 2)==0) {
			unsigned char *bot, *top, tmp;
			for (y=0;y<(height>>1);y++) {
				top = (*imgdata)(0, y);
				bot = (*imgdata)(0, (height-1)-y);
				for (x=0;x<width*imgbp;x++, top++, bot++) {
					tmp = *top;
					*top = *bot;
					*bot = tmp;
				}
			}
		}
		if (img_ori & 1) {
			unsigned char *lt, *rt, tmp;
			for (y=0;y<height;y++) {
				lt = (*imgdata)(y, 0);
				rt = lt + (ofy-imgbp);
				for (x=0;x<(width>>1);x++, rt-=imgbp*2) {
					tmp=*lt;  *lt++=*rt; *rt++=tmp;
					tmp=*lt;  *lt++=*rt; *rt++=tmp;
					tmp=*lt;  *lt++=*rt; *rt++=tmp;
					tmp=*lt;  *lt++=*rt; *rt++=tmp;
				}
			}
		}

		err_str = "No errors";
		return imgdata;
	}

}

//--------------------------------------------------------------------------------
// pixel access
/*
color_t targaImg_t::operator() (int x, int y) const
{
	if (imgdata==NULL) return color_t(0.0);
	if ((x<0) || (x>=width) || (y<0) || (y>=height)) return color_t(0.0);
	color_t col;
	(*imgdata)(x, y) >> col;
	return col;
}
*/

void targaImg_t::getColor(unsigned char* scan)
{
	unsigned char c1=0, c2=0;
	unsigned short cmap_idx=0;
	if (byte_per_pix==1) {
		if (scan)
			R = G = B = scan[0];
		else
			R = G = B = (unsigned char)fgetc(fp);
		// a bit strange this, but works..
		if (IS_CMAP) cmap_idx = (unsigned short)(R<<2);
	}
	else {
		if (byte_per_pix==2) {
			// 16 bit color
			if (scan) {
				c1 = scan[0];
				c2 = scan[1];
			}
			else {
				c1 = (unsigned char)fgetc(fp);
				c2 = (unsigned char)fgetc(fp);
			}
			if (IS_CMAP)
				cmap_idx = (unsigned short)(c1 + (c2<<8));
			else {
				B = (unsigned char)(((c1 & 31)*255)/31);
				G = (unsigned char)(((((c1 & 0xe0)>>5) + ((c2 & 3)<<3))*255)/31);
				R = (unsigned char)(((c2>>2)*255)/31);
			}
		}
		else {
			if (scan) {
				B = scan[0];
				G = scan[1];
				R = scan[2];
			}
			else {
				B = (unsigned char)fgetc(fp);
				G = (unsigned char)fgetc(fp);
				R = (unsigned char)fgetc(fp);
			}
		}
	}
	if (IS_CMAP) {
		// get all colors from colormap
		R = COLMAP[cmap_idx++];
		G = COLMAP[cmap_idx++];
		B = COLMAP[cmap_idx++];
		A = COLMAP[cmap_idx];
	}
	else {
		// get alpha value, for 16 bit case, 'hidden' in c2
		if (has_alpha) {
			if (byte_per_pix==2)
				A = (unsigned char)((c2 & 128)*255);
			else {
				if (scan) {
					if (byte_per_pix==1) A=scan[0]; else A=scan[3];
				}
				else {
					if (byte_per_pix==1) A=R; else A=(unsigned char)fgetc(fp);
				}
			}
		}
	}
}

cBuffer_t* loadTGA(const char* filename, bool verify)
{
	targaImg_t timg;
	cBuffer_t* res = timg.Load(filename, verify);
	if(!res) std::cout << timg.getErrorString();
	return  res;
}

__END_YAFRAY
