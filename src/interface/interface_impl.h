/****************************************************************************
 *
 *      interface_impl.h:
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#ifndef __INTERFACE_IMPL_H
#define __INTERFACE_IMPL_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include <map>
#include <string>
#include "texture.h"
#include "shader.h"
#include "object3d.h"
#include "filter.h"
#include "light.h"
#include "background.h"
#include "yafsystem.h"

#include "interface.h"

__BEGIN_YAFRAY

class interfaceImpl_t : public yafrayInterface_t
{
	public:
		interfaceImpl_t(int ncpus=1,
				const std::string &pluginpath="/usr/local/lib/yafray");
		virtual ~interfaceImpl_t();
		
		void loadPlugins(const std::string &path);
		virtual void transformPush(float *m);
		virtual void transformPop();
		virtual void addObject_trimesh(const std::string &name,
				std::vector<point3d_t> &verts, const std::vector<int> &faces,
				std::vector<GFLOAT> &uvcoords, std::vector<CFLOAT> &vcol,
				const std::vector<std::string> &shaders, const std::vector<int> &faceshader,
				float sm_angle, bool castShadows, bool useR, bool receiveR, bool caus, bool has_orco,
				const color_t &caus_rcolor, const color_t &caus_tcolor, float caus_IOR);

		virtual void addObject_reference(const std::string &name,const std::string &original);

		// lights
		virtual void addLight(paramMap_t &p);
    // textures
		virtual void addTexture(paramMap_t &p);
		// shaders
		virtual void addShader(paramMap_t &p,std::list<paramMap_t> &modulators);
		// filters
		virtual void addFilter(paramMap_t &p);
		// backgrounds
		virtual void addBackground(paramMap_t &p);
		//camera
    virtual void addCamera(paramMap_t &p);
		//render
		virtual void render(paramMap_t &p);
		//render
		virtual void render(paramMap_t &p,colorOutput_t &output);
		
		virtual void clear();

		virtual shader_t *getShader(const std::string name)const;
		virtual texture_t *getTexture(const std::string name)const;

		virtual void repeatFirstPass();
		
		virtual void registerFactory(const std::string &name,light_factory_t *f);
		virtual void registerFactory(const std::string &name,shader_factory_t *f);
		virtual void registerFactory(const std::string &name,texture_factory_t *f);
		virtual void registerFactory(const std::string &name,filter_factory_t *f);
		virtual void registerFactory(const std::string &name,background_factory_t *f);

	protected:

		filter_t *filter_dof(paramMap_t &);
		filter_t *filter_antinoise(paramMap_t &);


		std::map<std::string,texture_t *> texture_table;
		std::map<std::string,shader_t *> shader_table;
		std::map<std::string,object3d_t *> object_table;
		std::map<std::string,camera_t *> camera_table;
		std::map<std::string,light_t *> light_table;
		std::map<std::string,filter_t *> filter_table;
		std::map<std::string,background_t *> background_table;
		int cpus;
		matrix4x4_t M;
		std::vector<matrix4x4_t> tstack;

		bool cachedPathLight;
		std::list<sharedlibrary_t> pluginHandlers;
		
		std::map<std::string,light_factory_t *> light_factory;
		std::map<std::string,shader_factory_t *> shader_factory;
		std::map<std::string,texture_factory_t *> texture_factory;
		std::map<std::string,filter_factory_t *> filter_factory;
		std::map<std::string,background_factory_t *> background_factory;
};

__END_YAFRAY

#endif
