/****************************************************************************
 *
 * 			photonlight.cc: Photon (caustics) light implementation 
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

#include "photonlight.h"
#include "spectrum.h"

using namespace std;
#include <algorithm>

__BEGIN_YAFRAY

#define WARNING cerr<<"[photonLight]: "

photon_t::photon_t(const color_t &color,const point3d_t &_pos)
{
	pos=_pos;
	c=color;
}

void photon_t::position(const point3d_t &_pos,PFLOAT bias)
{
	PFLOAT ad=(pos-_pos).length();
	if(ad>bias)
	{
		lastpos=pos;
	}
	pos=_pos;
}


//------------------------------------------------------------------------------------------

void insert(hash3d_t<photoAccum_t> &h,const photonMark_t &e)
{
  photoAccum_t &A=h.findBox(e.position());
			  
	A.dir+=e.direction();
	A.pos=A.pos+e.position();
	A.color+=e.color();
	A.count+=1;
}


photonLight_t::photonLight_t(const point3d_t &f,const point3d_t _to,PFLOAT angle,
		const color_t &c,CFLOAT inte,int np,int search,int maxd,int mind,PFLOAT b,
		PFLOAT disp, PFLOAT fr,PFLOAT clus,int mod, bool useqmc)
{
	from=f;
	to=_to;
	angle_cos=cos((angle/180.0)*M_PI);
	dangle=angle;
	K=search;
	color=c;
	//pow=inte*(angle/180.0);
	pow = inte * 2.0*M_PI*(1.0-angle_cos);
	fixedRadius=fr;
	cluster=clus;
	if(mod==DIFFUSE)
		Np=np/(maxd+1-mind);
	else
		Np=np;
	Np=(int)sqrt((float)Np) * (int)sqrt((float)Np);
	emitted=0;
	stored=0;
	maxdepth=maxd;
	mindepth=mind;
	bias=b;
	dispersion=K*disp*(angle/180.0);
	tree=NULL;
	hash=NULL;
	mode=mod;
	// QMC init
	HSEQ = NULL;
	use_QMC = useqmc;
	if (use_QMC) {
		int base=2, md=(maxdepth+1)*2;
		HSEQ = new Halton[md];
		for (int i=0;i<md;i++) {
			HSEQ[i].setBase(base);
			base = nextPrime(base);
		}
	}
	use_in_indirect=false;
}

void photonLight_t::shoot_photon_caustic(scene_t &scene, photon_t &photon,
					const vector3d_t &dir, PFLOAT dis)
{
	if (depth>maxdepth) return;
	depth++;
	surfacePoint_t sp;
	if (!scene.firstHit(nullstate, sp, photon.position(), dir)) { depth--;  return; }
	dis += sp.Z();
	const void *oldorigin = nullstate.skipelement;
	nullstate.skipelement = sp.getOrigin();

	const object3d_t* obj = sp.getObject();
	const shader_t* sha = sp.getShader();
	color_t caus_rcolor, caus_tcolor;
	PFLOAT caus_IOR;
	// try to get caustics colors and ior from shader first
	// (which really should be done in the first place anyway, simplified here, so textures are still not taken into account)
	// if not available, use params from object
	bool caustics = sha->getCaustics(nullstate, sp, dir, caus_rcolor, caus_tcolor, caus_IOR);
	if (!caustics)
	{
		caustics = obj->caustics();
		caus_rcolor = obj->caus_rcolor;
		caus_tcolor = obj->caus_tcolor;
		caus_IOR = obj->caus_IOR;
	}

	// for caustics, using pure random instead of QMC seq. looks better in this case
	if ((!caustics) || (ourRandom()<sha->getDiffuse(nullstate, sp, dir).energy()))
	{
		if (depth>1)
		{
			photon.position(sp.P(), bias);
			insert(*hash, photonMark_t(photon));
			stored++;
		}
	}
	else
	{
		photon.position(sp.P(), bias);
		vector3d_t edir = photon.lastPosition()-photon.position();
		edir.normalize();
		vector3d_t N = FACE_FORWARD(sp.Ng(),sp.N(),edir);
		CFLOAT kr, kt;
		fresnel(edir, sp.N(), caus_IOR, kr, kt);

		bool dorefl=(!caus_rcolor.null());
		if (dorefl)
		{
			vector3d_t newdir = reflect(N,edir);
			photon_t rphoton = photon;
			rphoton.filter(caus_rcolor*kr);
			shoot_photon_caustic(scene, rphoton, newdir, dis);
		}
		if (!caus_tcolor.null())
		{
			vector3d_t newdir;
			PFLOAT disp_pw, cyA, cyB;
			color_t beer;
			sha->getDispersion(disp_pw, cyA, cyB, beer);
			if (nullstate.chromatic && (disp_pw>0.0))
			{
				color_t dcol(1.0);
				// instead of totally randomly selecting wavelength, just use current photon number
				nullstate.cur_ior = getIORcolor(((CFLOAT)emitted+ourRandom())/(CFLOAT)Np, cyA, cyB, dcol);
				newdir = refract(sp.N(), edir, nullstate.cur_ior);
				nullstate.chromatic = false;
				if (!newdir.null())
				{
					photon_t tphoton = photon;
					tphoton.filter(caus_tcolor*dcol*kt);
					shoot_photon_caustic(scene, tphoton, newdir, dis);
				}
			}
			else {
				if (disp_pw>0.0)
					newdir = refract(sp.N(), edir, nullstate.cur_ior);
				else
					newdir = refract(sp.N(), edir, caus_IOR);
				if (!newdir.null())
				{
					photon_t tphoton = photon;
					color_t ctc(caus_tcolor);
					if ((disp_pw>0.0) && (!beer.null())) {
						color_t be(-sp.Z()*beer);
						be.set(exp(be.getR()), exp(be.getG()), exp(be.getB()));
						ctc *= be;
					}
					tphoton.filter(ctc*kt);
					shoot_photon_caustic(scene, tphoton, newdir, dis);
				}
			}
		}
	}
	nullstate.skipelement=oldorigin;
	depth--;
}

void photonLight_t::shoot_photon_diffuse(scene_t &scene,photon_t &photon,
		const vector3d_t &dir,PFLOAT dis)
{
	depth++;
	surfacePoint_t sp;
	if(!scene.firstHit(nullstate,sp,photon.position(),dir)) {depth--;return;}
	dis+=sp.Z();
	const void *oldorigin=nullstate.skipelement;
	nullstate.skipelement=sp.getOrigin();

	photon.position(sp.P(),bias);
	const shader_t *sha= sp.getShader();
	vector3d_t edir=photon.lastPosition()-photon.position();
	edir.normalize();
	vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
	vector3d_t Ng=FACE_FORWARD(sp.Ng(),sp.Ng(),edir);
	bool canreceive=(depth>mindepth) && (sp.getObject())->reciveRadiosity();

	if( canreceive )
	{
		insert(*hash,photonMark_t(photon));
		stored++;
	}
	if( (sp.getObject())->useForRadiosity() && (depth<=maxdepth) )
	{
		edir.normalize();
		energy_t ene(edir,photon.color());
		PFLOAT r1, r2;
		if (use_QMC) {
 			int d2 = (depth<<1);
 			r1=HSEQ[d2].getNext();  r2=HSEQ[d2+1].getNext();
		}
		else { r1=ourRandom();  r2=ourRandom(); }
 		vector3d_t refDir = randomVectorCone(Ng, sp.NU(), sp.NV(), 0.05, r1, r2);
		color_t newcolor=sha->fromRadiosity(nullstate,sp,ene,refDir);
		photon.color(newcolor);
		shoot_photon_diffuse(scene,photon,refDir,dis);
	}

	nullstate.skipelement=oldorigin;
	depth--;
}

#ifdef HAVE_PTHREAD
typedef priority_queue<foundPhoton_t,vector<foundPhoton_t>,compareFound_f>
	photonQueue_t;
#else
typedef priority_queue<foundPhoton_t,vector<foundPhoton_t>,
	compareFound_f> photonQueue_t;
#endif

struct sample_t
{
	PFLOAT x,y,weight;
};

static PFLOAT fillArea(const vector<sample_t> &samples,PFLOAT offset)
{
	PFLOAT avgX=0,avgY=0;
	PFLOAT avgR=0;
	PFLOAT den=0;
	const PFLOAT SQRT2=sqrt(2.0);

	if(samples.size()<2) return 1.0;
	for(vector<sample_t>::const_iterator i=samples.begin();i!=samples.end();
			++i)
	{
		avgX+=(*i).x*(*i).weight;
		avgY+=(*i).y*(*i).weight;
		den+=(*i).weight;
	}
	if(den==0.0) return 1.0;
	avgX/=den;
	avgY/=den;
	for(vector<sample_t>::const_iterator i=samples.begin();i!=samples.end();
			++i)
	{
		PFLOAT dx=(*i).x-avgX;
		PFLOAT dy=(*i).y-avgY;
		avgR+=sqrt(dx*dx+dy*dy)*(*i).weight;
	}
	avgR=(avgR/den)*SQRT2*offset;
	//if(avgR>1.0) avgR=(SQRT2-avgR)/(SQRT2-1.0);
	//avgR*=offset;
	avgR*=avgR;
	if(avgR<0.25) avgR=0.25;//avgR=1.0-avgR;
	return avgR;
}

struct pointCross_f
{
	bool operator() (const point3d_t &p,const bound_t &b) {return b.includes(p);};
};

color_t photonLight_t::illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
															const vector3d_t &eye)const
{
	if(!sp.getObject()->reciveRadiosity()) return color_t(0, 0, 0);
	// Allocate an vector that will be used as a heap with
	// space for K photons. We are looking for the closest K
	// photons, so the exact size is known.
	vector<foundPhoton_t> found(0);
	found.reserve(K);
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), eye);
	gObjectIterator_t<photonMark_t *,point3d_t,pointCross_f> ite(tree,sp.P());
	for(;!ite;++ite)
	{
		// First check if the direction of the photon is appropriate
		// for the current surface position. This is quite cheap, so
		// do it before testing if the photon is in the search radius.
		// However, this test is not only for early rejection but 
		// also a required property for a gathered photon.
		if (((*ite)->direction()*N)<=0.0)
		{
			continue;
		}
		// Now check if the photon is so close that it should be put
		// in the heap.
		const PFLOAT dis = (sp.P()-(*ite)->position()).length();		
		if (dis<=fixedRadius)
		{
			if(found.size()==K)
			{
				if(dis<found.front().dis)
				{
					// Swap the most distant photon at the end.
					pop_heap(found.begin(),found.end(), cfound);
					// Override it with the current photon.
					found.back().p = *ite;
					found.back().dis = dis;
					// Heap the new photon into the heap.
					push_heap(found.begin(),found.end(), cfound);
				}
			}
			else
			{
				// Put the photon in the vector and heap it.
				foundPhoton_t temp;
				temp.p = *ite;
				temp.dis = dis;
				found.push_back(temp);
				push_heap(found.begin(),found.end(), cfound);
			}
		}
	}

	if(found.size()<2) return color_t(0,0,0);

	const PFLOAT radius=found.front().dis;
	color_t total(0.0);

	const shader_t *sha= sp.getShader();
	vector<sample_t> samples(0);
	vector3d_t VX,VY;
	if(mode==DIFFUSE)
	{
		createCS(N,VX,VY);
		samples.reserve(found.size());
	}

	for(vector<foundPhoton_t>::iterator i=found.begin();i!=found.end();++i)
	{
		const photonMark_t *p=i->p;
		const PFLOAT dis=i->dis;
		const CFLOAT filter=filterCone(dis,radius);
		const energy_t ene(p->direction(),p->color()*filter);
		total += sha->fromLight(state,sp,ene,eye);
		if(mode==DIFFUSE)
		{
			vector3d_t sub=p->position()-sp.P();
			sub/=radius;
			sample_t sample;
			sample.x=sub*VX;
			sample.y=sub*VY;
			sample.weight=(p->direction()*N)*filter*0.5;
			samples.push_back(sample);
		}
	}
	PFLOAT area = M_PI*radius*radius;
	if(mode==DIFFUSE)
	{
		const PFLOAT SQRT2=1.414213562373095;
		area*=fillArea(samples,SQRT2);
	}	
	const PFLOAT minarea=4.0*M_PI/(PFLOAT)Np;
	if(area<minarea)
	{
		area=minarea;
	}
	const CFLOAT factor=4.0*M_PI/((CFLOAT)Np*area);
	total *= factor;
	return total;
}

void photonLight_t::preGathering()
{
	photons.clear();
	photons.reserve(hash->numBoxes());
	for(hash3d_t<photoAccum_t>::iterator i=hash->begin();
			i!=hash->end();++i)
	{
		vector3d_t dir=(*i).dir;
		if(dir.null()) continue;
		dir.normalize();
		point3d_t pos=(*i).pos/(PFLOAT)(*i).count;
		photonMark_t newp(dir,pos,(*i).color);
		photons.push_back(newp);
	}
}

static PFLOAT bound_add;

bound_t photon_calc_bound_fixed(const vector<photonMark_t *> &v)
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
	minx-=bound_add;
	miny-=bound_add;
	minz-=bound_add;
	maxx+=bound_add;
	maxy+=bound_add;
	maxz+=bound_add;
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

bool photon_is_in_bound(photonMark_t * const & p,bound_t &b)
{
	return b.includes(p->position());
}

point3d_t photon_get_pos(photonMark_t * const & p)
{
	return p->position();
}


void photonLight_t::init(scene_t &scene)
{
	fprintf(stderr,"Shooting photons ... ");
	vector3d_t dir;
	vector3d_t light_dir=to-from;
	light_dir.normalize();
	depth=0;
	randStep=1.0/sqrt((PFLOAT)Np);

	// needed for cone
	vector3d_t LU, LV;
	createCS(light_dir, LU, LV);

	if(mode==DIFFUSE)
		hash=new hash3d_t<photoAccum_t>(cluster,(maxdepth+1-mindepth)*Np/10+1);
	else
		hash=new hash3d_t<photoAccum_t>(cluster,Np/10+1);

	for(int i=0;emitted<Np;++i)
	{
		photon_t photon(color*pow,from);
		PFLOAT r1, r2;
		if (use_QMC) { r1=HSEQ[0].getNext();  r2=HSEQ[1].getNext(); }
		else { r1=ourRandom();  r2=ourRandom(); }
		dir = randomVectorCone(light_dir, LU, LV, angle_cos, r1, r2);
		if (dir.null()) continue;
		nullstate.chromatic = true;
		if (mode==CAUSTIC) shoot_photon_caustic(scene, photon, dir);
		if (mode==DIFFUSE) shoot_photon_diffuse(scene, photon, dir);
		emitted++;
	}

	cerr << "OK\nEmitted " << emitted << " Stored " << stored << " search " << K << endl;
	cerr << "Pre-Gathering ("<<hash->numBoxes()<<") ... ";
	preGathering();
	delete hash;hash=NULL;

	vector<photonMark_t *> lpho(photons.size());
	for(vector<photonMark_t>::iterator i=photons.begin();i!=photons.end();++i)
		lpho[i-photons.begin()]=&(*i);

	bound_add=fixedRadius;
	if(tree!=NULL) delete tree;
	tree=buildGenericTree(lpho,photon_calc_bound_fixed,photon_is_in_bound,
				photon_get_pos,8);

	cerr<<"OK "<<photons.size()<<" photons kept\n";

}
/*
struct compX_f
{
	bool operator () (const photon_t *a,const photon_t *b)
	{return (a->position().x)<(b->position().x);};
};
struct compY_f
{
	bool operator () (const photon_t *a,const photon_t *b) 
	{return (a->position().y)<(b->position().y);};
};
struct compZ_f
{
	bool operator () (const photon_t *a,const photon_t *b) 
	{return (a->position().z)<(b->position().z);};
};

void photonMap_t::load(vector<photon_t> &vp)
{
	if(!vp.size())
		root=NULL;
	else
	{
		lpho.resize(vp.size());
		for(vector<photon_t>::iterator i=vp.begin();i!=vp.end();++i)
			lpho[i-vp.begin()]=&(*i);
		root=buildPhotonTree(lpho.begin(),lpho.end());
	}
}

bound_t photon_calc_bound(const vector<photon_t *>::iterator begin,
		const vector<photon_t *>::iterator end)
{
	int size=end-begin;
	if(size==0) return bound_t(point3d_t(),point3d_t());
	double maxarea=0;
	double maxx,maxy,maxz,minx,miny,minz;
	maxx=minx=begin[0]->position().x;
	maxy=miny=begin[0]->position().y;
	maxz=minz=begin[0]->position().z;
	
	for(vector<photon_t *>::const_iterator i=begin;i!=end;++i)
	{
		const point3d_t &p=(*i)->position();
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
		if((*i)->A>maxarea) maxarea=(*i)->A;
	}
	minx-=maxarea;
	miny-=maxarea;
	minz-=maxarea;
	maxx+=maxarea;
	maxy+=maxarea;
	maxz+=maxarea;
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

photonNode_t * photonMap_t::buildPhotonTree(vector<photon_t *>::iterator begin,
		vector<photon_t *>::iterator end)
{
	photonNode_t *node=new photonNode_t;
	node->parent=NULL;
	node->bound=photon_calc_bound(begin,end);
	if((end-begin)<8)
	{
		node->begin=begin;
		node->end=end;
		return node;
	}
	else node->begin=node->end=begin;

	float minx=begin[0]->position().x,miny=begin[0]->position().y,
				minz=begin[0]->position().z;
	float maxx=minx,maxy=miny,maxz=minz,x,y,z;
	for(vector<photon_t *>::iterator i=begin;i!=end;++i)
	{
		x=(*i)->position().x;
		y=(*i)->position().y;
		z=(*i)->position().z;
		if(x<minx) minx=x;else if(x>maxx) maxx=x;
		if(y<miny) miny=y;else if(y>maxy) maxy=y;
		if(z<minz) minz=z;else if(z>maxz) maxz=z;
	}
	float lx=maxx-minx,ly=maxy-miny,lz=maxz-minz;
	float mins,maxs;
	int mid=(end-begin)/2;
	if((lx>ly) && (lx>lz))
	{
		compX_f comp;
		node->divtype=SPLITX;
		nth_element(begin,begin+mid,end,comp);
		maxs=mins=(*(begin+mid))->position().x;
		for(vector<photon_t *>::iterator i=begin;i!=(begin+mid);++i)
			if((*i)->position().x>maxs) maxs=(*i)->position().x;
	}
	else if((ly>lx) && (ly>lz))
	{
		compY_f comp;
		node->divtype=SPLITY;
		nth_element(begin,begin+mid,end,comp);
		maxs=mins=(*(begin+mid))->position().y;
		for(vector<photon_t *>::iterator i=begin;i!=(begin+mid);++i)
			if((*i)->position().y>maxs) maxs=(*i)->position().y;
	}
	else
	{
		compZ_f comp;
		node->divtype=SPLITZ;
		nth_element(begin,begin+mid,end,comp);
		maxs=mins=(*(begin+mid))->position().z;
		for(vector<photon_t *>::iterator i=begin;i!=(begin+mid);++i)
			if((*i)->position().z>maxs) maxs=(*i)->position().z;
	}
	node->left=buildPhotonTree(begin,begin+mid);
	node->right=buildPhotonTree(begin+mid,end);
	node->left->parent=node;
	node->right->parent=node;
	return node;
}

inline void upFirstRight(photonNode_t * &node,const point3d_t &P)
{
	photonNode_t *child=node;
	node=node->parent;
	while(node!=NULL)
	{
		if((child==node->left) && node->right->bound.includes(P))
		{
			node=node->right;
			return;
		}
		child=node;
		node=node->parent;
	}
}
		
inline void DownLeft(photonNode_t * &node,const point3d_t &P)
{
	while(!node->isLeaf())
	{
		if(node->left->bound.includes(P))
			node=node->left;
		else if(node->right->bound.includes(P))
			node=node->right;
		else return;
	}
}

void photonMap_t::find(priority_queue<foundPhoton_t,vector<foundPhoton_t>,
				compareFound_f> &found,const point3d_t &P,unsigned int K)const 
{
	if(root==NULL) return;
	photonNode_t *node=root;
	if(!node->bound.includes(P)) return;
	while(node!=NULL)
	{
		DownLeft(node,P);
		while(!node->isLeaf())
		{
			upFirstRight(node,P);
			if(node==NULL) return;
			DownLeft(node,P);
		}
		foundPhoton_t temp;
		for(vector<photon_t *>::iterator p=node->begin;p!=node->end;++p)
		{
			temp.dis=(P-(*p)->position()).length();
			temp.p=(*p);
		
			if(found.size()==K)
			{
				if(temp.dis<found.top().dis)
				{
					found.push(temp);
					found.pop();
				}
			}
			else
				found.push(temp);
		}
		upFirstRight(node,P);
	}
	
}
*/

