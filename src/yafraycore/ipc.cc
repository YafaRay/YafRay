#include "ipc.h"

#ifndef MSVC

#include "scene.h"
#include <unistd.h>

using namespace std;

static const int bdepth = 4;

bool useZ=false;

__BEGIN_YAFRAY

int writePipe(int pipeline, void * buffer, int length)
{
	int lengthAux = length;
	int _write;
	while(lengthAux > 0) {
		_write=write(pipeline,buffer,lengthAux);
		if(_write == -1)
		{
			lengthAux = -1;
			break;
		}
		lengthAux-=_write;
	}
	return lengthAux;
}

int readPipe(int pipeline, void * buffer, int length)
{
	int lengthAux = length;
	int _read;
	while(lengthAux > 0) {
		_read=read(pipeline,buffer,lengthAux);
		if(_read == -1)
		{
			lengthAux = -1;
			break;
		}
		lengthAux-=_read;
	}
	return lengthAux;
}



bool sendColor(cBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off)
{
	if(useZ)
		return sendZColor(out,pipeline,resx,resy,cpus,off);
	return sendRAWColor(out,pipeline,resx,resy,cpus,off);
}

bool sendNColor(cBuffer_t &out,vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
	if(useZ)
		return sendNZColor(out,pipeVector,resx,resy,cpus);
	return sendNRAWColor(out,pipeVector,resx,resy,cpus);
}

bool sendFloat(fBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off)
{
	if(useZ)
		return sendZFloat(out,pipeline,resx,resy,cpus,off);
	return sendRAWFloat(out,pipeline,resx,resy,cpus,off);
}

bool sendOversample(Buffer_t<char> &out,int pipeline, int resx, int resy, int cpus, int off)
{
	if(useZ)
		return sendZOversample(out,pipeline,resx,resy,cpus,off);
	return sendRAWOversample(out,pipeline,resx,resy,cpus,off);
}

bool sendNOversample(Buffer_t<char> &out, vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
	if(useZ)
		return sendNZOversample(out,pipeVector,resx,resy,cpus);
	return sendNRAWOversample(out,pipeVector,resx,resy,cpus);
}

void mixColor(cBuffer_t &out,int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
	if(useZ)
		mixZColor(out,resx,resy,cpus,pipeVector);
	else
		mixRAWColor(out,resx,resy,cpus,pipeVector);
}

void receiveColor(cBuffer_t &out,int resx,int resy,int pipeVector)
{
	if(useZ)
		receiveZColor(out,resx,resy,pipeVector);
	else
		receiveRAWColor(out,resx,resy,pipeVector);
}

void receiveOversample(Buffer_t<char> &out,int resx,int resy,int pipeline)
{
	if(useZ)
		receiveZOversample(out,resx,resy,pipeline);
	else
		receiveRAWOversample(out,resx,resy,pipeline);
}

void mixFloat(fBuffer_t &out,int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
	if(useZ)
		mixZFloat(out,resx,resy,cpus,pipeVector);
	else
		mixRAWFloat(out,resx,resy,cpus,pipeVector);
}

////////////////////////////////////////////////
//With NET_OPTIMIZE
///////////////////////////////////////////////

bool sendZColor(cBuffer_t &out, int pipeline, int resx, int resy, int cpus, int off)
{
        Bytef * buffer=(Bytef *)malloc(2*resx*resy*bdepth*sizeof(Bytef));
        uLongf length=2*resx*resy*bdepth*sizeof(Bytef);
	
        compress(buffer,&length,(Bytef *)out(0,0),(uLong)resx*resy*bdepth*sizeof(Bytef));
	
	writePipe(pipeline,&length,sizeof(uLongf));
	writePipe(pipeline,buffer,length);
	
	free(buffer);
  
	return true;
}

bool sendNZColor(cBuffer_t &out, vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
        Bytef * buffer=(Bytef *)malloc(2*resx*resy*bdepth*sizeof(Bytef));
        uLongf length=2*resx*resy*bdepth*sizeof(Bytef);
	
        compress(buffer,&length,(Bytef *)out(0,0),(uLong)resx*resy*bdepth*sizeof(Bytef));

	for(int iFork=0;iFork<cpus;iFork++)
	{
		writePipe(pipeVector[iFork].second,&length,sizeof(uLongf));
		writePipe(pipeVector[iFork].second,buffer,length);
	}

	free(buffer);
  
	return true;
}


