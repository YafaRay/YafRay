/****************************************************************************
 *
 *                      scene.h: Scene manipulation and rendering api
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
#ifndef __THREADEDSCENE_H
#define __THREADEDSCENE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#if HAVE_PTHREAD
#include<pthread.h>
#include <semaphore.h>
#include "jobdealer.h"

#include<map>

#include "scene.h"
#include "ccthreads.h"

__BEGIN_YAFRAY
class YAFRAYCORE_EXPORT threadedscene_t : public scene_t
{
	public:
		//virtual void renderPart(colorOutput_t &out, int curpass, int off);
		virtual void render(colorOutput_t &out);
		static scene_t *factory();
	protected:
		jobDealer_t<renderArea_t *> dealer;

		class renderWorker : public yafthreads::thread_t
		{
			public:
				renderWorker(threadedscene_t &s):fake(false),scene(&s) {};
				virtual void body();

				bool fake;
			protected:
				threadedscene_t *scene;
		};
};

__END_YAFRAY

#endif // PTHREAD

#endif // __THREADEDSCENE_H
