#ifndef __JOBDEALER_H
#define __JOBDEALER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<vector>
#include <list>
#include "ccthreads.h"

__BEGIN_YAFRAY

template<class J>
class jobDealer_t
{
	public:
		jobDealer_t():finish(false) {};
		
		void imFinished(J job);
		J giveMeWork();

		J getFinished();
		void addWork(J job);
	protected:
		jobDealer_t(const jobDealer_t &j);
		jobDealer_t & operator = (const jobDealer_t &j);

		bool finish;
		yafthreads::locked_t<std::list<J> > finished;
		yafthreads::mysemaphore_t finished_jobs;

		yafthreads::locked_t<std::list<J> > ready;
		yafthreads::mysemaphore_t ready_jobs;
};

template<class J>
void jobDealer_t<J>::imFinished(J job)
{
	finished.wait();
	finished.push_back(job);
	finished.signal();
	finished_jobs.signal();
}

template<class J>
J jobDealer_t<J>::giveMeWork()
{
	ready_jobs.wait();
	ready.wait();
	J res=ready.front();
	ready.pop_front();
	ready.signal();
	return res;
}

template<class J>
J jobDealer_t<J>::getFinished()
{
	finished_jobs.wait();
	finished.wait();
	J res=finished.front();
	finished.pop_front();
	finished.signal();
	return res;
}

template<class J>
void jobDealer_t<J>::addWork(J job)
{
	ready.wait();
	ready.push_back(job);
	ready.signal();
	ready_jobs.signal();
}

__END_YAFRAY

#endif
