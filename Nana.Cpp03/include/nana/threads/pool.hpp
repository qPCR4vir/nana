/*
 *	A Thread Pool Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *
 *	@file: nana/threads/pool.hpp
 */

#ifndef NANA_THREADS_POOL_HPP
#define NANA_THREADS_POOL_HPP

#include <nana/traits.hpp>
#include <nana/functor.hpp>
#include <cstddef>


namespace nana{
namespace threads
{
	class pool
	{
		struct task
		{
			enum t{general, signal};

			const t kind;

			task(t);
			virtual ~task() = 0;
			virtual void run() = 0;
		};

		template<typename Function>
		struct task_wrapper
			: task
		{
			typedef Function function_type;
			function_type taskobj;

			task_wrapper(const function_type& f)
				: task(task::general), taskobj(f)
			{}

			void run()
			{
				taskobj();
			}
		};

		struct task_signal;
		class impl;
	public:
		pool();
		pool(std::size_t thread_number);
		~pool();

		template<typename Function>
		void push(const Function& f)
		{
			task * taskptr = 0;

			try
			{
				taskptr = new task_wrapper<typename nana::meta::conditional<nana::traits::is_function_type<Function>::value, Function*, Function>::type>(f);
				_m_push(taskptr);
			}
			catch(std::bad_alloc&)
			{
				delete taskptr;
			}
		}

		void signal();
		void wait_for_signal();
		void wait_for_finished();
	private:
		void _m_push(task* task_ptr);
	private:
		impl * impl_;
	};//end class pool


	template<typename Function>
	class pool_pusher
	{
	public:
		typedef typename nana::meta::conditional<nana::traits::is_function_type<Function>::value, Function*, Function>::type value_type;

		pool_pusher(pool& pobj, value_type fn)
			:pobj_(pobj), value_(fn)
		{}

		void operator()() const
		{
			pobj_.push(value_);
		}
	private:
		pool & pobj_;
		value_type value_;
	};

	template<typename Function>
	pool_pusher<Function> pool_push(pool& pobj, const Function& fn)
	{
		return pool_pusher<Function>(pobj, fn);
	}

	template<typename Class, typename Concept>
	pool_pusher<nana::functor<void()> > pool_push(pool& pobj, Class& obj, void(Concept::*mf)())
	{
		return pool_pusher<nana::functor<void()> >(pobj, make_fun(obj, mf));
	}

}//end namespace threads
}//end namespace nana
#endif

