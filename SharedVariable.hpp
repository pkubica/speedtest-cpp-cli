#pragma once
#include <mutex>

template <typename T> class SharedVariable
{
	T variable;
	std::mutex m_mutex;

public:
	SharedVariable(){}
	
	void set(T var)
	{
		m_mutex.lock();
		variable = var;
		m_mutex.unlock();
		return;
	}

	T get()
	{
		m_mutex.lock();
		T var = variable;
		m_mutex.unlock();
		return var;
	}
};