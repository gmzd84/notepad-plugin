

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

#include "stdafx.h"

std::string uint_to_str( unsigned long long d )
{
	std::ostringstream os;
	os<<d;
	return os.str();
}


std::string int_to_str( long long d )
{
	std::ostringstream os;
	os<<d;
	return os.str();
}

void str_break( const std::string &s,const std::string &b,std::string &part1,std::string &part2 )
{
	std::string::size_type p = s.find(b);

	if ( p == std::string::npos )
	{
		part1 = s;
		part2 = "";
	}
	else
	{
		part1 = s.substr(0,p);
		part2 = s.substr(p+b.size());
	}
}

void split( const std::string &s,const std::string &delms,std::vector<std::string> &parts )
{
	std::string::size_type b=0,p;

	p = s.find(delms,b);

	while( p != std::string::npos )
	{
		parts.push_back( s.substr(b,p-b) );

		b = p+delms.size();

		p = s.find(delms,b);
	}

	if( b < s.size() )
	{
		parts.push_back( s.substr(b) );
	}	
}

void split2( const std::string &s,const std::string &delms,std::vector<std::string> &parts )
{
	std::string::size_type b=0,p;

	p = s.find_first_of(delms,b);

	while( p != std::string::npos )
	{
		parts.push_back( s.substr(b,p-b) );

		b = s.find_first_not_of(delms,p);

		if( b == std::string::npos )
		{
			break;
		}

		p = s.find_first_of(delms,b);
	}

	if( b != std::string::npos )
	{
		parts.push_back( s.substr(b) );
	}	
}

void trim( std::string &s )
{
	std::string::size_type p1,p2;

	p1 = s.find_first_not_of(" \t");
	p2 = s.find_last_not_of(" \t");

	if( p1 == std::string::npos )
	{
		return;
	}

	s = s.substr( p1,p2-p1+1 );
}

void replace( std::string &s,const std::string &src,const std::string &dst )
{
	std::string::size_type p = s.find( src );

	if( p != std::string::npos )
	{
		std::ostringstream os;

		os<< s.substr( 0,p );
		os<< dst;
		os<< s.substr( p+src.size() );

		s = os.str();
	}
}

void replace_all( std::string &s,const std::string &src,const std::string &dst )
{
	std::ostringstream os;

	std::string::size_type b = 0;

	std::string::size_type p = s.find( src,b );
	
	while( p != std::string::npos )
	{
		os<< s.substr( b,p );
		os<< dst;
	
		b = p + src.size();
		p = s.find( src,b );
	}

	os<<s.substr(b);

	s = os.str();
}

void replace_set( std::string &s ,std::map<char,std::string> &ss )
{
	std::ostringstream os;

	std::string::size_type i = 0;

	std::string::size_type n = s.size();

	for( ;i<n;++i )
	{
		auto ii = ss.find( s[i] );

		if( ii != ss.end() )
		{
			os<< ii->second;
		}
		else
		{
			os<< s[i];
		}
	}

	s = os.str();
}

void unique_sort( std::string & s,bool needsort )
{
	std::vector<std::string> parts;

	split2( s,"\r\n",parts );

	std::vector<std::string> p2;

	for( auto it=parts.begin();it!=parts.end();++it )
	{
		auto ii = std::find( p2.begin(),p2.end(),*it );
		if( ii == p2.end() )
		{
			p2.push_back(*it);
		}
	}

	if( needsort )
	{
		std::sort( p2.begin(),p2.end() );
	}	

	std::ostringstream os;

	for( auto i3 = p2.begin();i3 != p2.end();++i3 )
	{
		if( i3 != p2.begin() )
		{
			os<<"\r\n";
		}
		os<<*i3;
	}

	s = os.str();
}

void sort( std::string & s )
{
	std::vector<std::string> parts;

	split2( s,"\r\n",parts );
	
	std::sort( parts.begin(),parts.end() );

	std::ostringstream os;

	for( auto i3 = parts.begin();i3 != parts.end();++i3 )
	{
		if( i3 != parts.begin() )
		{
			os<<"\r\n";
		}
		os<<*i3;
	}

	s = os.str();
}

extern "C"
{
	__declspec(dllexport) void unique_text()
	{
		std::string t;
		getCurText(t);

		unique_sort( t,true );

		setCurText(t);
	}

	void sort_text()
	{
		std::string t;
		getCurText(t);

		sort( t );

		setCurText(t);
	}

	void html_encode()
	{
		std::string t;
		getCurText(t);

		std::map<char,std::string> cm;
		cm['<'] = "&lt;";
		cm['>'] = "&gt;";
		cm['('] = "&#40;";
		cm[')'] = "&#41;";
		cm['&'] = "&amp;";

		replace_set( t,cm );

		setCurText(t);
	}
}