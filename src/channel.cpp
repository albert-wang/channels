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
		boost::intrusive_ptr<Channel> Channel::create() 
		{
			Channel * ch = new Channel();
			return boost::intrusive_ptr<Channel>(ch);
		}

		Channel::Channel()
			:referenceCount(0)
		{}

		bool Channel::pop(Message * msg, Message * prototype, size_t sig)
		{
			boost::mutex::scoped_lock lock(mutex);
			if (messages.empty())
			{
				msg->type = NULL_MESSAGE;
				return false;
			}

			if (prototype)
			{

			} 
			else 
			{
				*msg = messages.front();
				messages.pop_front();
			}
		}

		void Channel::emplace(Message&& msg)
		{
			messages.push_back(msg);
		}
	}
}
