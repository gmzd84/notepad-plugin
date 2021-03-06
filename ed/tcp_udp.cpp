
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <io.h>
#include <string>
#include <map>
#include <sstream>
#include <list>

#include "strExt.h"
#include "lock.h"

#include "lua/lua.hpp"

#pragma comment( lib,"ws2_32.lib" )


void sock_thr( void *d );

namespace {
	struct socketInit
	{
		socketInit()
		{
			WSADATA d;
			::WSAStartup(MAKEWORD(2,2),&d);

			::_beginthread( sock_thr,(128<<10),NULL );
		}
		~socketInit()
		{
			::WSACleanup();
		}
	}g_socketInit;
}

struct socket_info_s
{
	static const std::string OK;

	socket_info_s()
		:s(INVALID_SOCKET),
		istcp(false),
		isclient(true)
	{}

	

	~socket_info_s()
	{
		if( !isNullSocket() )
		{
			close();

			wait_notify.notify();
		}
	}
	void close()
	{
		if( INVALID_SOCKET != s )
		{
			closesocket(s);
			s = INVALID_SOCKET;
		}
	}

	void reset(SOCKET s,bool istcp,bool isclient)
	{
		if( !isNullSocket() )
		{
			close();
		}

		socket_info_s::s = s;
		socket_info_s::istcp = istcp;
		socket_info_s::isclient = isclient;
	}

	void operator=( socket_info_s &r )
	{
		reset( r.s,r.istcp,r.isclient );

		r.reset( INVALID_SOCKET,false,false );
	}

	bool init( const std::string & sip,unsigned short port,bool istcp,bool isclient)
	{
		if( !isNullSocket() )
		{
			close();
		}

		this->istcp = istcp;
		this->isclient = isclient;

		if( istcp )
		{
			s = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP );
		}
		else
		{
			s = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );
		}

		if( INVALID_SOCKET == s )
		{
			return false;
		}	

		unsigned ip = (unsigned)str_to_uint(sip);

		struct sockaddr_in sin = {0};
		sin.sin_family = AF_INET;
		sin.sin_addr.S_un.S_addr   = ip;
		sin.sin_port = htons( port );

		if( !isclient )
		{
			// 服务端要绑定
			int ret = ::bind( s,(sockaddr*)&sin,sizeof(sin) );
			if( SOCKET_ERROR == ret )
			{
				close();

				return false;
			}

			if( istcp )
			{
				int ret = listen( s,0 );
				if( SOCKET_ERROR == ret )
				{
					close();

					return false;
				}
			}
		}
		else
		{
			// 客户端 TCP 要连接
			if( istcp )
			{
				int ret = ::connect(s,(sockaddr*)&sin,sizeof(sin));
				if( SOCKET_ERROR == ret )
				{
					close();

					return false;
				}
			}
		}

		return true;
	}

	bool isNullSocket()
	{
		return s == INVALID_SOCKET;
	}

	std::string recv( char *buf,int &len )
	{
		wait_notify.wait();

		int ret = ::recv( s,buf,len,0 );

		if( ret < 0 )
		{
			// 失败
			return std::string("error");
		}
		else if( ret == 0 )
		{
			// 超时
			return std::string("timeout");
		}
		else
		{
			len = ret;
			return std::string("ok");
		}		
	}

	std::string send( char *buf,int &len )
	{
		int ret = ::send( s,buf,len,0 );

		if( ret < 0 )
		{
			// 失败
			return std::string("error");
		}
		else if( ret == 0 )
		{
			// 超时
			return std::string("timeout");
		}
		else
		{
			len = ret;
			return std::string("ok");
		}		
	}

	std::string recv_from( char *buf,int &len,sockaddr &sa )
	{
		wait_notify.wait();

		int slen = sizeof(sa);

		int ret = ::recvfrom( s,buf,len,0,(sockaddr*)&sa,&slen );

		if( ret < 0 )
		{
			// 失败
			return std::string("error");
		}
		else if( ret == 0 )
		{
			// 超时
			return std::string("timeout");
		}
		else
		{
			len = ret;
			return std::string("ok");
		}		
	}

	std::string send_to( char *buf,int &len,sockaddr &sa )
	{
		int ret = ::sendto( s,buf,len,0,&sa,sizeof(sa) );

		if( ret < 0 )
		{
			// 失败
			return std::string("error");
		}
		else if( ret == 0 )
		{
			// 超时
			return std::string("timeout");
		}
		else
		{
			len = ret;
			return std::string("ok");
		}		
	}

	SOCKET socket()
	{
		return s;
	}

	void notify()
	{
		wait_notify.notify();
	}
	void wait()
	{
		wait_notify.wait();
	}
