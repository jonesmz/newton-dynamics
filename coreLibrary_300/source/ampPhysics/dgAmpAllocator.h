/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _DG_AMP_ALLOCATOR_H_
#define _DG_AMP_ALLOCATOR_H_

#include "dgAMP.h"

template <typename T>
class dgAmpAllocator: public std::allocator<T>
{
	public:
	typedef size_t size_type;
	typedef T* pointer;
	typedef const T* const_pointer;

	template<typename _Tp1>
	struct rebind
	{
		typedef dgAmpAllocator<_Tp1> other;
	};

	pointer allocate(size_type n, const void *hint=0)
	{
		return m_memory;
	}

	void deallocate(pointer p, size_type n)
	{
	}

	dgAmpAllocator(void* const buffer) throw()
		:std::allocator<T>() 
		,m_memory((pointer)buffer)
	{ 
	}

	dgAmpAllocator() throw()
		:std::allocator<T>() 
		,m_memory(NULL)
	{ 
	}

	dgAmpAllocator(const dgAmpAllocator &a) throw()
		:std::allocator<T>(a) 
		,m_memory(a.m_memory)
	{ 
	}

	template <class U>                    
	dgAmpAllocator(const dgAmpAllocator<U> &a) throw()
		:std::allocator<T>(a) 
		,m_memory((pointer)a.m_memory)
	{ 
	}

	~dgAmpAllocator() throw() 
	{ 
	}

	void SetPointer(void* const memory)
	{
		m_memory = (pointer) memory;
	}

	pointer m_memory;
};


#endif