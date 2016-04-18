
#include "cacheproxy.h"

#include<algorithm>
#include<vector>
using namespace std;

__BEGIN_YAFRAY

cacheProxy_t::cacheProxy_t(lightCache_t &ca,const scene_t &sc,PFLOAT mradius):
cache(ca),scene(sc),pixelid(-1),maxradius(mradius),entry(sc.getMaxRayDepth())
{
	radius=maxradius;
}

vector<foundSample_t> &
cacheProxy_t::gatherSamples(renderState_t &state,const point3d_t &P,
										const point3d_t &rP,const vector3d_t &N,
										int search,int minimun, CFLOAT (*W)(const lightSample_t &,
										const point3d_t &,const vector3d_t &,CFLOAT), PFLOAT wlimit)
{
	if(state.pixelNumber!=pixelid)
	{
		reset();
		pixelid=state.pixelNumber;
		//return newSearch(state,P,rP,N,search,minimun,W,wlimit).found;
	}
		/*
	else
	{
		proxyEntry_t *e=findCompatible(state.raylevel,P,N);
		if(e!=NULL)
		{
			fakeSearch(*e,P,rP,N,search,minimun,W,wlimit,result);
			return result;
		}
		else 
	}
		*/
	newSearch(state,P,rP,N,search,minimun,W,wlimit,result);
	return result;
}

struct compareFound_f
{
	bool operator () (const foundSample_t &a,const foundSample_t &b)
	{
		return a.weight>b.weight;
	}
};

void cacheProxy_t::newSearch(renderState_t &state,const point3d_t &P,const point3d_t &rP,
										const vector3d_t &N,int search,int minimun, 
										CFLOAT (*W)(const lightSample_t &,const point3d_t &,
										const vector3d_t &,CFLOAT), PFLOAT wlimit,
										vector<foundSample_t> &found)
{
	/*
	entry[state.raylevel].push_front(proxyEntry_t());
	proxyEntry_t &newentry=entry[state.raylevel].front();
	newentry.P=P;
	newentry.N=N;
	newentry.precision=2.0*state.traveled*scene.getWorldResolution();
	cache.gatherSamples(P,rP,N,newentry.found,search,radius,maxradius,minimun,W,wlimit);
	return newentry;
	*/
	cache.gatherSamples(P,rP,N,found,search,radius,maxradius,minimun,W,wlimit);
	foundSample_t temp;
	compareFound_f cfound;
	CFLOAT maxweight=wlimit*2.5;
	if(found.empty())
	{
		for(vector<lightSample_t>::const_iterator i=created.begin();i!=created.end();++i)
		{
			//temp.dis=polarDist(rP,i->realPolar);
			temp.dis=cache.polarDist(rP,i->pP);
			if(temp.dis>maxradius) continue;
			temp.S=&(*i);
			temp.weight=W(*i,P,N,maxweight);
			unsigned int limit;
			if(temp.weight<=wlimit) 
				limit=minimun; 
			else 
				limit=search;
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
	}
}

/*
void cacheProxy_t::fakeSearch(const proxyEntry_t &e,const point3d_t &P,const point3d_t &rP,
										const vector3d_t &N,
										int search,int minimun,CFLOAT (*W)(const lightSample_t &,
										const point3d_t &,const vector3d_t &,CFLOAT), PFLOAT wlimit,
										std::vector<foundSample_t> &found)
{
	foundSample_t temp;
	compareFound_f cfound;
	found.reserve(search+1);
	CFLOAT maxweight=wlimit*2.5;
		
	found.clear();

	for(vector<foundSample_t>::const_iterator i=e.found.begin();i!=e.found.end();++i)
	{
		temp.weight=W(*(temp.S),P,N,maxweight);
		unsigned int limit;
		if(temp.weight<=wlimit) 
			limit=minimun; 
		else 
			limit=search;
		if((found.size()>=limit) && (temp.weight<found.front().weight)) continue;
		temp.S=i->S;
		temp.dis=i->dis;
		found.push_back(temp);
		push_heap(found.begin(),found.end(),cfound);
		if(found.size()>limit)
		{
			pop_heap(found.begin(),found.end(),cfound);
			found.pop_back();
		}
	}
	if(found.empty())
	{
		for(vector<lightSample_t>::const_iterator i=created.begin();i!=created.end();++i)
		{
			temp.dis=polarDist(rP,i->realPolar);
			if(temp.dis>maxradius) continue;
			temp.S=&(*i);
			temp.weight=W(*i,P,N,maxweight);
			unsigned int limit;
			if(temp.weight<=wlimit) 
				limit=minimun; 
			else 
				limit=search;
			if((found.size()>=limit) && (temp.weight<found.front().weight)) continue;
			found.push_back(temp);
			push_heap(found.begin(),found.end(),cfound);
			if(found.size()>limit)
			{
				pop_heap(found.begin(),found.end(),cfound);
				found.pop_back();
			}
		}
	}
}
*/
proxyEntry_t * cacheProxy_t::findCompatible(int level,const point3d_t &P,const vector3d_t &N)
{
	for(list<proxyEntry_t>::iterator i=entry[level].begin();i!=entry[level].end();++i)
	{
		vector3d_t diff=P-i->P;
		PFLOAT dis=diff.length();
		if(dis>i->precision) continue;
		diff*=1.0/dis;
		if(fabs(diff*i->N)>0.1) continue;
		if((N*i->N)<0.98) continue;
		return &(*i);
	}
	return NULL;
}

void cacheProxy_t::reset()
{
	for(vector<std::list<proxyEntry_t> >::iterator i=entry.begin();i!=entry.end();++i)
		i->clear();
	if(created.size()>600)
		created.clear();
}

void cacheProxy_t::addSample(renderState_t &state,const lightSample_t &sample)
{
	created.push_back(sample);
}

__END_YAFRAY
