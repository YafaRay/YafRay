#ifndef __CCTHREADS_H
#define __CCTHREADS_H

#include<iostream>
#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<errno.h>

#if HAVE_PTHREAD
#include<pthread.h>
#include<semaphore.h>
#endif

namespace yafthreads {

class YAFRAYCORE_EXPORT mutex_t
{
	public:
		mutex_t();
		void wait();
		void signal();
		~mutex_t();
	protected:
		mutex_t(const mutex_t &m);
		mutex_t & operator = (const mutex_t &m);
#if HAVE_PTHREAD
		pthread_mutex_t m;
#endif
};

template<class T>
class YAFRAYCORE_EXPORT locked_t : public T
{
  public:
    void wait() 
		{
#if HAVE_PTHREAD
			mutex.wait();
#endif
		};
    void signal() 
		{
#if HAVE_PTHREAD
			mutex.signal();
#endif
		};
  protected:
#if HAVE_PTHREAD
    mutex_t mutex;
#endif
};
                                                                                                                
#if HAVE_PTHREAD


class YAFRAYCORE_EXPORT mysemaphore_t
{
  public:
    mysemaphore_t(int c=0);
    ~mysemaphore_t();
    void wait();
    void signal();
  protected:
		mysemaphore_t(const mysemaphore_t &m);
		mysemaphore_t & operator = (const mysemaphore_t &m);
#ifdef __APPLE__
    sem_t *s;
#else
    sem_t s;
#endif
};

class thread_t
{
	friend void * wrapper(void *data);
	public:
		thread_t() {running=false;};
		virtual ~thread_t();
		virtual void body()=0;
		void run();
		void wait();
		int getId() {return (int)id;};
		pthread_t getPid() {return id;};
		int getSelf() {return (int)pthread_self();};
		bool isRunning()const {return running;};
	protected:
		bool running;
		mutex_t lock;
		pthread_t id;
		pthread_attr_t attr;
};

#endif

} // yafthreads

#endif
