#ifndef NANA_PAT_PROTOTYPE_HPP
#define NANA_PAT_PROTOTYPE_HPP

#include <cstddef>
#include <type_traits>
#include <memory>

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

			cloneable_wrapper(value_t&& rv)
				:object_(std::move(rv))
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

		struct deleter
		{
			void operator()(interface_t * p)
			{
				if(p)
					p->self_delete();
			}
		};

		struct inner_bool
		{
			int true_stand;
		};

		typedef int inner_bool::* operator_bool_t;

		template<typename U>
		struct member_enabled
			: public std::enable_if<(!std::is_base_of<cloneable, typename std::remove_reference<U>::type>::value) && std::is_base_of<base_t, typename std::remove_reference<U>::type>::value, int>
		{};
	public:
		cloneable()
			: fast_ptr_(nullptr)
		{}

		cloneable(std::nullptr_t)
			: fast_ptr_(nullptr)
		{}

		template<typename T>
		cloneable(T&& t, typename member_enabled<T>::type = 0)
			:	cwrapper_(new detail::cloneable_wrapper<typename std::remove_cv<typename std::remove_reference<T>::type>::type, base_t>(std::forward<T>(t)), deleter()),
				fast_ptr_(&(cwrapper_->refer()))
		{}

		cloneable(const cloneable& r)
			:	cwrapper_(nullptr),
				fast_ptr_(nullptr)
		{
			if(r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
		}

		cloneable(cloneable && r)
			:	cwrapper_(std::move(r.cwrapper_)),
				fast_ptr_(r.fast_ptr_)
		{
			r.fast_ptr_ = nullptr;
		}

		cloneable & operator=(const cloneable& r)
		{
			if((this != &r) && r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
			return *this;
		}

		cloneable & operator=(cloneable&& r)
		{
			if(this != &r)
			{
				cwrapper_ = r.cwrapper_;
				fast_ptr_ = r.fast_ptr_;

				r.cwrapper_.reset();
				r.fast_ptr_ = nullptr;
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
			fast_ptr_ = nullptr;
			cwrapper_.reset();
		}

		operator operator_bool_t() const volatile
		{
			return (fast_ptr_ ? &inner_bool::true_stand : nullptr);
		}
	private:
		std::shared_ptr<interface_t> cwrapper_;
		base_t * fast_ptr_;
	};

	template<typename Base>
	class mutable_cloneable
	{
		typedef Base base_t;
		typedef detail::cloneable_interface<base_t> interface_t;

		struct deleter
		{
			void operator()(interface_t * p)
			{
				if(p)
					p->self_delete();
			}
		};

		struct inner_bool
		{
			int true_stand;
		};

		typedef int inner_bool::* operator_bool_t;

		template<typename U>
		struct member_enabled
			: public std::enable_if<std::is_base_of<base_t, typename std::remove_reference<U>::type>::value && (!std::is_base_of<mutable_cloneable, typename std::remove_reference<U>::type>::value), int>
		{};
	public:
		mutable_cloneable()
			: fast_ptr_(nullptr)
		{}

		mutable_cloneable(std::nullptr_t)
			: fast_ptr_(nullptr)
		{}

		template<typename T>
		mutable_cloneable(T&& t, typename member_enabled<T>::type = 0)
			: cwrapper_(new detail::cloneable_wrapper<typename std::remove_cv<typename std::remove_reference<T>::type>::type, base_t>(std::forward<T>(t)), deleter()),
			fast_ptr_(&(cwrapper_->refer()))
		{
		}

		mutable_cloneable(const mutable_cloneable& r)
			:	cwrapper_(nullptr),
				fast_ptr_(nullptr)
		{
			if(r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
		}

		mutable_cloneable(mutable_cloneable && r)
			:	cwrapper_(std::move(r.cwrapper_)),
				fast_ptr_(r.fast_ptr_)
		{
			r.fast_ptr_ = nullptr;
		}

		mutable_cloneable & operator=(const mutable_cloneable& r)
		{
			if((this != &r) && r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
			return *this;
		}

		mutable_cloneable & operator=(mutable_cloneable&& r)
		{
			if(this != &r)
			{
				cwrapper_ = r.cwrapper_;
				fast_ptr_ = r.fast_ptr_;

				r.cwrapper_.reset();
				r.fast_ptr_ = nullptr;
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
			fast_ptr_ = nullptr;
			cwrapper_.reset();
		}

		operator operator_bool_t() const volatile
		{
			return (fast_ptr_ ? &inner_bool::true_stand : nullptr);
		}
	private:
		std::shared_ptr<interface_t> cwrapper_;
		base_t * fast_ptr_;
	};
}//end namespace pat
}//end namespace nana

#endif
