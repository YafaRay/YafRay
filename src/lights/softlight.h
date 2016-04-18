#ifndef __SOFTLIGHT_H
#define __SOFTLIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "params.h"
#include"vector3d.h"
#include"buffer.h"
#include"object3d.h"
#include"light.h"

__BEGIN_YAFRAY

#define SIDE_UP 0
#define SIDE_RIGHT 1
#define SIDE_DOWN 2
#define SIDE_LEFT 3

class softLight_t : public light_t
{
	public:
		softLight_t(const point3d_t &f, const color_t &c, CFLOAT p,
				int resol, int radius, GFLOAT biass, CFLOAT gli=0, CFLOAT glo=0, int glt=0);
		virtual color_t illuminate(renderState_t &state, const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye)const;
		virtual point3d_t position() const { return from; }
		virtual void init(scene_t &scene);
		virtual ~softLight_t() {};

		static light_t *factory(paramMap_t &params, renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		GFLOAT getSample(int face,int x, int y/*,object3d_t * &o*/)const;
		int guessSide(const vector3d_t &v,GFLOAT &x,GFLOAT &y)const;
		void fillSide(int s,const vector3d_t &corner,const vector3d_t &cx,
				const vector3d_t &y,scene_t &scene);
		void fillCube(scene_t &scene);
		CFLOAT mixShadow(int face,int ix,int iy,int fx,int fy,
				GFLOAT cx,GFLOAT cy,GFLOAT Z/*,const object3d_t *ob*/)const;
		
		vector3d_t cube[6];
		char ad[6][4];
		char side[6][4];
		bool flip[6][4];
		int res;
		GFLOAT R2,slim,bias;
		int R;
		CFLOAT pow;
		point3d_t from;
		color_t color;
		fBuffer_t buffer[6];
		// glow
		CFLOAT glow_int, glow_ofs;
		int glow_type;
};

#define SIDE_ISOUT(a) ((a<0) || (a>=res))
#define SIDE_ISIN(a) ((a>=0) && (a<res))

inline GFLOAT softLight_t::getSample(int face,int x, int y/*,object3d_t * &object*/)const
{
	int s;
	int in,out;
	if( SIDE_ISIN(x) && SIDE_ISIN(y)) 
	{
		//object=obj[face](x,y);
		return buffer[face](x,y);
	}
	if(SIDE_ISOUT(x) && SIDE_ISOUT(y)) 
	{
		//object=(object3d_t *)NULL;
		return -2.0;
	}
	if(x<0) 
	{
		s=SIDE_LEFT;
		in=y;out=-1-x;
	}
	else if(x>=res) 
	{
		s=SIDE_RIGHT;
		in=y;out=x-res;
	}
	else if(y<0) 
	{
		s=SIDE_UP;
		in=x;out=-1-y;
	}
	else //if(y>=res) 
	{
		s=SIDE_DOWN;
		in=x;out=y-res;
	}
	if(flip[face][s])
		in=res-in-1;
	int of=ad[face][s];
	int os=side[face][s];
	switch(os)
	{
		case SIDE_UP :
			return buffer[of](in,out);
		case SIDE_RIGHT :
			return buffer[of](res-out-1,in);
		case SIDE_DOWN :
			return buffer[of](in,res-out-1);
		case SIDE_LEFT :
			return buffer[of](out,in);
	}
	return -1.0;
}

inline int softLight_t::guessSide(const vector3d_t &v,GFLOAT &x,GFLOAT &y)const
{
	vector3d_t vx=v,vy=v;
	
			vx.z=0;vx.normalize();
			vy.x=0;vy.normalize();
			if(((-vx.y)>=slim) && ((-vy.y)>=slim)) 
			{
				x=(vx.x/(-vx.y))/2+0.5;
				y=(-vy.z/(-vy.y))/2+0.5;
				x=x*res;
				y=y*res;
				return 0;
			}
	vx=v;vy=v;
			vx.z=0;vx.normalize();
			vy.y=0;vy.normalize();
			if((vx.x>=slim) && (vy.x>=slim)) 
			{
				x=(vx.y/vx.x)/2+0.5;
				y=(-vy.z/vy.x)/2+0.5;
				x=x*res;
				y=y*res;
				return 1;
			}
	vx=v;vy=v;
			vx.z=0;vx.normalize();
			vy.x=0;vy.normalize();
			if((vx.y>=slim) && (vy.y>=slim)) 
			{
				x=(-vx.x/vx.y)/2+0.5;
				y=(-vy.z/vy.y)/2+0.5;
				x=x*res;
				y=y*res;
				return 2;
			}
	vx=v;vy=v;
			vx.z=0;vx.normalize();
			vy.y=0;vy.normalize();
			if(((-vx.x)>=slim) && ((-vy.x)>=slim)) 
			{
				x=(-vx.y/(-vx.x))/2+0.5;
				y=(-vy.z/(-vy.x))/2+0.5;
				x=x*res;
				y=y*res;
				return 3;
			}
	vx=v;vy=v;
			vx.y=0;vx.normalize();
			vy.x=0;vy.normalize();
			x=(vx.x/fabs(vx.z))/2 + 0.5;
			y=(-vy.y/fabs(vy.z))/2 + 0.5;
			x=x*res;
			y=y*res;
			if(v.z>0) return 4;
			else return 5;
}

inline CFLOAT softLight_t::mixShadow(int face,int ix,int iy,int fx,int fy,
		GFLOAT cx,GFLOAT cy,GFLOAT Z/*,const object3d_t *ob*/)const
{
	GFLOAT num=0.0,den=0.0;
	for(int y=iy;y<=fy;++y)
		for(int x=ix;x<=fx;++x)
		{
			//object3d_t *sobj;
			GFLOAT sample=getSample(face,x,y/*,sobj*/);
			if(sample<-1.5) continue;
			if(/*(sobj==ob) &&*/ (sample>0))
				sample+=bias;
			GFLOAT px=(GFLOAT)x+0.5;
			GFLOAT py=(GFLOAT)y+0.5;
			px-=cx;
			py-=cy;
			GFLOAT dis=sqrt(px*px+py*py);
			if(dis<(GFLOAT)R)
			{
				dis=1.0-(dis*dis)/R2;
				dis*=dis;
				den+=dis;
				if((Z<=sample) || (sample<0))
					num+=dis;
			}
		}
	if(num==0.0) return 0.0;
	else return num/den;
}

__END_YAFRAY
#endif
