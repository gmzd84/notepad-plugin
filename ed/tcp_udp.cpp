
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
	bool (*fn)(const std::string &rcv,std::string &snd ,void *arg); // ���� false ��ʾֹͣ����
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