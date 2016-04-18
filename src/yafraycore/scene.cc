/****************************************************************************
 *
 * 			scene.cc: Scene manipulation and rendering implementation 
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

#include "scene.h"
#include "matrix4.h"
#include <cstdio>
#include <cstdlib>
#include<fstream>
#include "ipc.h"
#include "renderblock.h"
#include "geometree.h"


using namespace std;

__BEGIN_YAFRAY

int bcount;
int pcount;

renderState_t::renderState_t() :raylevel(0),depth(0),contribution(1.0),/*lastobject(NULL)
	,lastobjectelement(NULL),*/ skipelement(NULL),currentPass(0),rayDivision(1),traveled(0)
	,pixelNumber(0), chromatic(true), cur_ior(1)
{
}

renderState_t::~renderState_t() 
{
}

scene_t::scene_t()
{
	//min_raydis=0.1;
	//min_raydis=0.00000000000001;
	min_raydis=MIN_RAYDIST;
        cpus = 1;
	self_bias=0.1;
	maxraylevel=3;
	world_resolution=1.0;
	radio_light=NULL;
	BTree=NULL;
	background=NULL;
	repeatFirst=false;
	scymin=scxmin=-2;
	scymax=scxmax=2;
	alpha_maskbackground = alpha_premultiply = false;
	clamp_rgb = false;
}

scene_t::~scene_t()
{
	/*
	for(list<object3d_t *>::iterator ite=obj_list.begin();
			ite!=obj_list.end();ite++)
		delete *ite;
	delete render_camera;
	if(radio_light!=NULL) delete radio_light;
	*/
}

void scene_t::addObject(object3d_t *obj) 
{
	obj_list.push_back(obj);
}

void scene_t::addLight(light_t *light) 
{
	light_list.push_back(light);
}

void scene_t::addFilter(filter_t *filter) 
{
	filter_list.push_back(filter);
}

void scene_t::publishVoidData(const std::string &key,const void *data)
{
	published[key]=data;
}

void scene_t::setCamera(camera_t *cam)
{
	render_camera=cam;
	world_resolution=(1.0/(PFLOAT)cam->resX())/cam->getFocal();
	cerr<<"Using a world resolution of "<<world_resolution<<" per unit\n";
}

bool scene_t::isShadowed(renderState_t &state,const surfacePoint_t &sp,
		const point3d_t &l)const
{
	point3d_t p=sp.P();
	surfacePoint_t temp;
	vector3d_t ray=(l-p);
	PFLOAT dist=ray.length();
	ray.normalize();
	point3d_t self=p+ray*self_bias;
	p=p+ray*min_raydis;
	/*
	const object3d_t * &lasto=state.lastobject;

	if(lasto!=NULL)
	{
		if(lasto==sp.getObject())
		{
			if(lasto->shoot(state,temp,self,ray,true,dist)) return true;
		}
		else
			if(lasto->shoot(state,temp,p,ray,true,dist)) return true;
	}
	*/
	//for(objectIterator_t ite(*BTree,p,ray,dist);!ite;ite++)
	for(geomeIterator_t<object3d_t> ite(BTree,dist,p,ray);!ite;ite++)
	{
		if( (*ite)->castShadows()/* && (*ite!=lasto) */)
		{

			if(*ite==sp.getObject())
			{
				if((*ite)->shoot(state,temp,self,ray,true,dist)) {/*lasto=*ite;*/return true;}
			}
			else
				if((*ite)->shoot(state,temp,p,ray,true,dist)) {/*lasto=*ite;*/return true;}
		}
	}
	//lasto=NULL;
	return false;
}

