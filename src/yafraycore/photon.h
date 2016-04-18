#ifndef __PHOTON_H
#define __PHOTON_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "object3d.h"
#include "hash3d.h"
#include "params.h"
#include "scene.h"

__BEGIN_YAFRAY


class dirConverter_t
{
	public:
		dirConverter_t();

		vector3d_t convert(unsigned char theta,unsigned char phi)
		{
			return vector3d_t(sintheta[theta]*cosphi[phi],
												sintheta[theta]*sinphi[phi],
												costheta[theta]);
		}
		std::pair<unsigned char,unsigned char> convert(const vector3d_t &dir)
		{
			int t=(int)(acos(dir.z)*(255.0/M_PI));
			int p=(int)(atan2(dir.y,dir.x)*(256.0/(2.0*M_PI)));
			if(t>254) t=254;
			else if(t<0) t=0;
			if(p>255) p=255;
			else if(p<0) p+=256;
			return std::pair<unsigned char,unsigned char>(t,p);
		}
		
	protected:
		PFLOAT cosphi[256];
		PFLOAT sinphi[256];
		PFLOAT costheta[255];
		PFLOAT sintheta[255];
};

extern dirConverter_t dirconverter;

class storedPhoton_t;

class YAFRAYCORE_EXPORT runningPhoton_t
{
	friend class storedPhoton_t;
	public:
		runningPhoton_t(const color_t &color,const point3d_t &_pos)
		{pos=_pos;c=color;};
		const point3d_t & position()const {return pos;};
		void position(const point3d_t &_pos,PFLOAT bias=0);
		point3d_t lastPosition()const {return lastpos;};
		void filter(const color_t & color) {c=c*color;};
		const runningPhoton_t & operator = ( const runningPhoton_t & photon)
			{pos=photon.pos;lastpos=photon.lastpos;c=photon.c;return *this;};
		const color_t & color() {return c;};
		void color(const color_t &col) {c=col;};
	protected:
		point3d_t pos;
		point3d_t lastpos;
		color_t c;
};

class globalPhotonMap_t;

class YAFRAYCORE_EXPORT storedPhoton_t
{
	friend class globalPhotonMap_t;
	public:
		storedPhoton_t() {theta=255;};
		storedPhoton_t(const vector3d_t &d,const point3d_t &p,
				const color_t &col)
		{
			direction(d);
			pos=p;
			c=col;};
		storedPhoton_t(const runningPhoton_t &p)
		{
			pos=p.pos;
			c=p.c;
			vector3d_t dir=p.lastpos-p.pos;
			dir.normalize();
			direction(dir);
		};
		const point3d_t & position()const {return pos;};
		const color_t color()const {return c;};
		void color(const color_t &col) {c=col;};
		const vector3d_t direction()const 
		{
			if(theta==255) return vector3d_t(0,0,0);
			else return dirconverter.convert(theta,phi);
		};
		void direction(const vector3d_t &dir)
		{
			if(dir.null()) theta=255;
			else
			{
				std::pair<unsigned char,unsigned char> cd=dirconverter.convert(dir);
				theta=cd.first;
				phi=cd.second;
			}
		}
	protected:
		point3d_t pos;
		rgbe_t c;
		unsigned char theta,phi;
};

struct foundPhoton_t
{
	const storedPhoton_t *photon;
	PFLOAT dis;
};


class YAFRAYCORE_EXPORT globalPhotonMap_t
{
	public:

		typedef std::vector<storedPhoton_t>::iterator iterator;
		typedef std::vector<storedPhoton_t>::const_iterator const_iterator;
		
		globalPhotonMap_t(PFLOAT r);
		~globalPhotonMap_t();

		//void store(const runningPhoton_t &p,const vector3d_t &N);
		void store(const storedPhoton_t &p);
		void buildTree();

		void gather(const point3d_t &P,const vector3d_t &N,
				std::vector<foundPhoton_t> &found,
				unsigned int K,PFLOAT &radius,PFLOAT mincos=0.0)const;

		int count()const {return photons.size();};
		PFLOAT getMaxRadius()const {return maxradius;};

		iterator begin() {return photons.begin();}; 
		const_iterator begin()const {return photons.begin();}; 
		iterator end() {return photons.end();}; 
		const_iterator end()const {return photons.end();}; 

	protected:
		globalPhotonMap_t(const globalPhotonMap_t &s) {}; //forbiden
		PFLOAT maxradius;
		//hash3d_t<storedPhoton_t> hash;
		std::vector<storedPhoton_t> photons;
		gBoundTreeNode_t<const storedPhoton_t *> *tree;
};


__END_YAFRAY

#endif
