

#include "stdafx.h"

#include "json/json.h"

extern "C"
{
	void json_format()
	{
		std::string t;
		getCurText(t);

		Json::Reader r;

		Json::Value v;

		if( !r.parse( t,v,true ) )
		{
			::MessageBoxA(NULL,"this is not json string","error",MB_OK);
			return;
		}

		Json::StyledWriter w;
		std::string s = w.write(v);

		setCurText(s);
	}
};