protected:
	SOCKET s;
	waitNotifyLock wait_notify; 

	bool istcp;
	bool isclient;
};

const std::string socket_info_s::OK("ok");

static socket_info_s gs_nullSocket;

struct socket_mgr_s
{
	std::map<std::string,socket_info_s> connectInfo;
	msgLock lock;

	unsigned init_val;

	static const unsigned MAGIC = 0x789abcdeUL;;

	socket_mgr_s()
	{
		init_val = MAGIC;
	}

	~socket_mgr_s()
	{
		
	}

	bool is_init()
	{
		return socket_mgr_s::MAGIC != init_val;
	}
};
static socket_mgr_s gs_socketMgr;

static void sock_thr( void *d )
{
	// 等待变量初始化
	while(  !gs_socketMgr.is_init() )
	{
		Sleep(3);
	}

	//
	while( 1 )
	{
		Sleep( 2 );

		fd_set fs;
		FD_ZERO(&fs);

		unsigned cnt = 0;

		{
			msgGuard mg(gs_socketMgr.lock);

			for( auto i=gs_socketMgr.connectInfo.begin();i != gs_socketMgr.connectInfo.end(); ++i )
			{
				FD_SET( i->second.socket(),&fs );

				++cnt;
			}
		}
		

		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 8*1000;

		int ret = select( cnt,&fs,NULL,NULL,&tv );

		if( ret < 0 )
		{
			// 失败
		}
		else if( ret == 0 )
		{
			// 超时
		}
		else
		{
			msgGuard mg2(gs_socketMgr.lock);

			for( auto i=gs_socketMgr.connectInfo.begin();i != gs_socketMgr.connectInfo.end(); ++i )
			{
				if( FD_ISSET( i->second.socket(),&fs ) )
				{
					i->second.notify();
				}
			}
		}
	}
}


static std::string getConnPos( const std::string & sip,unsigned short port )
{
	unsigned ip = htonl(0x7f000001);
	if( sip.empty() )
	{
		ip = ::inet_addr(sip.c_str());

		if( INADDR_NONE == ip )
		{
			return std::string("");
		}
	}

	std::ostringstream os;
	os<<ip<<":"<<port;

	return os.str();
}
socket_info_s &getSocket( const std::string & sip,unsigned short port,bool istcp,bool isclient )
{
	msgGuard g(gs_socketMgr.lock);
	
	// 查找老的连接是否存在
	std::string ci = getConnPos(sip,port);

	::MessageBoxA(NULL,ci.c_str(),"",MB_OK );

	auto c = gs_socketMgr.connectInfo.find(ci);
	if( c != gs_socketMgr.connectInfo.end() )
	{
		return c->second;
	}

	// 创建一个新的连接

	socket_info_s &si = gs_socketMgr.connectInfo[ci];
	si.init( sip,port,istcp,isclient );

	return si;
}

socket_info_s &getSocket( const std::string &pos,bool istcp,bool isclient )
{
	std::string ip,port;
	str_break( pos,":",ip,port );

	if( port.empty() )
	{
		return gs_nullSocket;
	}

	unsigned p = ::atoi( port.c_str() );
	if( p > 0xfffful )
	{
		return gs_nullSocket;
	}

	return getSocket( ip,p,istcp,isclient );
}

void releaseSocket( const std::string &pos )
{
	msgGuard g(gs_socketMgr.lock);

	auto c = gs_socketMgr.connectInfo.find(pos);
	if( c != gs_socketMgr.connectInfo.end() )
	{
		gs_socketMgr.connectInfo.erase( c );
	}
}

void releaseSocket( const std::string &sip,unsigned short port )
{
	return releaseSocket( getConnPos( sip,port ) );
}

