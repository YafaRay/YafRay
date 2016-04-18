/****************************************************************************
 *
 *	  pathlight.cc: implementation of simplified pathtracing
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez and Alfredo de Greef
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

#include "pathlight.h"
using namespace std;

__BEGIN_YAFRAY

#define WARNING cerr<<"[pathLight]: "

lightCache_t *lightcache=NULL;


pathLight_t::pathLight_t(int nsam, CFLOAT pwr, int depth,int cdepth,bool uQ,
		bool ca,PFLOAT casiz,CFLOAT thr,bool recal,bool di,bool shows,int grids,int ref,
		bool _occmode, PFLOAT occdist, bool _ignorms)
		: samples(nsam), power(pwr), maxdepth(depth),maxcausdepth(cdepth),use_QMC(uQ),
cache(ca),maxrefinement(ref),recalculate(recal),direct(di),show_samples(shows),
gridsize(grids),threshold(thr), occmode(_occmode), occ_maxdistance(occdist), ignorms(_ignorms)
{
	if(cache) 
	{
		if(lightcache!=NULL)
		{
			cout<<"Several cached pathlights at the same time"<<endl;
			exit(1);
		}
		lightcache=new lightCache_t(casiz);
		searchRadius=casiz*2.0;
		dist_to_sample=casiz*0.1;
	}

	if (use_QMC) 
	{
		depth = (depth+1)*2;
		HSEQ = new Halton[depth];
		int base = 2;
		for (int i=0;i<depth;i++) {
			HSEQ[i].setBase(base);
			base = nextPrime(base);
		}
	}
	else 
	{
		// samples must be integer squared value for jittered sampling
		int g = int(sqrt((float)samples));
		g *= g;
		if (g!=samples) {
			cout << "Samples value changed from " << samples << " to " << g << endl;
			samples = g;
		}
		grid = int(sqrt((float)samples));
		gridiv = 1.0/PFLOAT(grid);
		HSEQ = NULL;
	}
	sampdiv = 1.0 / PFLOAT(samples);

	shadow_threshold=0.1;
	desiredWeight=1.0/shadow_threshold;
	weightLimit=0.8*desiredWeight;
	search=9;
	devaluated=1.0;
	refined=0;
}

pathLight_t::~pathLight_t() 
{ 
	if (HSEQ) delete[] HSEQ;  HSEQ=NULL; 
	if (cache) {delete lightcache;lightcache=NULL;};
}

void pathLight_t::init(scene_t &scene)
{
	if(cache)
	{
		lightcache->setAspect(scene.getAspectRatio());
		lightcache->startFill();
		scene.setRepeatFirst();
		devaluated = 1.0;
	}
	use_in_indirect=false;
	scene.getPublishedData("globalPhotonMap",pmap);
	scene.getPublishedData("irradianceGlobalPhotonMap",imap);
	scene.getPublishedData("irradianceHashMap",irhash);
}


color_t pathLight_t::illuminate(renderState_t &state,const scene_t &sc, const surfacePoint_t sp,
				const vector3d_t &eye) const
{
	if(cache)
	{
		if (lightcache->ready())
			return interpolate(state,sc,sp,eye);
		else
			return cached(state,sc,sp,eye);
	}
	else
		return normalSample(state, sc, sp, eye);
}


photonData_t *pathLight_t::getPhotonData(renderState_t &state)const
{
	photonData_t *data=NULL;
	if(imap!=NULL)
	{
		bool present;
		data=state.context.getDestructible(photonData,present);
		if(!present)
		{
			data=new photonData_t(imap->getMaxRadius(),new vector<foundPhoton_t>(5+1));
			state.context.storeDestructible(photonData,data);
		}
	}
	return data;
}

color_t pathLight_t::getLight(renderState_t &state,const surfacePoint_t &sp,
		const scene_t &sc,const vector3d_t &eye,photonData_t *data)const
{
	vector3d_t N;
	if (ignorms && (!lightcache->ready()))
		N = FACE_FORWARD(sp.Ng(), sp.Nd(), eye);
	else
		N = FACE_FORWARD(sp.Ng(), sp.N(), eye);
	color_t total(0,0,0);
	if(imap!=NULL)
	{
		bool slowway=false;
		const globalPhotonLight_t::compPhoton_t *irr=irhash->findExistingBox(sp.P());
		if(irr!=NULL) 
		{
			CFLOAT factor=irr->N*N;
			if(factor>0.7)	total=irr->irr*factor;
			else slowway=true;
		} else slowway=true;
		if(slowway)
		{
			vector<foundPhoton_t> &found=*(data->found);
			found.reserve(6);
			imap->gather(sp.P(),N,found,5,data->radius,0.25);
			if(found.size())
			{
				PFLOAT farest;
				if(found.size()==1) farest=data->radius;
				else farest=found.front().dis;
				if(farest==0.0) farest=1.0;
				PFLOAT div=0.0;
				for(vector<foundPhoton_t>::iterator i=found.begin();i!=found.end();++i)
				{
					PFLOAT factor=i->photon->direction()*N*(1.0-i->dis/farest);
					if(factor>0)
					{
						total+=i->photon->color()*factor;
						div+=factor;
					}
				}
				if(div>0.0) total*=1.0/div;
			}
		}
		total = total * sp.getShader()->getDiffuse(state,sp,N);
		total += sc.light(state,sp,sp.P()+eye,true);
		total += sp.getShader()->fromRadiosity(state,sp,
					energy_t(N,color_t(0.0)),eye);
	}
	else
	{
			total = sc.light(state,sp,sp.P()+eye,true);
			total += sp.getShader()->fromRadiosity(state,sp,
					energy_t(N,color_t(0.0)),eye);
	}
	return total;
}

static bool followCaustic(vector3d_t &ray,color_t &raycolor,
		const vector3d_t &N,const vector3d_t &FN,object3d_t *obj)
{
	if(!obj->caustics()) return false;
	color_t caus_rcolor,caus_tcolor;
	PFLOAT caus_IOR;
	obj->getCaustic(caus_rcolor, caus_tcolor, caus_IOR);
	CFLOAT kr,kt;
	vector3d_t edir=-ray;
	fresnel(edir,N,caus_IOR,kr,kt);
	color_t ref=caus_rcolor*kr;
	color_t trans=caus_tcolor*kt;
	CFLOAT pref = ref.getR() + ref.getG() + ref.getB();
	CFLOAT ptrans = trans.getR() + trans.getG() + trans.getB();
	if( (pref==0.0) && (ptrans==0.0) ) return false;
	if((pref/(pref+ptrans))>ourRandom())
	{
			ray=reflect(FN,edir);
			raycolor*=ref;
	}
	else
	{
			ray=refract(N,edir,caus_IOR);
			raycolor*=trans;
	}
	return true;
}

color_t pathLight_t::takeSample(renderState_t &state, const vector3d_t &N,
		const surfacePoint_t &sp, const scene_t &sc, PFLOAT &avgD, PFLOAT &minD,
		bool caching) const
{
	PFLOAT maxdist=1000000*sc.getWorldResolution()*sp.Z();
	int oldlevel=state.rayDivision;
	const void *oldorigin=state.skipelement;
	if(!direct) state.rayDivision=oldlevel*samples;
	int localsamples;
	if(!caching)	localsamples=samples/oldlevel;
	else localsamples=samples;

	if(localsamples==0) localsamples=1;
	color_t total(0.0),subtotal[4];
	PFLOAT HNUM=0,HD=0,H,M=0;

	photonData_t *data=getPhotonData(state);
	if(direct) {avgD=maxdist;minD=maxdist;return getLight(state,sp,sc,N,data);}
	hemiSampler_t *sampler=getSampler(state,sc);
	sampler->samplingFrom(state,sp.P(),N,sp.NU(),sp.NV());
	bool first=true;
	CFLOAT repetitions=0.0;
	
	for(int ite=1;ite>0;--ite,repetitions+=1.0)
	{
		sampler->reset();

		if (occmode) {
			// occlusion mode
			color_t tcol(1.0);
			surfacePoint_t tempsp;
			state.skipelement = sp.getOrigin();
			if (caching) {
				for (int sm=0;sm<samples;sm++)
				{
					HNUM += 1;
					vector3d_t dir = sampler->nextDirection(sp.P(), N, sp.NU(), sp.NV(), sm, 0, tcol);
					if (occ_maxdistance>0) {
						// distance limited mode
						// normal unbiased hittest (no distance limit) to record correct mean harmdist.
						bool bghit = true;
						if (sc.firstHit(state, tempsp, sp.P(), dir, true))
						{
							if (tempsp.Z()>0) HD += 1.0/tempsp.Z();
							if ((tempsp.Z()<M) || (M==0)) M = tempsp.Z();
							bghit = false;
						}
						// except that if z>maxdistance, assume background hit as well
						if (bghit || (tempsp.Z()>occ_maxdistance)) {
							color_t contri(sc.getBackground(dir, state, true) * fabs(dir*N));
							total += contri;
							if (first) subtotal[sm & 3] += contri;
						}
					}
					else {
						// normal mode (unlimited distance, hemilight)
						if (!sc.firstHit(state, tempsp, sp.P(), dir, true))
						{
							color_t contri(sc.getBackground(dir, state, true) * fabs(dir*N));
							total += contri;
							if (first) subtotal[sm & 3] += contri;
						}
						else {
							if (tempsp.Z()>0) HD += 1.0/tempsp.Z();
							if ((tempsp.Z()<M) || (M==0)) M = tempsp.Z();
						}
					}
				}
			}
			else {
				for (int sm=0;sm<samples;sm++)
				{
					vector3d_t dir = sampler->nextDirection(sp.P(), N, sp.NU(), sp.NV(), sm, 0, tcol);
					if (!((occ_maxdistance>0) ?
								sc.isShadowed(state, sp, sp.P()+occ_maxdistance*dir) :
								sc.isShadowed(state, sp, dir)))
						total += sc.getBackground(dir, state, true) * fabs(dir*N);
				}
			}
		}
		else {
			// full GI
			for(int i=0;i<samples;++i)
			{
				color_t raycolor(1.0);
				vector3d_t ray = sampler->nextDirection(sp.P(),N, sp.NU(), sp.NV(), i, 0,raycolor);
				vector3d_t startray = ray;
				point3d_t where = sp.P();
				if(caching) HNUM+=1;
				state.skipelement=sp.getOrigin();
				for(int j=0,cj=0;j<maxdepth;++j)
				{
					if (raycolor.energy()<0.05) break;
					surfacePoint_t tempsp;
					if (!sc.firstHit(state,tempsp, where, ray, true)) //background reached
					{
						color_t contri=(startray*N)*raycolor*sc.getBackground(ray, state, true);
						total += contri;
						if(first) subtotal[i%4]+=contri;
						break;
					}

					if(caching)
					{
						if((j==0) && (cj==0) && (tempsp.Z()>0)) HD+=1.0/tempsp.Z();
						if((j==0) && (cj==0) && ((tempsp.Z()<M) || (M==0)) ) M=tempsp.Z();
					}
					color_t light = getLight(state,tempsp,sc,-ray,data);
					color_t contri=(startray*N)*raycolor*light;
					total += contri;
					if(first) subtotal[i%4]+=contri;
					vector3d_t NN;
					if (ignorms && caching) NN=tempsp.Nd(); else NN=tempsp.N();
					vector3d_t HN = FACE_FORWARD(tempsp.Ng(), NN, -ray);
					if(!followCaustic(ray,raycolor, NN, HN, tempsp.getObject()))
					{
						raycolor *= tempsp.getShader()->getDiffuse(state, tempsp, -ray);
						ray = sampler->nextDirection(tempsp.P(),HN, tempsp.NU(), tempsp.NV(),
								i, j+1,raycolor);
					}
					else if(cj<maxcausdepth) {cj++;j--;}
					where = tempsp.P();
					state.skipelement=tempsp.getOrigin();
				}
			}
		}
		if(first)
		{
			first=false;
			CFLOAT minR=1000,minG=1000,minB=1000;
			CFLOAT maxR=0,maxG=0,maxB=0;
			for(int i=0;i<4;++i)
			{
				subtotal[i]*=sampler->multiplier();
				if (subtotal[i].getR()>maxR) maxR = subtotal[i].getR();
				if (subtotal[i].getG()>maxG) maxG = subtotal[i].getG();
				if (subtotal[i].getB()>maxB) maxB = subtotal[i].getB();
				if (subtotal[i].getR()<minR) minR = subtotal[i].getR();
				if (subtotal[i].getG()<minG) minG = subtotal[i].getG();
				if (subtotal[i].getB()<minB) minB = subtotal[i].getB();
			}
			color_t min(minR,minG,minB),max(maxR,maxG,maxB);
			sc.adjustColor(min);
			sc.adjustColor(max);
			CFLOAT ene=maxAbsDiff(min,color_t(0.0));
			CFLOAT diff=maxAbsDiff(max,min) /(( (ene<1.0) ? 1.0 : ene )*2.0*2.0);
			diff/=threshold;
			int extra=(int)(diff*diff)-1;
			if(extra>3) extra=3;
			if(extra<0) extra=0;
			ite+=extra;
		}
	}
	if(caching)
	{
		if(HD>0) H=HNUM/HD; else H=maxdist;
		if(M<=0) M=maxdist;
		avgD=H;
		minD=M;
	}
	state.rayDivision=oldlevel;
	state.skipelement=oldorigin;
	total*=sampler->multiplier()/repetitions;
	return total;
}

color_t pathLight_t::normalSample(renderState_t &state,const scene_t &sc, 
		const surfacePoint_t sp, const vector3d_t &eye) const
{
	color_t light;
	vector3d_t N; 
	N = FACE_FORWARD(sp.Ng(), sp.N(), eye);
	const shader_t *sha = sp.getShader();
	color_t total(0.0);

	if (sha->getDiffuse(state, sp, eye).energy()<0.05f) return total;

	PFLOAT foo1,foo2;
	total=takeSample(state,N,sp,sc,foo1,foo2);
	return sp.getShader()->getDiffuse(state, sp, eye)*total*power;
}

//------------- CACHE

#define MAX(a,b) ( ((a)>(b)) ? (a) : (b) )

CFLOAT pathLight_t::weight(const lightSample_t &sample,const point3d_t &P,
		const vector3d_t &N,CFLOAT maxweight)
{
	vector3d_t PP=P - sample.P;
	//vector3d_t avgN=N+sample.N;
	//avgN.normalize();
	const CFLOAT &avgOclu=sample.adist;
	if(avgOclu==0.0) return 0;
	
	CFLOAT D=PP.normLen()-sample.precision*2.0;
	if(D<0) D=0;
	CFLOAT a=D/avgOclu;
	CFLOAT cinv=1.000001-(sample.N*N);
	CFLOAT b=sqrt(cinv);
	//cinv*=cinv;
	//cinv*=cinv;
	//b+=4*cinv;
	CFLOAT c=fabs(N*PP);//10.0*fabs(N*PP);
	CFLOAT f=sample.devaluated*(a+MAX(b,c));
	if(f==0) return maxweight;
	f=1.0/f;
	if(f>maxweight) return maxweight;
	else return f;
	/*
	f*=maxweight;
	f*=f;
	f*=f;
	return maxweight/pow(1.0+f, 0.25);
	*/
}


