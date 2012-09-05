#include "channel.hpp"



int main(int argc, char * argv[])
{
	using namespace Engine::Threading;

	spChannel channel = Channel::create();
	for (size_t i = 0; i < 10; ++i)
	{
		channel->push(MSG(13, i, std::string("Hello, World!")));
	}

	for (size_t i = 0; i < 10; ++i)
	{
		Message msg = channel->pop();
		std::cout << msg.integralValue << " " << msg.arbitraryParam<std::string>() << "\n";
	}
}


