#ifndef __PATHTOOLS_H
#define __PATHTOOLS_H

#include "photon.h"

__BEGIN_YAFRAY

class hemiSampler_t : public context_t::destructible
{
	public:
		virtual ~hemiSampler_t() {};
		
		virtual void samplingFrom(renderState_t &state,const point3d_t &P,
				const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv)=0;
		
		virtual vector3d_t nextDirection(const point3d_t &P,const vector3d_t &N,
				const vector3d_t &Ru,const vector3d_t &Rv,int cursam,int curlev,
				color_t &raycolor)=0;
		virtual CFLOAT multiplier()const=0;
		virtual void reset()=0;
};

class haltonSampler_t : public hemiSampler_t
{
	public:
		haltonSampler_t(int d,int samples);
		virtual ~haltonSampler_t();
		
		virtual void samplingFrom(renderState_t &state,const point3d_t &P,
				const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv);
		
		virtual vector3d_t nextDirection(const point3d_t &P,const vector3d_t &N,
				const vector3d_t &Ru,const vector3d_t &Rv,int cursam,int curlev,
				color_t &raycolor);
		virtual CFLOAT multiplier()const {return 1.0/(PFLOAT)(taken+1);};
		virtual void reset() {taken=0;};
		
	protected:
		int taken;
		Halton* HSEQ;
};

class randomSampler_t : public hemiSampler_t
{
	public:
		randomSampler_t(int samples);
		virtual ~randomSampler_t();
		
		virtual void samplingFrom(renderState_t &state,const point3d_t &P,
				const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv);
		
		virtual vector3d_t nextDirection(const point3d_t &P,const vector3d_t &N,
				const vector3d_t &Ru,const vector3d_t &Rv,int cursam,int curlev,
				color_t &raycolor);
		virtual CFLOAT multiplier()const {return 1.0/(PFLOAT)(taken+1);};
		virtual void reset() {taken=0;};
		
	protected:
		int taken;
		int grid;	// number of samples, sqrt of samples
		PFLOAT gridiv;	// reciprocal of gridside & samples
};

class photonSampler_t : public hemiSampler_t
{
	public:
		photonSampler_t(int samples,int depth,const globalPhotonMap_t &map,int grid=36);
		virtual ~photonSampler_t();
		
		virtual void samplingFrom(renderState_t &state,const point3d_t &P,
				const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv);
		
		virtual vector3d_t nextDirection(const point3d_t &P,const vector3d_t &N,
				const vector3d_t &Ru,const vector3d_t &Rv,int cursam,int curlev,
				color_t &raycolor);
		virtual CFLOAT multiplier()const;
		virtual void reset() 
		{
			current[0]=0;
			current[1]=0;
			current[2]=0;
		};
		
	protected:

		std::pair<int,int> getCoords(const vector3d_t &v,const vector3d_t &N,
				const vector3d_t &Ru,const vector3d_t &Rv)const;
		CFLOAT giveMaxDiff(int i,int j)const;
		void nextSample()
		{
			current[2]++;
			if(current[2]==hits[current[0]][current[1]])
			{
				current[2]=0;
				current[1]++;
				if(current[1]==meridians)
				{
					current[1]=0;
					current[0]++;
					if(current[0]==paralels) current[0]=0;
				}
			}
		}
		
		int samples;
		const globalPhotonMap_t &photonmap;
		int paralels;
		int meridians;
		int search;
		int sectors;
		PFLOAT pdiv;
		PFLOAT mdiv;
		
		std::vector< std::vector<int> > hits;
		std::vector< std::vector<CFLOAT> > weight;
		std::vector< std::vector<color_t> > energy;
		std::vector< foundPhoton_t > found;

		PFLOAT radius;
		int taken;
		CFLOAT multi;

		int current[3];
		Halton* HSEQ;
};


__END_YAFRAY

#endif
