/****************************************************************************
 *
 *                      scene.cc: Scene manipulation and rendering implementation
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

#include "geometree.h"
#include "matrix4.h"
#include <cstdio>
#include <cstdlib>
#include<fstream>

using namespace std;

#if HAVE_PTHREAD

#include "threadedscene.h"
#include<pthread.h>
#include <semaphore.h>
#include<map>

sem_t pstop;


__BEGIN_YAFRAY

#ifndef WIN32

#include<signal.h>
#include<sys/time.h>

void blockSignals(sigset_t *origmask)
{
	sigset_t mask;
	sigfillset(&mask);
	if(pthread_sigmask(SIG_SETMASK, &mask, origmask))
	{
		cout<<"Error blocking signals"<<endl;
		exit(1);
	}
}

void restoreSignals(sigset_t *origmask)
{
	if(pthread_sigmask(SIG_SETMASK, origmask, NULL))
	{
		cout<<"Error restoring signals"<<endl;
		exit(1);
	}
}

bool underItimer()
{
	struct itimerval timer;
	getitimer(ITIMER_VIRTUAL, &timer);
	return (timer.it_value.tv_sec != 0) || (timer.it_value.tv_usec != 0);
}

#endif // WIN32

void threadedscene_t::renderWorker::body()
{
#ifndef WIN32
	sigset_t origmask;
	blockSignals(&origmask);
#endif
	renderArea_t *area=scene->dealer.giveMeWork();
	while(area!=NULL)
	{
		if(fake)
			((scene_t *)scene)->fakeRender(*area);
		else
			((scene_t *)scene)->render(*area);
		cout.flush();
		scene->dealer.imFinished(area);
		cout.flush();
		area=scene->dealer.giveMeWork();
	}
#ifndef WIN32
	restoreSignals(&origmask);
#endif
}

void threadedscene_t::render(colorOutput_t &out)
{
	int resx,resy;
	resx=render_camera->resX();
	resy=render_camera->resY();
	blockSpliter_t spliter(resx,resy,64);

	vector<renderArea_t> areas(cpus);
	vector<renderWorker *> workers;

	for(int i=0;i<cpus;++i) workers.push_back(new renderWorker(*this));

	cout<<"Building bounding tree ... ";cout.flush();
	BTree=buildObjectTree (obj_list);
	cout<<"OK"<<endl;

	cout<<"Light setup ..."<<endl;
	setupLights();
	cout<<endl<<"Launching "<<cpus<<" threads"<<endl;

#ifndef WIN32
	sigset_t origmask;
	blockSignals(&origmask);
#endif

	while(repeatFirst)
	{
		cout<<"\rFake   pass: [";
		cout.flush();
		repeatFirst=false;
		blockSpliter_t fakespliter(resx,resy,64);
		
		int total=fakespliter.size();
		for(int i=0;i<cpus;++i)
		{
			fakespliter.getArea(areas[i]);
			dealer.addWork(&(areas[i]));
		}
		for(int i=0;i<cpus;++i) 
		{
			workers[i]->fake=true;
			workers[i]->run();
		}
		int finished=0;
		while(finished<total)
		{
			if((finished>0) && !(finished%10)) {cout<<"#";cout.flush();}
			renderArea_t *finished_area=dealer.getFinished();
#ifndef WIN32
#ifdef linux
			/* WORKAROUND for linux. Since SIGVTALRM caunts thread
			 * time instead of process time, linux hardly rises it.
			 *
			 * This is a fix for blender to catch ESC key. We have to 
			 * generate the signal ourselves.
			 *
			 */
			if(underItimer()) kill(getpid(), SIGVTALRM);
#endif
			restoreSignals(&origmask);
#endif
			if(!finished_area->out(out))
			{
				cout<<"Aborted"<<endl;
				for(int i=0;i<cpus;++i) dealer.addWork(NULL);
				for(int i=0;i<cpus;++i) workers[i]->wait();
				for(int i=0;i<cpus;++i) delete workers[i];
				delete BTree;
				BTree=NULL;
				return;
			}
#ifndef WIN32
			blockSignals(&origmask);
#endif
			if(!fakespliter.empty())
			{
				fakespliter.getArea(*finished_area);
				dealer.addWork(finished_area);
			}
			finished++;
		}
		for(int i=0;i<cpus;++i) dealer.addWork(NULL);
		for(int i=0;i<cpus;++i) workers[i]->wait();
		cout<<"#]"<<endl;
		postSetupLights();
	}
	cout<<endl;

	cout<<"\rRender pass: [";
	cout.flush();

	int total=spliter.size();
	for(int i=0;i<cpus;++i)
	{
		spliter.getArea(areas[i]);
		dealer.addWork(&(areas[i]));
	}
	for(int i=0;i<cpus;++i) 
	{
		workers[i]->fake=false;
		workers[i]->run();
	}
	int finished=0;
	while(finished<total)
	{
		if((finished>0) && !(finished%10)) {cout<<"#";cout.flush();}
		renderArea_t *finished_area=dealer.getFinished();
#ifndef WIN32
#ifdef linux
		if(underItimer()) kill(getpid(), SIGVTALRM);
#endif
		restoreSignals(&origmask);
#endif
		if(!finished_area->out(out))
		{
			cout<<"Aborted"<<endl;
			for(int i=0;i<cpus;++i) dealer.addWork(NULL);
			for(int i=0;i<cpus;++i) workers[i]->wait();
			for(int i=0;i<cpus;++i) delete workers[i];
			delete BTree;
			BTree=NULL;
			return;
		}
#ifndef WIN32
		blockSignals(&origmask);
#endif
		if(!spliter.empty())
		{
			spliter.getArea(*finished_area);
			dealer.addWork(finished_area);
		}
		finished++;
	}
	for(int i=0;i<cpus;++i) dealer.addWork(NULL);
	for(int i=0;i<cpus;++i) workers[i]->wait();
	for(int i=0;i<cpus;++i) delete workers[i];
	cout<<"#]"<<endl;
	delete BTree;
	BTree=NULL;
	
#ifndef WIN32
	restoreSignals(&origmask);
#endif

}

scene_t *threadedscene_t::factory()
{
	return new threadedscene_t();
}


__END_YAFRAY

#else

#endif

