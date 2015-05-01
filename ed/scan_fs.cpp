
#include <io.h>
#include <direct.h>

#include <string>

#include "lua/lua.hpp"

#include "stdafx.h"

struct find_info_s
{
	intptr_t hdl;
	std::string cur_dir;
};

void scan_dir( const std::string &dirname,bool recursive ,bool (*fn)(const std::string &n,unsigned s,unsigned t,void *),void *arg )
{
	_finddata_t data = {0};

	std::string path = dirname;
	path += "\\";
	path += "*.*";

	intptr_t hdl = ::_findfirst( path.c_str(),&data );

	if( hdl <= 0 )
	{
		::MessageBoxA(NULL,path.c_str(),"Error",MB_OK );
		return;
	}

	std::vector<find_info_s> hdls;
	hdls.push_back( find_info_s() );
	hdls.back().hdl = hdl;
	hdls.back().cur_dir = dirname;

	do
	{
		std::string p = hdls.back().cur_dir ;
		p += "\\";
		p += data.name;

		if( NULL == fn || !fn( p,data.size,data.attrib,arg ) )
		{
			for( auto i=hdls.begin();i!=hdls.end();++i )
			{
				_findclose(i->hdl); 
			}

			hdls.erase(hdls.begin(),hdls.end());

			break;
		}
		
		if( recursive 
			&& 16 == data.attrib 
			&& 0 != strcmp( ".",data.name )
			&& 0 != strcmp( "..",data.name )
			)
		{
			// 目录
			find_info_s t;
			t.cur_dir = p;

			p += "\\";
			p += "*.*";
			t.hdl = ::_findfirst( p.c_str(),&data );
			
			hdls.push_back( t );
		}
		else
		{
			memset( &data,0,sizeof(data) );

			if( _findnext(hdls.back().hdl,&data) == 0 ) 
			{
				// 成功
			}
			else
			{
				// 失败，表示此目录已经查找完毕
				_findclose(hdls.back().hdl); 

				hdls.pop_back();
				if( hdls.empty() )
				{
					break;
				}
			}
		}
	}while( 1 );
}

extern "C"
{
	struct scan_info_s
	{
		lua_State *L;
		std::string lua_fn;
	};

	static bool call_lun_fun(const std::string &path,unsigned s,unsigned t,void *arg )
	{
		scan_info_s *pmon = (scan_info_s*)arg;

		int ret = lua_getglobal( pmon->L,pmon->lua_fn.c_str() );

		if( LUA_TFUNCTION != ret )  // LUA_ERRERR
		{
			std::string err = "'";
			err += pmon->lua_fn;
			err += "' is not a function name;";
			
			::MessageBoxA(NULL,err.c_str(),"lua error",MB_OK);
			return false;
		}

		lua_pushlstring( pmon->L,path.c_str(),path.size() );
		lua_pushinteger( pmon->L,s );
		lua_pushinteger( pmon->L,t );

		ret = lua_pcall( pmon->L,3,1,0 );
		if( LUA_OK != ret )
		{
			std::string es = lua_tostring( pmon->L,-1 );

			lua_pop(pmon->L,1);

			::MessageBoxA(NULL,es.c_str(),"lua error",MB_OK);
			return false;
		}

		int r = lua_toboolean( pmon->L,-1 );
		lua_pop(pmon->L,1 );

		return r != 0;
	}


	

	int lua_scandir( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 3 || !lua_isboolean(L,-1) || !lua_isstring(L,-2) || !lua_isstring(L,-3) )
		{
			lua_pushliteral(L,"scanDir( dirname,fname,recursive );");
			return lua_error(L);
		}

		std::string dirname = lua_tostring(L,-3);
		std::string funname = lua_tostring(L,-2);
		bool resursive =( (lua_toboolean( L,-1 )) != 0 );
		lua_pop( L,3 );	


		scan_info_s *si = new scan_info_s;

		si->L = L;
		si->lua_fn = funname;

		scan_dir( dirname,resursive,call_lun_fun,si );

		delete si;

		return 0;
	}

	void scan_reg( lua_State *L )
	{
		lua_register( L,"scanDir",lua_scandir );
	}
}