bool scene_t::isShadowed(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &dir)const
{
	point3d_t p=sp.P();
	surfacePoint_t temp;
	vector3d_t ray=dir;
	ray.normalize();
	point3d_t self=p+ray*self_bias;
	p=p+ray*min_raydis;
	/*
	const object3d_t * &lasto=state.lastobject;

	if(lasto!=NULL)
	{
		if(lasto==sp.getObject())
		{
			if(lasto->shoot(state,temp,self,ray,true)) return true;
		}
		else
			if(lasto->shoot(state,temp,p,ray,true)) return true;
	}
	*/
	//for(objectIterator_t ite(*BTree,p,ray);!ite;ite++)
	for(geomeIterator_t<object3d_t> ite(BTree,numeric_limits<PFLOAT>::infinity(),p,ray);!ite;ite++)
	{
		if( (*ite)->castShadows()/* && (*ite!=lasto) */)
		{
			if(*ite==sp.getObject())
			{
				if((*ite)->shoot(state,temp,self,ray,true)) {/*lasto=*ite;*/return true;}
			}
			else
				if((*ite)->shoot(state,temp,p,ray,true)) {/*lasto=*ite;*/return true;}
		}
	}
	//lasto=NULL;
	return false;
}

// simple exponential fog
void scene_t::fog_addToCol(CFLOAT depth, color_t &curcol) const
{
	if (fog_density!=0.0) {
		if (depth==-1.0)
			curcol = fog_color;
		else {
			CFLOAT fgi = exp(-depth*fog_density);
			curcol = fgi*curcol + (1.0-fgi)*fog_color;
		}
	}
}

color_t scene_t::raytrace(renderState_t &state, const point3d_t &from, const vector3d_t & ray)const
{
	int &l_raylevel=state.raylevel;
	CFLOAT &l_depth=state.depth;
	++l_raylevel;
	if(l_raylevel>=maxraylevel)
	{
		l_raylevel--;
		l_depth=-1;
		return color_t(0,0,0);
	}
	point3d_t f=from+ray*min_raydis;
	surfacePoint_t sp,temp;
	bool found=false;
	//for(objectIterator_t ite(*BTree,f,ray);!ite;ite++)
	PFLOAT limit=numeric_limits<PFLOAT>::infinity();
	for(geomeIterator_t<object3d_t> ite(BTree,numeric_limits<PFLOAT>::infinity(),f,ray);!ite;ite++)
	{
		if((*ite)->shoot(state,temp,f,ray,false,limit))
		{
			if(temp.Z()>0.0)
			{
				limit=temp.Z();
				ite.limit(limit);
				if(!found)
				{
					sp=temp;
					found=true;
				}
				else if( temp.Z()<sp.Z()  ) sp=temp;
			}
		}
	}
	// need to set screen position in calculated surfacepoint here for possible win texmap.
	sp.setScreenPos(state.screenpos);
	if(found && (sp.getShader()!=NULL))
	{
		vector3d_t eye = from-sp.P();	// also now needed in displace
		eye.normalize();
		PFLOAT oldtraveled = state.traveled;
		state.traveled += sp.Z();
		sp.getShader()->displace(state, sp, eye, world_resolution);
		color_t res=light(state,sp,from);
		l_raylevel--;
		l_depth = (from-sp.P()).length();
		// add simple fog if enabled
		fog_addToCol(l_depth, res);
		state.traveled = oldtraveled;
		return res;
	}
	l_raylevel--;
	l_depth=-1;
	// don't include background if alpha_maskbackground flag set (only primary rays)
	color_t bg(0.0);
	if (!(alpha_maskbackground && (l_raylevel!=0))) bg = getBackground(ray, state);
	// add simple fog if enabled
	fog_addToCol(l_depth, bg);
	return bg;
}

color_t scene_t::light(renderState_t &state,const surfacePoint_t &sp,const point3d_t &from,
		bool indirect)const
{
		const shader_t *sha= sp.getShader();
		if(sha==NULL)
			return color_t(0,0,0);
		color_t flights(0,0,0);
		vector3d_t eye=from-sp.P();
		for(list<light_t *>::const_iterator ite=light_list.begin();
				ite!=light_list.end();++ite)
		{
			if(!indirect && !((*ite)->useInRender())) continue;
			if(indirect && !((*ite)->useInIndirect())) continue;
			flights+=(*ite)->illuminate(state,*this,sp,eye);
		}
		if(!indirect) flights+=sha->fromWorld(state,sp,*this,eye);
		return flights;
}

