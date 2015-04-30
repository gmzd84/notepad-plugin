
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <io.h>
#include <string>

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


void udp_listen(const char *sip,unsigned short port,void (*fn)(SOCKET s,void*),void*arg )
{
	if( !port || !fn )
	{
		return;
	}

	unsigned ip = htonl(0x7f000001);
	if( sip )
	{
		ip = ::inet_addr(sip);

		if( INADDR_NONE == ip )
		{
			return;
		}
	}


	SOCKET s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );

	if( INVALID_SOCKET == s )
	{
		return ;
	}	

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr   = ip;
	sin.sin_port = htons( port );

	int ret = bind( s,(sockaddr*)&sin,sizeof(sin) );
	if( SOCKET_ERROR == ret )
	{
		closesocket(s);
		return;
	}

	fn( s,arg );

	closesocket(s);

	return;
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

	unsigned ip = htonl(0x7f000001);
	if( sip )
	{
		ip = ::inet_addr(sip);

		if( INADDR_NONE == ip )
		{
			return;
		}
	}


	SOCKET s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );

	if( INVALID_SOCKET == s )
	{
		return ;
	}	

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr   = ip;
	sin.sin_port = htons( port );

	int ret = bind( s,(sockaddr*)&sin,sizeof(sin) );
	if( SOCKET_ERROR == ret )
	{
		closesocket(s);
		return;
	}

	ret = listen( s,0 );
	if( SOCKET_ERROR == ret )
	{
		closesocket(s);
		return;
	}

	struct sockaddr sa = {0};
	int flen = sizeof(sa);

	while( 1 )
	{
		SOCKET ns = accept( s,&sa,&flen );
		if( INVALID_SOCKET == s )
		{
			break;
		}	

		tcpThrArg *tta = new tcpThrArg;
		tta->s   = ns;
		tta->fn  = fn;
		tta->arg = arg;

		::_beginthread( tcpThrFun,0,tta );
	}

	closesocket(s);
}

struct socketRsArg
{
	bool (*fn)(const std::string &rcv,std::string &snd ,void *arg); // 返回 false 表示停止处理
	void *arg;
};
void udp_rs( SOCKET s,void *arg )
{
	const int len = 8*1024;
	char *buf = new char[len+1];

	struct sockaddr sa = {0};
	int slen = sizeof(sa);

	socketRsArg *ura = (socketRsArg*)arg;

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
		bool goon = ura->fn( rcv,snd,ura->arg );

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
	delete ura;
}

void tcp_rs( SOCKET s,void *arg )
{
	const int len = 8*1024;
	char *buf = new char[len+1];
	
	socketRsArg *ura = (socketRsArg*)arg;

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
		bool goon = ura->fn( rcv,snd,ura->arg );

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
	delete ura;
}


bool udp_send_wait( SOCKET &s,const char *sip,unsigned port,const std::string &snd,char *buf,int &len )
{
	unsigned ip = htonl(0x7f000001);

	if( INVALID_SOCKET == s )
	{
		if( sip )
		{
			ip = ::inet_addr(sip);

			if( INADDR_NONE == ip )
			{
				return false;
			}
		}


		s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );

		if( INVALID_SOCKET == s )
		{
			return false;
		}	
	}

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr   = ip;
	sin.sin_port = htons( port );

	int ret = sendto( s,snd.c_str(),snd.size(),0,(sockaddr*)&sin,sizeof(sin) );

	if( SOCKET_ERROR == ret )
	{
		closesocket(s);
		s = INVALID_SOCKET;

		return false;
	}

	if( buf && len )
	{
		struct sockaddr sa={0};
		int len = sizeof(sa);

		ret = recvfrom( s,buf,len,0,&sa,&len );
		if( SOCKET_ERROR == ret )
		{
			closesocket(s);
			s = INVALID_SOCKET;

			return false;
		}

		len = ret;
	}

	return true;
}


bool tcp_send_wait( SOCKET &s,const char *sip,unsigned port,const std::string &snd,char *buf,int &len )
{
	unsigned ip = htonl(0x7f000001);

	if( INVALID_SOCKET == s )
	{
		if( sip )
		{
			ip = ::inet_addr(sip);

			if( INADDR_NONE == ip )
			{
				return false;
			}
		}


		s = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP );

		if( INVALID_SOCKET == s )
		{
			return false;
		}

		struct sockaddr_in sin = {0};
		sin.sin_family = AF_INET;
		sin.sin_addr.S_un.S_addr   = ip;
		sin.sin_port = htons( port );

		int ret = ::connect(s,(sockaddr*)&sin,sizeof(sin));
		if( SOCKET_ERROR == ret )
		{
			closesocket(s);
			return false;
		}
	}

	int ret = send( s,snd.c_str(),snd.size(),0 );

	if( SOCKET_ERROR == ret )
	{
		closesocket(s);
		s = INVALID_SOCKET;

		return false;
	}

	if( buf && len )
	{
		ret = recv( s,buf,len,0 );
		if( SOCKET_ERROR == ret )
		{
			closesocket(s);
			s = INVALID_SOCKET;

			return false;
		}

		len = ret;
	}

	return true;
}
extern "C"
{
	void closeSocket(int s)
	{
		SOCKET ss = s;

		::closesocket(ss);
	}

	unsigned udp_send_rcv( unsigned p,const std::string & ip,unsigned short port,const std::string &snd, std::string &rcv )
	{
		int len = 16*1024;
		char *buf = new char[len+1];

		SOCKET s = p;

		bool r = udp_send_wait( s,ip.c_str(),port,snd,buf,len );
		if( r )
		{
			rcv = "";
			rcv.append(buf,len);

			return s;
		}

		return (unsigned)-1;
	}

	unsigned tcp_send_rcv( unsigned p,const std::string & ip,unsigned short port,const std::string &snd, std::string &rcv )
	{
		int len = 16*1024;
		char *buf = new char[len+1];

		SOCKET s = p;

		bool r = tcp_send_wait( s,ip.c_str(),port,snd,buf,len );
		if( r )
		{
			rcv = "";
			rcv.append(buf,len);

			return s;
		}

		return (unsigned)-1;
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