light_t *photonLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0,0,1),to(0,0,0);
	color_t color(1,1,1);
	CFLOAT power=1.0,angle=45.0;
	PFLOAT bias=0.001,disp=50.0,fr=1,cluster=1;
	int nphotons=5000,search=50,depth=3,mindepth=1;
	int mode=CAUSTIC;
	string _smode;
	const string *smode=&_smode;
	bool useqmc=false;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("color", color);
	params.getParam("photons", nphotons);
	params.getParam("search", search);
	params.getParam("power", power);
	params.getParam("angle", angle);
	params.getParam("depth", depth);
	params.getParam("mindepth", mindepth);
	params.getParam("bias", bias);
	params.getParam("use_QMC", useqmc);
	if(params.getParam("dispersion",disp))
		WARNING<<"Dispersion value is deprecated, use fixedradius only.\n";
	params.getParam("mode",smode);
	if(!params.getParam("fixedradius",fr))
		WARNING<<"Missing fixedradius, using default won't work.\n";

	if(*smode=="diffuse") mode=DIFFUSE;

	if(!params.getParam("cluster",cluster))
	{
		cluster=fr/4;
		WARNING<<"Cluster value missing in photonlight, using "<<
			cluster<<endl;
	}

	return new photonLight_t(from,to,angle,color,power,nphotons,
			search,depth,mindepth,bias,disp,fr,cluster,mode, useqmc);
}

