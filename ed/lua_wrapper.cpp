
#include <string>

#include "stdafx.h"

#include "lua/lua.hpp"


const char *lua_help = 
	""
	;


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
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call setText.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call setText.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		setText( s );
		return 0;
	}

	int lua_appendText( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call appendText.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call appendText.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		appendText( s );
		return 0;
	}

	int lua_replaceSel( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call replaceSel.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call replaceSel.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		replaceCurSel( s );
		return 0;
	}

	int lua_msg( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		showInfo( "lua message", s,lua_help );
		return 0;
	}

	int lua_msg2( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		::MessageBoxA(NULL,s.c_str(),"lua message",MB_OK);
		return 0;
	}

	int lua_confirm( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call msg.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call msg.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		int sel = ::MessageBoxA(NULL,s.c_str(),"lua message",MB_YESNO);

		lua_pushinteger(L,sel);
		return 1;
	}

	int lua_prompt( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call prompt.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call prompt.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		std::string ii;
		extractInput(s,ii);

		lua_pushlstring(L,ii.c_str(),ii.size());
		return 1;
	}

	int lua_popen( lua_State *L )
	{
		int n = lua_gettop(L);
		if( n != 1 )
		{
			lua_pushliteral(L,"no argument when to call prompt.");
			return lua_error(L);
		}

		if( !lua_isstring(L,1) )
		{
			lua_pushliteral(L,"the argument is not string when to call prompt.");
			return lua_error(L);
		}

		std::string s = lua_tostring(L,1);

		std::string ii;
		getCmdOut(s,ii);

		lua_pushlstring(L,ii.c_str(),ii.size());
		return 1;
	}

	void regLuaFun(lua_State *L)
	{
		lua_register( L,"getText",lua_getText );
		lua_register( L,"getSelText",lua_getSelText );
		lua_register( L,"setText",lua_setText );
		lua_register( L,"appendText",lua_appendText );
		lua_register( L,"replaceSel",lua_replaceSel );

		lua_register( L,"msg",lua_msg );
		lua_register( L,"msg2",lua_msg2 );
		lua_register( L,"confirm",lua_confirm );
		lua_register( L,"prompt",lua_prompt );

		lua_register( L,"popen",lua_popen );
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

			showInfo("lua error",es,lua_help);
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