bool sendZFloat(fBuffer_t &out, int pipeline, int resx, int resy, int cpus, int off)
{
	GFLOAT * buffer=(GFLOAT *)malloc(2*resx*resy*sizeof(GFLOAT));
	uLongf length=2*resx*resy*sizeof(uLongf);
	
	compress((Bytef *)buffer,&length,(Bytef *) out.buffer(0,0),(uLong)resx*resy*sizeof(uLongf));
	
	writePipe(pipeline,&length,sizeof(uLongf));
	writePipe(pipeline,buffer,length);
	
	free(buffer);
  
	return true;
}

bool sendZOversample(Buffer_t<char> &out, int pipeline, int resx, int resy, int cpus, int off)
{
	Bytef * buffer=(Bytef *)malloc(2*resx*resy*sizeof(Bytef));
	uLongf length=2*resx*resy*sizeof(Bytef);
	
	compress(buffer,&length,(Bytef *)out.buffer(0,0),(uLong)resx*resy*sizeof(Bytef));

	writePipe(pipeline,&length,sizeof(uLongf));
	writePipe(pipeline,buffer,length);
	
	free(buffer);
	
	return true;
}

bool sendNZOversample(Buffer_t<char> &out, vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
	Bytef * buffer=(Bytef *)malloc(2*resx*resy*sizeof(Bytef));
	uLongf length=2*resx*resy*sizeof(Bytef);
	
	compress(buffer,&length,(Bytef *)out.buffer(0,0),(uLong)resx*resy*sizeof(Bytef));

	for(int iFork=0;iFork<cpus;iFork++)
	{
		writePipe(pipeVector[iFork].second,&length,sizeof(uLongf));
		writePipe(pipeVector[iFork].second,buffer,length);
	}
	
	free(buffer);
	
	return true;
}

void mixZColor(cBuffer_t &out,int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
	Bytef * buffer1;
        Bytef * buffer2=(Bytef *)malloc(2*resy*resx*bdepth*sizeof(Bytef));
	uLongf result;	
	uLongf length;
	
	for(int iFork=0;iFork<cpus;iFork++)
	{
                result=(2*resy*resx*bdepth*sizeof(Bytef));
		readPipe(pipeVector[iFork].first,&length,sizeof(uLongf));
		buffer1=(Bytef *)malloc(length*sizeof(Bytef));
		readPipe(pipeVector[iFork].first, buffer1, length);
		
		uncompress(buffer2,&result,buffer1,length);

		for(int y=iFork;y<resy;y+=cpus)
			for(int k=0;k<resx;k++)
			{
				out(k,y)[0]=buffer2[(y*resx+k)*bdepth];
				out(k,y)[1]=buffer2[((y*resx+k)*bdepth)+1];
				out(k,y)[2]=buffer2[((y*resx+k)*bdepth)+2];
			}
		free(buffer1);
	}
	free(buffer2);
}

void mixZFloat(fBuffer_t &out,int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
	Bytef * buffer1;
	GFLOAT * buffer2=(GFLOAT *)malloc(2*resy*resx*sizeof(GFLOAT));
	uLongf result;	
	uLongf length;
	
	for(int iFork=0;iFork<cpus;iFork++)
	{
		result=(2*resy*resx*sizeof(uLongf));
		readPipe(pipeVector[iFork].first,&length,sizeof(uLongf));
		
		buffer1=(Bytef *)malloc(length);
		readPipe(pipeVector[iFork].first, buffer1, length);
		
		uncompress((Bytef *)buffer2,&result,buffer1,length);

		for(int y=iFork;y<resy;y+=cpus)
			for(int k=0;k<resx;k++)
			{
				out(k,y)=buffer2[(y*resx)+k];
			}
		free(buffer1);
	}
	free(buffer2);
}

void receiveZColor(cBuffer_t &out,int resx,int resy,int pipeline)
{
        Bytef * buffer=(Bytef *)malloc(2*resx*resy*bdepth*sizeof(Bytef));
	
	uLongf length;
        uLongf result=2*resx*resy*bdepth*sizeof(Bytef);
	
	readPipe(pipeline, &length, sizeof(uLongf));
	
	readPipe(pipeline, buffer, length);
	
	uncompress(out(0,0),&result,buffer,length);

	free(buffer);
}

