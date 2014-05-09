#ifndef NANA_ANY_HPP
#define NANA_ANY_HPP
#include <typeinfo>
#include <nana/traits.hpp>

namespace nana
{

	class any
	{
		struct super_type
		{
			virtual ~super_type();
			super_type& operator=(const super_type&);
			virtual super_type& assign(const super_type&) = 0;
			virtual bool same(const super_type&) const = 0;
			virtual super_type* clone() const = 0;
		}; //end struct super_type

		template<typename T>
		struct object_type
			: public super_type
		{
			object_type(){}
			
			object_type(T const & obj)
				: object(obj)
			{}
			
			object_type(const object_type& rhs)
				:object(rhs.object)
			{}
			
			virtual super_type& assign(const super_type& rhs)
			{
				if(this != &rhs)
				{
					const object_type * other = dynamic_cast<const object_type*>(&rhs);
					if(other)
						object = other->object;	
				}
				return *this;	
			}
			
			virtual bool same(const super_type& rhs) const
			{
				return (dynamic_cast<const object_type*>(&rhs) != 0);
			}
			
			virtual super_type* clone() const
			{
				return new object_type(object);	
			}
			
			T object;
		}; //end struct object_type
	public:
		template<typename T>
		any(T const & obj)
			: super_(new object_type<T>(obj))
		{}
	
		any();
		any(const any& rhs);
		~any();
		
		bool same(const any &rhs) const;
		any& operator=(const any& rhs);
		
		template<typename T>
		any& operator=(T const &rhs)
		{
			T * obj = get<T>();
			if(obj == 0)
			{
				delete super_;
				super_ = new object_type<T>(rhs);
			}
			else
				*obj = rhs;
			return *this;
		}
		
		template<typename T>
		T * get() const
		{
			if(super_)
			{
				typedef typename nana::meta::rm_const<T>::value_type type;
				object_type<type>* obj = dynamic_cast<object_type<type>*>(super_);
				if(obj) return &(obj->object);
			}
			return 0;
		}
		
		template<typename T>
		operator T&() const
		{
			T *obj = get<T>();
			
			if(obj == 0)
				throw std::bad_cast();
			
			return *obj;
		}
		
		template<typename T>
		operator T*() const
		{
			return get<T>();	
		}
	private:
		super_type * super_;
	};
}//end namespace nana

#endif