CFLOAT pathLight_t::weightNoPrec(const lightSample_t &sample,const point3d_t &P,
		const vector3d_t &N,CFLOAT maxweight)
{
	vector3d_t PP=P - sample.P;
	//vector3d_t avgN=N+sample.N;
	//avgN.normalize();
	const CFLOAT &avgOclu=sample.adist;
	if(avgOclu==0.0) return 0;
	
	CFLOAT D=PP.normLen();//-sample.precision; //FIX
	//if(D<0) D=0;
	CFLOAT a=D/avgOclu;
	CFLOAT cinv=1.000001-(sample.N*N);
	CFLOAT b=sqrt(cinv);
	//cinv*=cinv;
	//cinv*=cinv;
	//b+=4*cinv;
	CFLOAT c=fabs(N*PP);//10.0*fabs(N*PP);
	CFLOAT f=sample.devaluated*(a+MAX(b,c));
	if(f==0) return maxweight;
	f=1.0/f;
	if(f>maxweight) return maxweight;
	else return f;
	/*
	f*=maxweight;
	f*=f;
	f*=f;
	return maxweight/pow(1.0+f, 0.25);
	*/
}


CFLOAT pathLight_t::weightNoDist(const lightSample_t &sample,const point3d_t &P,
		const vector3d_t &N,CFLOAT maxweight)
{
	vector3d_t PP=P - sample.P;
	//vector3d_t avgN=N+sample.N;
	//avgN.normalize();
	
	CFLOAT D=PP.normLen();
	CFLOAT a=D/(sample.precision*40.0);
	CFLOAT cinv=1.000001-(sample.N*N);
	CFLOAT b=sqrt(cinv);
	//cinv*=cinv;
	//cinv*=cinv;
	//b+=4*cinv;
	CFLOAT c=fabs(N*PP);//10.0*fabs(N*PP);
	CFLOAT f=a+MAX(b,c);
	if(f==0) return maxweight;
	f=1.0/f;
	if(f>maxweight) return maxweight;
	else return f;
	/*
	f*=maxweight;
	f*=f;
	f*=f;
	return maxweight/pow(1.0+f, 0.25);
	*/
}

