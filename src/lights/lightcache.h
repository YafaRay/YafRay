#ifndef __LIGHTCACHE_H
#define __LIGHTCACHE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "hash3d.h"
#include "bound.h"
#include "ccthreads.h"

__BEGIN_YAFRAY

struct lightSample_t
{
	lightSample_t(const vector3d_t &n,const color_t &c, PFLOAT a,const point3d_t &p,
			const point3d_t &pp, PFLOAT m, PFLOAT prec,CFLOAT dev=1.0):
		N(n),color(c),adist(a),M(m),precision(prec),P(p),pP(pp),deval(false),devaluated(dev)
		{
			/*
			realPolar=pp;
			PFLOAT corr=cos(pp.z);
			if(corr>0) realPolar.y/=corr;
			*/
		};
	lightSample_t():N(0,0,0) {};

	vector3d_t N;
	color_t color,mixed;
	PFLOAT adist,M,precision;
	point3d_t P;
	point3d_t pP;
	//point3d_t realPolar;
	bool deval;
	CFLOAT devaluated;
};

struct foundSample_t
{
	const lightSample_t *S;
	PFLOAT dis;
	PFLOAT weight;
};

struct lightAccum_t
{
	lightAccum_t():valid(false),resample(true) {subdivision=1;};
	std::list<lightSample_t> radiance;
	int subdivision;
	bool valid,resample;
};

class lightCache_t
{
	public:
		lightCache_t(PFLOAT size):
			state(FILL),cache_size(size),hash(size,50000),tree(NULL),
			inserted(0) {};
		~lightCache_t() {if(state==USE) delete tree;};

		void setAspect(PFLOAT aspect) { ycorrection=1.0/aspect;};
		void startFill()
		{
			if(state!=FILL)
			{
				delete tree;
				tree=NULL;
				state=FILL;
			}
		}
		void startUse();

		typedef enum { FILL, USE } state_e;
		bool ready()const {return state==USE;};
		int size()const {return inserted;};
		
		struct iterator
		{
			iterator(hash3d_t<lightAccum_t> &h);
			void operator ++();
			void operator ++(int) { operator ++();};

			lightSample_t & operator * () {return *j;};

			hash3d_t<lightAccum_t>::iterator i,iend;
			std::list<lightSample_t>::iterator j,jend;
		};

		iterator begin() {return iterator(hash);};
		char *   end() {return NULL;} // Hack to keep speed and stl look in loops.
		void wait() {hash_mutex.wait();};
		void signal() {hash_mutex.signal();};

		bool enoughFor(const point3d_t &P,const vector3d_t &N,const renderState_t &state,
				CFLOAT (*W)(const lightSample_t &,const point3d_t &,const vector3d_t &,CFLOAT),
				CFLOAT wlimit);

		void insert(const point3d_t &P,const renderState_t &state,const lightSample_t &sample);

		CFLOAT gatherSamples(const point3d_t &P,const point3d_t &pP,
				const vector3d_t &N,std::vector<foundSample_t> &found,
				unsigned int K,PFLOAT &radius,PFLOAT maxradius,unsigned int minimun,
				CFLOAT (*W)(const lightSample_t &,const point3d_t &,
					const vector3d_t &,CFLOAT), PFLOAT wlimit)const;
		
		point3d_t toPolar(const point3d_t &p,const renderState_t &state)const
		{
			return point3d_t(state.screenpos.x,
					state.screenpos.y*ycorrection,log(state.traveled));
		};
		PFLOAT polarDist(const point3d_t &a,const point3d_t &b)const {return (a-b).length();};
	protected:
		state_e state;
		PFLOAT cache_size;
		yafthreads::mutex_t hash_mutex;
		hash3d_t<lightAccum_t> hash;
		gBoundTreeNode_t<const lightSample_t *> *tree;
		int inserted;
		PFLOAT ycorrection;
};

inline lightCache_t::iterator::iterator(hash3d_t<lightAccum_t> &h)
{
	i=h.begin();
	iend=h.end();
	if(i!=iend)
	{
		j=(*i).radiance.begin();
		jend=(*i).radiance.end();
	}
}

inline void lightCache_t::iterator::operator ++()
{
	j++;
	if(j==jend)
	{
		i++;
		if(i!=iend)
		{
			j=(*i).radiance.begin();
			jend=(*i).radiance.end();
		}
	}
}

inline bool operator != (lightCache_t::iterator &ite,char *) // WARNING : hack to speed up i!=cache.end()
{
	return ite.i!=ite.iend;
}

/*
inline point3d_t toPolar(const point3d_t &P,const scene_t &sc)
{
	vector3d_t v=P-sc.getCenterOfView();
	PFLOAT dxy=sqrt(v.x*v.x+v.y*v.y);
	PFLOAT x=v.length();
	v.x/=dxy;
	v.y/=dxy;
	v.z/=x;
	PFLOAT y=(v.y>0) ? acos(v.x) : -acos(v.x);
	PFLOAT z=asin(v.z);
	x=log(x);
	y*=cos(z);
	return point3d_t(x,y,z);
}

inline point3d_t toRealPolar(const point3d_t &P,const scene_t &sc)
{
	vector3d_t v=P-sc.getCenterOfView();
	PFLOAT dxy=sqrt(v.x*v.x+v.y*v.y);
	PFLOAT x=v.length();
	v.x/=dxy;
	v.y/=dxy;
	v.z/=x;
	PFLOAT y=(v.y>0) ? acos(v.x) : -acos(v.x);
	PFLOAT z=asin(v.z);
	x=log(x);
	return point3d_t(x,y,z);
}

PFLOAT polarDist(const point3d_t &a,const point3d_t &b);
*/

__END_YAFRAY

#endif
