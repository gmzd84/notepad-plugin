
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <io.h>
#include <string>
#include <map>
#include <sstream>

#include "strExt.h"
#include "lock.h"

#include "lua/lua.hpp"

#pragma comment( lib,"ws2_32.lib" )

namespace {

	struct socketInit
	{
		socketInit()
		{
			WSADATA d;
			::WSAStartup(MAKEWORD(2,2),&d);
		}
		~socketInit()
		{
			::WSACleanup();
		}
	}g_socketInit;
}

struct socket_info_s
{
	SOCKET s;

	bool istcp;
	bool isclient;

	socket_info_s()
		:s(INVALID_SOCKET),
		istcp(false),
		isclient(true)
	{}
};
static socket_info_s gs_nullSocket;
bool isNullSocket(socket_info_s &s)
{
	return s.s == INVALID_SOCKET;
}

struct socket_mgr_s
{
	std::map<std::string,socket_info_s> connectInfo;
	msgLock lock;

	~socket_mgr_s()
	{
		for( auto i = connectInfo.begin();i!=connectInfo.end();++i )
		{
			::closesocket( i->second.s );
			i->second.s = INVALID_SOCKET;
		}
	}
};
static socket_mgr_s gs_socketMgr;


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
	
	// �����ϵ������Ƿ����
	std::string ci = getConnPos(sip,port);

	::MessageBoxA(NULL,ci.c_str(),"",MB_OK );

	auto c = gs_socketMgr.connectInfo.find(ci);
	if( c != gs_socketMgr.connectInfo.end() )
	{
		return c->second;
	}

	// ����һ���µ�����

	SOCKET s = INVALID_SOCKET;
	
	if( istcp )
	{
		s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP );
	}
	else
	{
		s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );
	}

	if( INVALID_SOCKET == s )
	{
		return gs_nullSocket;
	}	


	std::string ssi,sp;
	str_break( ci,":",ssi,sp );
	unsigned ip = (unsigned)str_to_uint(ssi);

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr   = ip;
	sin.sin_port = htons( port );

	if( !isclient )
	{
		// �����Ҫ��
		int ret = bind( s,(sockaddr*)&sin,sizeof(sin) );
		if( SOCKET_ERROR == ret )
		{
			closesocket(s);

			std::string err = "Failed to bind port :";
			err + ci;
			::MessageBoxA(NULL,err.c_str(),"Error",MB_OK );
			return gs_nullSocket;
		}

		if( istcp )
		{
			int ret = listen( s,0 );
			if( SOCKET_ERROR == ret )
			{
				closesocket(s);
				return gs_nullSocket;
			}
		}
	}
	else
	{
		// �ͻ��� TCP Ҫ����
		if( istcp )
		{
			int ret = ::connect(s,(sockaddr*)&sin,sizeof(sin));
			if( SOCKET_ERROR == ret )
			{
				closesocket(s);

				std::string err = "Failed to connect to port :";
				err + ci;
				::MessageBoxA(NULL,err.c_str(),"Error",MB_OK );
				return gs_nullSocket;
			}
		}
	}

	socket_info_s &si = gs_socketMgr.connectInfo[ci];
	si.s = s;
	si.isclient = isclient;
	si.istcp    = istcp;

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
		::closesocket( c->second.s );
		c->second.s = INVALID_SOCKET;
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
	bool (*fn)(const std::string &rcv,std::string &snd ,void *arg); // ���� false ��ʾֹͣ����
	void *arg;
};


void udp_rs( SOCKET s,void *arg )
{
	const int len = 8*1024;
	char *buf = new char[len+1];

	struct sockaddr sa = {0};
	int slen = sizeof(sa);

	socketRsArg *sra = (socketRsArg*)arg;

	while( 1 )
	{
		int ret = recvfrom( s,buf,len,0,(sockaddr*)&sa,&slen );
		if( SOCKET_ERROR == ret )
		{
			break;
		}

		std::string rcv;
		rcv.append( buf,ret );

		std::string snd;
		bool goon = sra->fn( rcv,snd,sra->arg );

		if(snd.size())
		{
			sendto( s,snd.c_str(),snd.size(),0,&sa,sizeof(sa) );
		}
		
		if( !goon )
		{
			break;
		}
	}

	delete []buf;
	delete sra;
}

