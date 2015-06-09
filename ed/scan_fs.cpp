
#include <io.h>
#include <direct.h>

#include <string>
#include <list>

#include "lua/lua.hpp"

#include "stdafx.h"

#include <iostream>

struct find_info_s
{
	intptr_t hdl;
	std::string cur_dir;
};

struct scanDirOp
{
	virtual bool scan_file( const std::string &n,unsigned s,unsigned t ) = 0;

	virtual void enter_dir( const std::string &d ) {}
	virtual void leave_dir( const std::string &d ) {}
	virtual void error( const std::string &e ) {}
};

bool get_next_data( std::list<find_info_s> & hdls,_finddata_t &d,scanDirOp &op )
{
	while( hdls.size() )
	{
		find_info_s &fi = hdls.back();

		int r = _findnext( fi.hdl,&d );
		if( r < 0 )
		{
			op.leave_dir( fi.cur_dir );

			_findclose( fi.hdl );

			hdls.pop_back();
		}
		else
		{
			return true;
		}
	}

	return false;
}

void scan_dir_ext( const std::string &dirname,bool recursive, scanDirOp &op )
{
	_finddata_t data = {0};

	std::string path = dirname;
	path += "\\*";

	intptr_t hdl = ::_findfirst( path.c_str(),&data );
	if( hdl < 0 )
	{
		op.error("the path is not valid.");
		return;
	}

	std::list<find_info_s> hdls;
	hdls.push_back( find_info_s() );
	hdls.back().hdl     = hdl;
	hdls.back().cur_dir = dirname;

	op.enter_dir( dirname );

	while( !hdls.empty() )
	{
		find_info_s &fi = hdls.back();

		if( _A_SUBDIR == data.attrib )
		{
			if( std::string(".") == data.name 
				|| std::string("..") == data.name )
			{
				if( !get_next_data( hdls,data,op ) )
				{
					// 
					return;
				}
			}
			else if( recursive )
			{
				std::string tp = fi.cur_dir;
				tp += "\\";
				tp += data.name;

				path = tp;
				path += "\\*";

				hdl = ::_findfirst( path.c_str(),&data );
				if( hdl < 0 )
				{
					if( !get_next_data( hdls,data,op ) )
					{
						// 
						return;
					}
				}
				else
				{
					hdls.push_back( find_info_s() );
					hdls.back().hdl     = hdl;
					hdls.back().cur_dir = tp;

					op.enter_dir( tp );
				}
			}
			else
			{
				std::string p = fi.cur_dir;
				p += "\\";
				p += data.name;

				if( !op.scan_file(p,data.size,data.attrib) )
				{
					return;
				}

				if( !get_next_data( hdls,data,op ) )
				{
					// 
					return;
				}
			}
		}
		else
		{
			std::string p = fi.cur_dir;
			p += "\\";
			p += data.name;
				
			if( !op.scan_file(p,data.size,data.attrib) )
			{
				return;
			}

			if( !get_next_data( hdls,data,op ) )
			{
				// 
				return;
			}
		}
	}
}

struct luaScanDirOp : scanDirOp
{
	bool recursive ;
	bool (*fn)(const std::string &n,unsigned s,unsigned t,void *);
	void *arg;

	virtual bool scan_file( const std::string &n,unsigned s,unsigned t )
	{
		if( !fn )
		{
			return false;
		}
		return fn( n,s,t,arg );
	}
};
void scan_dir( const std::string &dirname,bool recursive ,bool (*fn)(const std::string &n,unsigned s,unsigned t,void *),void *arg )
{
	luaScanDirOp op;
	op.recursive = recursive;
	op.fn = fn;
	op.arg = arg;

	scan_dir_ext( dirname,recursive,op );
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