CFLOAT pathLight_t::weightNoDev(const lightSample_t &sample,const point3d_t &P,
		const vector3d_t &N,CFLOAT maxweight)
{
	vector3d_t PP=P - sample.P;
	//vector3d_t avgN=N+sample.N;
	//avgN.normalize();
	const CFLOAT &avgOclu=sample.adist;
	if(avgOclu==0.0) return 0;
	
	CFLOAT D=PP.normLen()-sample.precision;
	if(D<0) D=0;
	CFLOAT a=D/avgOclu;
	CFLOAT cinv=1.000001-(sample.N*N);
	CFLOAT b=sqrt(cinv);
	//cinv*=cinv;
	//cinv*=cinv;
	//b+=4*cinv;
	CFLOAT c=fabs(N*PP);//10.0*fabs(N*PP);
	CFLOAT f=a+MAX(b,c);
	if(f==0) return maxweight;
	f=1.0/f;
	if(f>maxweight) return maxweight;
	else return f;
	/*
	f*=maxweight;
	f*=f;
	f*=f;
	return maxweight/pow(1.0+f, 0.25);
	*/
}


color_t pathLight_t::interpolate(renderState_t &state,const scene_t &s,
		const surfacePoint_t sp, const vector3d_t &eye) const
{
	if (sp.getShader()->getDiffuse(state, sp, eye).energy()<0.05f) return color_t(0.0);
	vector3d_t N;
	if (ignorms)
		N = FACE_FORWARD(sp.Ng(), sp.Nd(), eye);
	else
		N = FACE_FORWARD(sp.Ng(), sp.N(), eye);
	//point3d_t pP=toRealPolar(sp.P(),s);
	point3d_t pP=lightcache->toPolar(sp.P(),state);
	
	PFLOAT radius;
	int mins=(state.raylevel>0) ? 3 :0;

	cacheProxy_t *proxy=getProxy(state,s);
	CFLOAT farest=0;
	if(show_samples)
	{
		vector<foundSample_t> samples;
		radius=dist_to_sample*0.5;
		farest=lightcache->gatherSamples(sp.P(),pP,N,samples,1,radius,radius,
																			0,pathLight_t::weight,weightLimit);
		if(samples.size()) return color_t(1,1,1);
		else return color_t(0,0,0);
	}
	vector<foundSample_t> &samples=proxy->gatherSamples(state,sp.P(),pP,N,
																											search,mins,
																											pathLight_t::weight,
																											weightLimit);

	if(samples.size()) farest=samples.front().weight;
	if(samples.size()==1) farest=0;
	else if(farest>weightLimit) farest=weightLimit;

	for(vector<foundSample_t>::iterator i=samples.begin();i!=samples.end();++i)
		i->weight=(i->weight-farest)*(1.0-i->dis/searchRadius);

	color_t total(0,0,0);
	CFLOAT amount=0;
	for(vector<foundSample_t>::iterator i=samples.begin();i!=samples.end();++i)
	{
		total+=i->weight*i->S->color;
		amount+=i->weight;
	}
	if(amount!=0) 
	{
		amount=1.0/amount;
		total*=amount;
		return sp.getShader()->getDiffuse(state, sp, eye)*total*power;
	}
	else 
	{
		// Antialiasing , more samples needed
		cout<<".";cout.flush();
		PFLOAT H,M;
		if (ignorms) N = FACE_FORWARD(sp.Ng(), sp.Nd(), eye);
		color_t ncol=takeSample(state,N,sp,s,H,M,true);
		proxy->addSample(state,lightSample_t(N,ncol,H, sp.P(),
			            lightcache->toPolar(sp.P(),state),M,state.traveled*s.getWorldResolution(),1.0));
			            //toPolar(sp.P(),s),M,state.traveled*s.getWorldResolution(),1.0));
		return sp.getShader()->getDiffuse(state, sp, eye)*ncol*power;
	}
}

