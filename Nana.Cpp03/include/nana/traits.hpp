/*
 *	Traits Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	@file: nana/traits.hpp
 */

#ifndef NANA_TRAITS_HPP
#define NANA_TRAITS_HPP

namespace nana
{
	class null_type{};

	class noncopyable
	{
		noncopyable(const noncopyable&);
		noncopyable& operator=(const noncopyable&);
	public:
		noncopyable();
	};

	template<bool Test, typename T = void>
	struct enable_if
	{
		// type is undefined for assumed !_Test
	};

	template<typename T>
	struct enable_if<true, T>
	{
		typedef T type;
	};

	namespace meta
	{
		struct true_type
		{
			static const bool value = true;
		};

		struct false_type
		{
			static const bool value = false;
		private:
			int placeholder[2];
		};

		template<bool is_true>
		struct bool_type: public true_type
		{
			typedef true_type type;
		};

		template<>
		struct bool_type<false>: public false_type
		{
			typedef false_type type;
		};

		template<bool Test, typename Then, typename Else>
		struct conditional
		{
			typedef Else type;
		};

		template<typename Then, typename Else>
		struct conditional<true, Then, Else>
		{
			typedef Then type;
		};
	}//end namespace meta

	namespace traits
	{

		namespace detail
		{
			typedef nana::meta::true_type	true_type;
			typedef nana::meta::false_type	false_type;

			template<typename T> true_type match(T);
			template<typename> false_type match(...);

			template<typename T> false_type match_fcn_t(T(*)[10]);
			template<typename> true_type match_fcn_t(...);
		}

		template<typename T1, typename T2>
		struct same_type
			: meta::bool_type<false>
		{};

		template<typename T>
		struct same_type<T, T>
			: meta::bool_type<true>{};


		template<typename T>
		struct is_reference: meta::bool_type<false>{};

		template<typename T>
		struct is_reference<T&>: meta::bool_type<true>{};

		template<typename T>
		struct is_pointer: meta::bool_type<false>{};

		template<typename T>
		struct is_pointer<T*>: meta::bool_type<true>{};

		template<typename Derived, typename Base>
		struct is_derived: meta::bool_type<sizeof(detail::true_type) == sizeof(detail::match<Base*>((Derived*)0))>{};

		template<typename Base, typename Derived>
		struct is_base_of: meta::bool_type<sizeof(detail::true_type) == sizeof(detail::match<Base*>((Derived*)0))>
		{};

		//traits types for const-volatile specifier
		
		struct no_specifier{};
		struct const_specifier{};
		struct volatile_specifier{};
		struct const_volatile_specifier{};
		
		template<typename T>
		struct cv_specifier
		{
			typedef no_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<const T>
		{
			typedef const_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<volatile T>
		{
			typedef volatile_specifier value_type;
		};
		
		template<typename T>
		struct cv_specifier<const volatile T>
		{
			typedef const_volatile_specifier value_type;
		};
	}//end namespace traits

	namespace meta
	{
		template<typename T>
		struct rm_const
		{
			typedef T value_type;
		};

		template<typename T>
		struct rm_const<const T>
		{
			typedef T value_type;
		};

		template<typename T>
		struct rm_ref
		{
			typedef T type;
		};

		template<typename T>
		struct rm_ref<T&>
		{
			typedef T type;
		};

		template<typename T>
		struct mk_ref
		{
			typedef typename rm_ref<T>::value_type & value_type;
		};

		template<typename T>
		struct remove_pointer
		{
			typedef T type;
		};

		template<typename T>
		struct remove_pointer<T*>
		{
			typedef T type;
		};

	}//end namespace meta

	namespace traits
	{	
		template<typename T>
		struct is_function_pointer
			:	meta::conditional<is_pointer<T>::value,
						meta::bool_type<sizeof(detail::match_fcn_t<typename meta::remove_pointer<T>::type>(0)) == sizeof(meta::true_type)>,
						meta::bool_type<false>
					>::type
		{};

		template<typename T>
		struct is_function_type
			:meta::bool_type<(sizeof(detail::match_fcn_t<T>(0)) == sizeof(meta::true_type)) && (traits::same_type<T, void>::value == 0) >
		{};	

