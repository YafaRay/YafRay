#ifndef __TOOLS_H
#define __TOOLS_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<map>
#include<limits>
#include"color.h"
#include"vector3d.h"


__BEGIN_YAFRAY

template<class T>
inline bool validFloat(const T &f)
{
	  return (f<std::numeric_limits<T>::infinity()) &&
          (f>-std::numeric_limits<T>::infinity());
}

inline bool valid(const color_t &c)
{
	return validFloat(c.getR()) && validFloat(c.getG()) && validFloat(c.getB());
}

inline bool valid(const vector3d_t &c)
{
	return validFloat(c.x) && validFloat(c.y) && validFloat(c.y);
}

template<class S, class D>
class Conversion
{
	protected:
		typedef int small;
		class big   {int a[2];};
		static small test(D);
		static big test(...);
		static S dummy();
	public:
		static const bool exists=(sizeof(test(dummy()))==sizeof(small));
};

class scene_t;

class YAFRAYCORE_EXPORT context_t
{
	friend class renderState_t;
	public:

		class destructible
		{
			public:
				virtual ~destructible() {};
		};
	protected:
		context_t();
		context_t(const context_t &s) {}; //forbiden
		~context_t();
		
		
		static double & createRecord(std::map<void *,double> &data,void *k);
		static destructible *& createRecord(std::map<void *,destructible *> &data,void *k);

		template<class T,bool fits,bool des>
		struct back
		{
				static void store(const T &,T ,std::map<void *,double> &,
						std::map<void *,destructible *> &);
				static T get(const T &,const std::map<void *,double> &,bool present,
						const std::map<void *,destructible *> &);
		};
		template<class T>
		struct back<T,true,false>
		{
				static void store(const T &d,T v,std::map<void *,double> &data,
						std::map<void *,destructible *> &des)
				{
					//*((T *)(&data[(void *) &d]))=v;
					*((T *)(&createRecord(data,(void *) &d)))=v;
				};
				static T get(const T &d,const std::map<void *,double> &data,
						bool &present,const std::map<void *,destructible *> &des)
				{
					std::map<void *,double>::const_iterator i=data.find((void *) &d);
					if(i!=data.end()) return *( (T *) &(i->second));
					else {present=false;return T();}
				}
		};
		template<class T>
		struct back<T,true,true>
		{
				static void store(const T &d,T v,std::map<void *,double> &data,
						std::map<void *,destructible *> &des)
				{
					//des[(void *)&d]=(destructible *)v;
					createRecord(des,(void *)&d)=(destructible *)v;
				};
				static T get(const T &d,const std::map<void *,double> &data,
						bool &present,const std::map<void *,destructible *> &des)
				{
					std::map<void *,destructible *>::const_iterator 
						i=des.find((void *) &d);
					if(i!=des.end()) return (T) i->second;
					else {present=false;return (T)NULL;}
				}
		};

	public:

/* Temporary disabled because I'm tired of installing newest gcc version for it
		template<class T>
		void context_t::store(const T &d,T v)
		{
			back<T,
					sizeof(T)<=sizeof(double),
					Conversion<T,destructible *>::exists>
					::store(d,v,data,destructibles);
		}

		template<class T>
		T get(const T &d)
		{
			bool present=true;
			return back<T,
									sizeof(T)<=sizeof(double),
									Conversion<T,destructible *>::exists>
									::get(d,data,present,destructibles);
		};
		template<class T>
		T get(const T &d,bool &present)
		{
			present=true;
			return back<T,
									sizeof(T)<=sizeof(double),
									Conversion<T,destructible *>::exists>
									::get(d,data,present,destructibles);
		};
*/
		template<class T>
		void store(const T &d,T v)
		{
			back<T, sizeof(T)<=sizeof(double),false>::store(d,v,data,destructibles);
		}
		template<class T>
		T get(const T &d)
		{
			bool present=true;
			return back<T, sizeof(T)<=sizeof(double),false>::get(d,data,present,destructibles);
		};
		template<class T>
		T get(const T &d,bool &present)
		{
			present=true;
			return back<T, sizeof(T)<=sizeof(double),false>::get(d,data,present,destructibles);
		};
		template<class T>
		void storeDestructible(const T &d,T v)
		{
			back<T, sizeof(T)<=sizeof(double),true>::store(d,v,data,destructibles);
		}
		template<class T>
		T getDestructible(const T &d)
		{
			bool present=true;
			return back<T, sizeof(T)<=sizeof(double),true>::get(d,data,present,destructibles);
		};
		template<class T>
		T getDestructible(const T &d,bool &present)
		{
			present=true;
			return back<T, sizeof(T)<=sizeof(double),true>::get(d,data,present,destructibles);
		};

	protected:

		std::map<void *,double> data;
		std::map<void *,destructible *> destructibles;
};

__END_YAFRAY

#endif