void closeConnect( const std::string &conn )
{
	releaseSocket(conn);
}

////////////////////////////////////////////////////////////////////////////////////

struct socketRsArg
{
	bool (*fn)(const std::string &rcv,std::string &snd ,void *arg); // 返回 false 表示停止处理
	void *arg;
};


void udp_rs( socket_info_s& s,void *arg )
{
	int len = 8*1024;
	char *buf = new char[len+1];

	struct sockaddr sa = {0};
	int slen = sizeof(sa);

	socketRsArg *sra = (socketRsArg*)arg;

	while( 1 )
	{
		std::string ret = s.recv_from( buf,len,sa );
		if( socket_info_s::OK == ret )
		{
			break;
		}

		std::string rcv;
		rcv.append( buf,len );

		std::string snd;
		bool goon = sra->fn( rcv,snd,sra->arg );

		if(snd.size())
		{
			int len = snd.size();
			s.send_to( (char*)snd.c_str(),len,sa );
		}
		
		if( !goon )
		{
			break;
		}
	}

	delete []buf;
	delete sra;
}

void tcp_rs( socket_info_s& s,void *arg )
{
	const int len = 8*1024;
	char *buf = new char[len+1];
	
	socketRsArg *sra = (socketRsArg*)arg;

	while( 1 )
	{
		int si = len;
		std::string ret = s.recv( buf,si );
		if( socket_info_s::OK == ret )
		{
			break;
		}

		std::string rcv;
		rcv.append( buf,si );

		std::string snd;
		bool goon = sra->fn( rcv,snd,sra->arg );

		if(snd.size())
		{
			int c=snd.size();

			s.send( (char*)snd.c_str(),c );
		}

		if( !goon )
		{
			break;
		}
	}

	delete []buf;
	delete sra;
}
void udp_listen(const char *sip,unsigned short port,void (*fn)(socket_info_s& s,void*),void*arg )
{
	if( !port || !fn )
	{
		return;
	}

	socket_info_s &s = getSocket( sip,port,false,false );
	if( s.isNullSocket() )
	{
		return ;
	}
	
	fn( s,arg );

	releaseSocket( sip,port );


	return;
}

void udp_listen(const char *sip,unsigned short port,bool (*fn)(const std::string &,std::string & ,void *),void*arg )
{
	socketRsArg *urs = new socketRsArg;

	urs->fn  = fn;
	urs->arg = arg;

	udp_listen( sip,port,udp_rs,urs );
}

void udp_listen(const std::string &conn,bool (*fn)(const std::string &,std::string & ,void *),void*arg )
{
	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	udp_listen( ssi.c_str(),port,fn,arg );
}

struct tcpThrArg
{
	SOCKET s;
	void (*fn)(socket_info_s& s,void*);
	void *arg;

	tcpThrArg()
		:s( INVALID_SOCKET),
		fn( NULL ),
		arg(NULL)
	{

	}

	~tcpThrArg()
	{
		if( INVALID_SOCKET != s )
		{
			::closesocket(s);
			s = INVALID_SOCKET;
		}
	}

};

void tcpThrFun( void *d )
{
	tcpThrArg *tta = (tcpThrArg*)d;

	socket_info_s *si = NULL;

	std::string pos = uint_to_str(tta->s);
	{
		msgGuard g(gs_socketMgr.lock);

		si = &gs_socketMgr.connectInfo[pos];
	}

	si->reset( tta->s,true,false );

	tta->s = INVALID_SOCKET;

	tta->fn( *si,tta->arg );

	releaseSocket( pos );

	delete tta;
}

void tcp_listen(const char *sip,unsigned short port,void (*fn)(socket_info_s& s,void*),void*arg )
{
	if( !port || !fn )
	{
		return;
	}

	socket_info_s &s = getSocket( sip,port,true,false );
	if( s.isNullSocket() )
	{
		return ;
	}


	struct sockaddr sa = {0};
	int flen = sizeof(sa);

	socketRsArg *sra = (socketRsArg*)arg;

	//while( 1 )  lua 不支持多线程
	{
		s.wait();

		SOCKET ns = accept( s.socket(),&sa,&flen );
		if( INVALID_SOCKET == ns )
		{
			goto tcp_listen_end;
		}	

		tcpThrArg *tta = new tcpThrArg;
		tta->s = ns;

		tta->fn  = fn;
		tta->arg = arg;

		//::_beginthread( tcpThrFun,0,tta );
		tcpThrFun( tta );
	}

tcp_listen_end:
	releaseSocket( sip,port );
	delete sra;
}