void pathLight_t::setIrradiance(lightSample_t &sample,PFLOAT &radius)
{
	vector3d_t &N = sample.N;
	//point3d_t &pP= sample.realPolar;
	point3d_t &pP= sample.pP;
	point3d_t &P= sample.P;
	stsamples.clear();
	CFLOAT farest;

	farest=lightcache->gatherSamples(P,pP,N,stsamples,search,radius,searchRadius,
																		2,pathLight_t::weightNoDev,weightLimit);
	
	if(stsamples.size()==1) farest=0;
	else if(farest>weightLimit) farest=weightLimit;

	for(vector<foundSample_t>::iterator i=stsamples.begin();i!=stsamples.end();++i)
		i->weight=(i->weight-farest)*(1.0-i->dis/searchRadius);

	color_t total(0,0,0);
	CFLOAT amount=0;
	for(vector<foundSample_t>::iterator i=stsamples.begin();i!=stsamples.end();++i)
	{
		total+=i->weight*i->S->color;
		amount+=i->weight;
	}
	if(amount!=0) amount=1.0/amount;
	sample.mixed=total*power*amount;
}

bool pathLight_t::testRefinement(const scene_t &sc)
{
	if(threshold>=1.0) return false;
	if(refined>=maxrefinement)
	{
		for(lightCache_t::iterator i=lightcache->begin();i!=lightcache->end();++i)
			(*i).devaluated=1.0;
		return false;
	}
	devaluated*=2;
	refined++;
	bool changed=false;
	int change=0;
	int num=0;

	PFLOAT mixradius=searchRadius;
	for(lightCache_t::iterator i=lightcache->begin();i!=lightcache->end();++i)
		setIrradiance(*i,mixradius);

	vector<foundSample_t> samples;
	mixradius=searchRadius;
	for(lightCache_t::iterator i=lightcache->begin();i!=lightcache->end();++i)
	{
		CFLOAT minR=1000,minG=1000,minB=1000;
		CFLOAT maxR=0,maxG=0,maxB=0;
		samples.clear();
		//lightcache->gatherSamples((*i).P,(*i).realPolar,(*i).N,samples,5,mixradius,searchRadius,
		lightcache->gatherSamples((*i).P,(*i).pP,(*i).N,samples,5,mixradius,searchRadius,
				5,pathLight_t::weightNoDist,weightLimit);
		for(vector<foundSample_t>::iterator j=samples.begin();j!=samples.end();++j)
		{
			if (j->S->mixed.getR()>maxR) maxR = j->S->mixed.getR();
			if (j->S->mixed.getG()>maxG) maxG = j->S->mixed.getG();
			if (j->S->mixed.getB()>maxB) maxB = j->S->mixed.getB();
			if (j->S->mixed.getR()<minR) minR = j->S->mixed.getR();
			if (j->S->mixed.getG()<minG) minG = j->S->mixed.getG();
			if (j->S->mixed.getB()<minB) minB = j->S->mixed.getB();
		}
		
		color_t min(minR,minG,minB),max(maxR,maxG,maxB);
		min=min*power;
		max=max*power;
		sc.adjustColor(min);
		sc.adjustColor(max);
		min.clampRGB01();
		max.clampRGB01();
		if((maxAbsDiff(max,min))>threshold)
		{
			(*i).devaluated=devaluated;
			changed=true;
			change++;
		}
		num++;
	}
	cout<<"\nRefinement:"<<change<<"/"<<num<<"   "<<endl;
	return changed;
}

