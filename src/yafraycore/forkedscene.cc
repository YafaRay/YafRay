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
#include "forkedscene.h"
#include "matrix4.h"
#include "ipc.h"

#include <cstdio>
#include <cstdlib>
#include<fstream>



using namespace std;

__BEGIN_YAFRAY

// Support for multi-process rendering
void forkedscene_t::render(colorOutput_t &out)
{
	/*
	int steps;
	resx=render_camera->resX();
	resy=render_camera->resY();
	int CS=1, iFork=0;
	vector3d_t ray;
	color_t color;
	state_t state;

	fprintf(stderr,"Building the bounding tree ... ");
	fflush(stderr);
	BTree = new boundTree_t (obj_list);
	cout<<"OK\n";
	setupLights();

	cout<<"Forking "<<cpus<<" processes for rendering ...\n";
	cout<<"Rendering ...\n";

	colorBuffer.set(resx,resy);
	ZBuffer.set(resx,resy);
	ABuffer.set(resx,resy);
	oversample.set(resx,resy);

	for(int i=0;i<resy;++i)
			for(int j=0;j<resx;++j)
					oversample(j,i)=1;

	steps=resy/(int)(resy/STEPS);
	steps=cpus*((int)steps/cpus);

	cout << "0%";
	for(int i=0;i<((steps+1)/2)-3;++i) cout<<" ";
	cout << "50%";
	for(int i=0;i<((steps+1)/2)-3;++i) cout<<" ";
	cout << "100%\n";

	pid_t waitFork[cpus];
	vector<pair<int,int> > pipeVectorStoF(cpus);
	vector<pair<int,int> > pipeVectorFtoS(cpus);

	cout<<"\r[";
	for(int i=0;i<steps;++i) cout<<".";

	cout<<"] first render pass\r[";
	cout.flush();

	// Fork off the children
	for(iFork=0;iFork<cpus;++iFork) 
	{
		int temp[2];
		pipe(temp);
		pipeVectorStoF[iFork].first=temp[0];
		pipeVectorStoF[iFork].second=temp[1];
		pipe(temp);
		pipeVectorFtoS[iFork].first=temp[0];
		pipeVectorFtoS[iFork].second=temp[1];

		waitFork[iFork] = fork();
		if(waitFork[iFork] == 0) 
		{ // Child process
			close(pipeVectorStoF[iFork].first);
			close(pipeVectorFtoS[iFork].second);

			// Setup and pass to child.
			sndpipe = pipeVectorStoF[iFork].second;
			rcvpipe = pipeVectorFtoS[iFork].first;
			childnum = iFork;

			doChild(out);

			exit(0); // Child done


		} 
		else 
		{
			close(pipeVectorStoF[iFork].second);
			close(pipeVectorFtoS[iFork].first);
		}
	}

	// The rest of this function is the parent.  The children

	// Receive results from child
	mixColor(colorBuffer,resx,resy,cpus,pipeVectorStoF);
	mixFloat(ZBuffer,resx,resy,cpus,pipeVectorStoF);
	mixFloat(ABuffer,resx,resy,cpus,pipeVectorStoF);

	if (repeatFirst) 
	{
		repeatFirst = false;

		state = REPEATFIRST;
		for(int iFork=0;iFork<cpus;iFork++) {
				writePipe(pipeVectorFtoS[iFork].second, &state, sizeof(state_t));
		}

		cout<<"\nLight needs post init...\n\r[";
		for(int i=0;i<steps;++i) cout<<".";
		cout<<"] Repeating render pass\r[";
		cout.flush();

		// Receive results from child
		mixColor(colorBuffer,resx,resy,cpus,pipeVectorStoF);
		mixFloat(ZBuffer,resx,resy,cpus,pipeVectorStoF);
		mixFloat(ABuffer,resx,resy,cpus,pipeVectorStoF);
	}

	if ((!checkSampling()) || (AA_onepass_max))
			CS = 0;

	for(int pass=1; pass<(AA_passes+1) && CS; ++pass) {
		cout<<"\r[";
		for(int i=0;i<steps;++i)
			cout<<".";

		cout << "] AA pass " << pass << "                     \r";
		cout << "[";
		cout.flush();

		state = OVERSAMPLE; //CheckSampling
		for(int iFork=0;iFork<cpus;iFork++) 
		{
			writePipe(pipeVectorFtoS[iFork].second, &state, sizeof(state_t));
			writePipe(pipeVectorFtoS[iFork].second, &pass, sizeof(int));
		}

		// Send required info
		sendNOversample(oversample, pipeVectorFtoS, resx, resy, cpus);

		// Get results
		mixColor(colorBuffer,resx,resy,cpus,pipeVectorStoF);

		cout<<"]                  ";

		if(!checkSampling())
				CS = 0;
	}

	// Kill children and wait to finish
	state = EXIT;
	for(int iFork=0;iFork<cpus;iFork++)
		writePipe(pipeVectorFtoS[iFork].second, &state, sizeof(state_t));
	cout << "\nForks finished.\n";

	// Do final processing while children finish up
	for(list<filter_t *>::iterator ite=filter_list.begin();ite!=filter_list.end();
			ite++)
		(*ite)->apply(colorBuffer,ZBuffer,ABuffer);

	for(int i=0;i<resy;++i)
		for(int j=0;j<resx;++j)
		{
			colorBuffer(j, i) >> color;
			out.putPixel(j,i,color, ABuffer(j, i));
		}


	// Reap the children to finish up
	for(int iFork=0;iFork<cpus;++iFork) 
		waitpid(waitFork[iFork],(int *)0,(int)0);

	for(int i=0;i<cpus;i++)
	{
		close(pipeVectorStoF[i].first);
		close(pipeVectorFtoS[i].second);
	}
*/
}

void forkedscene_t::doChild(colorOutput_t &out)
{
/*
	// Do the first pass and send results back to parent
	renderPart(out, 0, childnum);

	sendColor(colorBuffer, sndpipe, resx, resy, cpus, childnum);
	sendFloat(ZBuffer, sndpipe, resx, resy, cpus, childnum);
	sendFloat(ABuffer, sndpipe, resx, resy, cpus, childnum);

	state_t state;
	readPipe(rcvpipe, &state, sizeof(state_t));
	while (state != EXIT) {
		switch (state) {
			case OVERSAMPLE:
				int pass;
				readPipe(rcvpipe, &pass, sizeof(int));
				receiveOversample(oversample, resx, resy, rcvpipe);
				renderPart(out, pass, childnum);
				sendColor(colorBuffer, sndpipe, resx, resy, cpus, childnum);
				break;

			case REPEATFIRST:
				postSetupLights();
				renderPart(out, 0, childnum);

				sendColor(colorBuffer, sndpipe, resx, resy, cpus, childnum);
				sendFloat(ZBuffer, sndpipe, resx, resy, cpus, childnum);
				sendFloat(ABuffer, sndpipe, resx, resy, cpus, childnum);
				break;

			default:
				cerr << "Invalid state transmission from parent!!\n";
				exit(-1);
				break;
		}
		readPipe(rcvpipe, &state, sizeof(state_t));
	}

	// End ....
	close(sndpipe);
	close(rcvpipe);
	delete BTree;
	BTree=NULL;
	*/
}



__END_YAFRAY
