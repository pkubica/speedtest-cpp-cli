#pragma once
#include <list>
#include <mutex>
#include <condition_variable>

#if 1 == 1 //__cplusplus >= 199711L

template <typename T> class SharedQueue
{
	std::list<T> m_list;
	std::mutex m_mutex;
	std::condition_variable  m_cond;

public:
	SharedQueue()
	{

	}

	~SharedQueue()
	{

	}

	void add(T item)
	{
		m_mutex.lock();
		m_list.emplace_back(item);
		m_mutex.unlock();
		m_cond.notify_one();
	}

	T remove()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		while (m_list.size() == 0)
		{
			m_cond.wait(lk);
		}
		T item = m_list.front();
		m_list.pop_front();
		return item;
	}

	size_t isThereaWork()
	{
		size_t uSize = 0;
		m_mutex.lock();
		uSize = m_list.size();
		m_mutex.unlock();
		return uSize;
	}

	T soIgetIt()
	{
		T oItem;
		m_mutex.lock();
		oItem = m_list.front();
		m_list.pop_front();
		m_mutex.unlock();
		return oItem;
	}

	size_t size()
	{
		size_t uSize = 0;
		m_mutex.lock();
		uSize = m_list.size();
		m_mutex.unlock();
		return uSize;
	}
};
#else
#ifdef _WIN32
#error Older version of Windows mutex is not implemented
#elif __linux__
#include <pthread.h>

template <typename T> class SharedQueue
{
	std::list<T> m_list;
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;

public:
	SharedQueue()
	{
		pthread_mutex_init(&m_mutex, NULL);
		pthread_cond_init(&m_cond, NULL);
	}

	~SharedQueue()
	{
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}

	void add(T item)
	{
		pthread_mutex_lock(&m_mutex);
		m_list.push_back(item);
		pthread_cond_signal(&m_cond);
		pthread_mutex_unlock(&m_mutex);
	}

	T remove()
	{
		pthread_mutex_lock(&m_mutex);
		while (m_list.size() == 0)
		{
			pthread_cond_wait(&m_cond, &m_mutex);
		}
		T item = m_list.front();
		m_list.pop_front();
		pthread_mutex_unlock(&m_mutex);
		return item;
	}

	int isThereaWork()
	{
		pthread_mutex_lock(&m_mutex);
		int size = m_list.size();
		if (size > 0) return size;
		else
		{
			//unlocking for next questioning thread
			pthread_mutex_unlock(&m_mutex);
			return size;
		}
	}

	T soIgetIt()
	{
		T item = m_list.front();
		m_list.pop_front();
		pthread_mutex_unlock(&m_mutex);
		return item;
	}

	int size()
	{
		pthread_mutex_lock(&m_mutex);
		int size = m_list.size();
		pthread_mutex_unlock(&m_mutex);
		return size;
	}
};
#else
#error Not supported platform for older C++, Only Linux or Windows!!
#endif
//
#endif

