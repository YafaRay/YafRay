
#include "tools.h"

__BEGIN_YAFRAY
// This is to workaround the stupid msvc restriction of MT library
// so blame microsoft ....

context_t::context_t()
{
}

context_t::~context_t()
{
	for(std::map<void *,destructible *>::iterator i=destructibles.begin();
		i!=destructibles.end();++i)
		delete i->second;
}

double & context_t::createRecord(std::map<void *,double> &data,void *k)
{
	return data[k];
}

context_t::destructible *& context_t::createRecord(
		std::map<void *,context_t::destructible *> &data,void *k)
{
	return data[k];
}

__END_YAFRAY
