#include "channel.hpp"

namespace Engine
{
	//Receving Channel
	namespace Threading
	{
		Message ReceivingChannel::pop()
		{
			Message result;
			if (pop(&result))
			{
				return result;
			}

			//Should return a proxy
			return result;
		}
	}

	//Channel
	namespace Threading
	{
		boost::intrusive_ptr<Channel> create() 
		{
			Channel * ch = new Channel();
			return boost::intrusive_ptr<Channel>(ch);
		}

		Channel::Channel()
			:referenceCount(0)
		{}

		bool Channel::pop(Message * msg)
		{
			return false;
		}
	}
}
