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
			NULL_MESSAGE = 0, 
			TIMER_FINISHED,
		};

		struct Message
		{
		public:
			template<typename T>
			Message(boost::uint32_t type, boost::uint32_t integral, T value)
				:type(type)
				,integralValue(integral)
				,storage(value)
			{}

			Message()
				:type(NULL_MESSAGE)
				,integralValue(0)
			{}

			operator bool() const
			{
				return type != NULL_MESSAGE;
			}

			template<typename T>
			T arbitraryParam() const
			{
				return boost::any_cast<T>(storage);
			}

			template<typename T>
			bool arbitraryParamIs() const
			{
				try 
				{
					boost::any_cast<T>(storage);
					return true;
				}
				catch (boost::bad_any_cast& e)
				{
					return false;
				}
			}


			boost::uint32_t type;
			boost::uint64_t integralValue;
			boost::any storage;

#ifdef _DEBUG
			const char * pushFile;
			size_t pushLine;
#endif
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
			void push(boost::uint32_t type, boost::uint64_t integral, const T& u)
			{
				Message result;
				result.type = type;
				result.integralValue = integral;
				result.storage = u;

				emplace(std::move(result));
			}

#ifdef _DEBUG
			template<typename T>
			void push(boost::uint32_t type, boost::uint64_t integral, const T& u, const char * buffer, size_t line)
			{
				Message result;
				result.type = type;
				result.integralValue = integral;
				result.storage = u;
				result.pushFile = buffer;
				result.pushLine = line;

				emplace(std::move(result));
			}
#endif

			virtual void emplace(Message&& value) = 0;
		};
	
//Used as push(MSG(a, b, c)
#ifdef _DEBUG
#define MSG(type, integral, u) type, integral, u, __FILE__, __LINE__
#else 
#define MSG(type, integral, u) type, integral, u
#endif

		bool matches(const Message& target, const Message& prototype, size_t significance);

		/*
		 * Channels should be very lightweight to create.
		 */
		class Channel : public ReceivingChannel, public SendingChannel
		{
			Channel();
		public:
			static boost::intrusive_ptr<Channel> create();

			using ReceivingChannel::pop;
			bool pop(Message * msg, Message * prototype = nullptr, size_t sig = 0);
			void emplace(Message&& value);
		private:
			boost::mutex mutex;
			std::deque<Message> messages;
		};

		typedef boost::shared_ptr<Channel> spChannel;
		typedef boost::shared_ptr<SendingChannel> spSendingChannel;
		typedef boost::shared_ptr<ReceivingChannel> spReceivingChannel;

		typedef boost::weak_ptr<Channel> wpChannel;
		typedef boost::weak_ptr<SendingChannel> wpSendingChannel;
		typedef boost::weak_ptr<ReceivingChannel> wpReceivingChannel;

		class TimerCollection;
		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds, const Message& msg);
		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds);

		template<typename It>
		Message pick(It begin, It end, ReceivingChannel ** out = nullptr, Message * prototype = nullptr, size_t significance = 0)
		{
			std::vector<ReceivingChannel *> channels;
			while(begin != end)
			{
				channels.push_back((*begin).get());
				++begin;
			}

			return pick(&channels[0], channels.size(), out, prototype, significance);
		}

		Message pick(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out = nullptr, Message * prototype = nullptr, size_t sig = 0);

		template<typename It>
		Message wait(It begin, It end, size_t timeoutMillis, ReceivingChannel ** out = nullptr, Message * prototype = nullptr, size_t significance = 0)
		{
			std::vector<ReceivingChannel *> channels;
			while(begin != end)
			{
				channels.push_back((*begin).get());
				++begin;
			}

			return wait(&channels[0], channels.size(), out, prototype, significance);
		}

		Message wait(ReceivingChannel ** channels, size_t length, size_t timeoutMillis, ReceivingChannel ** out = nullptr, Message * prototype = nullptr, size_t sig = 0);

		template<typename It, typename T> 
		size_t process(It begin, It end, const T& callbacks)
		{
			while(begin != end)
			{
				ReceivingChannel * channel = *begin;
				Message msg;

				while (channel->pop(&msg))
				{
					callbacks(msg);
				}

				++begin;
			}
		}

		template<typename T> 
		void process(ReceivingChannel ** channels, size_t length, const T& callback)
		{
			process(channels, channels + length, callback);
		}

		//Helpful utilities.
		/*
		 * schedule(sendingChannel, milliseconds, Message) - Sends the given message in the requested amount of time. 
		 * schedule(sendingChannel, milliseconds) - Sends a message with a reserved type with integralData = current_time.
		 * pick(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out = nullptr, Message& prototype, size_t significance) - grabs a message from a set of different channels. Nonblocking.
		 * wait(ReceivingChannel ** channels, size_t length, ReceivingCHannel ** out, Message& prototype, size_t significance) - essentially a blocking select.
		 * launch(ReceivingChannel *, size_t identifier, T lambda) - Calls a function on a different thread, and puts the return value into the storage parameter.
		 * ReceivingChannel * launch(T lambda) - Creates a channel and then launches a function.
		 *   There are also ThreadSynced versions of the above that will always terminate before a render step.
		 */
	}
}

#endif
