
#include "globalphotonlight.h"

__BEGIN_YAFRAY

using namespace std;

//------------------------------------------------------------------------------------------
// CONE ANGLE RESTRICTED VECTOR (replacement for random/discreteVectorCone)
inline vector3d_t HemiVec_CONE(const vector3d_t &nrm,
				const vector3d_t &Ru, const vector3d_t &Rv,
				PFLOAT cosang, PFLOAT z1, PFLOAT z2)
{
  PFLOAT t1=2.0*M_PI*z1, t2=1.0-(1.0-cosang)*z2;
  return (Ru*cos(t1) + Rv*sin(t1))*sqrt(1.0-t2*t2) + nrm*t2;
}
//------------------------------------------------------------------------------------------

void globalPhotonLight_t::shoot(runningPhoton_t &photon,const vector3d_t &dir,
		int depth,int cdepth,bool storeFirst,scene_t &scene)
{
	if(depth>maxdepth) return;
	surfacePoint_t sp;
	color_t originalcolor=photon.color();
	if(scene.firstHit(nullstate,sp,photon.position(),dir))
	{
		const void *oldorigin=nullstate.skipelement;
		nullstate.skipelement=sp.getOrigin();
		photon.position(sp.P(),MIN_RAYDIST);
		const shader_t *sha= sp.getShader();
		vector3d_t edir=photon.lastPosition()-photon.position();
		edir.normalize();
		vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
		vector3d_t Ng=FACE_FORWARD(sp.Ng(),sp.Ng(),edir);
		bool canreceive=((depth>0) || storeFirst) && (sp.getObject())->reciveRadiosity();
		if(canreceive) 
		{
			photonMap->store(photon/*,N*/);
			storeInHash(photon,N);
		}
		color_t diffcolor;
		color_t transcolor;
		CFLOAT trans=0.0;
		PFLOAT caus_IOR;
		CFLOAT diffuse=0.0;
		if((sp.getObject()->caustics()) && (cdepth<maxcdepth))
		{
			color_t caus_rcolor;
			sp.getObject()->getCaustic(caus_rcolor, transcolor, caus_IOR);
			trans=transcolor.energy();
		}
		if(sp.getObject()->useForRadiosity())
		{
			diffcolor=sha->getDiffuse(nullstate,sp,edir);
			diffuse=diffcolor.energy();
		}
		CFLOAT sum=trans+diffuse;
		if(sum>0.0) sum=1.0/sum;
		diffuse*=sum;
		trans*=sum;
		if(sum>0.0)
		{
			if(ourRandom()<trans)
			{
				transcolor*=1.0/trans;
				photon.filter(transcolor); //no need for fresnel cause this is an aproximation
				shoot(photon,refract(sp.N(),-dir,caus_IOR),depth,cdepth+1,storeFirst,scene);
			}
			else
			{
				diffcolor*=1.0/diffuse;
				PFLOAT r1=ourRandom(), r2=ourRandom();
	 			vector3d_t refDir = HemiVec_CONE(Ng, sp.NU(), sp.NV(), 0.05, r1, r2);
				photon.filter(diffcolor);
				shoot(photon,refDir,depth+1,cdepth,storeFirst,scene);
			}
		}
		/*
		if((sp.getObject()->caustics()) && (cdepth<maxcdepth))
		{
			color_t caus_rcolor,caus_tcolor;
			PFLOAT caus_IOR;
			sp.getObject()->getCaustic(caus_rcolor, caus_tcolor, caus_IOR);
			photon.filter(caus_tcolor); //no need for fresnel cause this is an aproximation
			shoot(photon,refract(sp.N(),-dir,caus_IOR),depth,cdepth+1,storeFirst,scene);
			photon.color(originalcolor);
		}
		if(sp.getObject()->useForRadiosity())
		{
			energy_t ene(edir,photon.color());
			PFLOAT r1=ourRandom(), r2=ourRandom();
	 		vector3d_t refDir = HemiVec_CONE(Ng, sp.NU(), sp.NV(), 0.05, r1, r2);
			photon.filter(sha->getDiffuse(nullstate,sp,edir));
			shoot(photon,refDir,depth+1,cdepth,storeFirst,scene);
		}
		*/
		nullstate.skipelement=oldorigin;
	}
}