void scene_t::setupLights()
{
	fprintf(stderr,"Setting up lights ...\n");
	for(list<light_t *>::iterator ite=light_list.begin();ite!=light_list.end();
			++ite)
	{
		(*ite)->init(*this);
	}
	fprintf(stderr,"Finished setting up lights\n");
}

void scene_t::postSetupLights()
{
	for(list<light_t *>::iterator ite=light_list.begin();ite!=light_list.end();
			++ite)
		(*ite)->postInit(*this);
}
/*
bool scene_t::checkSampling()
{
	bool need = false;
	int resx = colorBuffer.resx();
	int resy = colorBuffer.resy();
	int jm, jp, im, ip, i, j;
	color_t u, c;
	bool needAA;
	for(i=0;i<resy;++i) {
		if ((im=i-1)<0) im=0;
		if ((ip=i+1)==resy) ip=resy-1;
		for(j=0;j<resx;++j) {

			if ((jm=j-1)<0) jm=0;
			if ((jp=j+1)==resx) jp=resx-1;

			// center color
			colorBuffer(j,i) >> c;

			//check neighbours
			while (true) {
				// (-1,-1)
				colorBuffer(jm, im) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (0,-1)
				colorBuffer(j, im) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (1,-1)
				colorBuffer(jp, im) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (-1,0)
				colorBuffer(jm, i) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (1,0)
				colorBuffer(jp, i) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (-1,1)
				colorBuffer(jm, ip) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (0,1)
				colorBuffer(j, ip) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				if (needAA) break;

				// (1,1)
				colorBuffer(jp, ip) >> u;
				needAA = ((c-u).abscol2bri()) >= AA_threshold;
				break;
			}

			if (needAA)
			{
				oversample(j,i) = 1;
				need = true;
			}
			else oversample(j,i)=0;
		}
	}
	return need;
}


bool scene_t::doOnePass(renderState_t &state,
                vector<color_t> &line,
		vector<CFLOAT> &dep,
		vector<CFLOAT> &alpha,int numline,int pass)
{
	CFLOAT &contri=state.contribution;
	CFLOAT &pdep=state.depth;
	int &globalpass=state.currentPass;
	color_t fcol;
	for(unsigned int j=0;j<line.size();++j)
	{
		if (!oversample(j,numline)) continue;

		vector3d_t ray;

		if (pass==0) {
			// only used with 'win' texture mapping
			screenpos.set(2.0*((PFLOAT)j/colorBuffer.resx())-1.0, 1.0-2.0*((PFLOAT)numline/colorBuffer.resy()), 0);
			ray=render_camera->shootRay(j, numline);
			alpha[j] = 0;
			dep[j] = -1;
			contri = 1.0;
			globalpass=0;
			state.pixelNumber=j+numline*colorBuffer.resx();
			if((screenpos.x<scxmin) || (screenpos.x>scxmax) || (screenpos.y<scymin) || (screenpos.y>scymax))
				fcol=color_t(0,0,0);
			else
				fcol = raytrace(state,render_camera->position(),ray);
			fcol.expgam_Adjust(exposure, gamma_R);
			line[j] = fcol;
			if (pdep>=0) {
				alpha[j] = 1;
				dep[j] = pdep;
			}
		}
		else {
			color_t totcol(0.0);
			PFLOAT fx, fy, totsamdiv = AA_minsamples*AA_passes;
			if (totsamdiv!=0) totsamdiv = 1.0/totsamdiv;
			unsigned int cursam;
			CFLOAT tot_alpha = 0;	// alpha AA
			state.pixelNumber=j+numline*colorBuffer.resx();
			for (int ms=0;ms<AA_minsamples;ms++) {
			  globalpass = cursam = (pass-1)*AA_minsamples + ms;
			  fx = AA_pixelwidth*(RI_vdC(cursam) - 0.5);
			  fy = AA_pixelwidth*((cursam+0.5)*totsamdiv - 0.5);
				// only used with 'win' texture mapping
				screenpos.set(2.0*(((PFLOAT)j+fx)/colorBuffer.resx())-1.0, 1.0-2.0*(((PFLOAT)numline+fy)/colorBuffer.resy()), 0);
			  ray = render_camera->shootRay((PFLOAT)j+fx, (PFLOAT)numline+fy);
				if((screenpos.x<scxmin) || (screenpos.x>scxmax) || (screenpos.y<scymin) || (screenpos.y>scymax))
					fcol=color_t(0,0,0);
				else
			  	fcol = raytrace(state,render_camera->position(), ray);
			  fcol.expgam_Adjust(exposure, gamma_R);
			  totcol += fcol;
			  if (pdep>=0) tot_alpha += 1;
			}
			CFLOAT mf = (CFLOAT)((pass-1)*AA_minsamples+1);
			CFLOAT df = 1.0/(mf+(CFLOAT)AA_minsamples);
			line[j] = (mf*line[j] + totcol) * df ;
			alpha[j] = (mf*alpha[j] + tot_alpha) * df;
		}
	}
	return true;
}


bool scene_t::doAllPasses(renderState_t &state,
                vector<color_t> &line,
		vector<CFLOAT> &dep,
		vector<CFLOAT> &alpha,int numline)
{
	CFLOAT &contri=state.contribution;
	CFLOAT &pdep=state.depth;
	int &globalpass=state.currentPass;
	color_t fcol;
	vector3d_t ray;
	for(unsigned int j=0;j<line.size();++j)
	{
		color_t sum(0.0), avg(0.0), var(0.0);
		int i;
		PFLOAT fx, fy, sc;
		state.pixelNumber=j+numline*colorBuffer.resx();
		for(i=0;i<AA_onepass_max;++i)
		{
			// have to use Halton here
			fx = AA_pixelwidth*(HSEQ1.getNext() - 0.5);
			fy = AA_pixelwidth*(HSEQ2.getNext() - 0.5);
			// only used with 'win' texture mapping
			screenpos.set(2.0*(((PFLOAT)j+fx)/colorBuffer.resx())-1.0, 1.0-2.0*(((PFLOAT)numline+fy)/colorBuffer.resy()), 0);
			ray = render_camera->shootRay((PFLOAT)j+fx, (PFLOAT)numline+fy);
			contri=1.0;
			globalpass=i;
			alpha[j]=0;
			dep[j]=-1;
			if((screenpos.x<scxmin) || (screenpos.x>scxmax) || (screenpos.y<scymin) || (screenpos.y>scymax))
				fcol=color_t(0,0,0);
			else
				fcol = raytrace(state,render_camera->position(),ray);
			fcol.expgam_Adjust(exposure, gamma_R);
			sum += fcol;
			sc = 1.0/PFLOAT(i+1);
			avg = sum * sc;
			fcol -= avg;
			var += fcol*fcol;
			if ((i>=AA_minsamples) && ((var*sc).col2bri()<=AA_threshold)) break;
		}
		if(pdep>=0)
		{
			alpha[j]=1;
			dep[j]=pdep;
		}
		line[j]=avg;
	}
	return true;
}


void scene_t::renderPart(colorOutput_t &out, int curpass, int off)
{
	vector3d_t ray;
	color_t color;
	int resx,resy;
	int steps;
	int div;
	renderState_t state;
	resx=render_camera->resX();
	resy=render_camera->resY();
	state.raylevel=-1;
	vector<CFLOAT> dep(resx);
	vector<CFLOAT> alpha(resx);
	vector<color_t> line(resx);

	steps=resy/(int)(resy/STEPS);
	steps=cpus*((int)steps/cpus);
	div=resy/steps;

	int count=1;
	for(int i=off;i<resy;i+=cpus)
	{
		for(int j=0;j<resx;++j) {
			colorBuffer(j,i)>>line[j];
			// need to fill in alpha scan buffer too,
			// since AA is done on the alpha channel as well
                        if (curpass>0)
                            alpha[j]=ABuffer(j,i);
		}
		doPass(state,0, line, dep, alpha, i, curpass);
		for(int j=0;j<resx;++j)
		{
			colorBuffer(j,i)<<line[j];
                        if (curpass==0)
                            ZBuffer(j,i)=dep[j];
			// must save alpha for AA
			ABuffer(j,i) = alpha[j];
			out.putPixel(j, i,line[j],alpha[j]);
		}
		if((count%div)==0)
		{
			cout<<"#";
			cout.flush();
		}
		count++;
	}
}
*/


