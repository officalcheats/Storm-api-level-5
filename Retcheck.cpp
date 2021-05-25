#include "RetCheck.h"

uintptr_t Retcheck::unprotect(BYTE* funcaddr)
{
	static int total_alloc;
	static std::map<uintptr_t, uintptr_t> cache;
	try
	{
		uintptr_t& cached_func = cache.at((uintptr_t)funcaddr);
		return (uintptr_t)(cached_func);
	}
	catch (std::out_of_range&)
	{
	} //cache miss, do nothing and continue

	uintptr_t func_size = get_func_end(funcaddr) - funcaddr;
	if (!func_size)
	{
		return (uintptr_t)(funcaddr);
	}

	if (total_alloc + func_size > max_alloc)
	return (uintptr_t)(funcaddr); //failsafe, using too much memory (over 1MB)

	void* new_func = VirtualAlloc(NULL, func_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (new_func == NULL)
	return (uintptr_t)(funcaddr); //alloc failed

	total_alloc += func_size;
	memcpy(new_func, funcaddr, func_size);
	if (disable_retcheck((uintptr_t)new_func, func_size))
	{
		cache.emplace((uintptr_t)funcaddr, (uintptr_t)new_func);
		fix_calls((uintptr_t)new_func, (uintptr_t)funcaddr, func_size);
		return (uintptr_t)(new_func);
	}

	//no retcheck was found, abort
	VirtualFree(new_func, 0, MEM_RELEASE);
	VirtualFree(new_func, func_size, MEM_DECOMMIT);
	return (uintptr_t)(funcaddr);
}
