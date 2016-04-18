#ifndef __REFERENCE_H
#define __REFERENCE_H

#include "object3d.h"

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT referenceObject_t : public object3d_t
{
	friend class photonLight_t;
	public:
		virtual ~referenceObject_t();
		virtual int type() const {return REFERENCE;};
		virtual void transform(const matrix4x4_t &m);
		virtual point3d_t toObject(const point3d_t &p)const;
		virtual vector3d_t toObjectRot(const vector3d_t &v) const;
		virtual point3d_t toObjectOrco(const point3d_t &p) const;
		virtual bool shoot(renderState_t &state,surfacePoint_t &where, const point3d_t &from,
				const vector3d_t &ray,bool shadow=false,PFLOAT dis=-1)const;
		virtual bound_t getBound() const;

		static referenceObject_t *factory(const matrix4x4_t &M,object3d_t *org);
	protected:
		referenceObject_t(const matrix4x4_t &M,object3d_t *org); 
		referenceObject_t(const referenceObject_t &r) {}; //forbiden

		object3d_t *original;
		matrix4x4_t back, backRot, M, MRot;
};

__END_YAFRAY

#endif
