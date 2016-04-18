#ifndef __CACHEPROXY_H
#define __CACHEPROXY_H

#include "lightcache.h"

__BEGIN_YAFRAY

struct proxyEntry_t
{
	point3d_t P;
	vector3d_t N;
	PFLOAT precision;
	std::vector<foundSample_t> found;
};

class cacheProxy_t : public context_t::destructible
{
	public:
		cacheProxy_t(lightCache_t &ca,const scene_t &sc,PFLOAT mradius );
		virtual ~cacheProxy_t() {};

		std::vector<foundSample_t> &
		gatherSamples(renderState_t &state,const point3d_t &P,const point3d_t &rP,
												const vector3d_t &N,int search,int minimun, 
												CFLOAT (*W)(const lightSample_t &,const point3d_t &,
												const vector3d_t &,CFLOAT), PFLOAT wlimit);
		void addSample(renderState_t &state,const lightSample_t &sample);
	protected:

		void newSearch(renderState_t &state,const point3d_t &P,const point3d_t &rP,const vector3d_t &N,
												int search,int minimun, CFLOAT (*W)(const lightSample_t &,
												const point3d_t &,const vector3d_t &,CFLOAT), PFLOAT wlimit,
												std::vector<foundSample_t> &found);
		/*
		void fakeSearch(const proxyEntry_t &e,const point3d_t &P,const point3d_t &rP,
										const vector3d_t &N,
										int search,int minimun,CFLOAT (*W)(const lightSample_t &,
										const point3d_t &,const vector3d_t &,CFLOAT), PFLOAT wlimit,
										std::vector<foundSample_t> &found);
										*/
		proxyEntry_t * findCompatible(int level,const point3d_t &P,const vector3d_t &N);
		void reset();
												
		lightCache_t &cache;
		const scene_t &scene;
		int pixelid;
		PFLOAT maxradius,radius;
		std::vector<std::list<proxyEntry_t> > entry;
		std::vector<lightSample_t> created;
		std::vector<foundSample_t> result;
};

__END_YAFRAY

#endif
