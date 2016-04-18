/****************************************************************************
 *
 *      render.h: Yafray fileformat semantic proccesor api
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
#ifndef __RENDER_H
#define __RENDER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include <map>
#include "msin.h"
#include "texture.h"
#include "filter.h"
#include "light.h"
#include "background.h"
#include "yafsystem.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

__BEGIN_YAFRAY

class render_t : public renderEnvironment_t
{
	public:
                typedef enum {MONO, THREAD, FORK} strategy_t;

                render_t(int ncpus=1, strategy_t strat = THREAD,
                         const std::string &plugin_path="/usr/local/lib/yafray");
		~render_t();

		void setRegion(PFLOAT mix,PFLOAT max,PFLOAT miy,PFLOAT may)
		{ scxmin=mix;scxmax=max;scymin=miy;scymax=may;};

		void loadPlugins(const std::string &path);

		void * itemList(ast_t *);
		void * transform(ast_t *);

		void * texture(ast_t *);
		texture_t * texture_image(paramMap_t &);
		texture_t * texture_clouds(paramMap_t &);
		texture_t * texture_marble(paramMap_t &);
		texture_t * texture_wood(paramMap_t &);

		void * filter(ast_t *);
		filter_t *filter_dof(paramMap_t &);
		filter_t *filter_antinoise(paramMap_t &);

		void * shader(ast_t *);

		void * object(ast_t *);
		void * mesh(ast_t *);
		void * sphere(ast_t *);

		void * light(ast_t *);

		background_t *background_image(paramMap_t &);
		background_t *background_HDRI(paramMap_t &);
		background_t *background_sunsky(paramMap_t &);
		background_t *background_const(paramMap_t &);

		void * background(ast_t *);

		void * camera(ast_t *);
		void * render(ast_t *);

		void * call(ast_t *ast)
		{
			int n=ast->id;
			if((n>MAX_AST) || (n<0))
			{
				cout<<"Internal error ast = "<<n<<endl;
				return NULL;
			}
			if(handler[n]==(void * (render_t::*)(ast_t *))NULL)
			{
				cout<<"Unimplemented visitor for ast = "<<n<<endl;
				return NULL;
			}
			return (this->* handler[n])(ast);
		}
		virtual shader_t *getShader(const std::string name)const;
		virtual texture_t *getTexture(const std::string name)const;

		virtual void repeatFirstPass();
		
		virtual void registerFactory(const std::string &name,light_factory_t *f);
		virtual void registerFactory(const std::string &name,shader_factory_t *f);
		virtual void registerFactory(const std::string &name,texture_factory_t *f);
		virtual void registerFactory(const std::string &name,filter_factory_t *f);
		virtual void registerFactory(const std::string &name,background_factory_t *f);

	protected:

		typedef void * (render_t::*mPointer)(ast_t *);
		std::map<std::string,texture_t *> texture_table;
		std::map<std::string,shader_t *> shader_table;
		std::map<std::string,object3d_t *> object_table;
		std::map<std::string,camera_t *> camera_table;
		std::map<std::string,light_t *> light_table;
		std::map<std::string,filter_t *> filter_table;
		std::map<std::string,background_t *> background_table;
		int cpus;
                strategy_t strategy;
		PFLOAT scxmin,scxmax,scymin,scymax;
		matrix4x4_t M;

		void * (render_t::* handler[MAX_AST] )(ast_t *);
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
