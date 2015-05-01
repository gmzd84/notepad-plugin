

#include <string>


void udp_listen(const std::string &conn,bool (*fn)(const std::string &,std::string & ,void *),void*arg );

void tcp_listen(const std::string &conn,bool (*fn)(const std::string &,std::string & ,void *),void*arg );

bool udp_send_rcv( const std::string &conn,const std::string &snd, std::string &rcv );

bool tcp_send_rcv( const std::string &conn,const std::string &snd, std::string &rcv );

bool udp_send( const std::string &conn,const std::string &snd );

bool tcp_send( const std::string &conn,const std::string &snd );

bool udp_rcv( const std::string &conn,std::string &rcv );

bool tcp_rcv( const std::string &conn,std::string &rcv );

void closeConnect( const std::string &conn );
