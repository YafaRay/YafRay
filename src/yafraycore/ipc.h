#ifndef __IPC_H
#define __IPC_H



#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#ifndef MSVC

extern bool useZ;

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<zlib.h>
#include<vector>
#include <sys/types.h>
#include "scene.h"
//#include <sys/wait.h>



__BEGIN_YAFRAY

int writePipe(int pipeline, void * buffer, int length);
int readPipe(int pipeline, void * buffer, int length);


		bool sendColor(cBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNColor(cBuffer_t &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		bool sendFloat(fBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendOversample(Buffer_t<char> &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNOversample(Buffer_t<char> &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		void mixColor(cBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);
		void receiveColor(cBuffer_t &out,int resx,int resy,int pipeVector);
		void receiveOversample(Buffer_t<char> &out,int resx,int resy,int pipeline);
		void mixFloat(fBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);

	//With NET_OPTIMIZE

		bool sendZColor(cBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNZColor(cBuffer_t &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		bool sendZFloat(fBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendZOversample(Buffer_t<char> &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNZOversample(Buffer_t<char> &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		void mixZColor(cBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);
		void receiveZColor(cBuffer_t &out,int resx,int resy,int pipeVector);
		void receiveZOversample(Buffer_t<char> &out,int resx,int resy,int pipeline);
		void mixZFloat(fBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);

		//Without NET_OPTIMIZE

		bool sendRAWColor(cBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNRAWColor(cBuffer_t &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		bool sendRAWFloat(fBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendRAWOversample(Buffer_t<char> &out,int pipeline, int resx, int resy, int cpus, int off);
		bool sendNRAWOversample(Buffer_t<char> &out,std::vector<std::pair<int,int> > pipeVector, int resx, int resy, int cpus);
		void mixRAWColor(cBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);
		void receiveRAWColor(cBuffer_t &out,int resx,int resy,int pipeVector);
		void receiveRAWOversample(Buffer_t<char> &out,int resx,int resy,int pipeline);
		void mixRAWFloat(fBuffer_t &out,int resx,int resy,int cpus,std::vector<std::pair<int,int> > pipeVector);

__END_YAFRAY
#endif // MSVC
#endif
