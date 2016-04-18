
#include "imagebackg.h"

using namespace std;

__BEGIN_YAFRAY

imageBackground_t::imageBackground_t(const char* fname, const std::string &intp,
				CFLOAT bri_adj, const matrix4x4_t &m, mappingType mt, bool prefilt)
{
	img = new textureImage_t(fname, intp);
	if (img->loadFailed()) {
		delete img;
		img = NULL;
	}
	mType = mt;
	brightness_scale = pow((CFLOAT)2.0, bri_adj);
	if ((img!=NULL) && prefilt) {
		if (mt==IBG_TUBE)
			cout << "[background_image]: Can't do prefilter for tube mapping yet\n";
		else
			img->preFilter((mt==IBG_SPHERE));
	}
	mtx = m;
}

imageBackground_t::~imageBackground_t()
{
	if (img) delete img;
	img = NULL;
}

color_t imageBackground_t::operator() (const vector3d_t &dir, renderState_t &state, bool filtered) const
{
	if (!img) return color_t(0.0);
	// transformed direction
	vector3d_t tdir = mtx*dir;
	if (filtered && img->has_SH()) return brightness_scale * img->getColorSH(tdir);
	PFLOAT u=0, v=0;
	if (mType==IBG_ANGULAR) {
		// here upside down, which is strange, why is it different from HDRI backg.?
		angmap(tdir, u, v);
		v = 1.0-v;
	}
	else if (mType==IBG_TUBE)
		tubemap(tdir, u, v);
	else	// sphere default
		spheremap(tdir, u, v);
	return brightness_scale * img->getColor(point3d_t(u, v, 0));
}

background_t *imageBackground_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	string _filename, _mapping, _intp="bilinear";
	const string *filename=&_filename, *mapping=&_mapping, *intp=&_intp;
	CFLOAT expadj = 0;	// can be negative
	// power not used anymore, converted to exposure equivalent as in hdri
	if (params.getParam("power", expadj)) {
		cerr << "[background_image]: Warning, 'power' deprecated, use 'exposure_adjust' instead\n";
		if (expadj==0.0)
			expadj = -32;	// more than small enough
		else
			expadj = log(expadj)/log(2.0);
	}
	params.getParam("exposure_adjust", expadj);

	bool prefilt = false;
	params.getParam("prefilter", prefilt);

	// rotation matrix, mtx4 is used, but only need sub3x3 part
	matrix4x4_t mtx(1);
	params.getParam("m00", mtx[0][0]);
	params.getParam("m01", mtx[0][1]);
	params.getParam("m02", mtx[0][2]);
	params.getParam("m10", mtx[1][0]);
	params.getParam("m11", mtx[1][1]);
	params.getParam("m12", mtx[1][2]);
	params.getParam("m20", mtx[2][0]);
	params.getParam("m21", mtx[2][1]);
	params.getParam("m22", mtx[2][2]);
	// blender orientation correction
	mtx.scale(-1, 1, 1);

	// mapping
	params.getParam("mapping", mapping);
	mappingType mt = imageBackground_t::IBG_SPHERE;
	// also accept 'probe' as mapping keyword, to be compatible with old hdri background
	if ((*mapping=="angular") || (*mapping=="probe"))
		mt = imageBackground_t::IBG_ANGULAR;
	else if (*mapping=="tube")
		mt = imageBackground_t::IBG_TUBE;
	params.getParam("filename", filename);
	params.getParam("interpolate", intp);
	if (*filename=="") {
		cerr << "[background_image]: Error,  No filename given\n";
		return NULL;
	}
	return new imageBackground_t(filename->c_str(), *intp, expadj, mtx, mt, prefilt);
}

__END_YAFRAY
