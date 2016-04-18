/****************************************************************************
 *
 * 			scene.h: Scene manipulation and rendering api 
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
#ifndef __SCENE_H
#define __SCENE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

//#if HAVE_PTHREAD 
//#include <pthread.h> 
//#include <semaphore.h> 
//#include <map> 
//#ifdef WIN32 
//#include <winunistd.h> 
//#else 
//#include <unistd.h> 
//#endif 
//#endif 

#include <vector>
#include <list>

#include "tools.h"

__BEGIN_YAFRAY
class renderArea_t;
class scene_t;
class object3d_t;
template<class T> class geomeTree_t;

struct YAFRAYCORE_EXPORT renderState_t
{
	renderState_t();
	~renderState_t();

	int raylevel;
	CFLOAT depth;
	CFLOAT contribution;
	//const object3d_t * lastobject;
	//void * lastobjectelement;
	const void *skipelement;
	int currentPass;
	int rayDivision;
	context_t context;
	PFLOAT traveled;
	int pixelNumber;
	point3d_t screenpos;
	bool chromatic;
	PFLOAT cur_ior;

	protected:
		renderState_t(const renderState_t &r) {};//forbiden
};

__END_YAFRAY
#include "object3d.h"
#include "vector3d.h"
#include "camera.h"
#include "output.h"
#include "light.h"
#include "buffer.h"
#include "filter.h"
#include "bound.h"
#include "background.h"
#include "mcqmc.h"

#define STEPS 40

  #include <sys/types.h>
  //#include <sys/wait.h>
  #include <stdio.h>

#include "renderblock.h"

__BEGIN_YAFRAY


class YAFRAYCORE_EXPORT scene_t
{
	public:
		virtual ~scene_t();
		void setCamera(camera_t *cam);
		void addObject(object3d_t *obj);
		void addLight(light_t *light);
		void addFilter(filter_t *filter);
		color_t raytrace(renderState_t &state,const point3d_t &from,
				const vector3d_t & ray)const;
		bool isShadowed(renderState_t &state,const surfacePoint_t &p,
				const point3d_t &l)const;
		bool isShadowed(renderState_t &state,const surfacePoint_t &p,
				const vector3d_t &dir)const;
		void setupLights();
		void postSetupLights();

		/*
		bool checkSampling();

		bool doOnePass(renderState_t &state,
                                std::vector<color_t> &line,
				std::vector<CFLOAT> &dep,
				std::vector<CFLOAT> &alpha,int numline,int pass);
		bool doAllPasses(renderState_t &state,
                                std::vector<color_t> &line,
				std::vector<CFLOAT> &dep,
				std::vector<CFLOAT> &alpha,int numline);
		bool doPass(renderState_t &state,
				int thread,std::vector<color_t> &line,
				std::vector<CFLOAT> &dep,
				std::vector<CFLOAT> &alpha,int numline,int pass)
		{
			return doOnePass(state, line, dep, alpha, numline, pass);
		};

		virtual void renderPart(colorOutput_t &out, int curpass, int off);
		*/
		virtual void render(colorOutput_t &out);

		void render(renderArea_t &area)const;
		void fakeRender(renderArea_t &area)const;

		void setMaxRayDepth(int a) {maxraylevel=a;};
		int getMaxRayDepth()const {return maxraylevel;};
		bool firstHit(renderState_t &state,surfacePoint_t &sp,const point3d_t &p,
											const vector3d_t &ray,bool shadow=false)const;
		//bool firstHitRad(surfacePoint_t &sp,const point3d_t &p,
		//									const vector3d_t &ray)const;
		color_t light(renderState_t &state,const surfacePoint_t &sp,
				const point3d_t &from, bool indirect=false)const;
		void setBias(PFLOAT b) {self_bias=b;};
		PFLOAT selfBias()const {return self_bias;};

		void setBackground(const background_t *b) {background=b;};
		const color_t getBackground(const vector3d_t &dir, renderState_t &state, bool filtered=false) const
		{
			if (background==NULL) return color_t(0.0);
			return (*background)(dir, state, filtered);
		}

		void setCPUs(const int num) { cpus = num; }

		// gamma & exposure
		void setGamma(CFLOAT g) { gamma_R=0.0;  if (g!=0.0) gamma_R=1.0/g; }
		void setExposure(CFLOAT e) { exposure = -e; }
		void adjustColor(color_t &c)const { c.expgam_Adjust(exposure, gamma_R, clamp_rgb); }

		// fog
		void setFog(CFLOAT dens, const color_t &col) { fog_density=dens;  fog_color=col; }
		void fog_addToCol(CFLOAT depth, color_t &curcol) const;

		// new AA
		void setAASamples(int ps, int ms, PFLOAT pw, PFLOAT th, bool jf=false)
		{
			AA_passes = ps;
			AA_minsamples = ms;
			AA_samdiv = 1.0/(PFLOAT)ms;
			AA_pixelwidth = pw;
			AA_threshold = th;
			AA_jitterfirst = jf;
		}

		// for LDR output, it is useful to clamp light values in AA sampling
		// so that AA will look better in parts of the image where fast high contrast differences occur
		void clampRGB(bool cr) { clamp_rgb=cr; }
		// for float output, bypass tone mapping
		void tonemap(bool tm) { do_tonemap=tm; }
		// switch to enable/disable alpha premultiply, and background masking
		void alphaPremultiply(bool ap) { alpha_premultiply=ap; }
		void alphaMaskBackground(bool abm) { alpha_maskbackground=abm; }

		void setRepeatFirst() {repeatFirst=true;};
		bool getRepeatFirst()const {return repeatFirst;};
		PFLOAT getWorldResolution()const {return world_resolution;};
		point3d_t getCenterOfView()const {return render_camera->position();};
		PFLOAT getAspectRatio()const 
			{return (PFLOAT)(render_camera->resX())/(PFLOAT)(render_camera->resY());};

		typedef std::list<light_t *>::iterator light_iterator;
		typedef std::list<light_t *>::const_iterator const_light_iterator;

		light_iterator lightsBegin() {return light_list.begin();};
		const_light_iterator lightsBegin()const {return light_list.begin();};
		light_iterator lightsEnd() {return light_list.end();};
		const_light_iterator lightsEnd()const {return light_list.end();};

		void publishVoidData(const std::string &key,const void *data);

		template<class T>
		void publishData(const std::string &key,const T *data) {publishVoidData(key,(const void *)data);};
		template<class T>
		void getPublishedData(const std::string &key,const T *&data)const 
		{
			std::map<std::string,const void *>::const_iterator i=published.find(key);
			data=(i==published.end()) ? (const T *)NULL : (const T *)(i->second);
		};

		void setRegion(PFLOAT mix,PFLOAT max,PFLOAT miy, PFLOAT may)
		{ scxmin=mix;scxmax=max;scymin=miy;scymax=may;};

		static scene_t *factory();
	protected:
		scene_t();
		scene_t(const scene_t &s) {}; //forbiden

		camera_t *render_camera;
		int cpus;
		PFLOAT min_raydis;
		PFLOAT world_resolution;
		std::list<object3d_t *> obj_list;
		std::list<light_t *> light_list;
		std::list<filter_t *> filter_list;
		light_t *radio_light;
		int maxraylevel;
		//cBuffer_t colorBuffer;
		//fBuffer_t ZBuffer;
		//fBuffer_t ABuffer;
		//Buffer_t<char> oversample;
		
		//boundTree_t *BTree;
		geomeTree_t<object3d_t> *BTree;
		PFLOAT self_bias;
		const background_t *background;
		// exposure and gamma controls
		CFLOAT gamma_R, exposure;
		// simple fog, density & color
		CFLOAT fog_density;
		color_t fog_color;
		// new AA parameters
		int AA_passes, AA_minsamples;
		bool AA_jitterfirst;
		PFLOAT AA_pixelwidth, AA_threshold, AA_samdiv;
		// used to keep track of the screen sampling position, for 'win' texmap mode
		//point3d_t screenpos;
		PFLOAT scymin,scymax,scxmin,scxmax;
		bool repeatFirst;
		std::map<std::string,const void *> published;
		bool do_tonemap, clamp_rgb;
		bool alpha_premultiply, alpha_maskbackground;
};

__END_YAFRAY

#endif
