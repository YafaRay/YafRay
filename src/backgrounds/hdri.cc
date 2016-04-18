// can be totally replaced by imagebackground, now has exact same functionality for any imagetype
// kept for compatibility

#include "hdri.h"

using namespace std;

__BEGIN_YAFRAY

HDRI_Background_t::HDRI_Background_t(const char* fname, GFLOAT expadj, bool mp)
{
	img = new HDRimage_t();
	if (!img->LoadHDR(fname, HDRimage_t::HDR_RGBE)) {
		cout << "Error, HDRI_Background_t(): could not load " << fname << endl;
		delete img;
		img = NULL;
	}
	else {
		img->setExposureAdjust(expadj);
		cout << "HDR image " << fname << " load ok.\n";
		mapProbe = mp;
	}
}

HDRI_Background_t::~HDRI_Background_t()
{
	if (img) {
		delete img;
		img = NULL;
	}
}

// convert direction to uv and get color from HDR image
color_t HDRI_Background_t::operator() (const vector3d_t &dir, renderState_t &state, bool filtered) const
{
	if (img==NULL) return color_t(0.0);
	PFLOAT u=0, v=0;
	// need transform, currently y & z swap to conform to Blender axes
	if (mapProbe)
		angmap(dir, u, v);
	else {
		// currently only other possible type is sphere
		spheremap(dir, u, v);
		// v is upside down in this case
		v = 1.0-v;
	}
	return img->BilerpSample(u, v);
}

background_t *HDRI_Background_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	string _filename;
	GFLOAT expadj = 0;	// can be negative
	string _mapping = "probe";
	
	// image background can now be used instead for all image types
	cerr << "background type 'HDRI' deprecated, use type 'image' instead\n";

	const string *filename=&_filename,*mapping=&_mapping; // ask jandro about msvc hell

	params.getParam("exposure_adjust", expadj);
	params.getParam("filename", filename);
	params.getParam("mapping", mapping);
	// two mapping types only at the moment, probe or sphere
	bool mapprobe = (*mapping=="probe");
	if (*filename=="") {
		cerr<< "(background_HDRI) Error,  No filename given\n";
		return NULL;
	}
	return new HDRI_Background_t(filename->c_str(), expadj, mapprobe);
}

extern "C"
{

YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("HDRI",HDRI_Background_t::factory);

	std::cout<<"Registered HDRI background\n";
}

}
__END_YAFRAY
