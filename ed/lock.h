
#include <windows.h>

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
	absLock m_lock;
public:
	Guard()
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
					TranslateMessage(&msg);  // 处理键盘消息,没有这一名 edit 控件不正常
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
	msgLock m_lock;
public:
	msgGuard()
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