void tcp_listen(const char *sip,unsigned short port,bool (*fn)(const std::string &,std::string & ,void *),void*arg )
{
	socketRsArg *urs = new socketRsArg;

	urs->fn  = fn;
	urs->arg = arg;

	tcp_listen( sip,port,tcp_rs,urs );
}

void tcp_listen(const std::string &conn,bool (*fn)(const std::string &,std::string & ,void *),void*arg )
{
	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	tcp_listen( ssi.c_str(),port,fn,arg );
}

////////////////////////////////////////////////////////////////////////////////////

bool udp_send_wait( const char *sip,unsigned port,const std::string &snd,char *buf,int &len )
{
	socket_info_s &s = getSocket( sip,port,false,true );
	if( s.isNullSocket( ) )
	{
		return false;
	}

	std::string ci = getConnPos(sip,port);
	std::string ssi,sp;
	str_break( ci,":",ssi,sp );
	unsigned ip = (unsigned)str_to_uint(ssi);

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr   = ip;
	sin.sin_port = htons( port );

	int ss = snd.size();
	std::string ret = s.send_to( (char*)snd.c_str(),ss,*(sockaddr*)&sin );

	if( socket_info_s::OK != ret )
	{
		releaseSocket( ci );

		return false;
	}

	if( buf && len )
	{
		struct sockaddr sa={0};
		int len = sizeof(sa);
		
		ret = s.recv_from( (char*)buf,len,*(sockaddr*)&sa );
		if( socket_info_s::OK != ret )
		{
			releaseSocket(ci);

			return false;
		}
	}

	return true;
}


bool tcp_send_wait( const char *sip,unsigned port,const std::string &snd,char *buf,int &len )
{
	socket_info_s &s = getSocket( sip,port,true,true );
	if( s.isNullSocket() )
	{
		return false;
	}

	int ss = snd.size();
	std::string ret = s.send( (char*)snd.c_str(),ss );

	if( socket_info_s::OK != ret )
	{
		releaseSocket(sip,port);

		return false;
	}

	if( buf && len )
	{
		std::string ret = s.recv( buf,len );
		if( socket_info_s::OK != ret )
		{
			releaseSocket(sip,port);

			return false;
		}

		len = 1;
	}

	return true;
}

bool udp_send_rcv( const std::string &conn,const std::string &snd, std::string &rcv )
{
	int len = 16*1024;
	char *buf = new char[len+1];

	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	bool r = udp_send_wait( ssi.c_str(),port,snd,buf,len );
	if( r )
	{
		rcv = "";
		rcv.append(buf,len);

		return true;
	}

	return false;
}

bool tcp_send_rcv( const std::string &conn,const std::string &snd, std::string &rcv )
{
	int len = 16*1024;
	char *buf = new char[len+1];

	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	bool r = tcp_send_wait( ssi.c_str(),port,snd,buf,len );
	if( r )
	{
		rcv = "";
		rcv.append(buf,len);

		return true;
	}

	return false;
}


bool udp_send( const std::string &conn,const std::string &snd )
{

	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	int len = 0;
	bool r = udp_send_wait( ssi.c_str(),port,snd,NULL,len );
	if( r )
	{
		return true;
	}

	return false;
}

bool tcp_send( const std::string &conn,const std::string &snd )
{
	std::string ssi,sp;
	str_break( conn,":",ssi,sp );
	unsigned short port = (unsigned short)str_to_uint(sp);

	int len = 0;
	bool r = tcp_send_wait( ssi.c_str(),port,snd,NULL,len );
	if( r )
	{
		return true;
	}

	return false;
}


bool udp_rcv( const std::string &conn,std::string &rcv )
{
	socket_info_s &s = getSocket( conn,false,true );
	
	if( s.isNullSocket() )
	{
		return false;
	}
	
	struct sockaddr sa = {0};
	int slen = sizeof(sa);

	int len = 8*1024;
	char *buf = new char[len+1];


	std::string ret = s.recv_from( buf,len,sa );
	if( socket_info_s::OK != ret )
	{
		releaseSocket(conn);
		return false;
	}

	rcv = "";
	rcv.append( buf,len );

	return true;
}

