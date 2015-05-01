
#include "stdafx.h"

#include <sstream>
#include <list>

#include "tinyxml/tinyxml.h"

extern "C"
{
	static void xml_normalize(char *buf,long length)
	{
		const char* p = buf;	// the read head
		char* q = buf;			// the write head
		const char CR = 0x0d;
		const char LF = 0x0a;

		buf[length] = 0;
		while( *p ) {
			assert( p < (buf+length) );
			assert( q <= (buf+length) );
			assert( q <= p );

			if ( *p == CR ) {
				*q++ = LF;
				p++;
				if ( *p == LF ) {		// check for CR+LF (and skip LF)
					p++;
				}
			}
			else {
				*q++ = *p++;
			}
		}
		assert( q <= (buf+length) );
		*q = 0;
	}

	struct ElementInfo
	{
		TiXmlNode *node;
		int cnt;
		int depth;

		ElementInfo( TiXmlNode *e)
			:node(e),cnt(0),depth(0)
		{

		}
	};
	void pre( ElementInfo &ei,std::ostringstream &os )
	{
		for( int i=0;i<ei.depth;i++ )
		{
			os<<"    ";
		}
	}
	static void procElement( ElementInfo &ei,std::ostringstream &os,std::list<ElementInfo> &eis )
	{
		++ei.cnt;


		TiXmlElement *ele = ei.node->ToElement();

		if( !ele )
		{
			return ;
		}

		const char*n = ele->Value();

		if(ei.cnt==2)
		{
			// ±ÕºÏ
			os<<"</"<<n<<">\r\n";
		}
		else
		{
			os<<"<"<<n;

			TiXmlAttribute *attr = ele->FirstAttribute();
			while( attr )
			{
				const char *k = attr->Name();
				const char *v = attr->Value();

				os<<" "<<k<<"=\""<<v<<"\"";

				attr = attr->Next();
			}

			TiXmlNode *node = ele->FirstChild();
			if( !ele )
			{
				os<<"/>\r\n";

				++ei.cnt;
			}
			else
			{
				os<<">\r\n";

				ElementInfo i(node);

				i.depth = ei.depth + 1;

				eis.push_back(i);
			}
		}
	}

	static void procDelcare( ElementInfo &ei,std::ostringstream &os,std::list<ElementInfo> &eis )
	{
		TiXmlDeclaration *decl = ei.node->ToDeclaration();
		if( !decl )
		{
			return;
		}

		pre(ei,os);

		os<<"<?xml";

		const char *p =  decl->Encoding();
		if( p && *p )
		{
			os<<" encoding=\""<<p<<"\"";
		}

		p =  decl->Version();
		if( p && *p )
		{
			os<<" version=\""<<p<<"\"";
		}

		p =  decl->Standalone();
		if( p && *p )
		{
			os<<" standalone=\""<<p<<"\"";
		}

		os<<"?>\r\n";

		
	}

	static void procText( ElementInfo &ei,std::ostringstream &os,std::list<ElementInfo> &eis )
	{
		TiXmlText *txt = ei.node->ToText();

		if( !txt )
		{
			return;
		}

		os<<txt->Value()<<"\r\n";
	}

	static void procComment( ElementInfo &ei,std::ostringstream &os,std::list<ElementInfo> &eis )
	{
		TiXmlComment *txt = ei.node->ToComment();

		if( !txt )
		{
			return;
		}

		os<<"<!--"<<txt->Value()<<"-->\r\n";
	}

	static void procNode( ElementInfo &ei,std::ostringstream &os,std::list<ElementInfo> &eis )
	{
		pre(ei,os);

		switch( ei.node->Type() )
		{
		case TiXmlNode::TINYXML_ELEMENT:
			procElement( ei,os,eis );

			if( eis.back().cnt >=2 )
			{
				TiXmlNode *node = ei.node->NextSibling();

				eis.pop_back();

				if(node)
				{
					ElementInfo i(node);
					i.depth = ei.depth;
					eis.push_back(i);
				}
			}
			break;
		case TiXmlNode::TINYXML_DECLARATION:
			procDelcare(ei,os,eis);

			
			{
				TiXmlNode *node = ei.node->NextSibling();

				eis.pop_back();
				if(node)
				{
					ElementInfo i(node);
					i.depth = ei.depth;
					eis.push_back(i);
				}
			}
			break;
		case TiXmlNode::TINYXML_TEXT:
			procText(ei,os,eis);

			
			{
				TiXmlNode *node = ei.node->NextSibling();

				eis.pop_back();
				if(node)
				{
					ElementInfo i(node);
					i.depth = ei.depth;
					eis.push_back(i);
				}
			}
			break;
		case TiXmlNode::TINYXML_COMMENT:
			procComment(ei,os,eis);

			
			{
				TiXmlNode *node = ei.node->NextSibling();

				eis.pop_back();

				if(node)
				{
					ElementInfo i(node);
					i.depth = ei.depth;
					eis.push_back(i);
				}
			}
			break;
		default:
			break;
		}

		
	}

	__declspec(dllexport) void xml_format()
	{
		std::string t; // ("<?xml encoding=\"UTF-8\"?><a b=\"c\"><e d=\"111\"/></a>")

		getCurText(t);

		xml_normalize( (char *)t.c_str(),t.size() );

		TiXmlDocument doc;
		doc.Parse( t.c_str() );
		if( doc.Error() )
		{
			std::ostringstream os;
			os<<"Error["<<doc.ErrorRow()<<","<<doc.ErrorCol()<<"] id:"<<doc.ErrorId()<<" desc:"<<doc.ErrorDesc();

			::MessageBoxA(NULL,os.str().c_str(),"Error",MB_OK);
			return;
		}

		std::ostringstream os;

		TiXmlHandle hdl = doc.FirstChild();

		TiXmlNode *n = hdl.Node();
		if( !n )
		{
			return;
		}
			

		int depth = 0;

		ElementInfo ei(n);

		std::list<ElementInfo> eis;

		eis.push_back(ei);

		while( eis.size() )
		{
			procNode(eis.back(),os,eis);
		}

		setCurText(os.str());
	}
};