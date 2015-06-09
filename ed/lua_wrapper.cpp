
#include <string>
#include <map>

#include "stdafx.h"

#include "lua/lua.hpp"


std::map<std::string,std::string> g_luaFunDesc;

void lua_reg_fun( lua_State *L,const char * fn,int (*pfn)( lua_State * ),const char *desc )
{
	lua_register( L,fn,pfn );

	g_luaFunDesc[fn] = desc;
}

std::string lua_help()
{
	std::string ret;

	for( auto i=g_luaFunDesc.begin();i!=g_luaFunDesc.end();++i )
	{
		ret += i->first;
		ret += ": ";
		ret += i->second;
		ret += "\r\n";
	}


	return ret;
}

extern "C"
{
	int lua_getText( lua_State *L )
	{
		std::string s;
		getText(s);

		lua_pushlstring(L,s.c_str(),s.size());
		return 1;
	}
	int lua_getSelText( lua_State *L )
	{
		std::string s;
		getSelText(s);

		lua_pushlstring(L,s.c_str(),s.size());
		return 1;
	}

	int lua_setText( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call setText.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call setText.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		setText( s );
		return 0;
	}

	int lua_appendText( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call appendText.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call appendText.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		appendText( s );
		return 0;
	}

	int lua_replaceSel( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call replaceSel.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call replaceSel.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		replaceCurSel( s );
		return 0;
	}

	int lua_msg( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		showInfo( "lua message", s,lua_help() );
		return 0;
	}

	int lua_msg2( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		::MessageBoxA(NULL,s.c_str(),"lua message",MB_OK);
		return 0;
	}

	int lua_confirm( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		int sel = ::MessageBoxA(NULL,s.c_str(),"lua message",MB_YESNO);

		lua_pushinteger(L,sel);
		return 1;
	}

	int lua_prompt( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n < 1 )
		{
			lua_pushliteral(L,"no argument when to call prompt.");
			return lua_error(L);
		}

		if( !lua_isstring(L,-1) )
		{
			lua_pushliteral(L,"the argument is not string when to call prompt.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,-1);
		lua_pop( L,1 );

		std::string ii;
		extractInput(s,ii);

		lua_pushlstring(L,ii.c_str(),ii.size());
		return 1;
	}
	
	extern void popen_reg(lua_State *L );
	extern void tcp_udp_reg(lua_State *L );
	extern void scan_reg( lua_State *L );

	void regLuaFun(lua_State *L)
	{
		lua_reg_fun( L,"getText",lua_getText,"string getText()" );
		lua_reg_fun( L,"getSelText",lua_getSelText,"string getSelText()" );
		lua_reg_fun( L,"setText",lua_setText,"void setText(string)" );
		lua_reg_fun( L,"appendText",lua_appendText,"void appendText(string)" );
		lua_reg_fun( L,"replaceSel",lua_replaceSel,"void replaceSel(string)" );

		lua_reg_fun( L,"msg",lua_msg,"void msg(string)" );
		lua_reg_fun( L,"msg2",lua_msg2,"void msg2(string)" );
		lua_reg_fun( L,"confirm",lua_confirm,"int confirm(string)" );
		lua_reg_fun( L,"prompt",lua_prompt,"string prompt(string)" );

		scan_reg( L );
		popen_reg( L );
		tcp_udp_reg( L );
	}
	void execLua( const std::string& lua_script )
	{
		lua_State *L = luaL_newstate();

		if( !L )
		{
			return;
		}

		luaL_openlibs(L);

		regLuaFun( L );
		
		int ret = ( luaL_loadbuffer( L,lua_script.c_str(),lua_script.size(),"luaScript" )
			|| lua_pcall(L, 0, LUA_MULTRET, 0));

		if( LUA_OK != ret )
		{
			std::string es = lua_tostring( L,-1 );

			lua_pop(L,1);

			showInfo("lua error",es,lua_help());
		}


		lua_close(L);
	}


	void lua_run()
	{
		std::string s;
		getText(s);
		execLua(s);
	}

	void lua_script()
	{
		getInput( "lua script",execLua );
	}
};