// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�:
#include <windows.h>



#include <CommCtrl.h>
//#include <commdlg.h>
// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�

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