#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#pragma once

#include <deque>
#include <utility>
#include <boost/cstdint.hpp>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

#ifdef _WIN32
#	include <Windows.h>
#else
namespace std
{
	//proxy move.
	//argh.
	template<typename T>
	T&& move(const T& t)
	{
		return static_cast<T&&>(t);
	}
}
#endif
namespace Engine
{
	namespace Threading
	{
		//Significance bits
		enum SignificanceBits
		{
			SIGNIFICANCE_TYPE = 1 << 0,
			SIGNIFICANCE_INTEGRAL = 1 << 1,
			SIGNIFICANCE_STORAGE_TYPE = 1 << 2,
		};

		enum ReservedMessageTypes
		{
			NULL_MESSAGE = 0
		};

		struct Message
		{
			Message()
				:type(NULL_MESSAGE)
				,integralValue(0)
			{}

			operator bool() const
			{
				return type != NULL_MESSAGE;
			}

			boost::uint32_t type;
			boost::uint32_t integralValue;

			boost::any storage;

			const char * pushPoint;
			size_t pushLine;
		};

		class ReceivingChannel 
		{
		public:
			Message pop();

			virtual bool pop(Message * msg, Message * prototype = nullptr, size_t significance = 0) = 0;
		};

		class SendingChannel
		{
		public:
			template<typename T>
			void push(boost::uint32_t type, boost::uint32_t integral, const T& u)
			{
				Message result;
				result.type = type;
				result.integralValue = integral;
				result.storage = u;

				emplace(std::move(result));
			}

			virtual void emplace(Message&& value) = 0;
		};

		/*
		 * Channels should be very lightweight to create.
		 */
		class Channel : public ReceivingChannel, public SendingChannel
		{
			friend void intrusive_ptr_add_ref(Channel *);
			friend void intrusive_ptr_release(Channel *);

			Channel();
		public:
			static boost::intrusive_ptr<Channel> create();

			bool pop(Message * msg, Message * prototype = nullptr, size_t sig = 0);
			void emplace(Message&& value);
		private:
			boost::mutex mutex;
			std::deque<Message> messages;

			volatile long referenceCount;
		};

		inline void intrusive_ptr_add_ref(Channel * ch)
		{
#ifdef _WIN32
			InterlockedIncrement(&ch->referenceCount);
#else
			ch->referenceCount++;
#endif
		}

		inline void intrusive_ptr_release(Channel * ch) 
		{
#ifdef _WIN32
			if (InterlockedDecrement(&ch->referenceCount) == 0)
			{
				delete ch;
			}
#else
			ch->referenceCount--;
			if (ch->referenceCount == 0)
			{
				delete ch;
			}
#endif
		}

		//Helpful utilities.
		/*
		 * schedule(sendingChannel, milliseconds, Message) - Sends the given message in the requested amount of time. 
		 * schedule(sendingChannel, milliseconds) - Sends a message with a reserved type with integralData = current_time.
		 * select(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out = nullptr, Message& prototype, size_t significance) - grabs a message from a set of different channels. Nonblocking.
		 * wait(ReceivingChannel ** channels, size_t length, ReceivingCHannel ** out, Message& prototype, size_t significance) - essentially a blocking select.
		 * launch(ReceivingChannel *, size_t identifier, T lambda) - Calls a function on a different thread, and puts the return value into the storage parameter.
		 * ReceivingChannel * launch(T lambda) - Creates a channel and then launches a function.
		 *   There are also ThreadSynced versions of the above that will always terminate before a render step.
		 */
	}
}

#endif