// Support for single-process rendering
void scene_t::render(colorOutput_t &out)
{
	int resx,resy;
	resx=render_camera->resX();
	resy=render_camera->resY();
	blockSpliter_t spliter(resx,resy,64);

	renderArea_t area;

	cout<<"Building bounding tree ... ";cout.flush();
	//BTree=new boundTree_t (obj_list);
	BTree=buildObjectTree (obj_list);
	cout<<"OK"<<endl;

	cout<<"Light setup ..."<<endl;
	setupLights();

	cout<<endl;
	
	while(repeatFirst)
	{
		cout<<"\rFake   pass: [";
		cout.flush();
		repeatFirst=false;
		blockSpliter_t fakespliter(resx,resy,64);
		int finished=0;
		
		while(!fakespliter.empty())
		{
			if((finished>0) && !(finished%10)) {cout<<"#";cout.flush();}
			fakespliter.getArea(area);
			fakeRender(area);
			if(!area.out(out))
			{
				cout<<"Aborted"<<endl;
				delete BTree;
				BTree=NULL;
				return;
			}
			finished++;
		}
		cout<<"#]"<<endl;
		postSetupLights();
	}
	cout<<endl;

	cout<<"\rRender pass: [";
	cout.flush();
	int finished=0;
	while(!spliter.empty())
	{
		if((finished>0) && !(finished%10)) {cout<<"#";cout.flush();}
		spliter.getArea(area);
		render(area);
		if(!area.out(out))
		{
			cout<<"Aborted"<<endl;
			delete BTree;
			BTree=NULL;
			return;
		}
		finished++;
	}
	cout<<"#]"<<endl;
	delete BTree;
	BTree=NULL;
	/*
	int resx,resy;
	int steps;
	resx=render_camera->resX();
	resy=render_camera->resY();
	vector3d_t ray;
	color_t color;


	fprintf(stderr,"Building the bounding tree ... ");
	fflush(stderr);
	BTree=new boundTree_t (obj_list);
	cout<<"OK\n";
	setupLights();

        cout<<"Rendering in monolithic mode ...\n";


	colorBuffer.set(resx,resy);
	ZBuffer.set(resx,resy);
	ABuffer.set(resx,resy);
	oversample.set(resx,resy);

	for(int i=0;i<resy;++i)
		for(int j=0;j<resx;++j)
			oversample(j,i)=1;


	steps=resy/(int)(resy/STEPS);


	cout << "0%";
	for(int i=0;i<((steps+1)/2)-3;++i) cout<<" ";
	cout << "50%";
	for(int i=0;i<((steps+1)/2)-3;++i) cout<<" ";
	cout << "100%\n";

	bool first=true;
	int numpass;
	if (AA_onepass_max) numpass=1; else numpass=AA_passes+1;
        cout << "Rendering in "<<numpass<<" passes\n";
	for(int pass=0;pass<numpass;++pass)
	{
		cout<<"\r[";
		for(int i=0;i<steps;++i) cout<<".";
		if (pass==0)
			cout << "] first render pass\r";
		else
			cout << "] AA pass " << pass << "\r";
		cout << "[";
		cout.flush();
                renderPart(out, pass,0);
		cout << "]                  ";

		if(repeatFirst && first)
		{
			first=false;
			--pass;
			cout<<"\nLight needs post init ...";cout.flush();
			postSetupLights();
			cout<<"OK, repeating first pass\n";
		}
		else if (!checkSampling()) break;
	}
	cout<<"\nRender finished\n";
	delete BTree;
	BTree=NULL;

	for(list<filter_t *>::iterator ite=filter_list.begin();ite!=filter_list.end();
			ite++)
		(*ite)->apply(colorBuffer,ZBuffer,ABuffer);

	for(int i=0;i<resy;++i)
		for(int j=0;j<resx;++j)
		{
			colorBuffer(j,i) >> color;
			out.putPixel(j, i, color, ABuffer(j, i));
		}
	*/
}