void receiveZOversample(Buffer_t<char> &out,int resx,int resy,int pipeline)
{
	Bytef * buffer=(Bytef *)malloc(2*resx*resy*sizeof(Bytef));	
	
	uLongf length;
	uLongf result=2*resx*resy*sizeof(Bytef);
	
	readPipe(pipeline, &length, sizeof(uLongf));
	readPipe(pipeline, buffer, length*sizeof(Bytef));
	
	uncompress((Bytef *)out.buffer(0,0),&result,buffer,length);
  
	free(buffer);
}

/////////////////////////////////////////////////////////
//Without NET_OPTIMIZE
////////////////////////////////////////////////////////

bool sendRAWColor(cBuffer_t &out, int pipeline, int resx, int resy, int cpus, int off)
{
  for (int y=off;y<resy;y+=cpus) {
                        writePipe(pipeline,out(0,y),resx*bdepth*sizeof(unsigned char));
		}
  return true;
}

bool sendNRAWColor(cBuffer_t &out, vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
  for(int iFork=0;iFork<cpus;iFork++)
		for (int y=iFork;y<resy;y+=cpus)
			writePipe(pipeVector[iFork].second,out(0,y),resx*bdepth*sizeof(unsigned char));

  return true;
}


bool sendRAWFloat(fBuffer_t &out,int pipeline, int resx, int resy, int cpus, int off)
{
  for (int y=off;y<resy;y+=cpus) {
			writePipe(pipeline,out.buffer(0,y), resx*sizeof(GFLOAT));
		}
  return true;
}

bool sendRAWOversample(Buffer_t<char> &out,int pipeline, int resx, int resy, int cpus, int off)
{
	writePipe(pipeline,out.buffer(0,0), resx*resy*sizeof(char));
	return true;
}

bool sendNRAWOversample(Buffer_t<char> &out,vector<pair<int,int> > pipeVector, int resx, int resy, int cpus)
{
	for(int iFork=0;iFork<cpus;iFork++)
		writePipe(pipeVector[iFork].second,out.buffer(0,0), resx*resy*sizeof(char));
	return true;
}


void mixRAWColor(cBuffer_t &out,int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
        unsigned char * line=(unsigned char *)malloc(resx*bdepth*sizeof(unsigned char));
	int iFork=0;
	for(int y=0;y<resy;y++)
	{
		if(iFork == cpus) iFork=0;
                readPipe(pipeVector[iFork].first, line, resx*bdepth*sizeof(unsigned char));
		
		for(int k=0;k<resx;k++)
		{
			out(k,y)[0]=line[bdepth*k];
			out(k,y)[1]=line[(bdepth*k)+1];
			out(k,y)[2]=line[(bdepth*k)+2];
		}
		iFork++;
	}
	free(line);
}

void receiveRAWColor(cBuffer_t &out,int resx,int resy,int pipeVector)
{
        unsigned char * line=(unsigned char *)malloc(resx*bdepth*sizeof(unsigned char));
	for(int y=0;y<resy;y++)
	{
		readPipe(pipeVector, line, resx*bdepth*sizeof(unsigned char));
		
		for(int k=0;k<resx;k++)
		{
			out(k,y)[0]=line[bdepth*k];
			out(k,y)[1]=line[(bdepth*k)+1];
			out(k,y)[2]=line[(bdepth*k)+2];
		}
	}
	free(line);
}

void receiveRAWOversample(Buffer_t<char> &out,int resx,int resy,int pipeline)
{
	readPipe(pipeline, out.buffer(0,0), resx*resy*sizeof(char));
}

void mixRAWFloat(fBuffer_t &out, int resx,int resy,int cpus,vector<pair<int,int> > pipeVector)
{
	GFLOAT * line=(GFLOAT *)malloc(resx*sizeof(GFLOAT));
	int iFork=0;
	for(int y=0;y<resy;y++)
	{
		if(iFork == cpus) iFork=0;
		readPipe(pipeVector[iFork].first, line, resx*sizeof(GFLOAT));

		for(int k=0;k<resx;k++)
		{
			out(k,y)=line[k];
		}
		iFork++;
	}
	free(line);
}

__END_YAFRAY

#endif
