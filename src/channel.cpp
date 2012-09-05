#include "channel.hpp"

namespace Engine
{
	//Matching.
	namespace Threading
	{
		bool matches(const Message& target, const Message& prototype, size_t significance)
		{
			if ((significance & SIGNIFICANCE_TYPE) && prototype.type != target.type)
			{
				return false; 
			}

			if ((significance & SIGNIFICANCE_INTEGRAL) && prototype.integralValue != target.integralValue)
			{
				return false;
			}

			if ((significance & SIGNIFICANCE_STORAGE_TYPE) && prototype.storage.type() != target.storage.type())
			{
				return false;
			}

			return true;
		}
	}

	//Receiving Channel
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
				Message& attemptedMatch = messages.front();
				if (matches(attemptedMatch, *prototype, sig))
				{
					*msg = std::move(attemptedMatch);
					messages.pop_front();
					return true;
				}
				return false;
			}
			else 
			{
				*msg = std::move(messages.front());
				messages.pop_front();
				return true;
			}
		}

		void Channel::emplace(Message&& msg)
		{
			messages.emplace_back(msg);
		}
	}

	//Utilities
	namespace Threading
	{
		void schedule(SendingChannel * channel, size_t milliseconds)
		{
			
		}
	}
}
