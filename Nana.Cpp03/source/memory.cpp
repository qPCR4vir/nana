#include <nana/memory.hpp>
#include <nana/config.hpp>

//Atomic Implementation
#if defined(NANA_WINDOWS) && !defined(NANA_MINGW)
#include <windows.h>
typedef long atomic_integer_t;

void atomic_add(atomic_integer_t * pw)
{
	InterlockedIncrement(pw);
}

atomic_integer_t atomic_dec(atomic_integer_t * pw)
{
	return InterlockedDecrement(pw);
}

#else
typedef int atomic_integer_t;
void atomic_add(atomic_integer_t * pw)
{
    __asm__
    (
        "lock\n\t"
        "incl %0":
        "=m"( *pw ): // output (%0)
        "m"( *pw ): // input (%1)
        "cc" // clobbers
    );		
}

atomic_integer_t atomic_dec(atomic_integer_t * pw)
{
    int r;
    int dv = -1;

    __asm__ __volatile__
    (
        "lock\n\t"
        "xadd %1, %0":
        "=m"( *pw ), "=r"( r ): // outputs (%0, %1)
        "m"( *pw ), "1"( dv ): // inputs (%2, %3 == %1)
        "memory", "cc" // clobbers
    );

    return r - 1;
}
#endif

namespace nana
{
	namespace detail
	{
		//class shared_block
			struct shared_block::block_impl
			{
				block_impl(nana::functor<void()> d)
					:	use_count(1),
						deleter(d)
				{}
				
				~block_impl()
				{
					if(deleter)
						deleter();
				}
			
				atomic_integer_t use_count;
				nana::functor<void()> deleter;
			};

			shared_block::shared_block()
				:	impl_(0)
			{
			}
			
			shared_block::shared_block(nana::functor<void()> deleter)
				:	impl_(new block_impl(deleter))
			{
			}
			
			shared_block::shared_block(const shared_block& r)
				: impl_(r.impl_)
			{
				if(impl_)
					atomic_add(&impl_->use_count);
			}
			
			shared_block& shared_block::operator=(const shared_block& r)
			{
				block_impl * tmp = r.impl_;
				if(impl_ != tmp)
				{
					if(tmp)
						atomic_add(&tmp->use_count);
						
					if(impl_ && (atomic_dec(&impl_->use_count) == 0))
						delete impl_;
				}
				return *this;
			}

			shared_block::~shared_block()
			{
				if(impl_ && (atomic_dec(&impl_->use_count) == 0))
					delete impl_;				
			}
			
			bool shared_block::unique() const
			{
				return (impl_ && impl_->use_count == 1);	
			}
			
			void shared_block::swap(shared_block& r)
			{
				block_impl * tmp = impl_;
				impl_ = r.impl_;
				r.impl_ = tmp;
			}
		//end class shared_block
	}//end namespace detail
}