static PFLOAT fillArea(const vector<fPoint_t> &samples,PFLOAT offset)
{
	PFLOAT avgX=0,avgY=0;
	PFLOAT avgR=0;
	PFLOAT den=0;
	const PFLOAT SQRT2=sqrt(2.0);

	if(samples.size()<2) return 1.0;
	for(vector<fPoint_t>::const_iterator i=samples.begin();i!=samples.end();
			++i)
	{
		avgX+=(*i).x*(*i).weight;
		avgY+=(*i).y*(*i).weight;
		den+=(*i).weight;
	}
	if(den==0.0) return 1.0;
	avgX/=den;
	avgY/=den;
	for(vector<fPoint_t>::const_iterator i=samples.begin();i!=samples.end();
			++i)
	{
		PFLOAT dx=(*i).x-avgX;
		PFLOAT dy=(*i).y-avgY;
		avgR+=sqrt(dx*dx+dy*dy)*(*i).weight;
	}
	avgR=(avgR/den)*SQRT2*offset;
	avgR*=avgR;
	if(avgR<0.25) avgR=0.25;
	return avgR;
}

void globalPhotonLight_t::setIrradiance(compPhoton_t &cp)
{
	found.reserve(search+1);
	irradiance->gather(cp.photon.position(),cp.N,found,search,radius);
	color_t total(0,0,0);
	if(found.empty())
	{
		cp.irr=total;
		return;
	}
	if((found.size()==1) || (found.front().dis==0))
	{
		CFLOAT factor=found[0].photon->direction()*cp.N;
		if(factor<0.0) factor=0.0;
		cp.irr=found[0].photon->color()*factor;
		return;
	}
		
	PFLOAT farest=found.front().dis;
	//vector3d_t VX,VY;
	//createCS(cp.N,VX,VY);
	//fPoint_t point;
	//points.clear();
	for(vector< foundPhoton_t >::const_iterator i=found.begin();i!=found.end();++i)
	{
		const storedPhoton_t &p=*(i->photon);
		CFLOAT factor=(1.0 - i->dis/farest)*(p.direction()*cp.N);
		if(factor>0)
		{
			total+=factor*p.color();
			vector3d_t sub=p.position()-cp.photon.position();
			sub/=farest;
			//point.x=sub*VX;
			//point.y=sub*VY;
			//point.weight=factor*0.5;
			//points.push_back(point);
		}
	}
	PFLOAT area=M_PI*farest*farest;
	//const PFLOAT SQRT2=sqrt(2.0);
	//area*=fillArea(points,SQRT2);

	if(area<MIN_RAYDIST) area=MIN_RAYDIST;
	cp.irr=total*(4*M_PI/(area));
}

void globalPhotonLight_t::storeInHash(const runningPhoton_t &p,const vector3d_t &N) 
{
	storedPhoton_t nuevo(p);
  compPhoton_t &A=hash.findBox(nuevo.position());
	if(A.photon.direction().null())
	{
		A.photon=nuevo;
		//A.photon.direction(A.photon.direction()*A.photon.color().energy());
		A.N=N;
	}
	else if(nuevo.direction()*A.N>0.0)
	{
		//A.photon.direction(A.photon.direction()+nuevo.direction()*nuevo.color().energy());
		vector3d_t mixedd=A.photon.direction()*A.photon.color().energy() +
											nuevo.direction()*nuevo.color().energy();
		mixedd.normalize();
		A.photon.direction(mixedd);
		A.photon.color(A.photon.color()+nuevo.color());
	}
}

