
#include "object3d.h"
#include "geometree.h"
#include "yafutils.h"

using namespace std;

__BEGIN_YAFRAY

typedef geomeTree_t<object3d_t> onode_t;

struct oTreeDist_f
{
	PFLOAT operator () (const onode_t *a,const onode_t *b)const
	{
		return bound_distance(a->getBound(),b->getBound()) /* *
			(fabs((GFLOAT)( a->getCount() - b->getCount() ) ))*/;
	}
};


struct oTreeJoin_f
{
	onode_t * operator () (onode_t *a,onode_t *b)const
	{
		return new onode_t(a,b);
	}
};

onode_t * buildObjectTree(list<object3d_t *> &obj_list)
{
	treeBuilder_t<onode_t*,PFLOAT,oTreeDist_f,oTreeJoin_f> builder;
	onode_t *nuevo,*root=NULL;

	for(list<object3d_t *>::const_iterator ite=obj_list.begin();
			ite!=obj_list.end();++ite)
	{
		nuevo=new onode_t(*ite,(*ite)->getBound());
		if (nuevo==NULL)
		{
			cout<<"Error allocating memory in bound tree\n";
			exit(1);
		}
		builder.push(nuevo);
	}
	if(obj_list.size())
	{
		builder.build();
		root=builder.solution();
		cout<<"Object count= "<<root->getCount()<<endl;
	}
	return root;
}

__END_YAFRAY
