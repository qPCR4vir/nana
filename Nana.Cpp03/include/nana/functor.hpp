/*
 *	A Functor implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/functor.hpp
 *	@description:
 *		This functor is used for binding a member function of a class, it's class-independent
 */

#ifndef NANA_FUNCTOR_HPP
#define NANA_FUNCTOR_HPP
#include "traits.hpp"
#include "detail/functor_invoker_0.hpp"
#include "detail/functor_invoker_1.hpp"
#include "detail/functor_invoker_2.hpp"
#include "detail/functor_invoker_3.hpp"
#include "detail/functor_invoker_4.hpp"
#include "detail/functor_invoker_5.hpp"
#include <vector>

namespace nana
{
	template<typename T>
	const T & const_forward(const T& t)
	{
		return t;
	}

	template<typename T>
	volatile T & volatile_forward(volatile T& t)
	{
		return t;
	}

	template<typename T>
	const volatile T& const_volatile_forward(const volatile T& t)
	{
		return t;
	}

	template<typename Function>
	class functor
		:	public detail::interface_holder<typename meta::conditional<true, Function, typename meta::remove_pointer<Function>::type>::type>
	{
		typedef typename meta::conditional<traits::is_function_type<Function>::value, Function, typename meta::remove_pointer<Function>::type>::type private_signature;
		typedef detail::interface_holder<private_signature> base_type;
	public:
		typedef private_signature signature;

		functor(){}

		functor(signature faddr)
		{
			this->assign(faddr);
		}

		template<typename FO>
		functor(FO fo)
		{
			this->assign(fo);
		}

		template<typename T>
		functor(T& obj, typename traits::make_mf<signature, T, typename traits::cv_specifier<T>::value_type>::type mf)
		{
			this->assign(obj, mf);
		}

		functor& operator=(const functor& rhs)
		{
			base_type::operator=(rhs);
			return *this;
		}

		template<typename FO>
		functor& operator=(FO fo)
		{
			this->assign(fo);
			return *this;
		}

		operator void*() const
		{
			return (this->empty() ? 0 : const_cast<functor*>(this));
		}
		
		void assign(signature faddr)
		{
			this->assign_invoker(new (std::nothrow) detail::invoker<signature>(faddr));
		}
				
		template<typename T>
		void assign(T& obj, typename traits::make_mf<signature, T, typename traits::cv_specifier<T>::value_type>::type mf)
		{
			typedef typename traits::make_mf<signature, T, typename traits::cv_specifier<T>::value_type>::type mfptr;
			this->assign_invoker(new (std::nothrow) detail::invoker<mfptr>(obj, mf));
		}
	
		template<typename FO>
		void assign(FO fo)
		{
			this->assign_invoker(new (std::nothrow) detail::fo_invoker<FO, signature>(fo));
		}
	};

	template<typename T, typename MFPtr>
	functor<typename traits::mfptr_traits<MFPtr>::function> make_fun(T& obj, MFPtr mfaddr)
	{
		typedef typename traits::mfptr_traits<MFPtr>::function function;
		typedef typename traits::mfptr_traits<MFPtr>::concept_type concept_type;
		if(traits::is_derived<T, concept_type>::value)
			return functor<function>(static_cast<concept_type&>(obj), mfaddr);
		return functor<function>();
	}

	namespace detail
	{
		template<typename Ft, typename FnGroup>
		class functors_holder
		{
		public:
			typedef Ft function_type;
			typedef FnGroup fn_group_type;
			typedef std::vector<functor<function_type> > container;

			fn_group_type & operator=(const functor<function_type> & f)
			{
				this->clear();
				this->append(f);
				return *static_cast<fn_group_type*>(this);
			}

			fn_group_type & operator+=(const functor<function_type> & f)
			{
				this->append(f);
				return *static_cast<fn_group_type*>(this);
			}

			void append(const functor<function_type> & f)
			{
				fobjs_.push_back(f);
			}

			void assign(const functor<function_type> & f)
			{
				clear();
				fobjs_.push_back(f);
			}

