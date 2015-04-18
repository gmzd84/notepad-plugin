

#include <string>
#include <sstream>

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