#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#pragma once

#include <deque>
#include <utility>
#include <boost/cstdint.hpp>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include <concurrent_queue.h>

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
			Message(boost::uint32_t type, boost::uint64_t integral, T value)
				:type(type)
				,integralValue(integral)
				,storage(value)
			{}

			Message(boost::uint32_t type, boost::uint64_t integral)
				:type(type)
				,integralValue(integral)
				,storage()
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

			virtual bool pop(Message * msg) = 0;
		};

		class IChannelCallback
		{
		public:
			virtual bool pushed(const Message& msg) = 0;
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
			virtual void observe(IChannelCallback * observer) = 0;
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
		public:
			static boost::shared_ptr<Channel> create();

			Channel();
			~Channel();
			
			using ReceivingChannel::pop;
			bool pop(Message * msg);
			void emplace(Message&& value);
			void observe(IChannelCallback * observer);
		private:
			concurrency::concurrent_queue<Message> messages;

			CRITICAL_SECTION observerLock;
			concurrency::concurrent_queue<IChannelCallback *> observers;
		};

		typedef boost::shared_ptr<Channel> spChannel;
		typedef boost::shared_ptr<SendingChannel> spSendingChannel;
		typedef boost::shared_ptr<ReceivingChannel> spReceivingChannel;

		typedef boost::weak_ptr<Channel> wpChannel;
		typedef boost::weak_ptr<SendingChannel> wpSendingChannel;
		typedef boost::weak_ptr<ReceivingChannel> wpReceivingChannel;

#ifdef USE_TIMERS
		class TimerCollection;
		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds, const Message& msg);
		void schedule(TimerCollection * collection, wpSendingChannel channel, size_t milliseconds);
#endif

		template<typename It>
		Message pick(It begin, It end, ReceivingChannel ** out = nullptr)
		{
			std::vector<ReceivingChannel *> channels;
			while(begin != end)
			{
				channels.push_back((*begin).get());
				++begin;
			}

			return pick(&channels[0], channels.size(), out, prototype, significance);
		}

		Message pick(ReceivingChannel ** channels, size_t length, ReceivingChannel ** out = nullptr);

		template<typename It, typename T> 
		size_t process(It begin, It end, const T& callbacks)
		{
			size_t count = 0;
			while(begin != end)
			{
				ReceivingChannel * channel = *begin;
				Message msg;

				while (channel->pop(&msg))
				{
					callbacks(msg);
					++count;
				}

				++begin;
			}
			return count;
		}

		template<typename T> 
		void process(ReceivingChannel ** channels, size_t length, const T& callback)
		{
			process(channels, channels + length, callback);
		}

		template<typename T>
		void process(spReceivingChannel channel, const T& callback)
		{
			ReceivingChannel * ch = channel.get();
			process(&ch, 1, callback);
		}

		class ChannelWait : public IChannelCallback
		{
		public:
			ChannelWait(SendingChannel * channel);
			ChannelWait(spSendingChannel channel);

			~ChannelWait();
			
			void setWait(const Message& proto, size_t significance);
			void wait(size_t timeoutMillis = ~0u);
			
			bool pushed(const Message& msg);
		private:
			SendingChannel * channel;

			Message prototype;
			size_t significance;
			HANDLE evented;
		};

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