bool scene_t::firstHit(renderState_t &state,surfacePoint_t &sp,const point3d_t &from,
											const vector3d_t &ray,bool shadow)const
{
	surfacePoint_t temp;
	bool found=false;
	point3d_t f=from+ray*min_raydis;
	//for(objectIterator_t ite(*BTree,f,ray);!ite;ite++)
	for(geomeIterator_t<object3d_t> ite(BTree,numeric_limits<PFLOAT>::infinity(),f,ray);!ite;ite++)
	{
		if(shadow && !(*ite)->castShadows()) continue;
		if((*ite)->shoot(state,temp,f,ray))
		{
			if(temp.Z()>0.0)
			{
				ite.limit(temp.Z());
				if(!found)
				{
					sp=temp;
					found=true;
				}
				else if( temp.Z()<sp.Z()  ) sp=temp;
			}
		}
	}
	if(found && !shadow && (sp.getShader()!=NULL)) {
		vector3d_t eye = from-sp.P();		// is needed in displace
		eye.normalize();
		PFLOAT oldtraveled=state.traveled;
		state.traveled+=sp.Z();
		sp.getShader()->displace(state, sp, eye, world_resolution);
		state.traveled=oldtraveled;
	}
	return found;
}


void scene_t::render(renderArea_t &area) const
{
	renderState_t state;
	CFLOAT &contri=state.contribution;
	CFLOAT &pdep=state.depth;
	bool &chroma = state.chromatic;
	PFLOAT &cur_ior = state.cur_ior;
	int &globalpass=state.currentPass;
	int resx=render_camera->resX();
	int resy=render_camera->resY();
	// now includes alpha channel as well, no separate calculation
	colorA_t fcol;

	PFLOAT fx=0.5, fy=0.5;

	//First pass
	unsigned int sc1=0, sc2=0;
	PFLOAT wt;
	for(int i=area.Y;i<(area.Y+area.H);++i)
		for(int j=area.X;j<(area.X+area.W);++j)
		{
			if (AA_jitterfirst && (AA_passes!=0)) {
				fx = RI_vdC(++sc1);
				fy = RI_S(++sc2);
			}
			state.screenpos.set(2.0*(((PFLOAT)j+fx)/(PFLOAT)resx)-1.0, 
					1.0-2.0*(((PFLOAT)i+fy)/(PFLOAT)resy), 0);
			if ((state.screenpos.x>=scxmin) && (state.screenpos.x<scxmax) && 
					(state.screenpos.y>=scymin) && (state.screenpos.y<scymax))
			{
				state.raylevel = -1;
				vector3d_t ray = render_camera->shootRay((PFLOAT)j+fx, (PFLOAT)i+fy, wt);
				contri = 1.0;
				globalpass = 0;
				state.pixelNumber = j+i*resx;
				if (wt!=0.0) {
					chroma = true;
					cur_ior = 1.0;
					fcol = raytrace(state, render_camera->position(), ray);
					if (do_tonemap) fcol.expgam_Adjust(exposure, gamma_R, clamp_rgb);
					if (pdep>=0) fcol.setAlpha(1.0); else fcol.setAlpha(0.0);
					area.imagePixel(j,i) = fcol;
					area.depthPixel(j,i) = pdep;
				}
				else {
					area.imagePixel(j,i) = color_t(0.0);
					area.depthPixel(j,i) = numeric_limits<PFLOAT>::infinity();
				}
			}
			else area.imagePixel(j,i)=colorA_t(0.0);
		}

	PFLOAT totsamdiv = AA_minsamples*AA_passes;
	if (totsamdiv!=0) totsamdiv = 1.0/totsamdiv;
	for (int pass=0;pass<AA_passes;pass++)
	{
		area.checkResample(AA_threshold);
		for (int i=area.Y;i<(area.Y+area.H);++i)
			for (int j=area.X;j<(area.X+area.W);++j)
			{
				if (!area.resamplePixel(j,i)) continue;
				colorA_t totcol(0.0);
				state.pixelNumber = j+i*resx;
				unsigned int cursam=0;

				int totnumsam = 0;
				for (int ms=0;ms<AA_minsamples;ms++) 
				{
					globalpass = cursam = pass*AA_minsamples + ms;
					state.raylevel = -1;
					fx = 0.5 + AA_pixelwidth*(RI_LP(cursam+state.pixelNumber) - 0.5);
					fy = 0.5 + AA_pixelwidth*(cursam*totsamdiv - 0.5);
					//fx = 0.5 + AA_pixelwidth*(HSEQ1.getNext() - 0.5);
					//fy = 0.5 + AA_pixelwidth*(HSEQ2.getNext() - 0.5);
					state.screenpos.set(2.0*(((PFLOAT)j+fx)/(PFLOAT)resx)-1.0, 
							1.0-2.0*(((PFLOAT)i+fy)/(PFLOAT)resy), 0);
					vector3d_t ray = render_camera->shootRay((PFLOAT)j+fx, (PFLOAT)i+fy, wt);
					if ((wt!=0.0) && (state.screenpos.x>=scxmin) && (state.screenpos.x<scxmax) &&
							(state.screenpos.y>=scymin) && (state.screenpos.y<scymax))
					{
						chroma = true;
						cur_ior = 1.0;
						fcol = raytrace(state,render_camera->position(), ray);
						if (do_tonemap) fcol.expgam_Adjust(exposure, gamma_R, clamp_rgb);
						if (pdep>=0) fcol.setAlpha(1.0); else fcol.setAlpha(0.0);
						totcol += fcol;
						totnumsam++;
					}
				}
				CFLOAT mf = (CFLOAT)(pass*totnumsam+1);
				area.imagePixel(j,i) = (mf*area.imagePixel(j,i) + totcol) / (mf+(CFLOAT)totnumsam);
			}
	}

	if (alpha_premultiply) {
	for (int i=area.Y;i<(area.Y+area.H);++i)
		for (int j=area.X;j<(area.X+area.W);++j)
			area.imagePixel(j, i).alphaPremultiply();
	}
}

