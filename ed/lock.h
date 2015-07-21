
#include <windows.h>
#include <list>

class absLock
{
	HANDLE m_wait;
public:
	absLock()
	{
		m_wait = ::CreateSemaphore( NULL,1,1,NULL );
	}

	~absLock()
	{
		::CloseHandle(m_wait);
	}

	void lock()
	{
		::WaitForSingleObject( m_wait,INFINITE );
	}
	void unlock()
	{
		LONG pv = 0;
		::ReleaseSemaphore( m_wait,1,&pv );
	}
};

class Guard
{
	absLock &m_lock;
public:
	Guard(absLock &l)
		:m_lock(l)
	{
		m_lock.lock();
	}
	~Guard()
	{
		m_lock.unlock();
	}
};

class msgLock
{
	HANDLE m_wait;
public:
	msgLock(unsigned iv = 1)
	{
		m_wait = ::CreateSemaphore( NULL,iv,1,NULL );
	}

	virtual ~msgLock()
	{
		::CloseHandle(m_wait);
	}

	void lock()
	{
		/***
			WaitForSingleObject会阻塞对话框线程(Dialog thread)，同时也会导致了对话框的消息循环机制被阻塞 
		*/

		while( 1 )
		{
			DWORD ret = MsgWaitForMultipleObjects( 1,&m_wait,FALSE,INFINITE,QS_ALLINPUT );
			if( WAIT_OBJECT_0 == ret )
			{
				break;
			}
			else 
			{
				MSG msg;
				PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);  

				//if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
				{
					TranslateMessage(&msg);  // 处理键盘消息,没有这一句 edit 控件不正常
					DispatchMessage(&msg);
				}
			}
		}
	}

	void unlock()
	{
		LONG pv = 0;
		::ReleaseSemaphore( m_wait,1,&pv );
	}
};

class msgGuard
{
	msgLock &m_lock;
public:
	msgGuard(msgLock &m)
		:m_lock(m)
	{
		m_lock.lock();
	}
	~msgGuard()
	{
		m_lock.unlock();
	}
};

class waitNotifyLock : public msgLock
{
public:
	waitNotifyLock()
		:msgLock(0)
	{

	}

	void wait()
	{
		lock();
	}

	void notify()
	{
		unlock();
	}
};

template <typename T>
class guard
{
	T & m_lock;
public:
	guard( T & l )
		:m_lock(l)
	{
		m_lock.lock();
	}
	~guard()
	{
		m_lock.unlock();
	}
};

#define GUARD(x) _x(x)

template <typename dataT>
struct lock_queue_op
{
	virtual bool operator()( dataT &d ) = 0;
};

template <typename dataT,typename lockT>
class lock_queue
{
	lockT m_lock;
	std::list<dataT> m_datas;
public:
	void push_back( dataT & d )
	{
		GUARD( m_lock );

		m_datas.push_back( d );
	}

	void push_front( dataT & d )
	{
		GUARD( m_lock );

		m_datas.push_front( d );
	}

	void pop_front()
	{
		GUARD( m_lock );

		if( m_datas.empty() )
		{
			return;
		}

		m_datas.pop_front();
	}

	void pop_back()
	{
		GUARD( m_lock );

		if( m_datas.empty() )
		{
			return;
		}

		m_datas.pop_back();
	}

	unsigned scan( lock_queue_op &op )
	{
		GUARD( m_lock );

		unsigned cnt = 0;

		for( auto &i=m_datas.begin();i!=m_datas.end();++i )
		{
			++cnt;
			if( !op( *i ) )
			{
				return cnt;
			}
		}

		return cnt;
	}

	void proc_front( lock_queue_op &op )
	{
		GUARD( m_lock );

		if( m_datas.size() )
		{
			op( m_datas.front() );
		}
	}

	void proc_back( lock_queue_op &op )
	{
		GUARD( m_lock );

		if( m_datas.size() )
		{
			op( m_datas.back() );
		}
	}

	bool empty()
	{
		return m_datas.empty();
	}

	unsigned size()
	{
		return m_datas.size();
	}
};