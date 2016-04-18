#ifndef __INTERFACE_H
#define __INTERFACE_H

#include<vector>
#include<string>
#include<list>
#include<map>

#include "params.h"
#include "output.h"

__BEGIN_YAFRAY

class yafrayInterface_t : public renderEnvironment_t
{
	public:
		virtual void transformPush(float *m)=0;
		virtual void transformPop()=0;
		virtual void addObject_trimesh(const std::string &name,
				std::vector<point3d_t> &verts, const std::vector<int> &faces,
				std::vector<GFLOAT> &uvcoords, std::vector<CFLOAT> &vcol,
				const std::vector<std::string> &shaders, const std::vector<int> &faceshader,
				float sm_angle, bool castShadows, bool useR, bool receiveR, bool caus, bool has_orco,
				const color_t &caus_rcolor, const color_t &caus_tcolor, float caus_IOR)=0;

		virtual void addObject_reference(const std::string &name,const std::string &original)=0;
		// lights
		virtual void addLight(paramMap_t &p)=0;
    // textures
		virtual void addTexture(paramMap_t &p)=0;
		// shaders
		virtual void addShader(paramMap_t &p,std::list<paramMap_t> &modulators)=0;
		// filters
		virtual void addFilter(paramMap_t &p)=0;
		// backgrounds
		virtual void addBackground(paramMap_t &p)=0;
		//camera
    virtual void addCamera(paramMap_t &p)=0;
		//render
		virtual void render(paramMap_t &p)=0;
		//render
		virtual void render(paramMap_t &p,colorOutput_t &output)=0;

		virtual void clear()=0;
		
		virtual ~yafrayInterface_t() {};
};
__END_YAFRAY

#endif
