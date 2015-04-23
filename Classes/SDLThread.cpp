#include "SDLImp.h"

/*
SDL Thread 界面重新使用std::thread封装
*/
//等价于SDL_CreateMutex
mutex_t* createMutex()
{
	return new mutex_t();
}

//等价于SDL_LockMutex
int lockMutex(mutex_t *mutex)
{
	mutex->lock();
	return 0;
}

//等价于SDL_UnlockMutex
int unlockMutex(mutex_t *mutex)
{
	mutex->unlock();
	return 0;
}

//等价于SDL_DestroyMutex
void destroyMutex(mutex_t *mutex)
{
	delete mutex;
}

//等价于SDL_CreateCond
cond_t * createCond()
{
	return new cond_t();
}

//等价于SDL_CondWait
//不要使用本函数
int waitCond(cond_t* pcond, mutex_t *pmutx)
{
	std::unique_lock<mutex_t> lk(*pmutx);
	pcond->wait(lk);
	return 0;
}

//等价于SDL_CondSignal
int signalCond(cond_t* pcond)
{
	pcond->notify_one();
	return 0;
}

//等价于SDL_DestroyCond
void destroyCond(cond_t *pcond)
{
	delete pcond;
}

//等价于SDL_CondWaitTimeout
//不要使用本函数
int waitCondTimeout(cond_t* pcond, mutex_t* pmutex, unsigned int ms)
{
	std::unique_lock<mutex_t> lk(*pmutex);
	if (pcond->wait_for(lk, std::chrono::milliseconds(ms)) == std::cv_status::timeout)
		return 1;// SDL_MUTEX_TIMEDOUT;
	else
		return 0;
}

//等价于SDL_CreateThread
thread_t* createThread(int(*func)(void*), void *p)
{
	return new thread_t(func,p);
}

//等价于SDL_WaitThread,等待线程结束
void waitThread(thread_t *pthread, int* status)
{
	pthread->join();
	delete pthread;
}

/*
	延时ms毫秒
*/
void Delay(Uint32 ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}