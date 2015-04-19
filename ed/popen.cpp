
#include <process.h>
#include <io.h>

#include <string>


struct popenOutProc
{
	virtual void begin() {}
	virtual void exec(const std::string &line) = 0;
	virtual void end() {}

	virtual ~popenOutProc(){}
};

static void callPopen(const std::string &cmd,popenOutProc *proc)
{
	FILE *pf = ::_popen(cmd.c_str(),"r");

	if( !proc )
	{
		pf && _pclose(pf);

		return;
	}

	if(pf)
	{
		std::string str;
		str.reserve( 1*1024 );
		
		proc->begin();

		while( !feof(pf) )
		{
			char buf[2];
			buf[0] = 0;
			buf[1] = 0;

			fread( buf,1,1,pf );

			str += buf;

			if( buf[0] == '\n'|| str.size()+1 >= str.capacity() )
			{
				proc->exec( str );
				
				str = "";
			}
		}

		proc->exec( str );

		proc->end();

		_pclose(pf);
	}
}

struct procOnceOut : popenOutProc
{
	std::string allOut;

	void (*fn)(const std::string &line);

	procOnceOut(void (*f)(const std::string &line))
		:fn(f)
	{

	}

	virtual void exec(const std::string &line) 
	{
		allOut += line;

		if(line.size())
		{
			if(line[line.size()-1] == '\n' 
				&& (line.size() == 1 || line[line.size()-2] != '\r')
				)
			{
				allOut[allOut.size()-1] = '\r';
				allOut += "\n";
			}
		}
		
	}

	virtual void end() 
	{
		if( fn )
		{
			fn(allOut);
		}
	}
};


struct cmdOutMonitor : popenOutProc
{
	void (*fn)(const std::string &line,void*);
	void *arg;

	cmdOutMonitor(void (*f)(const std::string &line,void*),void*a)
		:fn(f),arg(a)
	{

	}

	virtual void exec(const std::string &line) 
	{
		if(fn)
		{
			fn(line,arg);
		}		
	}
};

extern "C"
{
	void execCmd( const std::string &cmd )
	{
		callPopen( cmd,NULL );
	}

	static void execCmdThr( void *arg )
	{
		std::string *cmd = (std::string *)arg;

		callPopen( *cmd,NULL );
		delete cmd;
	}
	void execCmdAsync( const std::string &cmd )
	{
		std::string *nc = new std::string(cmd);

		::_beginthread( execCmdThr,0,nc );
	}

	void getCmdOut( const std::string &cmd,std::string &s )
	{
		procOnceOut poo( NULL );

		callPopen( cmd,&poo );

		s = poo.allOut;
	}

	struct monitorArg
	{
		std::string cmd;

		void (*monitor)(const std::string &line,void *);
		void *arg;
	};

	static void monitorThr( void *d )
	{
		monitorArg *ma = (monitorArg*)d;
		
		cmdOutMonitor m(ma->monitor,ma->arg);

		callPopen( ma->cmd,&m );

		delete ma;
	}

	void monitorCmdOut( const std::string &cmd,void (*monitor)(const std::string &line,void *),void *arg)
	{
		monitorArg *ma = new monitorArg;

		ma->cmd = cmd;
		ma->monitor = monitor;
		ma->arg = arg;

		::_beginthread(monitorThr,0,ma);
	}
};