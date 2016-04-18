//---------------------------------------------------------------------
// Monte Carlo & Quasi Monte Carlo stuff
//---------------------------------------------------------------------

#ifndef __MCQMC_H
#define __MCQMC_H

__BEGIN_YAFRAY
// fast incremental Halton sequence generator
// calculation of value must be double prec.
class YAFRAYCORE_EXPORT Halton
{
public:
	Halton() {}
	Halton(int base) { setBase(base); }
	void setBase(int base)
	{
		_base = base;
		invBase = 1.0/double(base);
		value = 0;
	}
	~Halton() {}
	void reset() { value=0.0; }
	void setStart(unsigned int i)
	{
		value = 0.0;
		double f, factor;
		f = factor = invBase;
		while (i>0) {
			value += double(i % _base) * factor;
			i /= _base;
			factor *= f;
		}
	}
	PFLOAT getNext() const
	{
		double r = 1.0 - value - 0.0000000001;
		if (invBase < r)
			value += invBase;
		else {
			double hh, h=invBase;
			do {
				hh = h;
				h *= invBase;
			} while (h >= r);
			value += hh + h - 1.0;
		}
		return value;
	}
private:
	unsigned int _base;
	double invBase;
	mutable double value;
};


// fast base-2 van der Corput, Sobel, and Larcher & Pillichshammer sequences,
// all from "Efficient Multidimensional Sampling" by Alexander Keller
inline PFLOAT RI_vdC(unsigned int bits, unsigned int r=0)
{
	bits = ( bits << 16) | ( bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
	return double(bits^r)/4294967296.0;
}

inline PFLOAT RI_S(unsigned int i, unsigned int r=0)
{
	for (unsigned int v=1<<31; i; i>>=1, v^=v>>1)
		if (i & 1) r ^= v;
	return double(r)/4294967296.0;
}

inline PFLOAT RI_LP(unsigned int i, unsigned int r=0)
{
	for (unsigned int v=1<<31; i; i>>=1, v|=v>>1)
		if (i & 1) r ^= v;
	return double(r)/4294967296.0;
}


inline int nextPrime(int lastPrime)
{
	int newPrime = lastPrime + (lastPrime & 1) + 1;
	for (;;) {
		int dv=3;  bool ispr=true;
		while ((ispr) && (dv*dv<=newPrime)) {
			ispr = ((newPrime % dv)!=0);
			dv += 2;
		}
		if (ispr) break;
		newPrime += 2;
	}
	return newPrime;
}

__END_YAFRAY

#endif	//__MCQMC_H
