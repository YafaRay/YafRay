#ifndef __YAFUTILS_H
#define __YAFUTILS_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif
#include<list>

__BEGIN_YAFRAY

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
class treeBuilder_t
{
	public:
		treeBuilder_t() {};

		void push(const T obj);

		bool finish()const {return items.size()<2;};

		void build();
		T solution()const {return items.front().obj;};

	protected:
		void step();
		std::pair<T,T> pop();
		struct item_t
		{
			item_t(const T o):
				obj(o) {};
			const T obj;
			typename std::list<item_t>::iterator nearest;
			DISTANCE dist;
			std::list<typename std::list<item_t>::iterator> recalculate;
		};
		typedef typename std::list<item_t>::iterator item_iterator;
		
		void calculate(item_iterator n);

		std::list<item_t> items;
		item_iterator best;
		DISTANCE mindist;
		DISTANCEFUNC distance;
		JOINFUNC join;
};

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
void treeBuilder_t<T,DISTANCE,DISTANCEFUNC,JOINFUNC>::push(const T obj)
{
	items.push_front(item_t(obj));
	items.front().nearest=items.end();
	calculate(items.begin());
	if( (items.size()>1) && ((items.front().dist<mindist) || 
												 (best==items.end())))
	{
		best=items.begin();
		mindist=items.begin()->dist;
	}
}

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
void treeBuilder_t<T,DISTANCE,DISTANCEFUNC,JOINFUNC>::calculate(item_iterator n)
{
	if(items.size()==1) return;
	if(items.size()==2)
	{
		item_iterator a=items.begin();
		item_iterator b=a;
		b++;
		a->nearest=b;
		a->recalculate.push_back(b);
		b->nearest=a;
		b->recalculate.push_back(a);
		mindist=a->dist=b->dist=distance(a->obj,b->obj);
		best=a;
		return;
	}
	item_iterator before=n->nearest;
	item_t & nuevo=*n;
	for(item_iterator i=items.begin();i!=items.end();++i)
	{
		if(i==n) continue;
		DISTANCE d=distance(nuevo.obj,i->obj);
		if(i->nearest==items.end())
		{
			i->nearest=n;
			i->dist=d;
			nuevo.recalculate.push_back(i);
		}
		bool olduseless=(nuevo.nearest==items.end()) || 
			(nuevo.nearest->dist<nuevo.dist);
		bool newuseful=(d<i->dist);
		if( (nuevo.nearest==items.end()) || ((d<nuevo.dist) && (newuseful || olduseless))
				|| (olduseless && newuseful))
		{
			nuevo.nearest=i;
			nuevo.dist=d;
		}
	}
	if(nuevo.dist<nuevo.nearest->dist)
	{
		nuevo.nearest->nearest->recalculate.remove(nuevo.nearest);
		nuevo.nearest->nearest=n;
		nuevo.nearest->dist=nuevo.dist;
		nuevo.recalculate.push_back(nuevo.nearest);
	}
	if(before!=items.end())
		before->recalculate.remove(n);
	nuevo.nearest->recalculate.push_back(n);
}

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
std::pair<T,T> treeBuilder_t<T,DISTANCE,DISTANCEFUNC,JOINFUNC>::pop()
{
	std::pair<T,T> par;

	par.first=best->obj;
	par.second=best->nearest->obj;
	best->nearest->nearest->recalculate.remove(best->nearest);
	std::list<item_iterator> update=best->recalculate;
	best->nearest->recalculate.remove(best);
	update.insert(update.end(),best->nearest->recalculate.begin(),
															best->nearest->recalculate.end());

	items.erase(best->nearest);
	items.erase(best);

	if(items.size())
	{
		best=items.end();
		for(typename std::list<item_iterator>::iterator i=update.begin();
				i!=update.end(); ++i)
			(*i)->nearest=items.end();
		for(typename std::list<item_iterator>::iterator i=update.begin();
				i!=update.end(); ++i)
			calculate(*i);
		for(item_iterator i=items.begin();i!=items.end();++i)
			if((i->dist<mindist) || (best==items.end()))
			{
				best=i;
				mindist=i->dist;
			}
	}
	return par;
}

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
void treeBuilder_t<T,DISTANCE,DISTANCEFUNC,JOINFUNC>::step()
{
	T a=best->obj,b=best->nearest->obj;
	T nuevo=join(a,b);
	pop();
	push(nuevo);
}

template<class T,class DISTANCE,class DISTANCEFUNC,class JOINFUNC>
void treeBuilder_t<T,DISTANCE,DISTANCEFUNC,JOINFUNC>::build()
{
	while(!finish())
		step();
}

__END_YAFRAY

#endif