		//The traits of pointer to member function
		template<typename MF>
		struct mfptr_traits
		{
			typedef void function();
			typedef void return_type;
			typedef void concept_type;
			enum{Parameter = 0};
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)()>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{Parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{Parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{Parameter = 0};	
		};

		template<typename R, typename Concept>
		struct mfptr_traits<R(Concept::*)() const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef return_type function();
			enum{Parameter = 0};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{Parameter = 1};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P) const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{Parameter = 1};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P) volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{Parameter = 1};	
		};

		template<typename R, typename Concept, typename P>
		struct mfptr_traits<R(Concept::*)(P) const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef P param0_type;
			typedef return_type function(param0_type);	
			enum{Parameter = 1};	
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{Parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1) const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{Parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1) volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{Parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1>
		struct mfptr_traits<R(Concept::*)(P0, P1) const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef return_type function(param0_type, param1_type);
			enum{Parameter = 2};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{Parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{Parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{Parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2) const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef return_type function(param0_type, param1_type, param2_type);
			enum{Parameter =3};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{Parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{Parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{Parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3) const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type);
			enum{Parameter = 4};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4)>
		{
			typedef Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{Parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) const>
		{
			typedef const Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{Parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) volatile>
		{
			typedef volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{Parameter = 5};
		};

		template<typename R, typename Concept, typename P0, typename P1, typename P2, typename P3, typename P4>
		struct mfptr_traits<R(Concept::*)(P0, P1, P2, P3, P4) const volatile>
		{
			typedef const volatile Concept concept_type;
			typedef R return_type;
			typedef P0 param0_type;
			typedef P1 param1_type;
			typedef P2 param2_type;
			typedef P3 param3_type;
			typedef P4 param4_type;
			typedef return_type function(param0_type, param1_type, param2_type, param3_type, param4_type);
			enum{Parameter = 5};
		};
		
		

		template<typename Function, typename Concept, typename CVSpecifier>
		struct make_mf
		{
			typedef int type;
		};

		template<typename R, typename Concept>
		struct make_mf<R(), Concept, no_specifier>
		{
			typedef R(Concept::*type)();
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, const_specifier>
		{
			typedef R(Concept::*type)() const;
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)() volatile;
		};
		
		template<typename R, typename Concept>
		struct make_mf<R(), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)() const volatile;
		};

		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0);
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0) const;
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0) volatile;
		};
		
		template<typename R, typename P0, typename Concept>
		struct make_mf<R(P0), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0) const volatile;
		};

		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1);
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1) const;
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename Concept>
		struct make_mf<R(P0, P1), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename Concept>
		struct make_mf<R(P0, P1, P2), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename Concept>
		struct make_mf<R(P0, P1, P2, P3), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4) const volatile;
		};

		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, no_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5);
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, const_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) const;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) volatile;
		};
		
		template<typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename Concept>
		struct make_mf<R(P0, P1, P2, P3, P4, P5), Concept, const_volatile_specifier>
		{
			typedef R(Concept::*type)(P0, P1, P2, P3, P4, P5) const volatile;
		};
	}//end namespace traits

	namespace meta
	{
		template<	typename Param0 = null_type, typename Param1 = null_type,
					typename Param2 = null_type, typename Param3 = null_type,
					typename Param4 = null_type, typename Param5 = null_type,
					typename Param6 = null_type, typename Param7 = null_type,
					typename Param8 = null_type, typename Param9 = null_type>
		struct fixed_type_set
		{
			template<typename T>
			struct count
			{
				enum{value =	traits::same_type<Param0, T>::value +
								traits::same_type<Param1, T>::value +
								traits::same_type<Param2, T>::value +
								traits::same_type<Param3, T>::value +
								traits::same_type<Param4, T>::value +
								traits::same_type<Param5, T>::value +
								traits::same_type<Param6, T>::value +
								traits::same_type<Param7, T>::value +
								traits::same_type<Param8, T>::value +
								traits::same_type<Param9, T>::value};
			};
		};
	}//end namespace meta
}//end namespace nana

#endif
