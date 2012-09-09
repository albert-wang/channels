#include "channel.hpp"



int main(int argc, char * argv[])
{
	using namespace Engine::Threading;

	spChannel channel = Channel::create();

	ChannelWait wait(channel);
	wait.setWait(Message(13, 1000000 - 1), SIGNIFICANCE_INTEGRAL | SIGNIFICANCE_TYPE);

	boost::thread subthread([channel]()
	{
		for (size_t i = 0; i < 1000000; ++i)
		{
			channel->push(13, i, std::string("Hello, World!"));
		}
	});

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	wait.wait();

	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	std::cout << (end.QuadPart - start.QuadPart) / static_cast<double>(freq.QuadPart) << "\n";

	process(channel, [](const Message& msg) 
	{
		//std::cout << msg.integralValue << " " << msg.arbitraryParam<std::string>() << "\n";
	});
}
