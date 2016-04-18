#ifndef __PARAMS_H
#define __PARAMS_H

#include "color.h"
#include "vector3d.h"
#include "light.h"
#include "shader.h"
#include "texture.h"
#include "filter.h"
#include "background.h"

#include <map>
#include <string>

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY
#define TYPE_FLOAT  0
#define TYPE_STRING 1
#define TYPE_POINT  2
#define TYPE_COLOR  3
#define TYPE_NONE   -1

class paramMap_t;

class YAFRAYCORE_EXPORT parameter_t
{
	public:
		parameter_t(const std::string &s);
		parameter_t(float f);
		parameter_t(float r,float g,float b,float a=1.0);
		parameter_t(const point3d_t &p);
		parameter_t(const parameter_t &p);
		parameter_t();
		~parameter_t();

		const std::string 		&getStr() {used=true;return str;};
		float 					&getFnum() {used=true;return fnum;};
		const point3d_t &getP() {used=true;return P;};
		const color_t 	&getC() {used=true;return C;};
		const colorA_t 	&getAC() {used=true;return C;};
		int type;
		bool used;
	protected:

		std::string str;
		float fnum;
		point3d_t P;
		colorA_t C;
};

class YAFRAYCORE_EXPORT paramMap_t
{
	public:
		paramMap_t();
		virtual bool getParam(const std::string &name,const std::string *&s);
		virtual bool getParam(const std::string &name,bool &b);
		virtual bool getParam(const std::string &name,float &f);
		virtual bool getParam(const std::string &name,double &f);
		virtual bool getParam(const std::string &name,int &i);
		virtual bool getParam(const std::string &name,point3d_t &p);
		virtual bool getParam(const std::string &name,color_t &c);
		virtual bool getParam(const std::string &name,colorA_t &c);
		virtual bool includes(const std::string &label,int type)const;
		virtual void checkUnused(const std::string &env)const;
		virtual parameter_t & operator [] (const std::string &key);
		virtual void clear();
		virtual ~paramMap_t();
	protected:
		std::map<std::string,parameter_t> dicc;
};

typedef enum
{
	INT,
	FLOAT,
	POINT,
	COLOR,
	BOOL,
	ENUM
}type_e;

template<bool> struct assert;
template<> struct assert<true> {};

struct YAFRAYCORE_EXPORT paramInfo_t
{
	paramInfo_t(type_e t,const std::string &l,const std::string &d):
		type(t),label(l),description(d)	{};
			
	type_e type;
	float rangeBegin,rangeEnd;
	std::list<std::string> values;
	std::string label;
	std::string description;
	float defaultInRange;
	std::string defaultInEnum;
};

template<type_e E> 
struct buildInfo : public paramInfo_t
{
	buildInfo(const std::string &l,const std::string &d):
		paramInfo_t(E,l,d)
	{
		assert<(E==POINT) || (E==COLOR) || (E==BOOL)>();
	}
};

template<> struct buildInfo<INT> : public paramInfo_t
{
	buildInfo(const std::string &l,int beg,int end,int def,const std::string &d):
		paramInfo_t(INT,l,d)
	{
		rangeBegin=beg;
		rangeEnd=end;
		defaultInRange=def;
	}
};

template<> struct buildInfo<FLOAT> : public paramInfo_t
{
	buildInfo(const std::string &l,float beg,float end,float def,
			const std::string &d):
		paramInfo_t(FLOAT,l,d)
	{
		rangeBegin=beg;
		rangeEnd=end;
		defaultInRange=def;
	}
};

template<> struct buildInfo<ENUM> : public paramInfo_t
{
	buildInfo(const std::string &l,const std::list<std::string> &v,
			      const std::string &def,const std::string &d):
		paramInfo_t(ENUM,l,d)
	{
		values=v;
		defaultInEnum=def;
	}
};

struct YAFRAYCORE_EXPORT pluginInfo_t
{
	std::string name;
	std::string description;
	std::list<paramInfo_t> params;
};

class YAFRAYCORE_EXPORT renderEnvironment_t
{
	public:
		typedef light_t * light_factory_t(paramMap_t &,renderEnvironment_t &);
		typedef shader_t *shader_factory_t(paramMap_t &,std::list<paramMap_t> &,
				renderEnvironment_t &);
		typedef texture_t *texture_factory_t(paramMap_t &,renderEnvironment_t &);
		typedef filter_t *filter_factory_t(paramMap_t &,renderEnvironment_t &);
		typedef background_t *background_factory_t(paramMap_t &,renderEnvironment_t &);
		typedef pluginInfo_t info_t();
		
		virtual shader_t *getShader(const std::string name)const=0;
		virtual texture_t *getTexture(const std::string name)const=0;

		virtual void repeatFirstPass()=0;

		virtual void registerFactory(const std::string &name,light_factory_t *f)=0;
		virtual void registerFactory(const std::string &name,shader_factory_t *f)=0;
		virtual void registerFactory(const std::string &name,texture_factory_t *f)=0;
		virtual void registerFactory(const std::string &name,filter_factory_t *f)=0;
		virtual void registerFactory(const std::string &name,background_factory_t *f)=0;

		renderEnvironment_t() {};
		virtual ~renderEnvironment_t() {};

};

__END_YAFRAY
#endif
