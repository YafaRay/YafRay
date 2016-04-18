
#include "pathlight.h"


using namespace std;

__BEGIN_YAFRAY

haltonSampler_t::haltonSampler_t(int depth,int samples)
{
	depth = (depth+1)*2;
	HSEQ = new Halton[depth];
	int base = 2;
	for (int i=0;i<depth;i++) 
	{
		HSEQ[i].setBase(base);
		base = nextPrime(base);
	}
}

haltonSampler_t::~haltonSampler_t()
{
	delete[] HSEQ;
}

void haltonSampler_t::samplingFrom(renderState_t &state,const point3d_t &P,
		const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv)
{
	taken=0;
}

vector3d_t haltonSampler_t::nextDirection(const point3d_t &P,
		const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv,
		int cursam,int curlev,color_t &raycolor)
{
	if(cursam>taken) taken=cursam;
	PFLOAT z1 = HSEQ[curlev<<=1].getNext();
	PFLOAT z2 = HSEQ[curlev+1].getNext();

	if(z1>1.0) z1=1.0;
  z2 *= 2.0*M_PI;
  return (Ru*cos(z2) + Rv*sin(z2))*sqrt(1.0-z1) + N*sqrt(z1);
}

randomSampler_t::randomSampler_t(int samples)
{
	int g = int(sqrt((float)samples));
  samples=g*g;
  grid = g;
  gridiv = 1.0/PFLOAT(grid);
}

randomSampler_t::~randomSampler_t()
{
}

void randomSampler_t::samplingFrom(renderState_t &state,const point3d_t &P,
		const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv)
{
	taken=0;
}

vector3d_t randomSampler_t::nextDirection(const point3d_t &P,
		const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv,
		int cursam,int curlev,color_t &raycolor)
{
	if(cursam>taken) taken=cursam;
  PFLOAT z1, z2;
  if (curlev==0) {
    z1 = (PFLOAT(cursam / grid) + ourRandom()) * gridiv;
    z2 = (PFLOAT(cursam % grid) + ourRandom()) * gridiv;
  }
  else { z1=ourRandom();  z2=ourRandom(); }
  
	if(z1>1.0) z1=1.0;
  z2 *= 2.0*M_PI;
  return (Ru*cos(z2) + Rv*sin(z2))*sqrt(1.0-z1) + N*sqrt(z1);
}

photonSampler_t::photonSampler_t(int s,int depth,const globalPhotonMap_t &map,
		int grid):samples(s),photonmap(map)
{
	depth = (depth+1)*2;
	HSEQ = new Halton[depth];
	int b = 2;
	for (int i=0;i<depth;i++) 
	{
		HSEQ[i].setBase(b);
		b = nextPrime(b);
		HSEQ[i].setStart(ourRandomI());
	}

	PFLOAT base=sqrt((PFLOAT)grid/2.0);
	paralels=(int)(base+0.5);
	meridians=paralels*2;
	pdiv=1.0/(PFLOAT)paralels;
	mdiv=2.0*M_PI/(PFLOAT)meridians;
	hits.resize(paralels);
	weight.resize(paralels);
	energy.resize(paralels);
	sectors=paralels*meridians;
	for(int i=0;i<paralels;++i)
	{
		hits[i].resize(meridians);
		weight[i].resize(meridians);
		energy[i].resize(meridians);
	}
	
	radius=photonmap.getMaxRadius();
	search=3*sectors;
	//cout<<"Using hemigrid of base "<<meridians<<"x"<<paralels<<endl;
}

photonSampler_t::~photonSampler_t()
{
	delete [] HSEQ;
}

pair<int,int> photonSampler_t::getCoords(const vector3d_t &v,
		const vector3d_t &N,const vector3d_t &Ru,
		const vector3d_t &Rv)const
{
	pair<int,int> coords;
	PFLOAT x=v*Ru;
	PFLOAT y=v*Rv;
	PFLOAT z=v*N;
	//PFLOAT fix=z*z;
	PFLOAT fix=sqrt(1.0-z*z);
	coords.first=(int)(fix/pdiv);
	if(coords.first>=paralels) coords.first--;
	if(fix>1.0) fix=1.0;
	//fix=sqrt(1.0-fix);
	if(fix!=0.0)
	{
		x/=fix;
		y/=fix;
	}
	if(x>1.0) x=1.0;
	if(x<-1.0) x=-1.0;
	PFLOAT an=acos(x);
	if(y<0) an=2*M_PI-an;
	coords.second=(int)(an/mdiv);
	if(coords.second>=meridians) coords.second--;
	return coords;
}

