/****

 所有权限归 gmzd84 所有

 联系方式： gmzd84@aliyun.com

 */

#include "stdafx.h"

#include <string>


NppData g_nppData;

HWND getSCI()
{
	int which = -1;
	::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return NULL;
	HWND curScintilla = (which == 0)?g_nppData._scintillaMainHandle:g_nppData._scintillaSecondHandle;

	return curScintilla;
}

void getText( std::string &t )
{
	HWND sci = getSCI();

	if( !sci )
	{
		return;
	}

	int len =::SendMessageA(sci, SCI_GETTEXTLENGTH, 0, 0);

	char *buf = new char[len+1];
	::SendMessageA(sci, SCI_GETTEXT, len+1,(WPARAM)buf);

	t = buf;

	delete []buf;
}

int getSelLen()
{
	HWND sci = getSCI();

	if( !sci )
	{
		return 0;
	}

	return ::SendMessageA(sci, SCI_GETSELTEXT, 0, 0);
}

void getSelText( std::string &t )
{
	HWND sci = getSCI();

	if( !sci )
	{
		return;
	}

	int len =::SendMessageA(sci, SCI_GETSELTEXT, 0, 0);

	if( len )
	{
		char *buf = new char[len+1];
		::SendMessageA(sci, SCI_GETSELTEXT, 0,(WPARAM)buf);

		t = buf;

		delete []buf;
	}
}

void appendText( const std::string &t )
{
	HWND sci = getSCI();

	if( !sci )
	{
		return;
	}

	::SendMessageA(sci, SCI_SETSEL, -1, -1);

	::SendMessageA(sci, SCI_REPLACESEL, 0,(WPARAM)t.c_str());
}

void replaceCurSel( const std::string &t )
{
	HWND sci = getSCI();

	if( !sci )
	{
		return;
	}

	::SendMessageA(sci, SCI_REPLACESEL, 0,(WPARAM)t.c_str());
}

void setText( const std::string &t )
{
	HWND sci = getSCI();

	if( !sci )
	{
		return;
	}

	::SendMessageA(sci, SCI_SETTEXT, 0,(WPARAM)t.c_str());
}

void getCurText( std::string &t )
{
	getSelText(t);

	if( t.empty() )
	{
		getText(t);
	}
}

void setCurText( const std::string &t )
{
	int len = getSelLen();

	if( len > 1 )  // 当没有选择的时候返回了 1
	{
		replaceCurSel(t);
	}
	else
	{
		setText(t);
	}
}


extern "C"{

	void lua_run();
	void lua_script();
	void json_format();
	void xml_format();
	void unique_text();
	void sort_text();
	void html_encode();

	FuncItem g_fi[] = {
		{TEXT("lua run"),lua_run,0,false,NULL},
		{TEXT("lua script"),lua_script,1,false,NULL},
		{TEXT("json format"),json_format,2,false,NULL},
		{TEXT("xml format"),xml_format,3,false,NULL},
		{TEXT("unique"),unique_text,4,false,NULL},
		{TEXT("sort"),sort_text,5,false,NULL},
		{TEXT("html encode"),html_encode,6,false,NULL},
	};

	void setInfo(NppData nd)
	{
		g_nppData = nd;
	}
	const TCHAR * getName()
	{
		return TEXT("zg");
	}
	FuncItem * getFuncsArray(int *nb)
	{
		*nb = sizeof( g_fi ) / sizeof( g_fi[0] );

		return g_fi;
	}
	void beNotified(SCNotification *notifyCode)
	{

	}
	LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
	{
		return TRUE;
	}

	BOOL isUnicode()
	{
#ifdef UNICODE
		return TRUE;
#else
		return FALSE;
#endif
	}

};