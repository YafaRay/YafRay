/****************************************************************************
 *
 * 			photonlight.h: Photon (caustics) light api 
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *      
 */
#ifndef __PHOTONLIGHT_H
#define __PHOTONLIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "object3d.h"
#include "params.h"
#include "scene.h"
#include "light.h"
#include "mcqmc.h"
#include "hash3d.h"
#include<queue>

__BEGIN_YAFRAY

class photonMark_t;

class photon_t
{
	friend class photonMark_t;
	public:
		photon_t(const color_t &color,const point3d_t &_pos);
		const point3d_t & position()const {return pos;};
		void position(const point3d_t &_pos,PFLOAT bias);
		point3d_t lastPosition()const {return lastpos;};
		void filter(const color_t & color) {c=c*color;};
		const photon_t & operator = ( const photon_t & photon)
			{pos=photon.pos;lastpos=photon.lastpos;c=photon.c;return *this;};
		const color_t & color() {return c;};
		void color(const color_t &col) {c=col;};
	protected:
		point3d_t pos;
		point3d_t lastpos;
		color_t c;
};

class photonMark_t
{
	public:
		photonMark_t() {};
		photonMark_t(const vector3d_t &d,const point3d_t &p,
				const color_t &col)
		{dir=d;pos=p;c=col;};
		photonMark_t(const photon_t &p)
		{pos=p.pos;c=p.c;dir=p.lastpos-p.pos;dir.normalize();};
		const point3d_t & position()const {return pos;};
		const photonMark_t & operator = ( const photonMark_t & photon)
			{pos=photon.pos;c=photon.c;dir=photon.dir;return *this;};
		const color_t & color()const {return c;};
		void color(const color_t &col) {c=col;};
		const vector3d_t & direction()const {return dir;};
		void direction(const vector3d_t &d) {dir=d;};
	protected:
		vector3d_t dir;
		point3d_t pos;
		color_t c;
};

struct photoAccum_t
{
	photoAccum_t():dir(0,0,0),pos(0,0,0),color(0,0,0),count(0) {};
	vector3d_t dir;
	point3d_t pos;
	color_t color;
	GFLOAT count;
};


/*
#define SPLITX 0
#define SPLITY 1
#define SPLITZ 2

struct photonNode_t
{
	int divtype;
	bound_t bound;
	vector<photon_t *>::iterator begin,end;
	bool isLeaf()const {return begin!=end;};
	photonNode_t *left;
	photonNode_t *right;
	photonNode_t *parent;
};
*/
struct foundPhoton_t
{
	const photonMark_t *p;
	PFLOAT dis;
};

struct compareFound_f
{
	bool operator () (const foundPhoton_t &a,const foundPhoton_t &b)
	{
		return a.dis<b.dis;
	}
} cfound;


/*
class photonMap_t
{
	public:
		photonMap_t() {root=NULL;};
		~photonMap_t() {if(root!=NULL) delete root;};
		void load(vector<photon_t> &vp);

		void find(priority_queue<foundPhoton_t,vector<foundPhoton_t>,
				compareFound_f> &found,const point3d_t &P,unsigned int K)const;

	protected:
		photonNode_t *buildPhotonTree(vector<photon_t *>::iterator begin,
					vector<photon_t *>::iterator end);
		
		vector<photon_t *> lpho;
		photonNode_t *root;
};
*/


#define CAUSTIC 0
#define DIFFUSE 1

class photonLight_t : public  light_t
{
	public:
		photonLight_t(const point3d_t &f,const point3d_t _to,PFLOAT angle,
				const color_t &c,CFLOAT inte,int np,int search,int maxd=3,int mind=1,
				PFLOAT b=0.0001, PFLOAT disp=1.0,PFLOAT fr=-1,PFLOAT clus=1.0,
				int mode=CAUSTIC, bool useqmc=false);
		virtual color_t illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
															const vector3d_t &eye)const;
		virtual point3d_t position()const {return from;};
		virtual void init(scene_t &scene);
		virtual ~photonLight_t() 
		{
			if (tree!=NULL) delete tree;
			if (hash!=NULL) delete hash;
			if (HSEQ) { delete[] HSEQ;  HSEQ=NULL; }
		}
		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		void preGathering(photonMark_t &photon);
		void preGathering();
		void shoot_photon_caustic(scene_t &scene, photon_t &photon,
				const vector3d_t &dir, PFLOAT dis=0.0); 
		void shoot_photon_diffuse(scene_t &scene,photon_t &photon,const vector3d_t &dir
				,PFLOAT dis=0.0); 
		point3d_t from,to;
		color_t color;
		CFLOAT pow;
		unsigned int Np,K;
		unsigned int emitted, stored;
		int depth;
		int maxdepth;
		int mindepth;
		PFLOAT bias;
		PFLOAT angle_cos;
		PFLOAT dangle;
		PFLOAT dispersion;
		PFLOAT fixedRadius;
		PFLOAT randStep,cluster;
		int mode;
		std::vector<photonMark_t> photons;
		gBoundTreeNode_t<photonMark_t *> *tree;
		//hash3d_t<photonMark_t> *hash;
		hash3d_t<photoAccum_t> *hash;
		// qmc Halton sampling
		Halton* HSEQ;
		bool use_QMC;
		renderState_t nullstate;
};

inline CFLOAT filterGauss(const PFLOAT &x, const PFLOAT &limit)
{
		if(limit==0) return 1.0;
		const PFLOAT filter=1.0-(x*x)/(limit*limit);
		return filter*filter*2.0;// 15/8
}

inline CFLOAT filterCone(const PFLOAT &x, const PFLOAT &limit)
{
		if(limit==0) return 1.0;
		const PFLOAT filter=1.0-x/limit;
		return filter*2.0;
}

inline CFLOAT filterPara(const PFLOAT &x, const PFLOAT &limit)
{
		if(limit==0) return 1.0;
		PFLOAT filter=x/limit;
		filter*=filter;
		filter*=filter;
		filter=1.0-filter;
		return filter*5.0/4.0;
}

__END_YAFRAY
#endif
