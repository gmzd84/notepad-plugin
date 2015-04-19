
#include <io.h>
#include <direct.h>

#include <string>

void scan_dir( const std::string &dirname,bool (*fn)(const std::string &n,unsigned s,unsigned t,void *),void *arg )
{
	_finddata_t data = {0};

	intptr_t hdl = ::_findfirst( dirname.c_str(),&data );

	if( hdl <= 0 )
	{
		return;
	}


	do
	{
		if( !fn( data.name,data.size,data.attrib,arg ) )
		{
			break;
		}
	}while( _findnext(hdl,&data)==0 );

	_findclose(hdl);
}