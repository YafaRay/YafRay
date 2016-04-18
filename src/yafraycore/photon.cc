
#include "photon.h"

#include<algorithm>

using namespace std;

__BEGIN_YAFRAY

dirConverter_t dirconverter;

dirConverter_t::dirConverter_t()
{
	for(int i=0;i<255;++i)
	{
		PFLOAT angle=(PFLOAT)i*(1.0/255.0)*M_PI;
		costheta[i]=cos(angle);
		sintheta[i]=sin(angle);
	}
	for(int i=0;i<256;++i)
	{
		PFLOAT angle=(PFLOAT)i*(1.0/256.0)*2.0*M_PI;
		cosphi[i]=cos(angle);
		sinphi[i]=sin(angle);
	}
}

void runningPhoton_t::position(const point3d_t &_pos,PFLOAT bias)
{
	PFLOAT ad=(pos-_pos).length();
	if(ad>bias)
	{
		lastpos=pos;
	}
	pos=_pos;
}


// BEGIN OF FUNCTIONS FOR THE TEMPLATED BINARY TREE BUILD FUNTION
//
// --------------------------------------------------------------

static bound_t global_photon_calc_bound(const vector<const storedPhoton_t *> &v)
{
	int size=v.size();
	if(size==0) return bound_t(point3d_t(),point3d_t());
	PFLOAT maxx,maxy,maxz,minx,miny,minz;
	maxx=minx=v[0]->position().x;
	maxy=miny=v[0]->position().y;
	maxz=minz=v[0]->position().z;
	for(int i=0;i<size;++i)
	{
		const point3d_t &p=v[i]->position();
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
	}
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

static bool global_photon_is_in_bound(const storedPhoton_t * const & p,bound_t &b)
{
	return b.includes(p->position());
}

static point3d_t global_photon_get_pos(const storedPhoton_t * const & p)
{
	return p->position();
}

// END OF FUNCTIONS FOR THE TEMPLATED BINARY TREE BUILD FUNTION
//
// --------------------------------------------------------------
/*
void globalPhotonMap_t::store(const runningPhoton_t &p,const vector3d_t &N) 
{
	photons.push_back(storedPhoton_t(p,N));
	
	storedPhoton_t nuevo(p,N);
  storedPhoton_t &A=hash.findBox(nuevo.position());
	if(A.dir.null())
	{
		A=nuevo;
		A.dir*=A.c.energy();
	}
	else if(nuevo.direction()*A.normal()>0.0)
	{
		A.dir+=nuevo.direction()*nuevo.color().energy();
		A.c+=nuevo.color();
	}
	
}
*/

globalPhotonMap_t::globalPhotonMap_t(PFLOAT r):maxradius(r),tree(NULL)
{
}

globalPhotonMap_t::~globalPhotonMap_t()
{
	if(tree) delete tree;
}

void globalPhotonMap_t::store(const storedPhoton_t &p)
{
	photons.push_back(p);
}

void globalPhotonMap_t::buildTree()
{
	/*
	photons.clear();
	photons.reserve(hash.numBoxes());
	int c=0;
	for(hash3d_t<storedPhoton_t>::iterator i=hash.begin();
			i!=hash.end();++i)
	{
		if((*i).dir.null()) {c++;continue;}
		photons.push_back(*i);
		photons.back().dir.normalize();
	}
	hash.clear();
*/
	vector<const storedPhoton_t *> lpho(photons.size());
	for(unsigned int i=0;i<photons.size();++i)
		lpho[i]=&photons[i];

	if(tree!=NULL) delete tree;
	tree=buildGenericTree(lpho,global_photon_calc_bound,global_photon_is_in_bound,
				global_photon_get_pos,8);

}

struct searchCircle_t
{
	searchCircle_t(const point3d_t &pp,PFLOAT r):point(pp),radius(r) {};
	point3d_t point;
	PFLOAT radius;
};


struct circleCross_f
{
	bool operator()(const searchCircle_t &c,const bound_t &b)const 
	{
		point3d_t a,g;
		b.get(a,g);
		a+=-c.radius;
		g+=c.radius;
		bound_t bt(a,g);
		return bt.includes(c.point);
	};
};


struct compareFound_f
{
	bool operator () (const foundPhoton_t &a,const foundPhoton_t &b)
	{
		return a.dis<b.dis;
	}
};

void globalPhotonMap_t::gather(const point3d_t &P,const vector3d_t &N,
		std::vector<foundPhoton_t> &found,
		unsigned int K,PFLOAT &radius,PFLOAT mincos)const
{
	foundPhoton_t temp;
	compareFound_f cfound;
	//found.reserve(K+1);
	unsigned int reached=0;
	while((reached<K) && (radius<=maxradius))
	{
		reached=0;
		//found.clear();
		found.resize(0);
		searchCircle_t circle(P,radius);
		for(gObjectIterator_t<const storedPhoton_t *,searchCircle_t,circleCross_f> 
				i(tree,circle);!i;++i)
		{
			vector3d_t sep=(*i)->position()-P;
			CFLOAT D=sep.length();
			if((D>radius) || (((*i)->direction()*N)<=mincos)) continue;
			reached++;
			temp.photon=*i;
			temp.dis=D;
			if((found.size()==K) && (temp.dis>found.front().dis)) continue;
			if(found.size()==K)
			{
				found.push_back(temp);
				push_heap(found.begin(),found.end(),cfound);
				pop_heap(found.begin(),found.end(),cfound);
				found.pop_back();
			}
			else
			{
				found.push_back(temp);
				push_heap(found.begin(),found.end(),cfound);
			}
		}
		if(reached<K) radius*=2;
	}
	if(reached>K)
	{
		PFLOAT f=(PFLOAT)K/(PFLOAT)reached;
		if(f<(0.7*0.7)) radius*=0.95;
	}
	if(radius>maxradius) radius=maxradius;
}

__END_YAFRAY
