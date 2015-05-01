// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
#include <windows.h>



#include <CommCtrl.h>
//#include <commdlg.h>
// TODO: 在此处引用程序需要的其他头文件

#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"

#include "resource.h"

#include "lock.h"

#include <string>

#include "strExt.h"

#include "tcp_udp.h"

extern HINSTANCE g_Inst;
extern NppData g_nppData;

std::string uint_to_str( unsigned long long d );

void getText( std::string &t );
void getSelText( std::string &t );
void appendText( const std::string &t );
void replaceCurSel( const std::string &t );
void setText( const std::string &t );
void getCurText( std::string &t );
void setCurText( const std::string &t );


extern "C"
{
	void showInfo(const std::string &capation,const std::string &info ,const std::string help);

	bool extractInput( const std::string &capation,std::string &inputInfo );

	void getInput(const std::string &capation,void(*fn)(const std::string&));

	void getCmdOut( const std::string &cmd,std::string &s );

	extern void closeSocket(int s);
	unsigned udp_send_rcv( unsigned p,const std::string & ip,unsigned short port,const std::string &snd, std::string &rcv );
	unsigned tcp_send_rcv( unsigned p,const std::string & ip,unsigned short port,const std::string &snd, std::string &rcv );
};