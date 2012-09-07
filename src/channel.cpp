#include "channel.hpp"
#include "timer.h"

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
		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds)
		{
			collection->addTimer(static_cast<double>(milliseconds) / 1000.0, [channel]()
			{
				spSendingChannel strongChannel = channel.lock();
				if (strongChannel)
				{
					FILETIME time;
					GetSystemTimeAsFileTime(&time);

					LARGE_INTEGER unioncast;
					unioncast.HighPart = time.dwHighDateTime;
					unioncast.LowPart = time.dwLowDateTime;
					strongChannel->push(TIMER_FINISHED, unioncast.QuadPart, 0);
				}
			});
		}

		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds, const Message& msg)
		{
			collection->addTimer(static_cast<double>(milliseconds) / 1000.0, [channel, msg]()
			{
				spSendingChannel strong = channel.lock();
				if (strong)
				{
					strong->emplace(std::move(msg));
				}
			});
		}

		Message pick(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out, Message * prototype, size_t sig)
		{
			Message msg;
			for (size_t i = 0; i < length; ++i)
			{
				if (channels[i]->pop(&msg, prototype, sig))
				{
					if (out) 
					{
						*out = channels[i];
					}
					return msg;
				}
			}

			return Message();
		}

		Message wait(ReceivingChannel ** channels, size_t length, size_t timeoutMillis, ReceivingChannel ** out, Message * prototype, size_t sig)
		{
			LARGE_INTEGER timer;
			QueryPerformanceCounter(&timer);

			LARGE_INTEGER start;
			QueryPerformanceCounter(&start);

			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);

			while(timeoutMillis == ~0u || (timer.QuadPart - start.QuadPart) * 1000 < freq.QuadPart * timeoutMillis)
			{
				Message result = pick(channels, length, out, prototype, sig);
				if (result)
				{
					return result;
				}
				QueryPerformanceCounter(&timer);
			}

			return Message();
		}
	}
}
