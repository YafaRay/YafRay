
#include "lightcache.h"

#include<algorithm>
#include<vector>
using namespace std;

__BEGIN_YAFRAY


static bound_t path_calc_bound(const vector<const lightSample_t *> &v)
{
	int size=v.size();
	if(size==0) return bound_t(point3d_t(),point3d_t());
	PFLOAT maxx,maxy,maxz,minx,miny,minz;
	//maxx=minx=v[0]->realPolar.x;
	//maxy=miny=v[0]->realPolar.y;
	//maxz=minz=v[0]->realPolar.z;
	maxx=minx=v[0]->pP.x;
	maxy=miny=v[0]->pP.y;
	maxz=minz=v[0]->pP.z;
	for(int i=0;i<size;++i)
	{
		//const point3d_t &p=v[i]->realPolar;
		const point3d_t &p=v[i]->pP;
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
	}
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

static bool path_is_in_bound(const lightSample_t * const & p,bound_t &b)
{
	//return b.includes(p->realPolar);
	return b.includes(p->pP);
}

static point3d_t path_get_pos(const lightSample_t * const & p)
{
	//return p->realPolar;
	return p->pP;
}

void lightCache_t::startUse()
{
	if(state!=USE)
	{
		vector<const lightSample_t *> pointers;
		for(iterator i=begin();i!=end();++i) pointers.push_back(&(*i));
		tree=buildGenericTree(pointers,path_calc_bound,path_is_in_bound,
				      path_get_pos,1);
		state=USE;
	}
}

struct compareFound_f
{
	bool operator () (const foundSample_t &a,const foundSample_t &b)
	{
		return a.weight>b.weight;
	}
};

struct circle_t
{
	circle_t(const point3d_t &pp,PFLOAT r):point(pp),radius(r) {};
	point3d_t point;
	PFLOAT radius;
};


struct pointCross_f
{
	bool operator()(const circle_t &c,const bound_t &b)const 
	{
		bound_t temp=b;
		temp.grow(c.radius);
		return temp.includes(c.point);
		/*
		const point3d_t &p=c.point;
		PFLOAT cosa=cos(p.z);
		PFLOAT dis=(p.y-b.centerY())*cosa;
		PFLOAT dis2=(p.y-b.centerY() + 
									((p.y<0) ? 2*M_PI : -2*M_PI))*cosa;
		point3d_t temp(p.x,b.centerY()+dis,p.z);
		point3d_t temp2(p.x,b.centerY()+dis2,p.z),a,g;
		b.get(a,g);
		a+=-c.radius;
		g+=c.radius;
		bound_t bt(a,g);
		return bt.includes(temp) || bt.includes(temp2);
		*/
	};
};

/*
PFLOAT polarDist(const point3d_t &a,const point3d_t &b)
{
  PFLOAT raddif = b.x-a.x;
	PFLOAT temp=sin((a.y-b.y)*0.5);
	PFLOAT res = cos(a.z-b.z)-2*cos(a.z)*cos(b.z)*temp*temp;
	res = acos(res);
	return sqrt(res*res+raddif*raddif);
}
*/

CFLOAT lightCache_t::gatherSamples(const point3d_t &P,const point3d_t &pP,
		const vector3d_t &N,vector<foundSample_t> &found,unsigned int K,
		PFLOAT &radius,PFLOAT maxradius,unsigned int minimun,
		CFLOAT (*W)(const lightSample_t &,const point3d_t &,
			          const vector3d_t &,CFLOAT), PFLOAT wlimit)const
{
	if(state!=USE)
	{
		cout<<"Using unfinished cache"<<endl;
		return 0;
	}

	foundSample_t temp;
	compareFound_f cfound;
	found.reserve(K+1);
	CFLOAT best=0,adist=0;
	PFLOAT maxdist=0,lastradius=0;
	CFLOAT maxweight=wlimit*2.5,minweight=wlimit*0.6;
	unsigned int reached=0;
	bool repeat=true;
		
	found.clear();
	while(repeat)
	{
		circle_t circle(pP,radius);
		for(gObjectIterator_t<const lightSample_t *,circle_t,pointCross_f> 
				i(tree,circle);!i;++i)
		{
			//CFLOAT pD=polarDist(pP,(*i)->realPolar);
			CFLOAT pD=polarDist(pP,(*i)->pP);
			if(pD>=radius) continue;
			if(pD<lastradius) continue;
			reached++;
			temp.S=*i;
			temp.dis=pD;
			temp.weight=W(**i,P,N,maxweight);
			if(temp.weight>best) {best=temp.weight;adist=temp.S->adist;}
			unsigned int limit;
			if(temp.weight<=wlimit) 
				limit=minimun; 
			else 
			{
				if(pD>maxdist) maxdist=pD;
				limit=K;
			}
			if(!limit) continue;
			if((found.size()>=limit) && (temp.weight<found.front().weight)) continue;
			found.push_back(temp);
			push_heap(found.begin(),found.end(),cfound);
			if(found.size()>limit)
			{
				pop_heap(found.begin(),found.end(),cfound);
				found.pop_back();
			}
		}
		
		PFLOAT rrad=(found.empty() || (found.front().dis==0)) ? (0.0001*adist) : 
			radius*(found.front().S->P-P).length()/found.front().dis;
		if(rrad==0.0) rrad=1.0;
		repeat=( found.empty() || (((adist/rrad)>wlimit) && (reached<K)) || (best<=minweight)
				) && (radius<maxradius);
		if(repeat) 
		{
			lastradius=radius;
			radius*=2;
		}
		if(radius>maxradius) radius=maxradius;
	}
	if((maxdist>0) && ((maxdist/radius)<(1.0/sqrt(2.0)))) radius*=0.9;
	
	if(found.size())
		return found.front().weight;
	else
		return 0.0;
}

bool lightCache_t::enoughFor(const point3d_t &P,const vector3d_t &N,const renderState_t &state,
				CFLOAT (*W)(const lightSample_t &,const point3d_t &,const vector3d_t &,CFLOAT),
				CFLOAT wlimit)
{
	//point3d_t pP=toPolar(P,sc);
	point3d_t pP=toPolar(P,state);
	int cx,cy,cz;
	hash.getBox(pP,cx,cy,cz);
	/*
	PFLOAT corr=cos(pP.z);
	if(corr>0) pP.y/=corr; // realPolar
	*/
	lightAccum_t *a;
	CFLOAT maxw=wlimit*2.0;

	wait();
	for(int i=cx;i<=(cx+1);i+=(i==cx) ? -1 : ((i<cx) ? 2 : 1) )
		for(int j=cy;j<=(cy+1);j+=(j==cy) ? -1 : ((j<cy) ? 2 : 1) )
			for(int k=cz;k<=(cz+1);k+=(k==cz) ? -1 : ((k<cz) ? 2 : 1) )
			{
				a=hash.findExistingBox(i,j,k);
				if((a==NULL) || ! a->valid) continue;
				for(list<lightSample_t>::iterator l=a->radiance.begin();
							l!=a->radiance.end();++l)
				{
					//PFLOAT pD=polarDist(pP,l->realPolar);
					PFLOAT pD=polarDist(pP,l->pP);
					if(pD>cache_size) continue;
					if((W(*l,P,N,maxw))<wlimit) continue;
					a->radiance.push_front(*l);
					a->radiance.erase(l);
					signal();
					return true;
				}
			}
	signal();
	return false;
}

void lightCache_t::insert(const point3d_t &P,const renderState_t &state,const lightSample_t &sample)
{
	//point3d_t pP=toPolar(P,sc);
	point3d_t pP=toPolar(P,state);
	wait();
	lightAccum_t &nuevo=hash.findBox(pP);
	if(!nuevo.valid) nuevo.radiance.clear(); // This line could be removed
	nuevo.radiance.push_front(sample);
	nuevo.valid=true; // To remove together with the other line
	signal();
	inserted++;
}

__END_YAFRAY
