#include "channel.hpp"

#ifdef USE_TIMERS
#include "timer.h"
#endif

namespace Engine
{
	//Matching.
	namespace Channels2
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
	namespace Channels2
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
	namespace Channels2
	{
		boost::shared_ptr<Channel> Channel::create() 
		{
			return boost::make_shared<Channel>();
		}

		Channel::Channel()
		{
			InitializeCriticalSection(&observerLock);
		}

		Channel::~Channel()
		{
			DeleteCriticalSection(&observerLock);
		}

		bool Channel::pop(Message * msg)
		{
			bool popped = messages.try_pop(*msg);

			if (!popped)
			{
				msg->type = NULL_MESSAGE;
			}

			return popped;
		}

		void Channel::emplace(Message&& msg)
		{
			if (!observers.empty())
			{
				EnterCriticalSection(&observerLock);
				
				size_t size = observers.unsafe_size();
				size_t processed = 0;
				IChannelCallback * callback = nullptr;

				while(processed < size && observers.try_pop(callback))
				{
					++processed;
					if (!callback->pushed(static_cast<const Message&>(msg)))
					{
						observers.push(callback);
					}
				}

				LeaveCriticalSection(&observerLock);
			}


			messages.push(msg);
		}

		void Channel::observe(IChannelCallback * observer)
		{
			observers.push(observer);
		}
	}

	//Utilities
	namespace Channels2
	{
#ifdef USER_TIMERS
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
#endif

		Message pick(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out)
		{
			Message msg;
			for (size_t i = 0; i < length; ++i)
			{
				if (channels[i]->pop(&msg))
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

		namespace
		{
			//The Waiting callback. 
		}

		ChannelWait::ChannelWait(SendingChannel * channel)
			:channel(channel)
		{
			evented = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		}

		ChannelWait::ChannelWait(spSendingChannel channel)
			:channel(channel.get())
		{
			evented = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		}

		ChannelWait::~ChannelWait()
		{
			CloseHandle(evented);
		}

		bool ChannelWait::pushed(const Message& msg)
		{
			if (matches(msg, prototype, significance))
			{
				SetEvent(evented);
				return true;
			}
			
			return false;
		}

		void ChannelWait::setWait(const Message& proto, size_t sig)
		{
			prototype = proto;
			significance = sig;

			ResetEvent(evented);
			channel->observe(this);
		}
		
		void ChannelWait::wait(size_t timeoutNanos)
		{
			size_t waitDuration = timeoutNanos / 1000;
			if (timeoutNanos == ~0u)
			{
				waitDuration = INFINITE;
			}

			DWORD res = WaitForSingleObject(evented, waitDuration);
			if (res == WAIT_FAILED)
			{
				//failure state.
			}
		}

		bool ChannelWait::done() const
		{
			DWORD res = WaitForSingleObject(evented, 0);
			return res == WAIT_OBJECT_0;
		}
	}
}