bool tcp_rcv( const std::string &conn,std::string &rcv )
{
	socket_info_s &s = getSocket( conn,false,true );
	
	if( s.isNullSocket() )
	{
		return false;
	}
	
	int len = 8*1024;
	char *buf = new char[len+1];


	std::string ret = s.recv( buf,len );
	if( socket_info_s::OK != ret )
	{
		releaseSocket(conn);
		return false;
	}

	rcv = "";
	rcv.append( buf,len );

	return true;
}





extern "C"
{
	int lua_closeConnect( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call closeConnect.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call closeConnect.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		
		closeConnect(s);


		return 0;
	}

	int lua_udpSend( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,-1) || !lua_isstring(L,-2))
		{
			lua_pushliteral(L,"correct call is : r = udpSend( \"127.0.0.1:7374\",\"sendinfo\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string snd = lua_tostring(L,-1);
		lua_pop( L,2 );

		bool r = udp_send( conn,snd );
		
		lua_pushboolean(L,r);
		return 1;
	}

	int lua_tcpSend( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,-1) || !lua_isstring(L,-2))
		{
			lua_pushliteral(L,"correct call is : r = tcpSend( \"127.0.0.1:7374\",\"sendinfo\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string snd = lua_tostring(L,-1);
		lua_pop( L,2 );

		bool r = tcp_send( conn,snd );
		
		lua_pushboolean(L,r);
		return 1;
	}

	int lua_udpRcv( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 || !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"correct call is : r,s= udpSend( \"127.0.0.1:7374\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-1);
		lua_pop( L,1 );

		std::string rcv;

		bool r = udp_rcv( conn,rcv );
		
		lua_pushboolean(L,r);
		lua_pushlstring( L,rcv.c_str(),rcv.size() );
		return 2;
	}

	int lua_tcpRcv( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 || !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"correct call is : r,s = tcpSend( \"127.0.0.1:7374\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-1);
		lua_pop( L,1 );

		std::string rcv;

		bool r = tcp_rcv( conn,rcv );
		
		lua_pushboolean(L,r);
		lua_pushlstring( L,rcv.c_str(),rcv.size() );
		return 2;
	}

	int lua_udpSendRcv( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,-1) || !lua_isstring(L,-2))
		{
			lua_pushliteral(L,"correct call is : r,s = udpSend( \"127.0.0.1:7374\",\"sendinfo\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string snd = lua_tostring(L,-1);
		lua_pop( L,2 );

		std::string rv;
		bool r = udp_send_rcv( conn,snd,rv );
		
		lua_pushboolean(L,r);
		lua_pushlstring( L,rv.c_str(),rv.size() );
		return 2;
	}

	int lua_tcpSendRcv( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,-1) || !lua_isstring(L,-2))
		{
			lua_pushliteral(L,"correct call is : r,s = tcpSend( \"127.0.0.1:7374\",\"sendinfo\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string snd = lua_tostring(L,-1);
		lua_pop( L,2 );

		std::string rv;
		bool r = tcp_send_rcv( conn,snd,rv );
		
		lua_pushboolean(L,r);
		lua_pushlstring( L,rv.c_str(),rv.size() );
		return 2;
	}

	struct monitor_info_s
	{
		lua_State *L;
		std::string lua_fn;
	};

	static bool call_lun_fun(const std::string &rcv,std::string &snd ,void *arg )
	{
		monitor_info_s *pmon = (monitor_info_s*)arg;
		
		int ret = lua_getglobal( pmon->L,pmon->lua_fn.c_str() );

		if( LUA_TFUNCTION != ret )  //  变量不存在返回 0
		{
			std::string err = "'";
			err += pmon->lua_fn;
			err += "' is not a function name;";
			
			::MessageBoxA(NULL,err.c_str(),"lua error",MB_OK);
			return false;
		}

		lua_pushlstring( pmon->L,rcv.c_str(),rcv.size() );
		ret = lua_pcall( pmon->L,1,2,0 );
		if( LUA_OK != ret )
		{
			std::string es = lua_tostring( pmon->L,-1 );

			lua_pop(pmon->L,1);

			::MessageBoxA(NULL,es.c_str(),"lua error",MB_OK);
			return false;
		}

		int n = lua_gettop( pmon->L );
		if( n < 2 )
		{
			::MessageBoxA(NULL,"the number of return value is less than 2.",pmon->lua_fn.c_str(),MB_OK);
			return false;
		}

		snd = lua_tostring( pmon->L,-1 );
		int r = lua_toboolean( pmon->L,-2 );
		lua_pop(pmon->L,2 );

		return r != 0;
	}

	int lua_udpMonitor( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,1) || !lua_isstring(L,2))
		{
			lua_pushliteral(L,"correct call is : r,s = udpMonitor( \"127.0.0.1:7374\",\"monitorfunname\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string fn = lua_tostring(L,-1);
		lua_pop( L,2 );

		monitor_info_s *pmon = new monitor_info_s;
		pmon->L = L;
		pmon->lua_fn = fn;
		udp_listen( conn,call_lun_fun,pmon );


		delete pmon;
		return 0;
	}

	int lua_tcpMonitor( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 2 || !lua_isstring(L,1) || !lua_isstring(L,2))
		{
			lua_pushliteral(L,"correct call is : r,s = tcpMonitor( \"127.0.0.1:7374\",\"monitorfunname\" );");
			return lua_error(L);
		}

		std::string conn = lua_tostring(L,-2);
		std::string fn = lua_tostring(L,-1);
		lua_pop( L,2 );

		monitor_info_s *pmon = new monitor_info_s;
		pmon->L = L;
		pmon->lua_fn = fn;
		tcp_listen( conn,call_lun_fun,pmon );


		delete pmon;
		return 0;
	}

	void tcp_udp_reg(lua_State *L )
	{
		lua_register( L,"closeConnect",lua_closeConnect );
		lua_register( L,"udpSend",lua_udpSend );
		lua_register( L,"tcpSend",lua_tcpSend );
		lua_register( L,"udpRcv",lua_udpRcv );
		lua_register( L,"tcpRcv",lua_tcpRcv );
		lua_register( L,"udpSendRcv",lua_udpSendRcv );
		lua_register( L,"tcpSendRcv",lua_tcpSendRcv );
		lua_register( L,"udpMonitor",lua_udpMonitor );
		lua_register( L,"tcpMonitor",lua_tcpMonitor );
	}
};






