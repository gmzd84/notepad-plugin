

#include <string>
#include <list>

#include "StaticDialog.h"

struct showInfo_s
{
	std::string capation;
	std::string info;
	std::string help;
};

class infoDlg : public StaticDialog
{
public:
	infoDlg()
		:StaticDialog(),
		m_tab(0),
		m_hShow(0),
		m_hHelp(0)
	{
	}

	~infoDlg()
	{
	}

	void lock()
	{
		m_lock.lock();
	}

	void unlock()
	{
		m_lock.unlock();
	}

	void doDialog(const std::string &capation,const std::string &info ,const std::string help);
protected:
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	HWND m_tab;
	HWND m_hShow,m_hHelp;

	absLock m_lock;

	std::list<showInfo_s> m_infos;

	void setInfo();
};

void insertTab( HWND tab,int no,wchar_t *name )
{
	TCITEM ii;

	ii.mask = TCIF_TEXT;
	ii.pszText = name;
	

	TabCtrl_InsertItem( tab,no,&ii );
}

BOOL CALLBACK infoDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message) 
	{
	case WM_INITDIALOG:
		{
		m_tab = ::GetDlgItem( _hSelf,IDC_TAB_INFO );
		m_hShow = ::GetDlgItem( _hSelf,IDC_EDIT_SHOW );
		m_hHelp = ::GetDlgItem( _hSelf, IDC_EDIT_HELP );

		RECT r = {0};

		::GetClientRect( _hSelf,&r );

		::MoveWindow( m_tab,r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);

		::MoveWindow( m_hShow,r.left+1,r.top + 26,r.right-r.left-2,r.bottom-r.top - 28,TRUE);

		::MoveWindow( m_hHelp,r.left+1,r.top + 26,r.right-r.left-2,r.bottom-r.top - 28,TRUE);


		insertTab( m_tab,0,TEXT("info") );
		insertTab( m_tab,1,TEXT("help") );

		::SetParent( m_hShow ,m_tab );
		::SetParent( m_hHelp ,m_tab );

		setInfo();

		::ShowWindow( m_hShow,SW_SHOW );
		::ShowWindow( m_hHelp,SW_HIDE );

		TabCtrl_SetCurSel( m_tab,0 );
		
		return TRUE;
		}
	case WM_COMMAND: 
		{
			switch (wParam)
			{
			case IDOK :
				break;
			case IDCANCEL:
				lock();
				if( m_infos.size() )
				{
					m_infos.pop_front();
				}
				if( m_infos.empty() )
				{
					display(false);
				}
				unlock();
				
				setInfo();
				return TRUE;
			}
			return FALSE;
		}
	case WM_NOTIFY:
		{
			NMHDR * n = (NMHDR*)lParam;
			switch( n->code )
			{
			case TCN_SELCHANGE:
				{
					int sel = TabCtrl_GetCurSel(m_tab);
					switch( sel )
					{
					case 0:
						::ShowWindow( m_hShow,SW_SHOW );
						::ShowWindow( m_hHelp,SW_HIDE );
						break;
					default:
						::ShowWindow( m_hShow,SW_HIDE );
						::ShowWindow( m_hHelp,SW_SHOW );
						break;
					}
				}
				break;
			default:
				break;
			}
		}
		break;
	default :
		return FALSE;
	}

	return FALSE;
}
void infoDlg::doDialog(const std::string &capation,const std::string &info ,const std::string help)
{
	lock();
	m_infos.push_back(showInfo_s());
	showInfo_s &si = m_infos.back();
	si.capation = capation;
	si.info = info;
	si.help = help;
	unlock();

	if( !isCreated() )
	{
		init( g_Inst,g_nppData._nppHandle );

		create( IDD_INFO );
	}
	else
	{
		setInfo();
	}

	goToCenter();

	getFocus();
}

void infoDlg::setInfo()
{
	showInfo_s si;
	lock();
	if( m_infos.size() )
	{
		si = m_infos.front();
	}
	unlock();

	::SetWindowTextA( _hSelf,si.capation.c_str() );
	::SetWindowTextA( m_hShow,si.info.c_str() );
	::SetWindowTextA( m_hHelp,si.help.c_str() );
}

infoDlg g_infoDlg;

extern "C"
{
	void showInfo(const std::string &capation,const std::string &info ,const std::string help)
	{
		g_infoDlg.doDialog(capation,info,help);
	}
};