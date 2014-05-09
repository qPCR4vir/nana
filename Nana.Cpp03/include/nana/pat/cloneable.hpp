#ifndef NANA_PAT_PROTOTYPE_HPP
#define NANA_PAT_PROTOTYPE_HPP
#include <nana/traits.hpp>

namespace nana{ namespace pat{

	namespace detail
	{
		template<typename T>
		class cloneable_interface
		{
		public:
			typedef T interface_t;
			typedef cloneable_interface cloneable_t;
			virtual ~cloneable_interface(){}
			virtual interface_t& refer() = 0;
			virtual const interface_t& refer() const = 0;
			virtual cloneable_t* clone() const = 0;
			virtual void self_delete() const = 0;
		};

		template<typename T, typename SuperOfT>
		class cloneable_wrapper
			: public cloneable_interface<SuperOfT>
		{
		public:
			typedef T value_t;
			typedef typename cloneable_interface<SuperOfT>::interface_t interface_t;

			cloneable_wrapper()
			{}

			cloneable_wrapper(const value_t& obj)
				:object_(obj)
			{}

			template<typename U>
			cloneable_wrapper(const U& u)
				: object_(u)
			{}

			template<typename U>
			cloneable_wrapper(U& u)
				:object_(u)
			{}

			virtual interface_t& refer()
			{
				return object_;
			}

			virtual const interface_t& refer() const
			{
				return object_;
			}

			virtual cloneable_interface<interface_t>* clone() const
			{
				return (new cloneable_wrapper(object_));
			}

			virtual void self_delete() const
			{
				(delete this);
			}
		private:
			value_t object_;	
		};

	}//end namespace detail

	template<typename Base>
	class cloneable
	{
		typedef Base base_t;
		typedef detail::cloneable_interface<base_t> interface_t;

		struct inner_bool
		{
			int true_stand;
		};

		typedef int inner_bool::* operator_bool_t;

		template<typename U>
		struct member_enabled
			: public nana::enable_if<nana::traits::is_base_of<base_t, typename nana::meta::rm_ref<U>::type>::value && (!nana::traits::is_base_of<cloneable, typename nana::meta::rm_ref<U>::type>::value), int>
		{};
	public:
		cloneable()
			: real_(0), fast_ptr_(0)
		{}

		template<typename T>
		cloneable(const T& t, typename member_enabled<T>::type = 0)
			:	real_(new detail::cloneable_wrapper<T, base_t>(t)),
				fast_ptr_(&(real_->refer()))
		{}

		cloneable(const cloneable& r)
			:	real_(0),
				fast_ptr_(0)
		{
			if(r.real_)
			{
				real_ = r.real_->clone();
				fast_ptr_ = &(real_->refer());
			}
		}

		~cloneable()
		{
			if(real_)
				real_->self_delete();
		}

		cloneable & operator=(const cloneable& r)
		{
			if((this != &r) && r.real_)
			{
				real_ = r.real_->clone();
				fast_ptr_ = &(real_->refer());
			}
			return *this;
		}

		base_t& operator*()
		{
			return *fast_ptr_;
		}

		const base_t& operator*() const
		{
			return *fast_ptr_;
		}

		base_t * operator->()
		{
			return fast_ptr_;
		}

		const base_t * operator->() const
		{
			return fast_ptr_;
		}

		base_t * get() const
		{
			return fast_ptr_;
		}

		void reset()
		{
			fast_ptr_ = 0;

			if(real_)
			{
				real_->self_delete();
				real_ = 0;
			}
		}

		operator operator_bool_t() const volatile
		{
			return (fast_ptr_ ? &inner_bool::true_stand : 0);
		}
	private:
		interface_t * real_;
		base_t * fast_ptr_;
	};

	template<typename Base>
	class mutable_cloneable
	{
		typedef Base base_t;
		typedef detail::cloneable_interface<base_t> interface_t;

		struct inner_bool
		{
			int true_stand;
		};

		typedef int inner_bool::* operator_bool_t;

		template<typename U>
		struct member_enabled
			: public nana::enable_if<nana::traits::is_base_of<base_t, typename nana::meta::rm_ref<U>::type>::value && (!nana::traits::is_base_of<mutable_cloneable, typename nana::meta::rm_ref<U>::type>::value), int>
		{};
	public:
		mutable_cloneable()
			: real_(0), fast_ptr_(0)
		{}

		template<typename T>
		mutable_cloneable(const T& t, typename member_enabled<T>::type = 0)
			:	real_(new detail::cloneable_wrapper<T, base_t>(t)),
				fast_ptr_(&(real_->refer()))
		{}

		mutable_cloneable(const mutable_cloneable& r)
			:	real_(0),
				fast_ptr_(0)
		{
			if(r.real_)
			{
				real_ = r.real_->clone();
				fast_ptr_ = &(real_->refer());
			}
		}

		~mutable_cloneable()
		{
			if(real_)
				real_->self_delete();
		}

		mutable_cloneable & operator=(const mutable_cloneable& r)
		{
			if((this != &r) && r.real_)
			{
				real_ = r.real_->clone();
				fast_ptr_ = &(real_->refer());
			}
			return *this;
		}

		base_t& operator*() const
		{
			return *fast_ptr_;
		}

		base_t * operator->() const
		{
			return fast_ptr_;
		}

		base_t * get() const
		{
			return fast_ptr_;
		}

		void reset()
		{
			fast_ptr_ = 0;

			if(real_)
			{
				real_->self_delete();
				real_ = 0;
			}
		}

		operator operator_bool_t() const volatile
		{
			return (fast_ptr_ ? &inner_bool::true_stand : 0);
		}
	private:
		interface_t * real_;
		base_t * fast_ptr_;
	};
}//end namespace pat
}//end namespace nana

#endif