/***

EBADF 参数s非合法的socket处理代码
EFAULT 参数中有一指针指向无法存取的内存空间。
ENOTSOCK 参数s为一文件描述词，非socket。
EINTR 被信号所中断。
EAGAIN 此动作会令进程阻断，但参数s的socket为不可阻断。
ENOBUFS 系统的缓冲内存不足
ENOMEM 核心内存不足
EINVAL 传给系统调用的参数不正确。

  函数原型：
>>  int socket(int domain, int type, int protocol);
  参数说明：
　　
  domain：协议域，又称协议族（family）。常用的协议族有AF_INET、AF_INET6、AF_LOCAL（或称AF_UNIX，Unix域Socket）、AF_ROUTE等。协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合、AF_UNIX决定了要用一个绝对路径名作为地址。
  type：指定Socket类型。常用的socket类型有SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等。流式Socket（SOCK_STREAM）是一种面向连接的Socket，针对于面向连接的TCP服务应用。数据报式Socket（SOCK_DGRAM）是一种无连接的Socket，对应于无连接的UDP服务应用。
  protocol：指定协议。常用协议有IPPROTO_TCP、IPPROTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议。
  注意：type和protocol不可以随意组合，如SOCK_STREAM不可以跟IPPROTO_UDP组合。当第三个参数为0时，会自动选择第二个参数类型对应的默认协议。
  返回值：
  如果调用成功就返回新创建的套接字的描述符，如果失败就返回INVALID_SOCKET（Linux下失败返回-1）。套接字描述符是一个整数类型的值。每个进程的进程空间里都有一个套接字描述符表，该表中存放着套接字描述符和套接字数据结构的对应关系。该表中有一个字段存放新创建的套接字的描述符，另一个字段存放套接字数据结构的地址，因此根据套接字描述符就可以找到其对应的套接字数据结构。每个进程在自己的进程空间里都有一个套接字描述符表但是套接字数据结构都是在操作系统的内核缓冲里。
  绑定

  函数原型：
>>  int bind(SOCKET socket, const struct sockaddr* address, socklen_t address_len);
  参数说明：
  socket：是一个套接字描述符。
  address：是一个sockaddr结构指针，该结构中包含了要结合的地址和端口号。
  address_len：确定address缓冲区的长度。
  返回值：
  如果函数执行成功，返回值为0，否则为SOCKET_ERROR。
  接收

  函数原型：
>>  int recv(SOCKET socket, char FAR* buf, int len, int flags);
  参数说明：
  　　
	socket：一个标识已连接套接口的描述字。
	buf：用于接收数据的缓冲区。
	len：缓冲区长度。
	flags：指定调用方式。取值：MSG_PEEK 查看当前数据，数据将被复制到缓冲区中，但并不从输入队列中删除；MSG_OOB 处理带外数据。
	返回值：
	若无错误发生，recv()返回读入的字节数。如果连接已中止，返回0。否则的话，返回SOCKET_ERROR错误，应用程序可通过WSAGetLastError()获取相应错误代码。
	
	函数原型：
>>	ssize_t recvfrom(int sockfd, void buf, int len, unsigned int flags, struct socketaddr* from, socket_t* fromlen);
	参数说明：
	sockfd：标识一个已连接套接口的描述字。
	buf：接收数据缓冲区。
	len：缓冲区长度。
	flags：调用操作方式。是以下一个或者多个标志的组合体，可通过or操作连在一起：
	（1）MSG_DONTWAIT：操作不会被阻塞；
	（2）MSG_ERRQUEUE： 指示应该从套接字的错误队列上接收错误值，依据不同的协议，错误值以某种辅佐性消息的方式传递进来，使用者应该提供足够大的缓冲区。导致错误的原封包通过msg_iovec作为一般的数据来传递。导致错误的数据报原目标地址作为msg_name被提供。错误以sock_extended_err结构形态被使用。
	（3）MSG_PEEK：指示数据接收后，在接收队列中保留原数据，不将其删除，随后的读操作还可以接收相同的数据。
	（4）MSG_TRUNC：返回封包的实际长度，即使它比所提供的缓冲区更长， 只对packet套接字有效。
	（5）MSG_WAITALL：要求阻塞操作，直到请求得到完整的满足。然而，如果捕捉到信号，错误或者连接断开发生，或者下次被接收的数据类型不同，仍会返回少于请求量的数据。
	（6）MSG_EOR：指示记录的结束，返回的数据完成一个记录。
	（7）MSG_TRUNC：指明数据报尾部数据已被丢弃，因为它比所提供的缓冲区需要更多的空间。
	（8）MSG_CTRUNC：指明由于缓冲区空间不足，一些控制数据已被丢弃。
	（9）MSG_OOB：指示接收到out-of-band数据(即需要优先处理的数据)。
	（10）MSG_ERRQUEUE：指示除了来自套接字错误队列的错误外，没有接收到其它数据。
	from：（可选）指针，指向装有源地址的缓冲区。
	fromlen：（可选）指针，指向from缓冲区长度值。
	发送

	函数原型：
>>	int sendto( SOCKET s, const char FAR* buf, int size, int flags, const struct sockaddr FAR* to, int tolen);
	参数说明：
	s：套接字
	buf：待发送数据的缓冲区
	size：缓冲区长度
	flags：调用方式标志位, 一般为0, 改变Flags，将会改变Sendto发送的形式
	addr：（可选）指针，指向目的套接字的地址
	tolen：addr所指地址的长度
	返回值：
	如果成功，则返回发送的字节数，失败则返回SOCKET_ERROR。
	函数原型：
	int accept( int fd, struct socketaddr* addr, socklen_t* len);
	参数说明：
	fd：套接字描述符。
	addr：返回连接着的地址
	len：接收返回地址的缓冲区长度
	返回值：
	成功返回客户端的文件描述符，失败返回-1。
*/