pluginInfo_t photonLight_t::info()
{
	pluginInfo_t info;

	info.name="photonlight";
	info.description="Single directional photonlight for caustics";

	info.params.push_back(buildInfo<POINT>("from","Light position"));
	info.params.push_back(buildInfo<POINT>("to","Target of the light"));
	info.params.push_back(buildInfo<COLOR>("color","Color of the light"));
	info.params.push_back(buildInfo<INT>("photons",1000,100000000,5000,"Number of photons"));
	info.params.push_back(buildInfo<INT>("search",10,1000,50,"Number of photons to blur"));
	info.params.push_back(buildInfo<FLOAT>("power",0,10000,1,"Light power"));
	info.params.push_back(buildInfo<FLOAT>("angle",0,180,45,"Aperture of the cone"));
	info.params.push_back(buildInfo<FLOAT>("depth",1,50,3,"Number of photon boucnes"));
	info.params.push_back(buildInfo<FLOAT>("fixedradius",0,10000,1.0,"Photon search radius"));
	info.params.push_back(buildInfo<FLOAT>("cluster",0,10000,1.0,"Size of cluster \
				to join photons. Only one photon pe box (cluster sized)"));
	info.params.push_back(buildInfo<BOOL>("use_QMC","Whenever to use quasi montecarlo"));

			
	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("photonlight",photonLight_t::factory);
	std::cout<<"Registered photonlight\n";
}

}
__END_YAFRAY