CFLOAT photonSampler_t::giveMaxDiff(int i,int j)const
{
	CFLOAT max=0,temp;
	int il=i-1,ih=i+1;
	int jl=j-1,jh=j+1;
	if(il>=0)
	{
		if(jl>=0) {temp=maxAbsDiff(energy[il][jl],energy[i][j]);if(temp>max) max=temp;}
		temp=maxAbsDiff(energy[il][j],energy[i][j]);if(temp>max) max=temp;
		if(jh<meridians) {temp=maxAbsDiff(energy[il][jh],energy[i][j]);if(temp>max) max=temp;}
	}
	if(jl>=0) {temp=maxAbsDiff(energy[i][jl],energy[i][j]);if(temp>max) max=temp;}
	if(jh<meridians) {temp=maxAbsDiff(energy[i][jh],energy[i][j]);if(temp>max) max=temp;}
	if(ih<paralels)
	{
		if(jl>=0) {temp=maxAbsDiff(energy[ih][jl],energy[i][j]);if(temp>max) max=temp;}
		temp=maxAbsDiff(energy[ih][j],energy[i][j]);if(temp>max) max=temp;
		if(jh<meridians) {temp=maxAbsDiff(energy[ih][jh],energy[i][j]);if(temp>max) max=temp;}
	}
	return max;
}

void photonSampler_t::samplingFrom(renderState_t &state,const point3d_t &P,
				const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv)
{
	found.reserve(search+1);	
	photonmap.gather(P,N,found,search,radius);
	//int foundp=found.size();
	for(int i=0;i<paralels;++i)
		for(int j=0;j<meridians;++j)
		{
			hits[i][j]=0;
			energy[i][j].set(0,0,0);
		}
	
	for(vector< foundPhoton_t >::iterator i=found.begin();i!=found.end();++i)
		if((i->photon->direction()*N)>0.0)
		{
			pair<int,int> pos=getCoords(i->photon->direction(),N,Ru,Rv);
			energy[pos.first][pos.second]+=i->photon->color();
		}
	CFLOAT totaldiff=0;
	for(int i=0;i<paralels;++i)
		for(int j=0;j<meridians;++j)
			totaldiff+=weight[i][j]=giveMaxDiff(i,j);
			
	int assign=samples-sectors;
	CFLOAT sph=(totaldiff!=0.0) ? (CFLOAT)assign/totaldiff : 0;
	
	for(int i=0;i<paralels;++i)
		for(int j=0;j<meridians;++j)
		{
			int h=(int)(sph*weight[i][j]+0.5);
			if(h>assign) h=assign;
			assign-=h;
			hits[i][j]=h+1;
		}
	bool noway=false;
	while(assign)
	{
		for(int i=0;(i<paralels) && assign;++i)
			for(int j=0;(j<meridians) && assign;++j)
				if((hits[i][j]>1) || noway) {hits[i][j]++;assign--;}
		noway=true;
	}
	int max=1;
	for(int i=0;i<paralels;++i)
		for(int j=0;j<meridians;++j)
			if(hits[i][j]>max) max=hits[i][j];
	CFLOAT maxf=(CFLOAT)max;
	for(int i=0;i<paralels;++i)
		for(int j=0;j<meridians;++j)
			weight[i][j]=maxf/(CFLOAT)hits[i][j];
	taken=0;
	multi=1.0/(maxf*(CFLOAT)sectors);
	current[0]=0;
	current[1]=0;
	current[2]=0;
	/*
	cout<<"found "<<foundp<<" assign "<<assign<<" of "<<samples<<endl;
	for(int i=0;i<paralels;++i)
	{
		for(int j=0;j<meridians;++j)
			cout<<hits[i][j]<<" ";
		cout<<endl;
	}
	getchar();
	*/
}

vector3d_t photonSampler_t::nextDirection(const point3d_t &P,
		const vector3d_t &N,const vector3d_t &Ru,const vector3d_t &Rv,
		int cursam,int curlev,color_t &raycolor)
{
  PFLOAT z1, z2;
  if (curlev==0) {
		z1= ((PFLOAT)current[0]+HSEQ[0].getNext())*pdiv;
		z2= ((PFLOAT)current[1]+HSEQ[1].getNext())*mdiv;
		//z1= ((PFLOAT)current[0]+ourRandom())*pdiv;
		//z2= ((PFLOAT)current[1]+ourRandom())*mdiv;
		raycolor*=weight[current[0]][current[1]]*2*z1;
		nextSample();
  }
  else 
	{ 
		z1=HSEQ[curlev<<=1].getNext();  
		z2=HSEQ[curlev+1].getNext()*2.0*M_PI; 
		//z1=ourRandom();  
		//z2=ourRandom()*2.0*M_PI; 
	}
  
	if(z1>1.0) z1=1.0;
  //return (Ru*cos(z2) + Rv*sin(z2))*sqrt(1.0-z1) + N*sqrt(z1);
  return (Ru*cos(z2) + Rv*sin(z2))*z1 + N*sqrt(1.0-z1*z1);
}

CFLOAT photonSampler_t::multiplier()const 
{
	return multi;
}

__END_YAFRAY