			void clear()
			{
				container().swap(fobjs_);
			}

			bool empty() const
			{
				return (0 == fobjs_.size());
			}

			operator void*() const
			{
				return (fobjs_.size() ? const_cast<functors_holder*>(this) : 0);
			}
		protected:
			const container & _m_cont() const
			{
				return fobjs_;
			}
		private:
			container fobjs_;
		};
	}//end namespace detail

	template<typename Ft>
	class fn_group;

	template<typename R>
	class fn_group<R()>
		: public detail::functors_holder<R(), fn_group<R()> >
	{
	public:
		typedef R function_type();
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)())
		{
			this->append(make_fun(obj, mf));
		}

		R operator()() const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)();
				return (*last)();
			}
			return R();
		}
	};

	template<typename R, typename P0>
	class fn_group<R(P0)>
		: public detail::functors_holder<R(P0), fn_group<R(P0)> >
	{
	public:
		typedef R function_type(P0);
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)(P0))
		{
			this->append(make_fun(obj, mf));
		}

		R operator()(P0 p0) const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)(p0);
				return (*last)(p0);
			}
			return R();
		}
	};

	template<typename R, typename P0, typename P1>
	class fn_group<R(P0, P1)>
		: public detail::functors_holder<R(P0, P1), fn_group<R(P0, P1)> >
	{
	public:
		typedef R function_type(P0, P1);
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)(P0, P1))
		{
			this->append(make_fun(obj, mf));
		}

		R operator()(P0 p0, P1 p1) const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)(p0, p1);
				return (*last)(p0, p1);
			}
			return R();
		}
	};

	template<typename R, typename P0, typename P1, typename P2>
	class fn_group<R(P0, P1, P2)>
		: public detail::functors_holder<R(P0, P1, P2), fn_group<R(P0, P1, P2)> >
	{
	public:
		typedef R function_type(P0, P1, P2);
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)(P0, P1, P2))
		{
			this->append(make_fun(obj, mf));
		}

		R operator()(P0 p0, P1 p1, P2 p2) const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)(p0, p1, p2);
				return (*last)(p0, p1, p2);
			}
			return R();
		}
	};

	template<typename R, typename P0, typename P1, typename P2, typename P3>
	class fn_group<R(P0, P1, P2, P3)>
		: public detail::functors_holder<R(P0, P1, P2, P3), fn_group<R(P0, P1, P2, P3)> >
	{
	public:
		typedef R function_type(P0, P1, P2, P3);
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)(P0, P1, P2, P3))
		{
			this->append(make_fun(obj, mf));
		}

		R operator()(P0 p0, P1 p1, P2 p2, P3 p3) const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)(p0, p1, p2, p3);
				return (*last)(p0, p1, p2, p3);
			}
			return R();
		}
	};

	template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4>
	class fn_group<R(P0, P1, P2, P3, P4)>
		: public detail::functors_holder<R(P0, P1, P2, P3, P4), fn_group<R(P0, P1, P2, P3, P4)> >
	{
	public:
		typedef R function_type(P0, P1, P2, P3, P4);
		typedef typename detail::functors_holder<function_type, fn_group>::container container;
		using detail::functors_holder<function_type, fn_group>::assign;
		using detail::functors_holder<function_type, fn_group>::operator=;

		template<typename T, typename Concept>
		void assign(T& obj, R(Concept::*mf)(P0, P1, P2, P3, P4))
		{
			this->append(make_fun(obj, mf));
		}

		R operator()(P0 p0, P1 p1, P2 p2, P3 p3, P4 p4) const
		{
			const container & fobjs = this->_m_cont();
			if(fobjs.size())
			{
				typename container::const_iterator last = fobjs.end() - 1;
				for(typename container::const_iterator i = fobjs.begin(); i != last; ++i)
					(*i)(p0, p1, p2, p3, p4);
				return (*last)(p0, p1, p2, p3, p4);
			}
			return R();
		}
	};
}//end namespace nana

#endif
