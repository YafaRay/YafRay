#include"ccthreads.h"
#include<iostream>

using namespace std;

namespace yafthreads {

mutex_t::mutex_t() 
{
#if HAVE_PTHREAD
	int error=pthread_mutex_init(&m, NULL);
	switch(error)
	{
		case EINVAL: cout<<"pthread_mutex_init error EINVAL"<<endl;exit(1);break;
		case ENOMEM: cout<<"pthread_mutex_init error ENOMEM"<<endl;exit(1);break;
		case EAGAIN: cout<<"pthread_mutex_init error EAGAIN"<<endl;exit(1);break;
		default: break;
	}
#endif
}

void mutex_t::wait() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_lock(&m))
	{
		cout<<"Error mutex lock"<<endl;
		exit(1);
	}
#endif
}

void mutex_t::signal() 
{
#if HAVE_PTHREAD
	if(pthread_mutex_unlock(&m))
	{
		cout<<"Error mutex lock"<<endl;
		exit(1);
	}
#endif
}

mutex_t::~mutex_t() 
{
#if HAVE_PTHREAD
	pthread_mutex_destroy(&m);
#endif
}

#if HAVE_PTHREAD
void * wrapper(void *data)
{
	thread_t *obj=(thread_t *)data;
	obj->lock.wait();
	obj->body();
	obj->running=false;
	obj->lock.signal();
	pthread_exit(0);
	return NULL;
}

void thread_t::run()
{
	lock.wait();
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	pthread_create(&id,&attr,wrapper,this);
	running=true;
	lock.signal();
}

void thread_t::wait()
{
	if(running)
		pthread_join(id,NULL);
	running=false;
}

thread_t::~thread_t()
{
	wait();
}

mysemaphore_t::mysemaphore_t(int c) 
{
#ifdef __APPLE__
	static int id=0;
	char name[32];
	snprintf(name,32,"stupidosxsem%d",id);
	id++;
	s=sem_open(name, O_CREAT, 0777, c);
	if((int)s==SEM_FAILED) std::cout<<"Error sem_open"<<std::endl;
	sem_unlink(name);
#else
	int error=sem_init (&s,0,c);
	if(error!=0)
	{
		if(errno==EINVAL) std::cout<<"sem_init EINVAL"<<std::endl;
		if(errno==ENOSYS) std::cout<<"sem_init ENOSYS"<<std::endl;
	}
#endif
}

void mysemaphore_t::wait() 
{ 
#ifdef __APPLE__
	if(sem_wait(s))
	{
		cout<<"Error sem_wait"<<endl;
		switch(errno)
		{
			case EAGAIN: cout<<"EAGAIN"<<endl;break;
			case EINVAL: cout<<"EINVAL"<<endl;break;
			case EDEADLK: cout<<"EDEADLK"<<endl;break;
			case EINTR: cout<<"EINTR"<<endl;break;
			default: cout<<"NOSE"<<endl;break;
		}
		exit(1);
	}
#else
	sem_wait(&s); 
#endif
}

void mysemaphore_t::signal() 
{
#ifdef __APPLE__
	if(sem_post(s))
	{
		cout<<"Error sem_post"<<endl;
		exit(1);
	}
#else
	sem_post(&s);
#endif
}

mysemaphore_t::~mysemaphore_t() 
{
#ifdef __APPLE__
	if((int)s!=SEM_FAILED) sem_close(s);
#else
	sem_destroy (&s); 
#endif
}

#endif

} // yafthreads