void scene_t::fakeRender(renderArea_t &area)const
{
	renderState_t state;
	CFLOAT &contri=state.contribution;
	bool &chroma = state.chromatic;
	PFLOAT &cur_ior = state.cur_ior;
	int &globalpass=state.currentPass;
	int resx=render_camera->resX();
	int resy=render_camera->resY();

	//First pass
	PFLOAT wt;
	for(int i=area.Y;i<(area.Y+area.H);++i)
		for(int j=area.X;j<(area.X+area.W);++j)
		{
			state.raylevel = -1;
			state.screenpos.set(2.0*(((PFLOAT)j+0.5)/(PFLOAT)resx)-1.0, 
					1.0-2.0*(((PFLOAT)i+0.5)/(PFLOAT)resy), 0);
			vector3d_t ray = render_camera->shootRay((PFLOAT)j+0.5, (PFLOAT)i+0.5, wt);
			contri = 1.0;
			globalpass = 0;
			state.pixelNumber = j+i*resx;
			chroma = true;
			cur_ior = 1.0;
			if ((wt!=0.0) && (state.screenpos.x>=scxmin) && (state.screenpos.x<scxmax) &&
					(state.screenpos.y>=scymin) && (state.screenpos.y<scymax))
				area.imagePixel(j, i) = raytrace(state, render_camera->position(), ray);
			else area.imagePixel(j, i) = colorA_t(0.0);
		}
}

scene_t *scene_t::factory()
{
	return new scene_t();
}

__END_YAFRAY
