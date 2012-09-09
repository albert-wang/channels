#include "channel.hpp"



int main(int argc, char * argv[])
{
	using namespace Engine::Channels2;

	spChannel channel = Channel::create();

	ChannelWait wait(channel);
	wait.setWait(Message(7, 1000000 - 1), SIGNIFICANCE_INTEGRAL | SIGNIFICANCE_TYPE);

	for (size_t j = 0; j < 8; ++j)
	{
		boost::thread subthread([j, channel]()
		{
			for (size_t i = 0; i < 1000000; ++i)
			{
				channel->push(j, i, std::string("Hello, World!"));
			}
		});
	}

	for (size_t j = 0; j < 4; ++j)
	{
		boost::thread subthread([&wait, channel]()
		{
			while (!wait.done())
			{
				process(channel, [](const Message& msg) {});
			}
		});
	}

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	
	wait.wait();

	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	std::cout << (end.QuadPart - start.QuadPart) / static_cast<double>(freq.QuadPart) << "\n";

	
}
