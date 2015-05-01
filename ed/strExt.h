
#include <string>
#include <vector>
#include <map>

std::string uint_to_str( unsigned long long d );
std::string int_to_str( long long d );
unsigned long long str_to_uint( const std::string &s );
long long str_to_int( const std::string &s );

void str_break( const std::string &s,const std::string &b,std::string &part1,std::string &part2 );
void split( const std::string &s,const std::string &delms,std::vector<std::string> &parts );
void split2( const std::string &s,const std::string &delms,std::vector<std::string> &parts );
void trim( std::string &s );
void replace( std::string &s,const std::string &src,const std::string &dst );
void replace_all( std::string &s,const std::string &src,const std::string &dst );
void replace_set( std::string &s ,std::map<char,std::string> &ss );
void unique_sort( std::string & s,bool needsort = false );
void sort( std::string & s );