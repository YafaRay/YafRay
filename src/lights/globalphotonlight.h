#ifndef __GLOBALPHOTONLIGHT_H
#define __GLOBALPHOTONLIGHT_H

#include "light.h"
#include "photon.h"

__BEGIN_YAFRAY

struct fPoint_t
{
	PFLOAT x,y,weight;
};

class globalPhotonLight_t: public light_t
{
	public:
		globalPhotonLight_t(PFLOAT r,int md,int mcd,int p,int se):
			hash(r/sqrt((PFLOAT)se),500000),
			photonMap(new globalPhotonMap_t(r)),
			irradiance(new globalPhotonMap_t(r)),
			maxdepth(md),maxcdepth(mcd),numPhotons(p),search(se) {};
		virtual ~globalPhotonLight_t() {delete photonMap;delete irradiance;};
		
		virtual color_t illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
															const vector3d_t &eye)const {return color_t(0,0,0);};

		virtual point3d_t position()const {return point3d_t(0,0,0);};

		virtual void init(scene_t &scene);
		
		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
		
		struct compPhoton_t
		{
			storedPhoton_t photon;
			vector3d_t N;
			color_t irr;
		};
		typedef hash3d_t<compPhoton_t> irHash_t;
	protected:
		
		void shoot(runningPhoton_t &photon,const vector3d_t &dir,int depth,int cdepth,
				bool storeFirst,scene_t &scene);
		void storeInHash(const runningPhoton_t &p,const vector3d_t &N);
		void setIrradiance(compPhoton_t &p);
		void computeIrradiances();

		hash3d_t<compPhoton_t> hash;
		globalPhotonMap_t *photonMap;
		globalPhotonMap_t *irradiance;
		int maxdepth,maxcdepth,numPhotons,search;
		std::vector< foundPhoton_t > found;
		std::vector<fPoint_t> points;
		renderState_t nullstate;
		PFLOAT radius;
};

__END_YAFRAY
#endif
