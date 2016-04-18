
#include "renderblock.h"

using namespace std;

__BEGIN_YAFRAY

bool renderArea_t::out(colorOutput_t &o)
{
	int startX,startY;
	startX=realX-X;
	startY=realY-Y;
	for(int x=0;x<realW;++x)
		for(int y=0;y<realH;++y)
			if(!o.putPixel(realX+x, realY+y,
				image[(startY+y)*W+startX+x], image[(startY+y)*W+startX+x].getA(), 
				depth[(startY+y)*W+startX+x]))
				return false;
	return true;
}

bool renderArea_t::checkResample(CFLOAT threshold)
{
	bool need = false;
	for(int i=0;i<H;++i) 
	{
		int im,ip;
		if ((im=i-1)<0) im=0;
		if ((ip=i+1)==H) ip=H-1;
		for(int j=0;j<W;++j) 
		{

			int jm,jp;
			if ((jm=j-1)<0) jm=0;
			if ((jp=j+1)==W) jp=W-1;
			// center color
			colorA_t c=image[i*W+j];
			bool needAA;
			do 
			{
				// (-1,-1)
				needAA = ((c-image[im*W+jm]).abscol2bri()) >= threshold; if (needAA) break;
				// (0,-1)
				needAA = ((c-image[im*W+j]).abscol2bri()) >= threshold; if (needAA) break;
				// (1,-1)
				needAA = ((c-image[im*W+jp]).abscol2bri()) >= threshold; if (needAA) break;
				// (-1,0)
				needAA = ((c-image[i*W+jm]).abscol2bri()) >= threshold; if (needAA) break;
				// (1,0)
				needAA = ((c-image[i*W+jp]).abscol2bri()) >= threshold; if (needAA) break;
				// (-1,1)
				needAA = ((c-image[ip*W+jm]).abscol2bri()) >= threshold; if (needAA) break;
				// (0,1)
				needAA = ((c-image[ip*W+j]).abscol2bri()) >= threshold; if (needAA) break;
				// (1,1)
				needAA = ((c-image[ip*W+jp]).abscol2bri()) >= threshold; if (needAA) break;
				break;
			} while ((2+2)==5); 

			if (needAA)
			{
				resample[i*W+j] = true;
				need = true;
			}
			else resample[i*W+j] = false;
		}
	}
	return need;
}

blockSpliter_t::blockSpliter_t(int w,int h,int b):
width(w),height(h),block(b)
{
	int bw=width/b;
	int bh=height/b;
	if(width%b) bw++;
	if(height%b) bh++;

	regions.resize(bh*bw);
	vector<int> scram(bh*bw);
	for(int i=0;i<(bh*bw);++i) scram[i]=i;
	for(int i=0;i<(bh*bw);++i) swap(scram[i],scram[rand()%(bh*bw)]);
	int sec=0;
	for(int i=0;i<bh;++i)
		for(int j=0;j<bw;++j)
		{
			region_t region;
			region.x=region.rx=j*block;
			region.y=region.ry=i*block;
			region.w=region.rw=width-region.rx;
			region.h=region.rh=height-region.ry;
			if(region.w>block) region.w=region.rw=block;
			if(region.h>block) region.h=region.rh=block;
			if(region.x>0) {region.x--;region.w++;}
			if(region.y>0) {region.y--;region.h++;}
			if((region.x+region.w)<(width-1)) region.w++;
			if((region.y+region.h)<(height-1)) region.h++;
			regions[scram[sec]]=region;
			sec++;
		}
}

void blockSpliter_t::getArea(renderArea_t &area)
{
	area.set(regions.back().x,regions.back().y,regions.back().w,regions.back().h);
	area.setReal(regions.back().rx,regions.back().ry,regions.back().rw,regions.back().rh);
	regions.pop_back();
	/*
	int num=rand()%regions.size();
	list<region_t>::iterator it;
	for(list<region_t>::iterator i=regions.begin();i!=regions.end();++i,num--)
		if(!num) {it=i;break;}
	area.set(it->x,it->y,it->w,it->h);
	area.setReal(it->rx,it->ry,it->rw,it->rh);
	regions.erase(it);
	*/
}

__END_YAFRAY