color_t pathLight_t::cached(renderState_t &state,const scene_t &sc,
		const surfacePoint_t sp, const vector3d_t &eye) const
{
	if (sp.getShader()->getDiffuse(state, sp, eye).energy()<0.05f) return color_t(0.0);

	vector3d_t N;
	if (ignorms && (!lightcache->ready()))
		N = FACE_FORWARD(sp.Ng(), sp.Nd(), eye);
	else
		N = FACE_FORWARD(sp.Ng(), sp.N(), eye);
	PFLOAT rq=1.0/(state.raylevel+1);
	color_t total(0,0,0);
	if(!lightcache->enoughFor(sp.P(),N,state,pathLight_t::weightNoPrec,desiredWeight*rq))
	{
		PFLOAT H,M;
		total=takeSample(state,N,sp,sc,H,M,true);

		lightcache->insert(sp.P(),state,lightSample_t(N,total,H, sp.P(),
						lightcache->toPolar(sp.P(),state),M,state.traveled*sc.getWorldResolution(),devaluated));
						//toPolar(sp.P(),sc),M,state.traveled*sc.getWorldResolution(),devaluated));
		total.set(1,1,1);
	}

	return total;
}

void pathLight_t::postInit(scene_t &scene)
{
	if(!cache) return;
	lightcache->startUse();

	if(!direct && testRefinement(scene))
	{
		scene.setRepeatFirst();
		lightcache->startFill();
	}
	else
		cout << lightcache->size() << " samples taken\n";
}