void globalPhotonLight_t::computeIrradiances()
{
	//vector<compPhoton_t> photons;
	//photons.reserve(hash.numBoxes());
	for(hash3d_t<compPhoton_t>::iterator i=hash.begin();
			i!=hash.end();++i)
	{
		if((*i).photon.direction().null()) continue;
		/*
		vector3d_t dir=(*i).photon.direction();
		dir.normalize();
		(*i).photon.direction(dir);
		*/
		irradiance->store((*i).photon);
	}
	irradiance->buildTree();
	//for(vector<compPhoton_t>::iterator i=photons.begin();i!=photons.end();++i)
	for(hash3d_t<compPhoton_t>::iterator i=hash.begin();i!=hash.end();++i)
		setIrradiance(*i);

	PFLOAT r=irradiance->getMaxRadius();
	delete irradiance;
	irradiance=new globalPhotonMap_t(r);

	for(hash3d_t<compPhoton_t>::iterator i=hash.begin();i!=hash.end();++i)
	//for(vector<compPhoton_t>::iterator i=photons.begin();i!=photons.end();++i)
	{
		(*i).photon.direction((*i).N);
		(*i).photon.color((*i).irr);
		irradiance->store((*i).photon);
	}
	
	irradiance->buildTree();
}

void globalPhotonLight_t::init(scene_t &scene)
{
	found.reserve(search+1);
	points.reserve(search);
	radius=photonMap->getMaxRadius();
	int numemitters=0;
	for(scene_t::light_iterator i=scene.lightsBegin();i!=scene.lightsEnd();++i)
	{
		emitter_t *e=(*i)->getEmitter(numPhotons);
		if(e!=NULL)
		{
			delete e;
			numemitters++;
		}
	}
	if(!numemitters) return;
	int photonsperlight=numPhotons/numemitters;
	list<emitter_t *> emitters;
	for(scene_t::light_iterator i=scene.lightsBegin();i!=scene.lightsEnd();++i)
	{
		emitter_t *e=(*i)->getEmitter(photonsperlight);
		if(e!=NULL) emitters.push_back(e);
	}
	point3d_t from;
	vector3d_t dir;
	color_t color;
	for(list<emitter_t *>::iterator i=emitters.begin();i!=emitters.end();++i)
	{
		bool storeFirst=(*i)->storeDirect();
		(*i)->numSamples(photonsperlight);
		for(int j=0;j<photonsperlight;++j)
		{
			(*i)->getDirection(j,from,dir,color);
			runningPhoton_t photon(color,from);
			shoot(photon,dir,0,0,storeFirst,scene);
		}
	}
	cout<<"Shot "<<photonsperlight<<" photons from each light of "<<numemitters<<endl;

	for(list<emitter_t *>::iterator i=emitters.begin();i!=emitters.end();++i) delete *i;

	photonMap->buildTree();
	cout<<"Stored "<<photonMap->count()<<endl;

	cout<<"Pre-gathering ...";cout.flush();

	computeIrradiances();
	cout<<" "<<irradiance->count()<<" OK\n";

	//hash.clear();
	scene.publishData("globalPhotonMap",photonMap);
	scene.publishData("irradianceGlobalPhotonMap",irradiance);
	scene.publishData("irradianceHashMap",&hash);
}
		
light_t *globalPhotonLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	PFLOAT radius=1.0;
	int maxdepth=2,maxcdepth=4,photons=50000,search=200;

	params.getParam("radius",radius);
	params.getParam("depth",maxdepth);
	params.getParam("caus_depth",maxcdepth);
	params.getParam("photons",photons);
	params.getParam("search",search);

	return new globalPhotonLight_t(radius,maxdepth,maxcdepth,photons,search);
}

pluginInfo_t globalPhotonLight_t::info()
{
	pluginInfo_t info;

	info.name="globalphotonlight";
	info.description="Takes every direct light in scene, shoots photons \
		from them and publishes the photon map";

	info.params.push_back(buildInfo<FLOAT>("radius",0,10000,1.0,"Search radius"));
	info.params.push_back(buildInfo<INT>("depth",1,50,2,"Number of photon bounces"));
	info.params.push_back(buildInfo<INT>("caus_depth",1,50,2,"Number of photon bounces inside caustic"));
	info.params.push_back(buildInfo<INT>("photons",1000,100000000,50000,"Number of photons"));
	info.params.push_back(buildInfo<INT>("search",10,5000,200,"Number of photons to blur"));

	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("globalphotonlight",globalPhotonLight_t::factory);
	std::cout<<"Registered globalphotonlight\n";
}

}
__END_YAFRAY
