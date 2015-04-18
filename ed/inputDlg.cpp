

#include <string>
#include <process.h>
#include <io.h>

#include "StaticDialog.h"

struct inputProc
{
	bool m_needDelete;

	inputProc( bool needDelete)
		:m_needDelete(needDelete)
	{

	}

	virtual ~inputProc(){}

	void release()
	{
		if( m_needDelete )
		{
			delete this;
		}
	}

	virtual void do_ok( const std::string& ){}
	virtual void do_cancel(){}
};

class inputDlg : public StaticDialog
{
public:
	std::string m_capation;

	inputProc *m_proc;

	inputDlg()
		:StaticDialog(),
		m_edit(0),
		m_proc(0)
	{

	}

	~inputDlg()
	{
		if( m_proc )
		{
			m_proc->release();
		}
	}

	void doDialog(const std::string &capation,inputProc *fn);
protected:
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	HWND m_edit;
};

BOOL CALLBACK inputDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG:
		m_edit = ::GetDlgItem( _hSelf,IDC_EDIT_INPUT );

		::SetWindowTextA( _hSelf,m_capation.c_str() );
		::SetWindowTextA( m_edit,"" );
		return TRUE;
	case WM_COMMAND : 
		{
			switch (wParam)
			{
			case IDOK :
				{
					if( m_proc )
					{
						int len = ::GetWindowTextLengthA(m_edit);

						char *buf = new char[len+1];

						::GetWindowTextA( m_edit,buf,len );

						m_proc->do_ok( buf );

						delete []buf;
					}
					return TRUE;
				}
			case IDCANCEL:
				if( m_proc )
				{
					m_proc->do_cancel();
				}
				display(false);
				return TRUE;
			}
			return FALSE;
		}

	default :
		return FALSE;
	}
}
void inputDlg::doDialog(const std::string &capation,inputProc *fn )
{
	m_proc = fn;
	m_capation = capation;

	if( !isCreated() )
	{
		init( g_Inst,g_nppData._nppHandle );

		create( IDD_INPUT );
	}

	::SetWindowTextA( _hSelf,m_capation.c_str() );
	::SetWindowTextA( m_edit,"" );

	goToCenter();

	getFocus();
}

inputDlg g_inputDlg;

struct okProc : inputProc
{
	void (*okp)( const std::string& );

	okProc()
		:inputProc(false),
		okp(0)
	{

	}

	virtual void do_ok( const std::string& s)
	{
		if( okp )
		{
			okp( s );
		}
	}
};

okProc g_okProc;


struct waitInputProc : inputProc
{
	std::string &m_inputInfo;
	
	int state;

	waitNotifyLock m_lock;

	waitInputProc( std::string &inputInfo )
		:inputProc( true ),
		m_inputInfo(inputInfo),
		state(0)
	{
		
	}

	void wait()
	{
		m_lock.wait();
	}
	void notify()
	{
		m_lock.notify();
	}

	virtual ~waitInputProc()
	{
		
	}

	virtual void do_ok( const std::string& s)
	{
		m_inputInfo = s;

		state = 1;

		notify();
	}
	virtual void do_cancel()
	{
		state = 2;

		notify();
	}
};

extern "C"
{
	void getInput(const std::string &capation,void(*fn)(const std::string&))
	{
		g_okProc.okp = fn;

		g_inputDlg.doDialog(capation,&g_okProc);
	}

	
	bool extractInput( const std::string &capation,std::string &inputInfo )
	{
		inputDlg id;

		waitInputProc *wip = new waitInputProc( inputInfo );


		id.doDialog(capation,wip );

		wip->wait();

		return wip->state == 1;
	}
};