hemiSampler_t *pathLight_t::getSampler(renderState_t &state,const scene_t &sc)const
{
	bool present;
	hemiSampler_t *sam=state.context.getDestructible(_sampler,present);
	if(!present)
	{
		if((pmap!=NULL) && (samples>96)) sam=new photonSampler_t(samples,maxdepth,*pmap,gridsize);
		else 
		if(use_QMC) sam=new haltonSampler_t(maxdepth,samples);
		else sam=new randomSampler_t(samples);
		state.context.storeDestructible(_sampler,sam);
	}
	return sam;
}

cacheProxy_t *pathLight_t::getProxy(renderState_t &state,const scene_t &sc)const
{
	bool present;
	cacheProxy_t *proxy=state.context.getDestructible(_proxy,present);
	if(!present)
	{
		proxy=new cacheProxy_t(*lightcache,sc,searchRadius);
		state.context.storeDestructible(_proxy,proxy);
	}
	return proxy;
}


light_t *pathLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	CFLOAT power = 1.0,thr=0.1;
	int samples = 16,depth=3,cdepth=4,search=50,grid=36,ref=2;
	bool useqmc = false;
	bool cache = false;
	bool recalculate =true;
	bool direct=false;
	bool show_samples=false;
	bool useg=false;
	PFLOAT cache_size = 0.01,angt=0.2,shadt=0.3;
	// new param. switch to ignore displaced normals when building cache
	bool ignorms = false;	// default is old situation: do use normals (results in dense bad sample distrib.)

	params.getParam("power", power);
	params.getParam("depth", depth);
	params.getParam("caus_depth", cdepth);
	params.getParam("samples", samples);
	params.getParam("use_QMC", useqmc);
	params.getParam("cache", cache);
	params.getParam("direct", direct);
	params.getParam("grid", grid);
	params.getParam("threshold",thr);
	params.getParam("max_refinement",ref);

	// new mode parameter
	string _mode = ":)";
	const string *mode=&_mode;
	params.getParam("mode", mode);
	bool occmode = (*mode=="occlusion");
	PFLOAT occdist = -1;
	params.getParam("maxdistance", occdist);

	if (samples<1) {
		WARNING << "Samples value too low, minimum is one\n";
		samples = 1;
	}
	if(cache)
	{
		params.getParam("cache_size", cache_size);
		params.getParam("angle_threshold",angt);
		params.getParam("shadow_threshold",shadt);
		params.getParam("search",search);
		params.getParam("recalculate",recalculate);
		params.getParam("show_samples",show_samples);
		params.getParam("gradient",useg);
		params.getParam("ignore_bumpnormals", ignorms);
		if(search<3) search=3;
		//render.repeatFirstPass();
	}
	pathLight_t *path=new pathLight_t(samples, power, depth,cdepth, useqmc,
			cache,cache_size,thr,recalculate,direct,show_samples,grid,ref, occmode, occdist, ignorms);
	if(cache) path->setCacheThreshold(shadt,search);
	return path;
}