void tcp_rs( SOCKET s,void *arg )
{
	const int len = 8*1024;
	char *buf = new char[len+1];
	
	socketRsArg *sra = (socketRsArg*)arg;

	while( 1 )
	{
		int ret = recv( s,buf,len,0 );
		if( SOCKET_ERROR == ret )
		{
			break;
		}

		std::string rcv;
		rcv.append( buf,ret );

		std::string snd;
		bool goon = sra->fn( rcv,snd,sra->arg );

		if(snd.size())
		{
			send( s,snd.c_str(),snd.size(),0 );
		}

		if( !goon )
		{
			break;
		}
	}

	delete []buf;
	delete sra;
}
void udp_listen(const char *sip,unsigned short port,void (*fn)(SOCKET s,void*),void*arg )
{
	if( !port || !fn )
	{
		return;
	}

	socket_info_s &s = getSocket( sip,port,false,false );
	if( isNullSocket(s) )
	{
		return ;
	}

	fn( s.s,arg );

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
	void (*fn)(SOCKET s,void*);
	void *arg;
};

void tcpThrFun( void *d )
{
	tcpThrArg *tta = (tcpThrArg*)d;

	tta->fn( tta->s,tta->arg );

	closesocket( tta->s );

	delete tta;
}

void tcp_listen(const char *sip,unsigned short port,void (*fn)(SOCKET s,void*),void*arg )
{
	if( !port || !fn )
	{
		return;
	}

	socket_info_s &s = getSocket( sip,port,true,false );
	if( isNullSocket(s) )
	{
		return ;
	}


	struct sockaddr sa = {0};
	int flen = sizeof(sa);

	socketRsArg *sra = (socketRsArg*)arg;

	//while( 1 )  lua ��֧�ֶ��߳�
	{
		SOCKET ns = accept( s.s,&sa,&flen );
		if( INVALID_SOCKET == ns )
		{
			goto tcp_listen_end;
		}	

		tcpThrArg *tta = new tcpThrArg;
		tta->s   = ns;
		tta->fn  = fn;
		tta->arg = new socketRsArg;

		*(socketRsArg*)(tta->arg) = *sra;

		::_beginthread( tcpThrFun,0,tta );
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
	if( isNullSocket(s ) )
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

	int ret = sendto( s.s,snd.c_str(),snd.size(),0,(sockaddr*)&sin,sizeof(sin) );

	if( SOCKET_ERROR == ret )
	{
		releaseSocket( ci );

		return false;
	}

	if( buf && len )
	{
		struct sockaddr sa={0};
		int len = sizeof(sa);

		ret = recvfrom( s.s,buf,len,0,&sa,&len );
		if( SOCKET_ERROR == ret )
		{
			releaseSocket(ci);

			return false;
		}

		len = ret;
	}

	return true;
}


bool tcp_send_wait( const char *sip,unsigned port,const std::string &snd,char *buf,int &len )
{
	socket_info_s &s = getSocket( sip,port,true,true );
	if( isNullSocket(s ) )
	{
		return false;
	}

	int ret = send( s.s,snd.c_str(),snd.size(),0 );

	if( SOCKET_ERROR == ret )
	{
		releaseSocket(sip,port);

		return false;
	}

	if( buf && len )
	{
		ret = recv( s.s,buf,len,0 );
		if( SOCKET_ERROR == ret )
		{
			releaseSocket(sip,port);

			return false;
		}

		len = ret;
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
	
	if( isNullSocket(s) )
	{
		return false;
	}
	
	struct sockaddr sa = {0};
	int slen = sizeof(sa);

	const int len = 8*1024;
	char *buf = new char[len+1];

	int ret = recvfrom( s.s,buf,len,0,(sockaddr*)&sa,&slen );
	if( SOCKET_ERROR == ret )
	{
		releaseSocket(conn);
		return false;
	}

	rcv = "";
	rcv.append( buf,ret );

	return true;
}

bool tcp_rcv( const std::string &conn,std::string &rcv )
{
	socket_info_s &s = getSocket( conn,false,true );
	
	if( isNullSocket(s) )
	{
		return false;
	}
	
	const int len = 8*1024;
	char *buf = new char[len+1];

	int ret = recv( s.s,buf,len,0 );
	if( SOCKET_ERROR == ret )
	{
		releaseSocket(conn);
		return false;
	}

	rcv = "";
	rcv.append( buf,ret );

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

		if( LUA_TFUNCTION != ret )  //  ���������ڷ��� 0
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

EBADF ����s�ǺϷ���socket�������
EFAULT ��������һָ��ָ���޷���ȡ���ڴ�ռ䡣
ENOTSOCK ����sΪһ�ļ������ʣ���socket��
EINTR ���ź����жϡ�
EAGAIN �˶������������ϣ�������s��socketΪ������ϡ�
ENOBUFS ϵͳ�Ļ����ڴ治��
ENOMEM �����ڴ治��
EINVAL ����ϵͳ���õĲ�������ȷ��

  ����ԭ�ͣ�
>>  int socket(int domain, int type, int protocol);
  ����˵����
����
  domain��Э�����ֳ�Э���壨family�������õ�Э������AF_INET��AF_INET6��AF_LOCAL�����AF_UNIX��Unix��Socket����AF_ROUTE�ȡ�Э���������socket�ĵ�ַ���ͣ���ͨ���б�����ö�Ӧ�ĵ�ַ����AF_INET������Ҫ��ipv4��ַ��32λ�ģ���˿ںţ�16λ�ģ�����ϡ�AF_UNIX������Ҫ��һ������·������Ϊ��ַ��
  type��ָ��Socket���͡����õ�socket������SOCK_STREAM��SOCK_DGRAM��SOCK_RAW��SOCK_PACKET��SOCK_SEQPACKET�ȡ���ʽSocket��SOCK_STREAM����һ���������ӵ�Socket��������������ӵ�TCP����Ӧ�á����ݱ�ʽSocket��SOCK_DGRAM����һ�������ӵ�Socket����Ӧ�������ӵ�UDP����Ӧ�á�
  protocol��ָ��Э�顣����Э����IPPROTO_TCP��IPPROTO_UDP��IPPROTO_SCTP��IPPROTO_TIPC�ȣ��ֱ��ӦTCP����Э�顢UDP����Э�顢STCP����Э�顢TIPC����Э�顣
  ע�⣺type��protocol������������ϣ���SOCK_STREAM�����Ը�IPPROTO_UDP��ϡ�������������Ϊ0ʱ�����Զ�ѡ��ڶ����������Ͷ�Ӧ��Ĭ��Э�顣
  ����ֵ��
  ������óɹ��ͷ����´������׽��ֵ������������ʧ�ܾͷ���INVALID_SOCKET��Linux��ʧ�ܷ���-1�����׽�����������һ���������͵�ֵ��ÿ�����̵Ľ��̿ռ��ﶼ��һ���׽������������ñ��д�����׽������������׽������ݽṹ�Ķ�Ӧ��ϵ���ñ�����һ���ֶδ���´������׽��ֵ�����������һ���ֶδ���׽������ݽṹ�ĵ�ַ����˸����׽����������Ϳ����ҵ����Ӧ���׽������ݽṹ��ÿ���������Լ��Ľ��̿ռ��ﶼ��һ���׽��������������׽������ݽṹ�����ڲ���ϵͳ���ں˻����
  ��

  ����ԭ�ͣ�
>>  int bind(SOCKET socket, const struct sockaddr* address, socklen_t address_len);
  ����˵����
  socket����һ���׽�����������
  address����һ��sockaddr�ṹָ�룬�ýṹ�а�����Ҫ��ϵĵ�ַ�Ͷ˿ںš�
  address_len��ȷ��address�������ĳ��ȡ�
  ����ֵ��
  �������ִ�гɹ�������ֵΪ0������ΪSOCKET_ERROR��
  ����

  ����ԭ�ͣ�
>>  int recv(SOCKET socket, char FAR* buf, int len, int flags);
  ����˵����
  ����
	socket��һ����ʶ�������׽ӿڵ������֡�
	buf�����ڽ������ݵĻ�������
	len�����������ȡ�
	flags��ָ�����÷�ʽ��ȡֵ��MSG_PEEK �鿴��ǰ���ݣ����ݽ������Ƶ��������У������������������ɾ����MSG_OOB ����������ݡ�
	����ֵ��
	���޴�������recv()���ض�����ֽ����������������ֹ������0������Ļ�������SOCKET_ERROR����Ӧ�ó����ͨ��WSAGetLastError()��ȡ��Ӧ������롣
	
	����ԭ�ͣ�
>>	ssize_t recvfrom(int sockfd, void buf, int len, unsigned int flags, struct socketaddr* from, socket_t* fromlen);
	����˵����
	sockfd����ʶһ���������׽ӿڵ������֡�
	buf���������ݻ�������
	len�����������ȡ�
	flags�����ò�����ʽ��������һ�����߶����־������壬��ͨ��or��������һ��
	��1��MSG_DONTWAIT���������ᱻ������
	��2��MSG_ERRQUEUE�� ָʾӦ�ô��׽��ֵĴ�������Ͻ��մ���ֵ�����ݲ�ͬ��Э�飬����ֵ��ĳ�ָ�������Ϣ�ķ�ʽ���ݽ�����ʹ����Ӧ���ṩ�㹻��Ļ����������´����ԭ���ͨ��msg_iovec��Ϊһ������������ݡ����´�������ݱ�ԭĿ���ַ��Ϊmsg_name���ṩ��������sock_extended_err�ṹ��̬��ʹ�á�
	��3��MSG_PEEK��ָʾ���ݽ��պ��ڽ��ն����б���ԭ���ݣ�������ɾ�������Ķ����������Խ�����ͬ�����ݡ�
	��4��MSG_TRUNC�����ط����ʵ�ʳ��ȣ���ʹ�������ṩ�Ļ����������� ֻ��packet�׽�����Ч��
	��5��MSG_WAITALL��Ҫ������������ֱ������õ����������㡣Ȼ���������׽���źţ�����������ӶϿ������������´α����յ��������Ͳ�ͬ���Ի᷵�����������������ݡ�
	��6��MSG_EOR��ָʾ��¼�Ľ��������ص��������һ����¼��
	��7��MSG_TRUNC��ָ�����ݱ�β�������ѱ���������Ϊ�������ṩ�Ļ�������Ҫ����Ŀռ䡣
	��8��MSG_CTRUNC��ָ�����ڻ������ռ䲻�㣬һЩ���������ѱ�������
	��9��MSG_OOB��ָʾ���յ�out-of-band����(����Ҫ���ȴ��������)��
	��10��MSG_ERRQUEUE��ָʾ���������׽��ִ�����еĴ����⣬û�н��յ��������ݡ�
	from������ѡ��ָ�룬ָ��װ��Դ��ַ�Ļ�������
	fromlen������ѡ��ָ�룬ָ��from����������ֵ��
	����

	����ԭ�ͣ�
>>	int sendto( SOCKET s, const char FAR* buf, int size, int flags, const struct sockaddr FAR* to, int tolen);
	����˵����
	s���׽���
	buf�����������ݵĻ�����
	size������������
	flags�����÷�ʽ��־λ, һ��Ϊ0, �ı�Flags������ı�Sendto���͵���ʽ
	addr������ѡ��ָ�룬ָ��Ŀ���׽��ֵĵ�ַ
	tolen��addr��ָ��ַ�ĳ���
	����ֵ��
	����ɹ����򷵻ط��͵��ֽ�����ʧ���򷵻�SOCKET_ERROR��
	����ԭ�ͣ�
	int accept( int fd, struct socketaddr* addr, socklen_t* len);
	����˵����
	fd���׽�����������
	addr�����������ŵĵ�ַ
	len�����շ��ص�ַ�Ļ���������
	����ֵ��
	�ɹ����ؿͻ��˵��ļ���������ʧ�ܷ���-1��
*/