pluginInfo_t pathLight_t::info()
{
	pluginInfo_t info;

	info.name="pathlight";
	info.description="Montecarlo raytracing indirect lighting system";

	info.params.push_back(buildInfo<FLOAT>("power",0,10000,1.0,"Power of the indirect light"));
	info.params.push_back(buildInfo<INT>("depth",1,50,3,"Light bounces, set it to \
				1 if globalphotonmap present"));
	info.params.push_back(buildInfo<INT>("caus_depth",0,50,4,"Extra bounces when inside glass"));
	info.params.push_back(buildInfo<INT>("samples",1,5000,16,"Light samples, the \
			higher, the less noise and slower"));
	info.params.push_back(buildInfo<BOOL>("use_QMC","Whenever to use quasi montecarlo sampling"));
	info.params.push_back(buildInfo<BOOL>("cache","Whenever to cache iradiance"));
	info.params.push_back(buildInfo<BOOL>("direct","Shows the photonmap directly, use this for \
				tunning a globalphotonlight"));
	info.params.push_back(buildInfo<INT>("grid",36,36,36,"only for development"));
	info.params.push_back(buildInfo<FLOAT>("cache_size",0.000001,2,0.01,
				"Cache mode: Size of the cache cells, at least 1 sample per cell (polar coords)"));
	info.params.push_back(buildInfo<FLOAT>("threshold",0.000001,1000,0.3,
				"Cache mode: Threshold used to know when to resample a cached value"));
	info.params.push_back(buildInfo<FLOAT>("shadow_threshold",0.000001,1000,0.3,
				"Cache mode: Quality of the shadows/lighting, the lower, the better"));
	info.params.push_back(buildInfo<INT>("search",3,1000,50,"Cache mode: Maximun \
				number of values to do interpolation"));
	info.params.push_back(buildInfo<BOOL>("show_samples","Show the sample \
				distribution instead of lighting"));
	info.params.push_back(buildInfo<BOOL>("gradient","Activates the use of \
				gradients. Not working fine, but can solve some artifacts"));

	return info;
			
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("pathlight",pathLight_t::factory);
	cout<<"Registered pathlight\n";
}

}

__END_YAFRAY
