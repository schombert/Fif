#pragma once
#include "unordered_dense.h"
#include <vector>
#include <optional>
#include <variant>
#include "stdint.h"
#include <string>
#include <string_view>
#include <assert.h>
#include <charconv>
#include <span>
#include <limits>

#ifdef _WIN64
#define USE_LLVM
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "Memoryapi.h"
#else
#endif

#ifdef USE_LLVM

#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include "llvm-c/LLJIT.h"
#include "llvm-c/LLJITUtils.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Error.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Orc.h"
#include "llvm-c/OrcEE.h"
#include "llvm-c/Linker.h"
#include "llvm-c/lto.h"
#include "llvm-c/Support.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Transforms/PassBuilder.h"

#ifdef _WIN64
#define NATIVE_CC LLVMCallConv::LLVMWin64CallConv
#else
#define NATIVE_CC LLVMCallConv::LLVMX8664SysVCallConv
#endif

#endif
namespace fif {

class word_mem_pool {
private:
	uint16_t* allocation = nullptr;
	uint64_t word_page_size = 0;
	std::atomic<uint32_t> first_free = uint32_t(0);
public:
	word_mem_pool() {
		constexpr auto allocation_size = (static_cast<size_t>(std::numeric_limits<uint32_t>::max()) + 1) * sizeof(int16_t) / 2;
#ifdef _WIN64

		SYSTEM_INFO sSysInfo;
		GetSystemInfo(&sSysInfo);
		word_page_size = uint64_t(sSysInfo.dwPageSize) * 16 / sizeof(int16_t); // manage pages in groups of 16

		allocation = (uint16_t*)VirtualAlloc(nullptr, allocation_size, MEM_RESERVE, PAGE_NOACCESS);
		if(allocation == nullptr) {
			MessageBoxW(nullptr, L"Unable to reserve memory", L"Fatal error", MB_OK);
			abort();
		}
		VirtualAlloc(allocation, word_page_size * sizeof(int16_t), MEM_COMMIT, PAGE_READWRITE);
#else
		allocation = (uint16_t*)malloc(allocation_size);
#endif
	}

	~word_mem_pool() {
#ifdef _WIN64
		VirtualFree(allocation, 0, MEM_RELEASE);
#else
		free(allocation);
#endif
	}
	inline uint32_t used_mem() const {
		return first_free.load(std::memory_order_acquire);
	}
	inline uint16_t* memory_at(int32_t offset) {
		return allocation + offset;
	}
	inline int32_t return_new_memory(size_t requested_capacity) {
		const size_t word_size = requested_capacity;

#ifdef _WIN64
		uint32_t initial_base_address = 0;
		do {
			initial_base_address = first_free.load(std::memory_order_acquire);

			// determine if we predict that our allocation will go to uncommitted pages
			// if so: commit

			auto page_offset = initial_base_address / word_page_size;
			auto end_page = (initial_base_address + word_size) / word_page_size;
			if(page_offset != end_page) {
				VirtualAlloc(allocation + (page_offset + 1) * word_page_size, (end_page - page_offset) * word_page_size * sizeof(int16_t), MEM_COMMIT, PAGE_READWRITE);
			}

		} while(!first_free.compare_exchange_weak(initial_base_address, initial_base_address + uint32_t(word_size), std::memory_order_acq_rel));
#else
		uint32_t initial_base_address = first_free.fetch_add(dword_size);
#endif

		if(initial_base_address + word_size >= ((static_cast<size_t>(std::numeric_limits<uint32_t>::max()) + 1) * sizeof(int16_t)) / 2) {
			assert(false);
			std::abort();
		}

		return int32_t(initial_base_address);
	}
};

struct indirect_string_hash {
	using is_avalanching = void;
	using is_transparent = void;

	indirect_string_hash() {
	}
	auto operator()(std::string_view sv) const noexcept -> uint64_t {
		return ankerl::unordered_dense::detail::wyhash::hash(sv.data(), sv.size());
	}
	auto operator()(std::unique_ptr<char[]> const& str) const noexcept -> uint64_t {
		return ankerl::unordered_dense::detail::wyhash::hash(str.get(), std::string_view{ str.get() }.size());
	}
};
struct indirect_string_eq {
	using is_transparent = void;


	indirect_string_eq() {
	}

	bool operator()(std::unique_ptr<char[]> const& l, std::unique_ptr<char[]> const& r) const noexcept {
		return std::string_view{ l.get() } == std::string_view{ r.get() };
	}
	bool operator()(std::unique_ptr<char[]> const& l, std::string_view r) const noexcept {
		return std::string_view{ l.get() } == r;
	}
	bool operator()(std::string_view r, std::unique_ptr<char[]> const& l) const noexcept {
		return std::string_view{ l.get() } == r;
	}
	bool operator()(std::unique_ptr<char[]> const& l, std::string const& r) const noexcept {
		return std::string_view{ l.get() } == r;
	}
	bool operator()(std::string const& r, std::unique_ptr<char[]> const& l) const noexcept {
		return std::string_view{ l.get() } == r;
	}
};

#ifdef USE_LLVM
#else
#define LLVMValueRef void*
#define LLVMBasicBlockRef void*
#define LLVMTypeRef void*
#endif

class vsize_obj {
private:
	static constexpr size_t byte_size = 64;
public:
	struct by_value { };

	unsigned char internal_buffer[byte_size] = { 0 };
	int32_t type = 0;
	uint32_t size = 0;

	vsize_obj() noexcept { }
	vsize_obj(int32_t type, uint32_t size, unsigned char* dat) noexcept : type(type), size(size) {
		if(size <= byte_size) {
			memcpy(internal_buffer, dat, size);
		} else {
			auto cpy = new unsigned char[size];
			memcpy(cpy, dat, size);
			memcpy(internal_buffer, &cpy, 8);
		}
	}
	template<typename T>
	vsize_obj(int32_t type, T val, by_value) noexcept : vsize_obj(type, uint32_t(sizeof(T)), (unsigned char*)(&val)) {
	}
	vsize_obj(int32_t type, uint32_t size) noexcept : type(type), size(size) {
		if(size > byte_size) {
			unsigned char* cpy = nullptr;
			memcpy(internal_buffer, &cpy, 8);
		} else {
		}
	}
	vsize_obj(vsize_obj const& o) noexcept : type(o.type), size(o.size) {
		if(size <= byte_size) {
			memcpy(internal_buffer, o.internal_buffer, size);
		} else {
			unsigned char* optr = nullptr;
			memcpy(&optr, o.internal_buffer, 8);
			if(optr) {
				auto cpy = new unsigned char[size];
				memcpy(cpy, optr, size);
				memcpy(internal_buffer, &cpy, 8);
			} else {
				memcpy(internal_buffer, &optr, 8);
			}
		}
	}
	vsize_obj(vsize_obj&& o) noexcept : type(o.type), size(o.size) {
		if(size <= byte_size) {
			memcpy(internal_buffer, o.internal_buffer, size);
		} else {
			memcpy(internal_buffer, o.internal_buffer, 8);
		}
		o.size = 0;
		o.type = 0;
	}
	vsize_obj& operator=(vsize_obj const& o) noexcept {
		if(size <= byte_size) {

		} else {
			unsigned char* ptr = nullptr;
			memcpy(&ptr, internal_buffer, 8);
			delete[] ptr;
		}
		size = o.size;
		type = o.type;
		if(size <= byte_size) {
			memcpy(internal_buffer, o.internal_buffer, size);
		} else {
			unsigned char* optr = nullptr;
			memcpy(&optr, o.internal_buffer, 8);
			if(optr) {
				auto cpy = new unsigned char[size];
				memcpy(cpy, optr, size);
				memcpy(internal_buffer, &cpy, 8);
			} else {
				memcpy(internal_buffer, &optr, 8);
			}
		}

		return *this;
	}
	vsize_obj& operator=(vsize_obj&& o) noexcept {
		if(size <= byte_size) {

		} else {
			unsigned char* ptr = nullptr;
			memcpy(&ptr, internal_buffer, 8);
			delete[] ptr;
		}

		size = o.size;
		type = o.type;
		if(size <= byte_size) {
			memcpy(internal_buffer, o.internal_buffer, size);
		} else {
			memcpy(internal_buffer, o.internal_buffer, 8);
		}
		o.size = 0;
		o.type = 0;

		return *this;
	}
	~vsize_obj() {
		if(size <= byte_size) {
			
		} else {
			unsigned char* ptr = nullptr;
			memcpy(&ptr, internal_buffer, 8);
			delete[] ptr;
		}
		size = 0;
	}
	unsigned char* data() noexcept  {
		if(size <= byte_size) {
			return internal_buffer;
		} else {
			unsigned char* ptr = nullptr;
			memcpy(&ptr, internal_buffer, 8);
			return ptr;
		}
	}
	unsigned char const* data() const noexcept {
		if(size <= byte_size) {
			return internal_buffer;
		} else {
			unsigned char const* ptr = nullptr;
			memcpy(&ptr, internal_buffer, 8);
			return ptr;
		}
	}
	template<typename T>
	T as() const noexcept {
		return *((T const*)data());
	}
};

class single_allocation_2xvsize_vector {
public:
	struct type_size {
		int32_t type = 0;
		uint32_t size = 0;
	};
protected:
	unsigned char* allocation = nullptr;
	int32_t first_tcapacity = 0;
	int32_t second_tcapacity = 0;
	int32_t first_tsize = 0;
	int32_t second_tsize = 0;
	int32_t first_bcapacity = 0;
	int32_t second_bcapacity = 0;
	int32_t first_bsize = 0;
	int32_t second_bsize = 0;

	type_size* first_i32() const {
		return (type_size*)allocation;
	}
	type_size* second_i32() const {
		return (type_size*)(allocation + (sizeof(type_size) * first_tcapacity)  + first_bcapacity);
	}
	unsigned char* first_b() const {
		return (allocation + sizeof(type_size) * first_tcapacity);
	}
	unsigned char* second_b() const {
		return (allocation + (sizeof(type_size) * first_tcapacity) + first_bcapacity + sizeof(type_size) * second_tcapacity);
	}
public:
	single_allocation_2xvsize_vector() noexcept { }
	single_allocation_2xvsize_vector(single_allocation_2xvsize_vector const& o) noexcept : first_tcapacity(o.first_tcapacity), second_tcapacity(o.second_tcapacity), first_tsize(o.first_tsize), second_tsize(o.second_tsize), first_bcapacity(o.first_bcapacity), second_bcapacity(o.second_bcapacity), first_bsize(o.first_bsize), second_bsize(o.second_bsize) {
		allocation = new unsigned char[o.first_tcapacity * (sizeof(type_size)) + o.second_tcapacity * (sizeof(type_size)) + o.first_bcapacity + o.second_bcapacity];
		memcpy(allocation, o.allocation, o.first_tcapacity * (sizeof(type_size)) + o.second_tcapacity * (sizeof(type_size)) + o.first_bcapacity + o.second_bcapacity);
	}
	single_allocation_2xvsize_vector(single_allocation_2xvsize_vector&& o) noexcept : allocation(o.allocation), first_tcapacity(o.first_tcapacity), second_tcapacity(o.second_tcapacity), first_tsize(o.first_tsize), second_tsize(o.second_tsize), first_bcapacity(o.first_bcapacity), second_bcapacity(o.second_bcapacity), first_bsize(o.first_bsize), second_bsize(o.second_bsize) {
		o.allocation = nullptr;
		o.first_tcapacity = 0;
		o.second_tcapacity = 0;
		o.first_tsize = 0;
		o.second_tsize = 0;
		o.first_bcapacity = 0;
		o.second_bcapacity = 0;
		o.first_bsize = 0;
		o.second_bsize = 0;
	}
	~single_allocation_2xvsize_vector() noexcept {
		delete[] allocation;
	}

	single_allocation_2xvsize_vector& operator=(single_allocation_2xvsize_vector const& o) noexcept {
		first_tcapacity = o.first_tcapacity;
		second_tcapacity = o.second_tcapacity;
		first_tsize = o.first_tsize;
		second_tsize = o.second_tsize;
		first_bcapacity = o.first_bcapacity;
		second_bcapacity = o.second_bcapacity;
		first_bsize = o.first_bsize;
		second_bsize = o.second_bsize;
		
		delete[] allocation;
		allocation = new unsigned char[o.first_tcapacity * (sizeof(type_size)) + o.second_tcapacity * (sizeof(type_size)) + o.first_bcapacity + o.second_bcapacity];
		memcpy(allocation, o.allocation, o.first_tcapacity * (sizeof(type_size)) + o.second_tcapacity * (sizeof(type_size)) + o.first_bcapacity + o.second_bcapacity);

		return *this;
	}
	single_allocation_2xvsize_vector& operator=(single_allocation_2xvsize_vector&& o) noexcept {
		delete[] allocation;
		allocation = o.allocation;
		first_tcapacity = o.first_tcapacity;
		second_tcapacity = o.second_tcapacity;
		first_tsize = o.first_tsize;
		second_tsize = o.second_tsize;
		first_bcapacity = o.first_bcapacity;
		second_bcapacity = o.second_bcapacity;
		first_bsize = o.first_bsize;
		second_bsize = o.second_bsize;

		o.allocation = nullptr;
		o.first_tcapacity = 0;
		o.second_tcapacity = 0;
		o.first_tsize = 0;
		o.second_tsize = 0;
		o.first_bcapacity = 0;
		o.second_bcapacity = 0;
		o.first_bsize = 0;
		o.second_bsize = 0;

		return *this;
	}

	type_size& fi32(int64_t offset) const {
		assert(0 <= offset && offset < first_tsize);
		return *(first_i32() + offset);
	}
	type_size& si32(int64_t offset) const {
		assert(0 <= offset && offset < second_tsize);
		return *(second_i32() + offset);
	}
	unsigned char* fi64(int64_t offset) const {
		assert(0 <= offset && offset <= first_bsize);
		return (first_b() + offset);
	}
	unsigned char* si64(int64_t offset) const {
		assert(0 <= offset && offset <= second_bsize);
		return (second_b() + offset);
	}
	void clear_first() {
		first_tsize = 0;
		first_bsize = 0;
	}
	void clear_second() {
		second_tsize = 0;
		second_bsize = 0;
	}
	int32_t size_first() const {
		return first_tsize;
	}
	int32_t size_second() const {
		return second_tsize;
	}
	int32_t size_bfirst() const {
		return first_bsize;
	}
	int32_t size_bsecond() const {
		return second_bsize;
	}
	void pop_first() {
		if(first_tsize > 0) {
			first_bsize -= fi32(first_tsize - 1).size;
			first_tsize = first_tsize - 1;
		}
	}
	vsize_obj popr_first() {
		vsize_obj result;
		if(first_tsize > 0) {
			first_bsize -= fi32(first_tsize - 1).size;
			result = vsize_obj(fi32(first_tsize - 1).type, fi32(first_tsize - 1).size, first_b() + first_bsize);
			first_tsize = first_tsize - 1;
		}
		return result;
	}
	void pop_second() {
		if(second_tsize > 0) {
			second_bsize -= si32(second_tsize - 1).size;
			second_tsize = second_tsize - 1;
		}
	}
	vsize_obj popr_second() {
		vsize_obj result;
		if(second_tsize > 0) {
			second_bsize -= si32(second_tsize - 1).size;
			result = vsize_obj(si32(second_tsize - 1).type, si32(second_tsize - 1).size, second_b() + second_bsize);
			second_tsize = second_tsize - 1;
		}
		return result;
	}
	void push_first(vsize_obj const& obj) {
		if(first_tsize + 1 > first_tcapacity || first_bsize + int32_t(obj.size) > first_bcapacity) {
			auto temp = std::move(*this);

			auto new_tcap = first_tsize + 1 > first_tcapacity ? std::max(temp.first_tcapacity * 2, 8) : std::max(temp.first_tcapacity, 8);
			auto new_bcap = first_bsize + int32_t(obj.size) > first_bcapacity ? std::max(temp.first_bcapacity * 2, std::max(first_bsize + int32_t(obj.size), 64)) : std::max(temp.first_bcapacity, 64);
			auto new_stcap = std::max(temp.second_tcapacity, 8);
			auto new_sbcap = std::max(temp.second_bcapacity, 64);

			allocation = new unsigned char[new_tcap * (sizeof(type_size)) + new_stcap * (sizeof(type_size)) + new_bcap + new_sbcap];
			first_tcapacity = new_tcap;
			second_tcapacity = new_stcap;
			first_bcapacity = new_bcap;
			second_bcapacity = new_sbcap;
			first_tsize = temp.first_tsize;
			second_tsize = temp.second_tsize;
			first_bsize = temp.first_bsize;
			second_bsize = temp.second_bsize;

			memcpy(first_i32(), temp.first_i32(), sizeof(type_size) * first_tsize);
			memcpy(first_b(), temp.first_b(), first_bsize);
			memcpy(second_i32(), temp.second_i32(), sizeof(type_size) * second_tsize);
			memcpy(second_b(), temp.second_b(), second_bsize);
		}
		++first_tsize;
		fi32(first_tsize - 1) = type_size{ obj.type, obj.size };
		if(obj.data())
			memcpy(fi64(first_bsize), obj.data(), obj.size);
		first_bsize += int32_t(obj.size);
	}
	void push_first_direct(int32_t type, uint32_t size, unsigned char* dat) {
		if(first_tsize + 1 > first_tcapacity || first_bsize + int32_t(size) > first_bcapacity) {
			auto temp = std::move(*this);

			auto new_tcap = first_tsize + 1 > first_tcapacity ? std::max(temp.first_tcapacity * 2, 8) : std::max(temp.first_tcapacity, 8);
			auto new_bcap = first_bsize + int32_t(size) > first_bcapacity ? std::max(temp.first_bcapacity * 2, std::max(first_bsize + int32_t(size), 64)) : std::max(temp.first_bcapacity, 64);
			auto new_stcap = std::max(temp.second_tcapacity, 8);
			auto new_sbcap = std::max(temp.second_bcapacity, 64);

			allocation = new unsigned char[new_tcap * (sizeof(type_size)) + new_stcap * (sizeof(type_size)) + new_bcap + new_sbcap];
			first_tcapacity = new_tcap;
			second_tcapacity = new_stcap;
			first_bcapacity = new_bcap;
			second_bcapacity = new_sbcap;
			first_tsize = temp.first_tsize;
			second_tsize = temp.second_tsize;
			first_bsize = temp.first_bsize;
			second_bsize = temp.second_bsize;

			memcpy(first_i32(), temp.first_i32(), sizeof(type_size) * first_tsize);
			memcpy(first_b(), temp.first_b(), first_bsize);
			memcpy(second_i32(), temp.second_i32(), sizeof(type_size) * second_tsize);
			memcpy(second_b(), temp.second_b(), second_bsize);

			// ensure original allocation outlives possible use
			++first_tsize;
			fi32(first_tsize - 1) = type_size{ type, size };
			if(dat)
				memcpy(fi64(first_bsize), dat, size);
			first_bsize += int32_t(size);
		} else {
			++first_tsize;
			fi32(first_tsize - 1) = type_size{ type, size };
			if(dat)
				memcpy(fi64(first_bsize), dat, size);
			first_bsize += int32_t(size);
		}
	}
	void push_second(vsize_obj const& obj) {
		if(second_tsize + 1 > second_tcapacity || second_bsize + int32_t(obj.size) > second_bcapacity) {
			auto temp = std::move(*this);

			auto new_stcap = second_tsize + 1 > second_tcapacity ? std::max(temp.second_tcapacity * 2, 8) : std::max(temp.second_tcapacity, 8);
			auto new_sbcap = second_bsize + int32_t(obj.size) > second_bcapacity ? std::max(temp.second_bcapacity * 2, std::max(second_bsize + int32_t(obj.size), 64)) : std::max(temp.second_bcapacity, 64);
			auto new_tcap = std::max(temp.first_tcapacity, 8);
			auto new_bcap = std::max(temp.first_bcapacity, 64);

			allocation = new unsigned char[new_tcap * (sizeof(type_size)) + new_stcap * (sizeof(type_size)) + new_bcap + new_sbcap];
			first_tcapacity = new_tcap;
			second_tcapacity = new_stcap;
			first_bcapacity = new_bcap;
			second_bcapacity = new_sbcap;
			first_tsize = temp.first_tsize;
			second_tsize = temp.second_tsize;
			first_bsize = temp.first_bsize;
			second_bsize = temp.second_bsize;

			memcpy(first_i32(), temp.first_i32(), sizeof(type_size) * first_tsize);
			memcpy(first_b(), temp.first_b(), first_bsize);
			memcpy(second_i32(), temp.second_i32(), sizeof(type_size) * second_tsize);
			memcpy(second_b(), temp.second_b(), second_bsize);

			// ensure original allocation outlives possible use
			++second_tsize;
			si32(second_tsize - 1) = type_size{ obj.type, obj.size };
			if(obj.data())
				memcpy(si64(second_bsize), obj.data(), obj.size);
			second_bsize += int32_t(obj.size);
		} else {
			++second_tsize;
			si32(second_tsize - 1) = type_size{ obj.type, obj.size };
			if(obj.data())
				memcpy(si64(second_bsize), obj.data(), obj.size);
			second_bsize += int32_t(obj.size);
		}
	}
	void push_second_direct(int32_t type, uint32_t size, unsigned char* dat) {
		if(second_tsize + 1 > second_tcapacity || second_bsize + int32_t(size) > second_bcapacity) {
			auto temp = std::move(*this);

			auto new_stcap = second_tsize + 1 > second_tcapacity ? std::max(temp.second_tcapacity * 2, 8) : std::max(temp.second_tcapacity, 8);
			auto new_sbcap = second_bsize + int32_t(size) > second_bcapacity ? std::max(temp.second_bcapacity * 2, std::max(second_bsize + int32_t(size), 64)) : std::max(temp.second_bcapacity, 64);
			auto new_tcap = std::max(temp.first_tcapacity, 8);
			auto new_bcap = std::max(temp.first_bcapacity, 64);

			allocation = new unsigned char[new_tcap * (sizeof(type_size)) + new_stcap * (sizeof(type_size)) + new_bcap + new_sbcap];
			first_tcapacity = new_tcap;
			second_tcapacity = new_stcap;
			first_bcapacity = new_bcap;
			second_bcapacity = new_sbcap;
			first_tsize = temp.first_tsize;
			second_tsize = temp.second_tsize;
			first_bsize = temp.first_bsize;
			second_bsize = temp.second_bsize;

			memcpy(first_i32(), temp.first_i32(), sizeof(type_size) * first_tsize);
			memcpy(first_b(), temp.first_b(), first_bsize);
			memcpy(second_i32(), temp.second_i32(), sizeof(type_size) * second_tsize);
			memcpy(second_b(), temp.second_b(), second_bsize);
		}
		++second_tsize;
		si32(second_tsize - 1) = type_size{ type, size };
		if(dat)
			memcpy(si64(second_bsize), dat, size);
		second_bsize += int32_t(size);
	}
	void resize(int32_t fs, int32_t ss) {
		assert(fs <= first_tsize && ss <= second_tsize);
		while(first_tsize > fs)
			pop_first();
		while(second_tsize > ss)
			pop_second();
	}
	void trim_to(int32_t fs, int32_t ss) {
		fs = std::min(fs, first_tsize);
		ss = std::min(ss, second_tsize);
		uint32_t bytes_to_skip = 0;
		if(fs != first_tsize) {
			for(int32_t i = 0; i < (first_tsize - fs); ++i) {
				bytes_to_skip += fi32(i).size;
			}
			
			std::memmove(first_i32(), first_i32() + (first_tsize - fs), fs * sizeof(type_size));
			std::memmove(first_b(), first_b() + (bytes_to_skip), first_bsize - int32_t(bytes_to_skip));
			first_tsize = fs;
			first_bsize -= int32_t(bytes_to_skip);
		}
		if(ss != second_tsize) {
			for(int32_t i = 0; i < (second_tsize - ss); ++i) {
				bytes_to_skip += si32(i).size;
			}

			std::memmove(second_i32(), second_i32() + (second_tsize - ss), ss * sizeof(type_size));
			std::memmove(second_b(), second_b() + (bytes_to_skip), second_bsize - int32_t(bytes_to_skip));
			second_tsize = ss;
			second_bsize -= int32_t(bytes_to_skip);
		}
	}
};

class state_stack {
protected:
	single_allocation_2xvsize_vector contents;
public:
	int32_t min_main_depth = 0;
	int32_t min_return_depth = 0;
public:

	void mark_used_from_main(size_t count) {
		min_main_depth = std::min(std::max(contents.size_first(), int32_t(count)) - int32_t(count), min_main_depth);
	}
	void mark_used_from_return(size_t count) {
		min_return_depth = std::min(std::max(contents.size_second(), int32_t(count)) - int32_t(count), min_return_depth);
	}

	void pop_main() {
		contents.pop_first();
		min_main_depth = std::min(min_main_depth, contents.size_first());
	}
	void pop_return() {
		contents.pop_second();
		min_return_depth = std::min(min_return_depth, contents.size_second());
	}
	vsize_obj popr_main() {
		min_main_depth = std::min(min_main_depth, std::max(contents.size_first() - 1, 0));
		return contents.popr_first();
	}
	vsize_obj popr_return() {
		min_return_depth = std::min(min_return_depth, std::max(contents.size_second() - 1, 0));
		return contents.popr_second();
	}
	uint32_t main_byte_at(size_t index) const {
		uint32_t bskip = 0;
		for(int32_t i = 0; i < index; ++i) {
			bskip += contents.fi32(i).size;
		}
		return bskip;
	}
	vsize_obj main_at(size_t index) const {
		return vsize_obj(contents.fi32(index).type, contents.fi32(index).size, contents.fi64(main_byte_at(index)));
	}
	unsigned char* main_ptr_at(size_t index) const {
		return contents.fi64(main_byte_at(index));
	}
	uint32_t return_byte_at(size_t index) const {
		uint32_t bskip = 0;
		for(int32_t i = 0; i < index; ++i) {
			bskip += contents.si32(i).size;
		}
		return bskip;
	}
	unsigned char* return_ptr_at(size_t index) const {
		return contents.si64(return_byte_at(index));
	}
	vsize_obj return_at(size_t index) const {
		return vsize_obj(contents.si32(index).type, contents.si32(index).size, contents.si64(return_byte_at(index)));
	}
	uint32_t main_byte_back_at(size_t index) const {
		uint32_t bskip = 0;
		for(int32_t i = 0; i < index; ++i) {
			bskip += contents.fi32(contents.size_first() - (i + 1)).size;
		}
		return bskip;
	}
	unsigned char* main_back_ptr_at(size_t index) const {
		return contents.fi64(contents.size_bfirst() - int32_t(main_byte_back_at(index)));
	}
	vsize_obj main_back_at(size_t index) const {
		return vsize_obj(contents.fi32(contents.size_first() - (index + 1)).type, contents.fi32(contents.size_first() - (index + 1)).size, contents.fi64(contents.size_bfirst() - int32_t(main_byte_back_at(index))));
	}
	uint32_t return_byte_back_at(size_t index) const {
		uint32_t bskip = 0;
		for(int32_t i = 0; i < index; ++i) {
			bskip += contents.si32(contents.size_second() - (i + 1)).size;
		}
		return bskip;
	}
	unsigned char* return_back_ptr_at(size_t index) const {
		return contents.si64(contents.size_bsecond() - int32_t(return_byte_back_at(index)));
	}
	vsize_obj return_back_at(size_t index) const {
		return vsize_obj(contents.si32(contents.size_second() - (index + 1)).type, contents.si32(contents.size_second() - (index + 1)).size, contents.si64(contents.size_bsecond() - int32_t(return_byte_back_at(index))));
	}
	void move_into(state_stack&& other) {
		*this = std::move(other);
	}
	void copy_into(state_stack const& other) {
		*this = other;
	}
	void push_back_main(vsize_obj const& val) {
		contents.push_first(val);
	}
	void push_back_return(vsize_obj const& val) {
		contents.push_second(val);
	}
	void push_back_main(int32_t type, uint32_t size, unsigned char* dat) {
		contents.push_first_direct(type, size, dat);
	}
	void push_back_return(int32_t type, uint32_t size, unsigned char* dat) {
		contents.push_second_direct(type, size, dat);
	}
	template<typename T>
	void push_back_main(int32_t t, T val) {
		contents.push_first_direct(t, uint32_t(sizeof(T)), (unsigned char*)(&val));
	}
	template<typename T>
	void push_back_return(int32_t t, T val) {
		contents.push_second_direct(t, uint32_t(sizeof(T)), (unsigned char*)(&val));
	}
	state_stack copy() const {
		return *this;
	}
	state_stack new_copy() const {
		state_stack temp_new{ *this };
		temp_new.min_main_depth = contents.size_first();
		temp_new.min_return_depth = contents.size_second();
		return temp_new;
	}
	void resize(size_t main_sz, size_t return_sz) {
		min_return_depth = std::min(min_return_depth, int32_t(main_sz));
		min_main_depth = std::min(min_main_depth, int32_t(return_sz));
		contents.resize(int32_t(main_sz), int32_t(return_sz));
	}
	void trim_to(size_t main_sz, size_t return_sz) {
		if(contents.size_second() > int32_t(return_sz)) 
			min_return_depth = std::max(int32_t(min_return_depth) - int32_t(contents.size_second() - return_sz), 0);
		if(contents.size_first() > int32_t(main_sz)) 
			min_main_depth = std::max(int32_t(min_main_depth) - int32_t(contents.size_first() - main_sz), 0);
		
		contents.trim_to(int32_t(main_sz), int32_t(return_sz));
	}
	size_t main_size() const {
		return size_t(contents.size_first());
	}
	size_t return_size()const {
		return size_t(contents.size_second());
	}
	size_t main_byte_size() const {
		return size_t(contents.size_bfirst());
	}
	size_t return_byte_size() const {
		return size_t(contents.size_bsecond());
	}
	int32_t main_type(size_t index) const {
		return contents.fi32(int32_t(index)).type;
	}
	int32_t return_type(size_t index) const {
		return contents.si32(int32_t(index)).type;
	}
	int32_t main_type_back(size_t index) const {
		return contents.fi32(contents.size_first() - int32_t(1 + index)).type;
	}
	int32_t return_type_back(size_t index) const {
		return contents.si32(contents.size_second() - int32_t(1 + index)).type;
	}
};

using llvm_stack = state_stack;
using type_stack = state_stack;
using interpreter_stack = state_stack;

class environment;
struct type;

using fif_call = uint16_t * (*)(state_stack&, uint16_t*, environment*);

struct parse_result {
	std::string_view content;
	bool is_string = false;
};

struct interpreted_word_instance {
	std::vector<int32_t> llvm_parameter_permutation;
	LLVMValueRef llvm_function = nullptr;
	int32_t compiled_offset = -1;
	int32_t stack_types_start = 0;
	int16_t stack_types_count = 0;
	int8_t typechecking_level = 0;
	bool being_compiled = false;
	bool llvm_compilation_finished = false;
	bool is_imported_function = false;
};

struct compiled_word_instance {
	std::vector<int32_t> llvm_parameter_permutation;
	uint16_t implementation_index = std::numeric_limits<uint16_t>::max();
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
};

using word_types = std::variant<interpreted_word_instance, compiled_word_instance>;

struct word {
	std::vector<int32_t> instances;
	std::string source;
	int32_t specialization_of = -1;
	int32_t stack_types_start = 0;
	int16_t stack_types_count = 0;
	bool treat_as_base = false;
	bool immediate =  false;
	bool being_typechecked = false;
};

struct dup_evaluation {
	bool alters_source = false;
	bool copy_altered = false;
};

struct type {
	static constexpr uint32_t FLAG_STRUCT = 0x00000001;
	static constexpr uint32_t FLAG_MEMORY_TYPE = 0x00000002;
	static constexpr uint32_t FLAG_TEMPLATE = 0x00000004;
	static constexpr uint32_t FLAG_STATELESS = 0x00000008;
	static constexpr uint32_t FLAG_ARRAY = 0x00000010;
	static constexpr uint32_t FLAG_POINTER = 0x00000020;

	int64_t ntt_data = 0;
	int32_t ntt_base_type = -1;

	int32_t type_slots = 0;
	int32_t non_member_types = 0;
	int32_t decomposed_types_start = 0;
	int32_t decomposed_types_count = 0;
	int32_t byte_size = 0;
	int32_t cell_size = 0;
	uint32_t flags = 0;

	std::optional<dup_evaluation> duptype;

	bool is_struct() const {
		return (flags & FLAG_STRUCT) != 0;
	}
	bool is_pointer() const {
		return (flags & FLAG_POINTER) != 0;
	}
	bool is_array() const {
		return (flags & FLAG_ARRAY) != 0;
	}
	bool is_memory_type() const {
		return (flags & FLAG_MEMORY_TYPE) != 0;
	}
	bool is_struct_template() const {
		return (flags & FLAG_TEMPLATE) != 0;
	}
	bool stateless() const {
		return (flags & FLAG_STATELESS) != 0;
	}
};

constexpr inline int32_t fif_i32 = 0;
constexpr inline int32_t fif_f32 = 1;
constexpr inline int32_t fif_bool = 2;
constexpr inline int32_t fif_type = 3;
constexpr inline int32_t fif_i64 = 4;
constexpr inline int32_t fif_f64 = 5;
constexpr inline int32_t fif_u32 = 6;
constexpr inline int32_t fif_u64 = 7;
constexpr inline int32_t fif_i16 = 8;
constexpr inline int32_t fif_u16 = 9;
constexpr inline int32_t fif_i8 = 10;
constexpr inline int32_t fif_u8 = 11;
constexpr inline int32_t fif_nil = 12;
constexpr inline int32_t fif_ptr = 13;
constexpr inline int32_t fif_opaque_ptr = 14;
constexpr inline int32_t fif_struct = 15;
constexpr inline int32_t fif_anon_struct = 16;
constexpr inline int32_t fif_stack_token = 17;
constexpr inline int32_t fif_array = 18;

class environment;

class dictionary {
private:
	static constexpr int32_t max_builtins = 300;
public:
	ankerl::unordered_dense::map<std::string, int32_t> words;
	ankerl::unordered_dense::map<std::string, int32_t> types;
	ankerl::unordered_dense::map<fif_call, uint16_t> builtins;
	std::vector<word> word_array;
	std::vector<type> type_array;
	std::vector<word_types> all_instances;
	std::vector<int32_t> all_compiled;
	std::vector<int32_t> all_stack_types;
	fif_call builtin_array[max_builtins] = { 0 };
	uint16_t last_builtin = 0;

	dictionary() {
		types.insert_or_assign(std::string("i32"), fif_i32);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(int32_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("f32"), fif_f32);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(float));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("bool"), fif_bool);
		type_array.emplace_back();
		type_array.back().byte_size = 1;
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("type"), fif_type);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(int32_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("i64"), fif_i64);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(int64_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("f64"), fif_f64);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(double));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("u32"), fif_u32);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(uint32_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("u64"), fif_u64);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(uint64_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("i16"), fif_i16);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(int16_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("u16"), fif_u16);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(uint16_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("i8"), fif_i8);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(int8_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("u8"), fif_u8);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(uint8_t));
		type_array.back().cell_size = 1;

		types.insert_or_assign(std::string("nil"), fif_nil);
		type_array.emplace_back();
		type_array.back().byte_size = 0;
		type_array.back().cell_size = 0;
		type_array.back().flags = type::FLAG_STATELESS;

		types.insert_or_assign(std::string("ptr"), fif_ptr);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(void*));
		type_array.back().cell_size = 1;
		type_array.back().type_slots = 1;

		types.insert_or_assign(std::string("opaque_ptr"), fif_nil);
		type_array.emplace_back();
		type_array.back().byte_size = int32_t(sizeof(void*));
		type_array.back().cell_size = 1;
		type_array.back().flags = type::FLAG_POINTER;

		//types.insert_or_assign(std::string("llvm.struct"), fif_struct);
		type_array.emplace_back();

		types.insert_or_assign(std::string("struct"), fif_anon_struct);
		type_array.emplace_back();

		types.insert_or_assign(std::string("stack-token"), fif_stack_token);
		type_array.emplace_back();
		type_array.back().byte_size = 0;
		type_array.back().cell_size = 0;
		type_array.back().flags = type::FLAG_STATELESS;

		types.insert_or_assign(std::string("array"), fif_array);
		type_array.emplace_back();
		type_array.back().type_slots = 2;
	}

	uint16_t get_builtin(fif_call f) {
		if(auto it = builtins.find(f); it != builtins.end()) {
			return it->second;
		}
		assert(int32_t(last_builtin) < max_builtins);
		builtins.insert_or_assign(f, last_builtin);
		builtin_array[last_builtin] = f;
		return last_builtin++;
	}
};


enum class control_structure : uint8_t {
	none, function, str_if, str_while_loop, str_do_loop, mode_switch, globals,
};

class compiler_state;

struct typecheck_3_record {
	int32_t stack_height_added_at = 0;
	int32_t rstack_height_added_at = 0;
	int32_t stack_consumed = 0;
	int32_t rstack_consumed = 0;
};

struct lvar_description {
	int32_t type = 0;
	int32_t offset = 0;
	int32_t size = 0;
	bool memory_variable = false;
};

using internal_locals_storage = std::vector<unsigned char>;

template<typename T>
inline void c_append(std::vector<uint16_t>* v, T val) {
	auto cells = (sizeof(T) + 1) / sizeof(uint16_t);
	if(v) {
		v->resize(v->size() + cells, 0);
		std::memcpy(v->data() + v->size() - cells, &val, sizeof(val));
	}
}

class opaque_compiler_data {
public:
	opaque_compiler_data* parent = nullptr;

	opaque_compiler_data(opaque_compiler_data* parent) : parent(parent) { }

	virtual ~opaque_compiler_data() = default;
	virtual control_structure get_type() {
		return control_structure::none;
	}
	virtual LLVMValueRef llvm_function() {
		return parent ? parent->llvm_function() : nullptr;
	}
	virtual LLVMBasicBlockRef* llvm_block() {
		return parent ? parent->llvm_block() : nullptr;
	}
	virtual int32_t word_id() {
		return parent ? parent->word_id() : -1;
	}
	virtual int32_t instance_id() {
		return parent ? parent->instance_id() : -1;
	}
	virtual std::vector<uint16_t>* bytecode_compilation_progress() {
		return parent ? parent->bytecode_compilation_progress() : nullptr;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record() {
		return parent ? parent->typecheck_record() : nullptr;
	}
	virtual state_stack* working_state() {
		return parent ? parent->working_state() : nullptr;
	}
	virtual void set_working_state(state_stack&& p) {
		if(parent) 
			parent->set_working_state(std::move(p));
	}
	virtual bool finish(environment& env) {
		return true;
	}
	virtual lvar_description get_var(std::string const& name) {
		return parent ? parent->get_var(name) : lvar_description{ -1, -1, 0, false };
	}
	virtual std::vector<int32_t>* type_substitutions() {
		return parent ? parent->type_substitutions() : nullptr;
	}
	virtual LLVMValueRef build_alloca(LLVMTypeRef type) {
		return parent ? parent->build_alloca(type) : nullptr ;
	}
	virtual void increase_frame_size(int32_t sz) {
		if(parent) parent->increase_frame_size(sz);
	}
};

enum class fif_mode : uint16_t {
	primary_fn_mask			= 0x007,
	interpreting					= 0x001,
	compiling_bytecode			= 0x002,
	compiling_llvm				= 0x003,
	terminated					= 0x004,
	error_base					= 0x005,
	error						= 0x01D,
	typechecking				= 0x006,
	compilation_masked		= 0x010,
	failure						= 0x008,
	typecheck_provisional		= 0x020,
	typecheck_recursion		= 0x040,
	tc_level_mask				= 0xF00,
	tc_level_1					= 0x106,
	tc_level_2					= 0x206,
	tc_level_3					= 0x306
};

inline bool non_immediate_mode(fif_mode m) {
	auto pf_mode = uint16_t(m) & uint16_t(fif_mode::primary_fn_mask);
	return pf_mode == uint16_t(fif_mode::compiling_bytecode) || pf_mode == uint16_t(fif_mode::compiling_llvm);
}
inline fif_mode base_mode(fif_mode m) {
	return fif_mode(uint16_t(m) & uint16_t(fif_mode::primary_fn_mask));
}
inline bool typechecking_mode(fif_mode m) {
	return base_mode(m) == fif_mode::typechecking;
}
inline bool failed(fif_mode m) {
	return  (uint16_t(m) & uint16_t(fif_mode::failure)) != 0;
}
inline bool provisional_success(fif_mode m) {
	return  (uint16_t(m) & uint16_t(fif_mode::typecheck_provisional)) != 0;
}
inline bool recursive(fif_mode m) {
	return  (uint16_t(m) & uint16_t(fif_mode::typecheck_recursion)) != 0;
}
inline bool skip_compilation(fif_mode m) {
	return (uint16_t(m) & uint16_t(fif_mode::compilation_masked)) != 0;
}
inline int32_t typechecking_level(fif_mode m) {
	return int32_t((uint16_t(fif_mode::tc_level_mask) & uint16_t(m)) >> 8);
}
inline fif_mode surpress_branch(fif_mode m) {
	return fif_mode(uint16_t(m) | uint16_t(fif_mode::compilation_masked));
}
inline fif_mode fail_mode(fif_mode m) {
	return fif_mode(uint16_t(m) | uint16_t(fif_mode::failure) | uint16_t(fif_mode::compilation_masked));
}
inline fif_mode make_provisional(fif_mode m) {
	return fif_mode(uint16_t(m) | uint16_t(fif_mode::typecheck_provisional));
}
inline fif_mode make_recursive(fif_mode m) {
	return fif_mode(uint16_t(m) | uint16_t(fif_mode::compilation_masked) | uint16_t(fif_mode::typecheck_recursion));
}

inline fif_mode merge_modes(fif_mode a, fif_mode b) {
	auto ma = uint16_t(a);
	auto mb = uint16_t(b);
	auto i = ma | mb;
	
	if(failed(a) && failed(b))
		return fif_mode(i);

	if((uint16_t(fif_mode::typecheck_recursion) & ma) == 0 && (uint16_t(fif_mode::compilation_masked) & ma) == 0) {
		i &= ~uint16_t(fif_mode::typecheck_recursion);
		i &= ~uint16_t(fif_mode::compilation_masked);
	}
	if((uint16_t(fif_mode::typecheck_recursion) & mb) == 0 && (uint16_t(fif_mode::compilation_masked) & mb) == 0) {
		i &= ~uint16_t(fif_mode::typecheck_recursion);
		i &= ~uint16_t(fif_mode::compilation_masked);
	}
	if((uint16_t(fif_mode::typecheck_recursion) & ma) == 0 || (uint16_t(fif_mode::typecheck_recursion) & mb) == 0) {
		i &= ~uint16_t(fif_mode::typecheck_recursion);
	}
	if((uint16_t(fif_mode::compilation_masked) & ma) == 0 || (uint16_t(fif_mode::compilation_masked) & mb) == 0) {
		i &= ~uint16_t(fif_mode::compilation_masked);
	}
	return fif_mode(i);
}
inline fif_mode fail_typechecking(fif_mode m) {
	auto mv = uint16_t(m);
	mv |= uint16_t(fif_mode::failure);
	mv |= uint16_t(fif_mode::compilation_masked);
	return fif_mode(mv);
}
struct import_item {
	std::string name;
	void* ptr = nullptr;
};

int32_t index = 0;

struct global_item {
	std::unique_ptr<unsigned char[]> bytes;
	std::unique_ptr<LLVMValueRef[]> cells;
	int32_t type = 0;
	bool constant = false;
};

struct lexical_scope {
	ankerl::unordered_dense::map<std::string, lvar_description> vars;
	int32_t allocated_bytes = 0;
	bool allow_shadowing = false;
	bool new_top_scope = false;

	lexical_scope() noexcept = default;
	lexical_scope(int32_t a, bool b) noexcept : allocated_bytes(a), allow_shadowing(b), new_top_scope(false) { }
	lexical_scope(int32_t a, bool b, bool c) noexcept : allocated_bytes(a), allow_shadowing(b), new_top_scope(c){
	}
};

struct function_call {
	uint16_t* return_address = nullptr;
	int32_t return_local_size = 0;
};

class environment {
public:
	ankerl::unordered_dense::set<std::unique_ptr<char[]>, indirect_string_hash, indirect_string_eq> string_constants;
	std::vector<LLVMValueRef> exported_functions;
	std::vector<import_item> imported_functions;
#ifdef USE_LLVM
	LLVMOrcThreadSafeContextRef llvm_ts_context = nullptr;
	LLVMContextRef llvm_context = nullptr;
	LLVMModuleRef llvm_module = nullptr;
	LLVMBuilderRef llvm_builder = nullptr;
	LLVMTargetRef llvm_target = nullptr;
	LLVMTargetMachineRef llvm_target_machine = nullptr;
	LLVMTargetDataRef llvm_target_data = nullptr;
	LLVMOrcLLJITRef llvm_jit = nullptr;
	char* llvm_target_triple = nullptr;
	char* llvm_target_cpu = nullptr;
	char* llvm_target_cpu_features = nullptr;
#endif
	dictionary dict;
	std::vector<global_item> globals;
	ankerl::unordered_dense::map<std::string, int32_t> global_names;
	word_mem_pool compiled_bytes;

	std::vector<std::unique_ptr<opaque_compiler_data>> compiler_stack;
	std::vector<std::string_view> source_stack;
	std::vector<lexical_scope> lexical_stack;
	std::vector<int64_t> tc_local_variables;
	struct {
		size_t offset = 0;
		int32_t instance = -2;
	} last_compiled_call;
	
	static constexpr int32_t interpreter_stack_size = 1024 * 64;
	std::unique_ptr<unsigned char[]> interpreter_stack_space;
	std::vector< function_call> interpreter_call_stack;
	int32_t frame_offset = interpreter_stack_size;

	int64_t last_ssa_ident = -1;
	std::function<void(std::string_view)> report_error = [](std::string_view) { std::abort(); };

	fif_mode mode = fif_mode::interpreting;
	
	environment();
	~environment() {
#ifdef USE_LLVM
		LLVMDisposeMessage(llvm_target_triple);
		LLVMDisposeMessage(llvm_target_cpu);
		LLVMDisposeMessage(llvm_target_cpu_features);

		if(llvm_jit)
			LLVMOrcDisposeLLJIT(llvm_jit);
		if(llvm_target_data)
			LLVMDisposeTargetData(llvm_target_data);
		if(llvm_target_machine)
			LLVMDisposeTargetMachine(llvm_target_machine);
		if(llvm_builder)
			LLVMDisposeBuilder(llvm_builder);
		if(llvm_module)
			LLVMDisposeModule(llvm_module);
		if(llvm_ts_context)
			LLVMOrcDisposeThreadSafeContext(llvm_ts_context);
#endif
	}

	int64_t new_ident() {
		last_ssa_ident += 2;
		return last_ssa_ident;
	}

	std::string_view get_string_constant(std::string_view data) {
		if(auto it = string_constants.find(data); it != string_constants.end()) {
			return std::string_view{ it->get() };
		}
		std::unique_ptr<char[]> temp = std::unique_ptr<char[]>(new char[data.length() + 1]);
		memcpy(temp.get(), data.data(), data.length());
		temp.get()[data.length()] = 0;
		auto data_ptr = temp.get();
		string_constants.insert(std::move(temp));
		return std::string_view{ data_ptr };
	}
};

#ifdef USE_LLVM
inline LLVMTypeRef llvm_type(int32_t t, environment& env) {
	switch(t) {
		case fif_i32:
			return LLVMInt32TypeInContext(env.llvm_context);
		case fif_f32:
			return LLVMFloatTypeInContext(env.llvm_context);
		case fif_bool:
			return LLVMInt1TypeInContext(env.llvm_context);
		case fif_type:
			return LLVMInt32TypeInContext(env.llvm_context);
		case fif_i64:
			return LLVMInt64TypeInContext(env.llvm_context);
		case fif_f64:
			return LLVMDoubleTypeInContext(env.llvm_context);
		case fif_u32:
			return LLVMInt32TypeInContext(env.llvm_context);
		case fif_u64:
			return LLVMInt64TypeInContext(env.llvm_context);
		case fif_i16:
			return LLVMInt16TypeInContext(env.llvm_context);
		case fif_u16:
			return LLVMInt16TypeInContext(env.llvm_context);
		case fif_i8:
			return LLVMInt8TypeInContext(env.llvm_context);
		case fif_u8:
			return LLVMInt8TypeInContext(env.llvm_context);
		case fif_nil:
			return LLVMVoidTypeInContext(env.llvm_context);
		case fif_ptr:
			return LLVMPointerTypeInContext(env.llvm_context, 0);
		case fif_opaque_ptr:
			return LLVMPointerTypeInContext(env.llvm_context, 0);
		case fif_struct:
			return LLVMVoidTypeInContext(env.llvm_context);
		case fif_anon_struct:
			return LLVMVoidTypeInContext(env.llvm_context);
		case fif_stack_token:
			return LLVMVoidTypeInContext(env.llvm_context);
		case fif_array:
			return LLVMVoidTypeInContext(env.llvm_context);
		default:
			if(env.dict.type_array[t].is_struct_template()) {
				return LLVMVoidTypeInContext(env.llvm_context);
			} else if(env.dict.type_array[t].stateless()) {
				return LLVMVoidTypeInContext(env.llvm_context);
			} else if(env.dict.type_array[t].is_struct()) {
				std::vector<LLVMTypeRef> zvals;
				for(int32_t j = 1; j < env.dict.type_array[t].decomposed_types_count - env.dict.type_array[t].non_member_types; ++j) {
					auto st = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + j];
					if(st == fif_bool)
						zvals.push_back(LLVMInt8TypeInContext(env.llvm_context));
					else if(env.dict.type_array[st].stateless() == false)
						zvals.push_back(llvm_type(st, env));
					
				}
				return LLVMStructTypeInContext(env.llvm_context, zvals.data(), uint32_t(zvals.size()), true);
			} else if(env.dict.type_array[t].is_pointer()) {
				return LLVMPointerTypeInContext(env.llvm_context, 0);
			} else if(env.dict.type_array[t].is_array()) {
				auto array_t = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + 1];
				auto array_sz = env.dict.type_array[env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + 2]].ntt_data;
				return LLVMArrayType2(llvm_type(array_t, env), uint64_t(array_sz));
			} else {
				assert(false);
			}
	}
	return nullptr;
}
#else
inline void* llvm_type(int32_t t, environment& env) {
	return nullptr;
}
#endif


#ifdef USE_LLVM
inline void enum_llvm_type(int32_t t, std::vector< LLVMTypeRef>& ov, environment& env) {
	switch(t) {
		case fif_i32:
			ov.push_back(LLVMInt32TypeInContext(env.llvm_context)); return;
		case fif_f32:
			ov.push_back(LLVMFloatTypeInContext(env.llvm_context)); return;
		case fif_bool:
			ov.push_back(LLVMInt1TypeInContext(env.llvm_context)); return;
		case fif_type:
			ov.push_back(LLVMInt32TypeInContext(env.llvm_context)); return;
		case fif_i64:
			ov.push_back(LLVMInt64TypeInContext(env.llvm_context)); return;
		case fif_f64:
			ov.push_back(LLVMDoubleTypeInContext(env.llvm_context)); return;
		case fif_u32:
			ov.push_back(LLVMInt32TypeInContext(env.llvm_context)); return;
		case fif_u64:
			ov.push_back(LLVMInt64TypeInContext(env.llvm_context)); return;
		case fif_i16:
			ov.push_back(LLVMInt16TypeInContext(env.llvm_context)); return;
		case fif_u16:
			ov.push_back(LLVMInt16TypeInContext(env.llvm_context)); return;
		case fif_i8:
			ov.push_back(LLVMInt8TypeInContext(env.llvm_context)); return;
		case fif_u8:
			ov.push_back(LLVMInt8TypeInContext(env.llvm_context)); return;
		case fif_nil:
			return;
		case fif_ptr:
			ov.push_back(LLVMPointerTypeInContext(env.llvm_context, 0)); return;
		case fif_opaque_ptr:
			ov.push_back(LLVMPointerTypeInContext(env.llvm_context, 0)); return;
		case fif_struct:
			return;
		case fif_anon_struct:
			return;
		case fif_stack_token:
			return;
		case fif_array:
			return;
		default:
			if(env.dict.type_array[t].is_struct_template()) {
				return;
			} else if(env.dict.type_array[t].stateless()) {
				return;
			} else if(env.dict.type_array[t].is_struct()) {
				if(env.dict.type_array[t].is_memory_type()) {
					ov.push_back(LLVMPointerTypeInContext(env.llvm_context, 0));
					return;
				} else {
					for(int32_t j = 1; j < env.dict.type_array[t].decomposed_types_count - env.dict.type_array[t].non_member_types; ++j) {
						auto st = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + j];
						if(env.dict.type_array[st].stateless() == false)
							enum_llvm_type(st, ov, env);
					}
					return;
				}
			} else if(env.dict.type_array[t].is_pointer()) {
				ov.push_back(LLVMPointerTypeInContext(env.llvm_context, 0));
				return;
			} else if(env.dict.type_array[t].is_array()) {
				if(env.dict.type_array[t].is_memory_type()) {
					ov.push_back(LLVMPointerTypeInContext(env.llvm_context, 0));
					return;
				} else {
					assert(false);
				}
			} else {
				assert(false);
			}
	}
}
#else
inline void enum_llvm_type(int32_t t, std::vector< void*>& ov, environment& env) {

}
#endif

inline LLVMValueRef llvm_zero_constant(int32_t t, environment& env) {
#ifdef USE_LLVM
	switch(t) {
		case fif_i32:
			return LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false);
		case fif_f32:
			return LLVMConstReal(LLVMFloatTypeInContext(env.llvm_context), 0.0);
		case fif_bool:
			return LLVMConstInt(LLVMInt1TypeInContext(env.llvm_context), 0, false);
		case fif_type:
			return LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false);
		case fif_i64:
			return LLVMConstInt(LLVMInt64TypeInContext(env.llvm_context), 0, false);
		case fif_f64:
			return LLVMConstReal(LLVMDoubleTypeInContext(env.llvm_context), 0.0);
		case fif_u32:
			return LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false);
		case fif_u64:
			return LLVMConstInt(LLVMInt64TypeInContext(env.llvm_context), 0, false);
		case fif_i16:
			return LLVMConstInt(LLVMInt16TypeInContext(env.llvm_context), 0, false);
		case fif_u16:
			return LLVMConstInt(LLVMInt16TypeInContext(env.llvm_context), 0, false);
		case fif_i8:
			return LLVMConstInt(LLVMInt8TypeInContext(env.llvm_context), 0, false);
		case fif_u8:
			return LLVMConstInt(LLVMInt8TypeInContext(env.llvm_context), 0, false);
		case fif_nil:
			return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
		case fif_ptr:
			return LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0));
		case fif_opaque_ptr:
			return LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0));
		case fif_struct:
			return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
		case fif_anon_struct:
			return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
		case fif_stack_token:
			return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
		case fif_array:
			return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
		default:
			if(env.dict.type_array[t].is_struct_template()) {
				return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
			} else if(env.dict.type_array[t].stateless()) {
				return LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context));
			} else if(env.dict.type_array[t].is_struct()) {
				std::vector<LLVMValueRef> zvals;
				for(int32_t j = 1; j < env.dict.type_array[t].decomposed_types_count - env.dict.type_array[t].non_member_types; ++j) {
					auto st = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + j];
					if(st == fif_bool)
						zvals.push_back(LLVMConstInt(LLVMInt8TypeInContext(env.llvm_context), 0, false));
					else if(env.dict.type_array[st].stateless() == false)
						zvals.push_back(llvm_zero_constant(st, env));
				}
				return LLVMConstStructInContext(env.llvm_context, zvals.data(), uint32_t(zvals.size()), true);
			} else if(env.dict.type_array[t].is_pointer()) {
				return LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0));
			} else if(env.dict.type_array[t].is_array()) {
				auto array_t = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + 1];
				auto array_sz = env.dict.type_array[env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + 2]].ntt_data;
				std::vector<LLVMValueRef> zvals;
				zvals.resize(array_sz, llvm_zero_constant(array_t, env));
				return LLVMConstArray2(llvm_type(array_t, env), zvals.data(), zvals.size());
			} else {
				assert(false);
			}
	}
	return nullptr;
#else
	return nullptr;
#endif
}

inline void enum_llvm_zero_constant(int32_t t, std::vector< LLVMValueRef>& ov, environment& env) {
#ifdef USE_LLVM
	switch(t) {
		case fif_i32:
			ov.push_back(LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false)); return;
		case fif_f32:
			ov.push_back(LLVMConstReal(LLVMFloatTypeInContext(env.llvm_context), 0.0)); return;
		case fif_bool:
			ov.push_back(LLVMConstInt(LLVMInt1TypeInContext(env.llvm_context), 0, false)); return;
		case fif_type:
			ov.push_back(LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false)); return;
		case fif_i64:
			ov.push_back(LLVMConstInt(LLVMInt64TypeInContext(env.llvm_context), 0, false)); return;
		case fif_f64:
			ov.push_back(LLVMConstReal(LLVMDoubleTypeInContext(env.llvm_context), 0.0)); return;
		case fif_u32:
			ov.push_back(LLVMConstInt(LLVMInt32TypeInContext(env.llvm_context), 0, false)); return;
		case fif_u64:
			ov.push_back(LLVMConstInt(LLVMInt64TypeInContext(env.llvm_context), 0, false)); return;
		case fif_i16:
			ov.push_back(LLVMConstInt(LLVMInt16TypeInContext(env.llvm_context), 0, false)); return;
		case fif_u16:
			ov.push_back(LLVMConstInt(LLVMInt16TypeInContext(env.llvm_context), 0, false)); return;
		case fif_i8:
			ov.push_back(LLVMConstInt(LLVMInt8TypeInContext(env.llvm_context), 0, false)); return;
		case fif_u8:
			ov.push_back(LLVMConstInt(LLVMInt8TypeInContext(env.llvm_context), 0, false)); return;
		case fif_nil:
			ov.push_back(LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context))); return;
		case fif_ptr:
			ov.push_back(LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0))); return;
		case fif_opaque_ptr:
			ov.push_back(LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0))); return;
		case fif_struct:
			ov.push_back(LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context))); return;
		case fif_anon_struct:
			ov.push_back(LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context))); return;
		case fif_stack_token:
			ov.push_back(LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context))); return;
		case fif_array:
			ov.push_back(LLVMGetUndef(LLVMVoidTypeInContext(env.llvm_context))); return;
		default:
			if(env.dict.type_array[t].is_struct_template()) {
				return;
			} else if(env.dict.type_array[t].stateless()) {
				return;
			} else if(env.dict.type_array[t].is_struct()) {
				if(env.dict.type_array[t].is_memory_type()) {
					ov.push_back(LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0))); return;
				}
				for(int32_t j = 1; j < env.dict.type_array[t].decomposed_types_count - env.dict.type_array[t].non_member_types; ++j) {
					auto st = env.dict.all_stack_types[env.dict.type_array[t].decomposed_types_start + j];
					if(env.dict.type_array[st].stateless() == false)
						enum_llvm_zero_constant(st, ov, env);
				}
				return;
			} else if(env.dict.type_array[t].is_pointer()) {
				ov.push_back(LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0))); return;
			} else if(env.dict.type_array[t].is_array()) {
				ov.push_back(LLVMConstNull(LLVMPointerTypeInContext(env.llvm_context, 0))); return;
			} else {
				assert(false);
			}
	}
	return;
#else
	return;
#endif
}

inline environment::environment() {
#ifdef USE_LLVM
	llvm_ts_context = LLVMOrcCreateNewThreadSafeContext();
	llvm_context = LLVMOrcThreadSafeContextGetContext(llvm_ts_context);
	llvm_module = LLVMModuleCreateWithNameInContext("module_main", llvm_context);
	llvm_builder = LLVMCreateBuilderInContext(llvm_context);

	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();

	llvm_target = LLVMGetFirstTarget();
	llvm_target_triple = LLVMGetDefaultTargetTriple();
	llvm_target_cpu = LLVMGetHostCPUName();
	llvm_target_cpu_features = LLVMGetHostCPUFeatures();

	llvm_target_machine = LLVMCreateTargetMachine(llvm_target, llvm_target_triple, llvm_target_cpu, llvm_target_cpu_features, LLVMCodeGenOptLevel::LLVMCodeGenLevelAggressive, LLVMRelocMode::LLVMRelocDefault, LLVMCodeModel::LLVMCodeModelJITDefault);

	LLVMSetTarget(llvm_module, llvm_target_triple);
	llvm_target_data = LLVMCreateTargetDataLayout(llvm_target_machine);
	LLVMSetModuleDataLayout(llvm_module, llvm_target_data);
#endif

	interpreter_stack_space = std::unique_ptr<unsigned char[]>(new unsigned char[interpreter_stack_size]);
}

inline void enter_function_call(uint16_t* return_address, environment& env) {
	env.interpreter_call_stack.push_back(function_call{ return_address,  env.frame_offset });
}
inline uint16_t* leave_function_call(environment& env) {
	auto ret = env.interpreter_call_stack.back().return_address;
	env.frame_offset = env.interpreter_call_stack.back().return_local_size;
	env.interpreter_call_stack.pop_back();
	return ret;
}

lvar_description lexical_get_var(std::string const& name, environment& env) {
	for(auto j = env.lexical_stack.size(); j-- > 0;) {
		if(auto it = env.lexical_stack[j].vars.find(name); it != env.lexical_stack[j].vars.end()) {
			return it->second;
		}
		if(env.lexical_stack[j].new_top_scope)
			return lvar_description{ -1, -1, 0, false };
	}
	return lvar_description{ -1, -1, 0, false };
}

inline void execute_fif_word(parse_result word, environment& env, bool ignore_specializations);

inline uint16_t* do_local_to_stack(state_stack& s, uint16_t* p, environment* e) {
	int32_t index = *(p);
	int32_t type = *((int32_t*)(p + 1));
	auto* l = e->interpreter_stack_space.get() + e->frame_offset + index;
	s.push_back_main(type, uint32_t(e->dict.type_array[type].byte_size), l);
	return p + 3;
}

inline uint16_t* do_local_assign(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	int32_t offset = *(p);
	auto dest = e->interpreter_stack_space.get() + e->frame_offset + offset;
	memcpy(dest, s.main_back_ptr_at(1), s.main_byte_back_at(1));
	s.pop_main();
	return p + 1;
}

inline bool trivial_drop(int32_t type, environment& env);
inline bool trivial_dup(int32_t type, environment& env);
inline bool trivial_init(int32_t type, environment& env);
inline bool trivial_copy(int32_t type, environment& env);
inline void load_from_llvm_pointer(int32_t struct_type, state_stack& ws, LLVMValueRef ptr_expression, environment& env);

int32_t lexical_create_var(std::string const& name, int32_t type, int32_t size, unsigned char* data, bool memory_variable, bool reassign, environment& env) {
	auto& lscope = env.lexical_stack.back();

	if(env.mode == fif_mode::interpreting) {
		if(reassign) {
		} else {
		}
	}

	if(reassign) {
		for(auto j = env.lexical_stack.size(); j-- > 0;) {
			if(auto it = env.lexical_stack[j].vars.find(name); it != env.lexical_stack[j].vars.end()) {
				if(it->second.type != type || it->second.size != size || it->second.memory_variable) {
					return -1;
				}

				if(env.mode == fif_mode::compiling_bytecode) {
					auto compile_bytes = env.compiler_stack.back()->bytecode_compilation_progress();
					if(!trivial_drop(it->second.type, env)) {
						c_append(compile_bytes, env.dict.get_builtin(do_local_to_stack));
						c_append(compile_bytes, uint16_t(it->second.offset));
						c_append(compile_bytes, int32_t(it->second.type));

						auto ws = env.compiler_stack.back()->working_state();
						ws->push_back_main(vsize_obj(type, 0));
						execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					}
					if(compile_bytes) {
						c_append(compile_bytes, env.dict.get_builtin(do_local_assign));
						c_append(compile_bytes, uint16_t(it->second.offset));
					}
				} else if(env.mode == fif_mode::compiling_llvm) {
					auto ptr = env.tc_local_variables.data() + it->second.offset;
					auto ws = env.compiler_stack.back()->working_state();
					ws->push_back_main(vsize_obj(type, size, (unsigned char*)ptr));
					execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					memcpy(ptr, data, size);
				} else if(typechecking_mode(env.mode)) {
					auto ptr = env.tc_local_variables.data() + it->second.offset;
					memcpy(ptr, data, size);
				}
				return it->second.offset;
			}
			if(env.lexical_stack[j].new_top_scope)
				return -1;
		}
		
		return -1;
	} else {
		for(auto j = env.lexical_stack.size(); j-- > 0;) {
			if(auto it = env.lexical_stack[j].vars.find(name); it != env.lexical_stack[j].vars.end()) {
				return -1;
			}
			if(env.lexical_stack[j].allow_shadowing)
				break;
			if(env.lexical_stack[j].new_top_scope)
				break;
		}
		
		auto offset_pos = env.lexical_stack.back().allocated_bytes;
		env.lexical_stack.back().allocated_bytes += size;
		if(env.mode == fif_mode::compiling_bytecode) {
			env.compiler_stack.back()->increase_frame_size(env.lexical_stack.back().allocated_bytes);
			env.lexical_stack.back().vars.insert_or_assign(name, lvar_description{ type, offset_pos, size, memory_variable });
			auto compile_bytes = env.compiler_stack.back()->bytecode_compilation_progress();

			c_append(compile_bytes, env.dict.get_builtin(do_local_assign));
			c_append(compile_bytes, uint16_t(offset_pos));

			return offset_pos;
		} else if(env.mode == fif_mode::compiling_llvm) {
			env.tc_local_variables.resize(env.lexical_stack.back().allocated_bytes / 8);
			auto ptr = env.tc_local_variables.data() + offset_pos / 8;
			memcpy(ptr, data, size);
			env.lexical_stack.back().vars.insert_or_assign(name, lvar_description{ type, offset_pos / 8, size, memory_variable });
		} else if(typechecking_mode(env.mode)) {
			env.tc_local_variables.resize(env.lexical_stack.back().allocated_bytes / 8);
			auto ptr = env.tc_local_variables.data() + offset_pos / 8;
			memcpy(ptr, data, size);
			env.lexical_stack.back().vars.insert_or_assign(name, lvar_description{ type, offset_pos / 8, size, memory_variable });
		}
		return offset_pos / 8;
	}
}
void lexical_free_scope(lexical_scope& s, environment& env) {
	auto* ws = env.compiler_stack.back()->working_state();
	auto const mode = env.mode;
	for(auto& l : s.vars) {
		if(mode == fif_mode::compiling_bytecode) {
			auto compile_bytes = env.compiler_stack.back()->bytecode_compilation_progress();
			if(!trivial_drop(l.second.type, env)) {
				c_append(compile_bytes, env.dict.get_builtin(do_local_to_stack));
				c_append(compile_bytes, uint16_t(l.second.offset));
				c_append(compile_bytes, int32_t(l.second.type));

				execute_fif_word(fif::parse_result{ "drop", false }, env, false);
			}
		} else if(mode == fif_mode::interpreting) {
			assert(false);
		} else if(mode == fif_mode::compiling_llvm) {
			auto data_ptr = env.tc_local_variables.data() + l.second.offset;

			if(l.second.memory_variable) {
				vsize_obj temp(l.second.type, l.second.size, (unsigned char*)data_ptr);
				load_from_llvm_pointer(l.second.type, *ws, temp.as<LLVMValueRef>(), env);
				execute_fif_word(fif::parse_result{ "drop", false }, env, false);
			} else {
				ws->push_back_main(vsize_obj(l.second.type, l.second.size, (unsigned char*)data_ptr));
				execute_fif_word(fif::parse_result{ "drop", false }, env, false);
			}
		}
	}
}
void lexical_new_scope(bool allow_shadowing, environment& env) {
	int32_t prev_used_bytes = 0;
	if(env.lexical_stack.empty() == false) {
		prev_used_bytes = env.lexical_stack.back().allocated_bytes;
	}
	env.lexical_stack.emplace_back(prev_used_bytes, allow_shadowing);
}
void lexical_new_fn_scope(bool allow_shadowing, environment& env) {
	int32_t prev_used_bytes = 0;
	if(env.lexical_stack.empty() == false) {
		prev_used_bytes = env.lexical_stack.back().allocated_bytes;
	}
	env.lexical_stack.emplace_back(prev_used_bytes, false, true);
}
void lexical_end_scope(environment& env) {
	if(env.lexical_stack.empty() == false) {
		lexical_free_scope(env.lexical_stack.back(), env);
		env.lexical_stack.pop_back();
	}

	if(typechecking_mode(env.mode) || base_mode(env.mode) == fif_mode::compiling_llvm) {
		if(env.lexical_stack.empty() == false) {
			env.tc_local_variables.resize(env.lexical_stack.back().allocated_bytes / 8);
		} else {
			env.tc_local_variables.resize(0);
		}
	}
}

inline std::string word_name_from_id(int32_t w, environment const& e) {
	for(auto& p : e.dict.words) {
		if(p.second == w)
			return p.first;
	}
	return "@unknown (" + std::to_string(w)  + ")";
}

struct type_match_result {
	bool matched = false;
	int32_t stack_consumed = 0;
	int32_t ret_stack_consumed = 0;
};
struct type_match {
	int32_t type = 0;
	uint32_t end_match_pos = 0;
};

inline bool is_positive_integer(char const* start, char const* end) {
	if(start == end)
		return false;
	while(start < end) {
		if(!isdigit(*start))
			return false;
		++start;
	}
	return true;
}

inline bool is_integer(char const* start, char const* end) {
	if(start == end)
		return false;
	if(*start == '-')
		return is_positive_integer(start + 1, end);
	else
		return is_positive_integer(start, end);
}

inline bool is_positive_fp(char const* start, char const* end) {
	auto const decimal = std::find(start, end, '.');
	if(decimal == end) {
		return is_positive_integer(start, end);
	} else if(decimal == start) {
		return is_positive_integer(decimal + 1, end);
	} else {
		return is_positive_integer(start, decimal) && (decimal + 1 == end || is_positive_integer(decimal + 1, end));
	}
}

inline bool is_fp(char const* start, char const* end) {
	if(start == end)
		return false;
	if(*start == '-')
		return is_positive_fp(start + 1, end);
	else
		return is_positive_fp(start, end);
}

inline double pow_10(int n) {
	static double const e[] = {// 1e-0...1e308: 309 * 8 bytes = 2472 bytes
		1e+0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18,
		1e+19, 1e+20, 1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35,
		1e+36, 1e+37, 1e+38, 1e+39, 1e+40, 1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52,
		1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60, 1e+61, 1e+62, 1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69,
		1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80, 1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86,
		1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98, 1e+99, 1e+100, 1e+101, 1e+102, 1e+103,
		1e+104, 1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118,
		1e+119, 1e+120, 1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133,
		1e+134, 1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140, 1e+141, 1e+142, 1e+143, 1e+144, 1e+145, 1e+146, 1e+147, 1e+148,
		1e+149, 1e+150, 1e+151, 1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160, 1e+161, 1e+162, 1e+163,
		1e+164, 1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178,
		1e+179, 1e+180, 1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191, 1e+192, 1e+193,
		1e+194, 1e+195, 1e+196, 1e+197, 1e+198, 1e+199, 1e+200, 1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207, 1e+208,
		1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215, 1e+216, 1e+217, 1e+218, 1e+219, 1e+220, 1e+221, 1e+222, 1e+223,
		1e+224, 1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238,
		1e+239, 1e+240, 1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253,
		1e+254, 1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260, 1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268,
		1e+269, 1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279, 1e+280, 1e+281, 1e+282, 1e+283,
		1e+284, 1e+285, 1e+286, 1e+287, 1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296, 1e+297, 1e+298,
		1e+299, 1e+300, 1e+301, 1e+302, 1e+303, 1e+304, 1e+305, 1e+306, 1e+307, 1e+308 };
	return e[n];
}

inline bool float_from_chars(char const* start, char const* end, float& float_out) { // returns true on success
	// first read the chars into an int, keeping track of the magnitude
	// multiply by a pow of 10

	int32_t magnitude = 0;
	int64_t accumulated = 0;
	bool after_decimal = false;

	if(start == end) {
		float_out = 0.0f;
		return true;
	}

	bool is_negative = false;
	if(*start == '-') {
		is_negative = true;
		++start;
	} else if(*start == '+') {
		++start;
	}

	for(; start < end; ++start) {
		if(*start >= '0' && *start <= '9') {
			accumulated = accumulated * 10 + (*start - '0');
			magnitude += int32_t(after_decimal);
		} else if(*start == '.') {
			after_decimal = true;
		} else {
			// maybe check for non space and throw an error?
		}
	}
	if(!is_negative) {
		if(magnitude > 0)
			float_out = float(double(accumulated) / pow_10(magnitude));
		else
			float_out = float(accumulated);
	} else {
		if(magnitude > 0)
			float_out = -float(double(accumulated) / pow_10(magnitude));
		else
			float_out = -float(accumulated);
	}
	return true;
}

inline int32_t parse_int(std::string_view content) {
	int32_t rvalue = 0;
	auto result = std::from_chars(content.data(), content.data() + content.length(), rvalue);
	if(result.ec == std::errc::invalid_argument) {
		return 0;
	}
	return rvalue;
}
inline float parse_float(std::string_view content) {
	float rvalue = 0.0f;
	if(!float_from_chars(content.data(), content.data() + content.length(), rvalue)) {
		return 0.0f;
	}
	return rvalue;
}
struct variable_match_result {
	bool match_result = false;
	uint32_t match_end = 0;
};

inline type_match resolve_span_type(std::span<int32_t const> tlist, std::vector<int32_t> const& type_subs, environment& env);
inline variable_match_result fill_in_variable_types(int32_t source_type, std::span<int32_t const> match_span, std::vector<int32_t>& type_subs, environment& env);

inline int32_t struct_child_count(int32_t type, environment& env) {
	if(env.dict.type_array[type].decomposed_types_count == 0)
		return 0;
	auto main_type = env.dict.all_stack_types[env.dict.type_array[type].decomposed_types_start];
	return (env.dict.type_array[type].decomposed_types_count - 1 - env.dict.type_array[main_type].non_member_types);
}

inline uint32_t next_encoded_stack_type(std::span<int32_t const> desc) {
	if(desc.size() == 0)
		return 0;
	if(desc.size() == 1)
		return 1;
	if(desc[1] != std::numeric_limits<int32_t>::max())
		return 1;
	uint32_t i = 2;
	while(i < desc.size() && desc[i] != -1) {
		i += next_encoded_stack_type(desc.subspan(i));
	}
	if(i < desc.size() && desc[i] == -1)
		++i;
	return i;
}

inline int32_t instantiate_templated_struct_full(int32_t template_base, std::vector<int32_t> const& final_subtype_list, environment& env, bool skip_check = false) {
	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

	if(!skip_check) {
		uint32_t match_pos = 0;
		uint32_t mem_count = 0;
		std::vector<int32_t> type_subs_out;
		while(match_pos < desc.size()) {
			if(mem_count > final_subtype_list.size()) {
				env.report_error("attempted to instantiate a struct template with the wrong number of members");
				env.mode = fif_mode::error;
				return -1;
			}

			auto mr = fill_in_variable_types(final_subtype_list[mem_count], desc.subspan(match_pos), type_subs_out, env);
			if(!mr.match_result) {
				env.report_error("attempted to instantiate a struct template with types that do not match its definition");
				env.mode = fif_mode::error;
				return -1;
			}

			match_pos += next_encoded_stack_type(desc.subspan(match_pos));
			++mem_count;
		}
		if(mem_count < final_subtype_list.size()) {
			env.report_error("attempted to instantiate a struct template with the wrong number of members");
			env.mode = fif_mode::error;
			return -1;
		}
	}

	for(auto t : final_subtype_list) {
		if(t < 0) {
			env.report_error("attempted to instantiate a struct template with too few type parameters");
			env.mode = fif_mode::error;
			return -1;
		}
		if(env.dict.type_array[t].ntt_base_type != -1) {
			env.report_error("attempted to instantiate a struct template member type with a non-type template parameter");
			env.mode = fif_mode::error;
			return -1;
		}
	}

	for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
		if(env.dict.type_array[i].decomposed_types_count == int32_t(final_subtype_list.size() + 1)) {
			auto ta_start = env.dict.type_array[i].decomposed_types_start;
			bool match = env.dict.all_stack_types[ta_start] == template_base;

			for(uint32_t j = 0; match && j < final_subtype_list.size(); ++j) {
				if(env.dict.all_stack_types[ta_start + j + 1] != final_subtype_list[j]) {
					match = false;
				}
			}

			if(match) 
				return  int32_t(i);
		}
	}
	
	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();

	int32_t cells_count = 0;
	int32_t byte_count = 0;
	for(uint32_t j = 0; j < (final_subtype_list.size() - size_t(env.dict.type_array[template_base].non_member_types)); ++j) {
		if(env.dict.type_array[final_subtype_list[j]].stateless() == false) {
			if(env.dict.type_array[final_subtype_list[j]].is_memory_type() == false) {
				cells_count += env.dict.type_array[final_subtype_list[j]].cell_size;
				byte_count += env.dict.type_array[final_subtype_list[j]].byte_size;
			} else {
				cells_count += 1;
				byte_count += 8;
			}
		}
	}

	
	env.dict.type_array.back().decomposed_types_count = uint32_t(final_subtype_list.size() + 1);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	
	if(cells_count  == 0)
		env.dict.type_array.back().flags |= type::FLAG_STATELESS;

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_STRUCT;

	env.dict.type_array.back().non_member_types = env.dict.type_array[template_base].non_member_types;
	env.dict.type_array.back().type_slots = 0;
	env.dict.type_array.back().cell_size = cells_count;
	env.dict.type_array.back().byte_size = byte_count;
	env.dict.all_stack_types.push_back(template_base);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), final_subtype_list.begin(), final_subtype_list.end());

	return new_type;
}

inline int32_t instantiate_templated_struct(int32_t template_base, std::vector<int32_t> const& subtypes, environment& env) {
	std::vector<int32_t> final_subtype_list;

	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

	uint32_t match_pos = 0;
	while(match_pos < desc.size()) {
		auto mresult = resolve_span_type(desc.subspan(match_pos), subtypes, env);
		match_pos += mresult.end_match_pos;
		final_subtype_list.push_back(mresult.type);
	}

	return instantiate_templated_struct_full(template_base, final_subtype_list, env, true);
}

inline int32_t make_struct_type(std::string_view name, std::span<int32_t const> subtypes, std::vector<std::string_view> const& member_names, environment& env, int32_t template_types, int32_t extra_count) {

	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();
	env.dict.types.insert_or_assign(std::string(name), new_type);

	int32_t cells_count = 0;
	int32_t byte_count = 0;

	if(template_types == 0 && extra_count == 0) {
		std::vector<LLVMTypeRef> ctypes;
		int32_t count_real_members = 0;
		int32_t last_type = 0;

		for(uint32_t j = 0; j < subtypes.size(); ++j) {
			if(env.dict.type_array[subtypes[j]].ntt_base_type != -1) {
				env.report_error("attempted to make a struct with a member of a non-type (i.e. value) type");
				env.mode = fif_mode::error;
				return -1;
			}
			if(env.dict.type_array[subtypes[j]].stateless() == false) {
				if(env.dict.type_array[subtypes[j]].is_memory_type() == false) {
					cells_count += env.dict.type_array[subtypes[j]].cell_size;
					byte_count += env.dict.type_array[subtypes[j]].byte_size;
				} else {
					cells_count += 1;
					byte_count += 8;
				}
			}
		}

		env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
		env.dict.type_array.back().flags |= type::FLAG_STRUCT;

		if(cells_count == 0) {
			env.dict.type_array.back().flags |= type::FLAG_STATELESS;
		}
	} else {
		env.dict.type_array.back().non_member_types = extra_count;
		env.dict.type_array.back().flags |= type::FLAG_TEMPLATE;
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(subtypes.size() + (template_types + extra_count == 0 ? 1 : extra_count));
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	env.dict.type_array.back().cell_size = cells_count;
	env.dict.type_array.back().byte_size = byte_count;

	auto add_child_types = [&]() {
		if(template_types + extra_count > 0) {
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), subtypes.begin(), subtypes.end());
			for(int32_t e = 0; e < extra_count; ++e) {
				env.dict.all_stack_types.push_back(-(template_types + e + 2));
			}
			env.dict.all_stack_types.push_back(-1);
		}
	};

	env.dict.type_array.back().type_slots = template_types + extra_count;
	if(template_types + extra_count == 0)
		env.dict.all_stack_types.push_back(fif_struct);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), subtypes.begin(), subtypes.end());
	for(int32_t e = 0; e < extra_count; ++e) {
		env.dict.all_stack_types.push_back(-(template_types + e + 2));
	}

	int32_t start_types = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);
	add_child_types();
	int32_t end_zero = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(-1);
	env.dict.all_stack_types.push_back(new_type);
	add_child_types();
	int32_t end_one = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);
	add_child_types();

	int32_t end_two = int32_t(env.dict.all_stack_types.size());

	auto bury_word = [&](std::string const& name, int32_t id) { 
		int32_t start_word = 0;
		int32_t preceding_word = -1;
		if(auto it = env.dict.words.find(name); it != env.dict.words.end()) {
			start_word = it->second;
		} else {
			env.dict.words.insert_or_assign(name, id);
			return;
		}
		while(env.dict.word_array[start_word].specialization_of != -1) {
			preceding_word = start_word;
			start_word = env.dict.word_array[start_word].specialization_of;
		}
		if(preceding_word == -1) {
			env.dict.words.insert_or_assign(name, id);
			env.dict.word_array[id].specialization_of = start_word;
			assert(id != start_word);
		} else {
			env.dict.word_array[preceding_word].specialization_of = id;
			env.dict.word_array[id].specialization_of = start_word;
			assert(id != start_word);
		}
	};

	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 copy");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_two - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("copy", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 dup");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_two - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("dup", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map1 init");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_one - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("init", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map0 drop");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("drop", int32_t(env.dict.word_array.size() - 1));
	}

	uint32_t index = 0;
	auto desc = subtypes;
	while(desc.size() > 0) {
		auto next = next_encoded_stack_type(desc);

		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			env.dict.all_stack_types.push_back(-1);
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".") + std::string{ member_names[index] }, int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract-copy ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".") + std::string{ member_names[index] } + "@", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.insert ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".") + std::string{ member_names[index] } + "!", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(fif_ptr);
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(fif_ptr);
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			env.dict.all_stack_types.push_back(-1);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.gep ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".") + std::string{ member_names[index] }, int32_t(env.dict.word_array.size() - 1));
		}

		++index;
		desc = desc.subspan(next);
	}

	return new_type;
}

inline int32_t make_anon_struct_type(std::span<int32_t const> subtypes, environment& env) {

	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();

	int32_t cells_count = 0;
	int32_t byte_count = 0;

	for(uint32_t j = 0; j < subtypes.size(); ++j) {
		if(env.dict.type_array[subtypes[j]].stateless() == false) {
			if(env.dict.type_array[subtypes[j]].is_memory_type() == false) {
				cells_count += env.dict.type_array[subtypes[j]].cell_size;
				byte_count += env.dict.type_array[subtypes[j]].byte_size;
			} else {
				cells_count += 1;
				byte_count += 8;
			}
		}
	}


	env.dict.type_array.back().decomposed_types_count = uint32_t(subtypes.size() + 1);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_STRUCT;
	if(cells_count == 0) {
		env.dict.type_array.back().flags |= type::FLAG_STATELESS;
	}

	env.dict.type_array.back().cell_size = cells_count;
	env.dict.type_array.back().byte_size = byte_count;
	env.dict.type_array.back().type_slots = 0;

	env.dict.all_stack_types.push_back(fif_anon_struct);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), subtypes.begin(), subtypes.end());

	int32_t start_types = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);

	int32_t end_zero = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(-1);
	env.dict.all_stack_types.push_back(new_type);

	int32_t end_one = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);

	int32_t end_two = int32_t(env.dict.all_stack_types.size());

	auto bury_word = [&](std::string const& name, int32_t id) {
		int32_t start_word = 0;
		int32_t preceding_word = -1;
		if(auto it = env.dict.words.find(name); it != env.dict.words.end()) {
			start_word = it->second;
		} else {
			env.dict.words.insert_or_assign(name, id);
			return;
		}
		while(env.dict.word_array[start_word].specialization_of != -1) {
			preceding_word = start_word;
			start_word = env.dict.word_array[start_word].specialization_of;
		}
		if(preceding_word == -1) {
			env.dict.words.insert_or_assign(name, id);
			env.dict.word_array[id].specialization_of = start_word;
			assert(id != start_word);
		} else {
			env.dict.word_array[preceding_word].specialization_of = id;
			env.dict.word_array[id].specialization_of = start_word;
			assert(id != start_word);
		}
	};

	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 copy");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_two - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("copy", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 dup");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_two - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("dup", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map1 init");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_one - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("init", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map0 drop");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("drop", int32_t(env.dict.word_array.size() - 1));
	}

	uint32_t index = 0;
	auto desc = subtypes;
	while(desc.size() > 0) {
		auto next = next_encoded_stack_type(desc);

		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			env.dict.all_stack_types.push_back(-1);
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index), int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(new_type);
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract-copy ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index) + "@", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(new_type);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.insert ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index) + "!", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(fif_ptr);
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			env.dict.all_stack_types.push_back(new_type);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(-1);
			env.dict.all_stack_types.push_back(fif_ptr);
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			env.dict.all_stack_types.push_back(-1);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.gep ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index), int32_t(env.dict.word_array.size() - 1));
		}

		++index;
		desc = desc.subspan(next);
	}

	return new_type;
}

inline int32_t make_array_type(int32_t of_type, size_t sz, int32_t sz_as_type, environment& env) {
	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();

	env.dict.type_array.back().flags |= type::FLAG_ARRAY;
	env.dict.type_array.back().flags |= type::FLAG_MEMORY_TYPE;
	if(env.dict.type_array[of_type].cell_size == 0) {
		env.dict.type_array.back().flags |= type::FLAG_STATELESS;
		env.dict.type_array.back().cell_size = 0;
	} else {
		env.dict.type_array.back().cell_size = 1;
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(3);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	env.dict.type_array.back().type_slots = 0;
	env.dict.type_array.back().byte_size = int32_t(env.dict.type_array[of_type].byte_size * sz);
	env.dict.all_stack_types.push_back(fif_array);
	env.dict.all_stack_types.push_back(of_type);
	env.dict.all_stack_types.push_back(sz_as_type);

	return new_type;
}

inline int32_t make_pointer_type(int32_t to_type, environment& env) {
	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();
	env.dict.type_array.back().decomposed_types_count = uint32_t(2);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	env.dict.type_array.back().byte_size = int32_t(sizeof(void*));
	env.dict.type_array.back().cell_size = 1;
	env.dict.type_array.back().flags = type::FLAG_POINTER;
	env.dict.type_array.back().type_slots = 0;

	env.dict.all_stack_types.push_back(fif_ptr);
	env.dict.all_stack_types.push_back(to_type);
	
	return new_type;
}

bool is_memory_type_recursive(int32_t type, environment& env) {
	if(type == fif_nil)
		return false;
	if(env.dict.type_array[type].is_memory_type())
		return true;
	if(env.dict.type_array[type].is_pointer() == false)
		return false;

	auto ptr_contents = env.dict.all_stack_types[env.dict.type_array[type].decomposed_types_start + 1];
	return is_memory_type_recursive(ptr_contents, env);
}

inline int32_t find_existing_type_match(int32_t main_type, std::span<int32_t> subtypes, environment& env) {
	for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
		if(env.dict.type_array[i].decomposed_types_count == int32_t(1 + subtypes.size())) {
			auto ta_start = env.dict.type_array[i].decomposed_types_start;
			if(env.dict.all_stack_types[ta_start] == main_type) {
				bool match = true;
				for(uint32_t j = 0; j < subtypes.size(); ++j) {
					if(env.dict.all_stack_types[ta_start + 1 + j] != subtypes[j]) {
						match = false;
						break;
					}
				}
				if(match) {
					return  int32_t(i);
				}
			}
		}
	}
	return -1;
}
inline type_match internal_resolve_type(std::string_view text, environment& env, std::vector<int32_t>* type_subs) {
	uint32_t mt_end = 0;
	if(text.starts_with("ptr(nil)")) {
		return type_match{ fif_opaque_ptr, 8 };
	}
	for(; mt_end < text.size(); ++mt_end) {
		if(text[mt_end] == '(' || text[mt_end] == ')' || text[mt_end] == ',')
			break;
	}

	if(text.size() > 0 && text[0] == '$') {
		if(is_integer(text.data() + 1, text.data() + mt_end)) {
			auto v = parse_int(text.substr(1));
			if(type_subs && v >= 0 && size_t(v) < type_subs->size()) {
				return type_match{ (*type_subs)[v], mt_end };
			}
			return type_match{ fif_nil, mt_end };
		}
	}

	if(is_integer(text.data(), text.data() + mt_end)) {
		int64_t dat = parse_int(text.substr(0, mt_end));
		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].ntt_data == dat && env.dict.type_array[i].ntt_base_type == fif_i32) {
				return type_match{ int32_t(i), mt_end };
			}
		}
		int32_t new_type = int32_t(env.dict.type_array.size());
		env.dict.type_array.emplace_back();
		env.dict.type_array.back().ntt_data = dat;
		env.dict.type_array.back().ntt_base_type = fif_i32;
		env.dict.type_array.back().flags = type::FLAG_STATELESS;

		return type_match{ new_type, mt_end };
	} else if(is_fp(text.data(), text.data() + mt_end)) {
		int64_t dat = 0;
		float fdat = parse_float(text.substr(0, mt_end));
		memcpy(&dat, &fdat, 4);
		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].ntt_data == dat && env.dict.type_array[i].ntt_base_type == fif_f32) {
				return type_match{ int32_t(i), mt_end };
			}
		}
		int32_t new_type = int32_t(env.dict.type_array.size());
		env.dict.type_array.emplace_back();
		env.dict.type_array.back().ntt_data = dat;
		env.dict.type_array.back().ntt_base_type = fif_f32;
		env.dict.type_array.back().flags = type::FLAG_STATELESS;

		return type_match{ new_type, mt_end };
	} else if(text.substr(0, mt_end) == "true") {
		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].ntt_data == 1 && env.dict.type_array[i].ntt_base_type == fif_bool) {
				return type_match{ int32_t(i), mt_end };
			}
		}
		int32_t new_type = int32_t(env.dict.type_array.size());
		env.dict.type_array.emplace_back();
		env.dict.type_array.back().ntt_data = 1;
		env.dict.type_array.back().ntt_base_type = fif_bool;
		env.dict.type_array.back().flags = type::FLAG_STATELESS;

		return type_match{ new_type, mt_end };
	} else if(text.substr(0, mt_end) == "false") {
		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].ntt_data == 0 && env.dict.type_array[i].ntt_base_type == fif_bool) {
				return type_match{ int32_t(i), mt_end };
			}
		}
		int32_t new_type = int32_t(env.dict.type_array.size());
		env.dict.type_array.emplace_back();
		env.dict.type_array.back().ntt_data = 0;
		env.dict.type_array.back().ntt_base_type = fif_bool;
		env.dict.type_array.back().flags = type::FLAG_STATELESS;

		return type_match{ new_type, mt_end };
	} else if(auto it = env.dict.types.find(std::string(text.substr(0, mt_end))); it != env.dict.types.end()) {
		if(mt_end >= text.size() || text[mt_end] == ',' ||  text[mt_end] == ')') {	// case: plain type
			return type_match{ it->second, mt_end };
		}
		//followed by type list
		++mt_end;
		std::vector<int32_t> subtypes;
		while(mt_end < text.size() && text[mt_end] != ')') {
			auto sub_match = internal_resolve_type(text.substr(mt_end), env, type_subs);
			subtypes.push_back(sub_match.type);
			mt_end += sub_match.end_match_pos;
			if(mt_end < text.size() && text[mt_end] == ',')
				++mt_end;
		}
		if(mt_end < text.size() && text[mt_end] == ')')
			++mt_end;

		if(it->second == fif_anon_struct) {
			if(auto m = find_existing_type_match(it->second, subtypes,env); m != -1)
				return type_match{ m, mt_end };
			return type_match{ make_anon_struct_type(subtypes, env), mt_end };
		} else if(env.dict.type_array[it->second].type_slots != int32_t(subtypes.size())) {
			env.report_error("attempted to instantiate a type with the wrong number of parameters");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(env.dict.type_array[it->second].is_struct_template()) {
			return type_match{ instantiate_templated_struct(it->second, subtypes, env), mt_end };
		} else if(it->second == fif_ptr && env.dict.type_array[subtypes[0]].ntt_base_type != -1) {
			env.report_error("attempted to instantiate a pointer to a non-type");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(it->second == fif_array && env.dict.type_array[subtypes[1]].ntt_base_type != fif_i32) {
			env.report_error("attempted to instantiate an array with a non-integral size");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(it->second == fif_array) {
			if(auto m = find_existing_type_match(it->second, subtypes, env); m != -1)
				return type_match{ m, mt_end };
			auto array_size = std::max(int64_t(0), env.dict.type_array[subtypes[1]].ntt_data);
			return type_match{ make_array_type(subtypes[0], size_t(array_size), subtypes[1], env), mt_end };
		} else if(it->second == fif_ptr) {
			if(auto m = find_existing_type_match(it->second, subtypes, env); m != -1)
				return type_match{ m, mt_end };
			return type_match{ make_pointer_type(subtypes[0], env), mt_end };
		} else {
			env.report_error("attempted to instantiate a type without an instantiation guide");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		}
	}

	return type_match{ -1, mt_end };
}

inline int32_t resolve_type(std::string_view text, environment& env, std::vector<int32_t>* type_subs) {
	return internal_resolve_type(text, env, type_subs).type;
}

struct type_span_gen_result {
	std::vector<int32_t> type_array;
	uint32_t end_match_pos = 0;
	int32_t max_variable = -1;
};

inline type_span_gen_result internal_generate_type(std::string_view text, environment& env) {
	uint32_t mt_end = 0;
	type_span_gen_result r;

	if(text.starts_with("ptr(nil)")) {
		r.type_array.push_back(fif_opaque_ptr);
		r.end_match_pos = 8;
		return r;
	}
	for(; mt_end < text.size(); ++mt_end) {
		if(text[mt_end] == '(' || text[mt_end] == ')' || text[mt_end] == ',')
			break;
	}

	if(text.size() > 0 && text[0] == '$') {
		if(is_integer(text.data() + 1, text.data() + mt_end)) {
			auto v = parse_int(text.substr(1));
			r.max_variable = std::max(r.max_variable, v);
			r.type_array.push_back( -(v + 2) );
			r.end_match_pos = mt_end;
			return r;
		}
	}
	if(auto it = env.dict.types.find(std::string(text.substr(0, mt_end))); it != env.dict.types.end()) {
		r.type_array.push_back(it->second);
		if(mt_end >= text.size() || text[mt_end] == ',' || text[mt_end] == ')') {	// case: plain type
			r.end_match_pos = mt_end;
			return r;
		}
		//followed by type list
		++mt_end;
		r.type_array.push_back(std::numeric_limits<int32_t>::max());
		if((env.dict.type_array[it->second].flags & type::FLAG_TEMPLATE) != 0) {
			std::vector<type_span_gen_result> sub_matches;
			while(mt_end < text.size() && text[mt_end] != ')') {
				auto sub_match = internal_generate_type(text.substr(mt_end), env);
				r.max_variable = std::max(r.max_variable, sub_match.max_variable);
				sub_matches.push_back(std::move(sub_match));
				mt_end += sub_match.end_match_pos;
				if(mt_end < text.size() && text[mt_end] == ',')
					++mt_end;
			}
			auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[it->second].decomposed_types_start, size_t(env.dict.type_array[it->second].decomposed_types_count));
			for(auto v : desc) {
				if(v < -1) {
					auto index = -(v + 2);
					if(size_t(index) < sub_matches.size()) {
						r.type_array.insert(r.type_array.end(), sub_matches[index].type_array.begin(), sub_matches[index].type_array.end());
					} else {
						r.type_array.push_back(fif_nil);
					}
				} else {
					r.type_array.push_back(v);
				}
			}

		} else {
			while(mt_end < text.size() && text[mt_end] != ')') {
				auto sub_match = internal_generate_type(text.substr(mt_end), env);
				r.type_array.insert(r.type_array.end(), sub_match.type_array.begin(), sub_match.type_array.end());
				r.max_variable = std::max(r.max_variable, sub_match.max_variable);
				mt_end += sub_match.end_match_pos;
				if(mt_end < text.size() && text[mt_end] == ',')
					++mt_end;
			}
		}
		r.type_array.push_back(-1);
		if(mt_end < text.size() && text[mt_end] == ')')
			++mt_end;
	}
	r.end_match_pos = mt_end;
	if(r.max_variable == -1) {
		std::vector<int32_t> type_subs;
		auto resolved_type = resolve_span_type(std::span<int32_t const>(r.type_array.begin(), r.type_array.end()), type_subs, env);
		r.type_array.clear();
		r.type_array.push_back(resolved_type.type);
	}
	return r;
}

inline type_match resolve_span_type(std::span<int32_t const> tlist, std::vector<int32_t> const& type_subs, environment& env) {

	auto make_sub = [&](int32_t type_in) {
		if(type_in < -1) {
			auto slot = -(type_in + 2);
			if(slot < int32_t(type_subs.size()))
				return type_subs[slot];
			return -1;
		}
		return type_in;
	};

	if(tlist.size() == 0) {
		return type_match{ -1, 0 };
	}
	if(tlist.size() == 1) {
		return type_match{ make_sub(tlist[0]), 1 };
	}
	if(tlist[1] != std::numeric_limits<int32_t>::max()) {
		return type_match{ make_sub(tlist[0]), 1 };
	}
	int32_t base_type = tlist[0];
	assert(base_type >= 0);
	uint32_t mt_end = 2;

	//followed by type list
	std::vector<int32_t> subtypes;
	while(mt_end < tlist.size() && tlist[mt_end] != -1) {
		auto sub_match = resolve_span_type(tlist.subspan(mt_end), type_subs, env);
		subtypes.push_back(sub_match.type);
		mt_end += sub_match.end_match_pos;
	}
	if(mt_end < tlist.size() && tlist[mt_end] == -1)
		++mt_end;

	
	if(base_type == fif_anon_struct) {
		if(auto m = find_existing_type_match(base_type, subtypes, env); m != -1)
			return type_match{ m, mt_end };
		return type_match{ make_anon_struct_type(subtypes, env), mt_end };
	} else if(base_type == fif_ptr && !subtypes.empty() && subtypes[0] == fif_nil) {
		return type_match{ fif_opaque_ptr, mt_end };
	} else if(env.dict.type_array[base_type].is_struct_template()) {
		return type_match{ instantiate_templated_struct_full(base_type, subtypes, env), mt_end };
	} else if(base_type == fif_array && subtypes.size() == 2) {
		if(auto m = find_existing_type_match(base_type, subtypes, env); m != -1)
			return type_match{ m, mt_end };
		auto array_size = std::max(int64_t(0), env.dict.type_array[subtypes[1]].ntt_data);
		return type_match{ make_array_type(subtypes[0], size_t(array_size), subtypes[1], env), mt_end };
	} else if(base_type == fif_ptr) {
		if(auto m = find_existing_type_match(base_type, subtypes, env); m != -1)
			return type_match{ m, mt_end };
		return type_match{ make_pointer_type(subtypes[0], env), mt_end };
	} else {
		env.report_error("attempted to instantiate a type without an instantiation guide");
		env.mode = fif_mode::error;
		return type_match{ -1, mt_end };
	}
}

inline variable_match_result fill_in_variable_types(int32_t source_type, std::span<int32_t const> match_span, std::vector<int32_t>& type_subs, environment& env) { 
	if(match_span.size() == 0)
		return variable_match_result{ true, 0 };

	auto match_to_slot = [&](int32_t matching_type, int32_t matching_against) { 
		if(matching_type < -1) {
			auto slot = -(matching_type + 2);
			if(slot >= int32_t(type_subs.size())) {
				type_subs.resize(size_t(slot + 1), -2);
				type_subs[slot] = matching_against;
				return true;
			} else if(type_subs[slot] == -2 || type_subs[slot] == matching_against) {
				type_subs[slot] = matching_against;
				return true;
			} else {
				return false;
			}
		} else {
			return matching_type == matching_against;
		}
	};

	if(match_span.size() == 1) {
		return variable_match_result{ match_to_slot(match_span[0], source_type), 1 };
	}

	if(match_span[1] != std::numeric_limits<int32_t>::max()) {
		return variable_match_result{ match_to_slot(match_span[0], source_type), 1 };
	}

	uint32_t sub_offset = 2;
	
	std::span<const int32_t> destructured_source{ };
	if(source_type >= 0) {
		auto const& t = env.dict.type_array[source_type];
		destructured_source = std::span<const int32_t>(env.dict.all_stack_types.data() + t.decomposed_types_start, size_t(t.decomposed_types_count));
	}

	bool result = destructured_source.size() > 0 ? match_to_slot(match_span[0], destructured_source[0]) : false;
	uint32_t dest_offset = 1;
	while(sub_offset < match_span.size() && match_span[sub_offset] != -1) {
		auto dmatch = dest_offset < destructured_source.size() ? destructured_source[dest_offset] : -1;
		auto sub_result = fill_in_variable_types(dmatch, match_span.subspan(sub_offset), type_subs, env);
		sub_offset += sub_result.match_end;
		result &= sub_result.match_result;
		++dest_offset;
	}
	if(dest_offset < destructured_source.size())
		result = false;
	if(sub_offset < match_span.size() && match_span[sub_offset] == -1)
		++sub_offset;

	return variable_match_result{ result, sub_offset };
}

inline type_match_result match_stack_description(std::span<int32_t const> desc, state_stack& ts, environment& env, std::vector<int32_t>& type_subs) { // ret: true if function matched
	int32_t match_position = 0;
	// stack matching

	auto const ssize = int32_t(ts.main_size());
	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		if(consumed_stack_cells >= ssize)
			return type_match_result{ false, 0, 0 };
		auto match_result = fill_in_variable_types(ts.main_type_back(size_t(consumed_stack_cells)), desc.subspan(match_position), type_subs, env);
		if(!match_result.match_result)
			return type_match_result{ false, 0, 0 };
		match_position += match_result.match_end;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		//++output_stack_types;
	}
	//int32_t last_output_stack = match_position;
	++match_position; // skip -1

	// return stack matching
	auto const rsize = int32_t(ts.return_size());
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		if(consumed_rstack_cells >= rsize)
			return type_match_result{ false, 0, 0 }; 
		auto match_result = fill_in_variable_types(ts.return_type_back(size_t(consumed_rstack_cells)), desc.subspan(match_position), type_subs, env);
		if(!match_result.match_result)
			return type_match_result{ false, 0, 0 };
		match_position += match_result.match_end;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	//int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto result = resolve_span_type(desc.subspan(match_position), type_subs, env);
		match_position += result.end_match_pos;
		//++ret_added;
	}

	return type_match_result{ true, consumed_stack_cells, consumed_rstack_cells };
}


inline std::vector<int32_t> make_parameter_permutation(std::span<int32_t const> desc, state_stack const& final_state, state_stack const& initial_state) {
	std::vector<int32_t> permutation;

	int32_t match_position = 0;
	// stack matching

	int32_t parameter_count = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++parameter_count;
	}

	++match_position; // skip -1

	// output stack
	int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t rparameter_count = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++rparameter_count;
	}

	++match_position; // skip -1

	// output ret stack
	int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++ret_added;
	}

	auto ind_bytes = initial_state.main_byte_back_at(parameter_count);
	auto inr_bytes = initial_state.return_byte_back_at(rparameter_count);
	auto outd_bytes = final_state.main_byte_back_at(output_stack_types);
	auto outr_bytes = final_state.return_byte_back_at(ret_added);

	int64_t* input_dptr = (int64_t * )(initial_state.main_back_ptr_at(parameter_count));
	int64_t* input_rptr = (int64_t*)(initial_state.return_back_ptr_at(rparameter_count));
	int64_t* output_dptr = (int64_t*)(final_state.main_back_ptr_at(output_stack_types));
	int64_t* output_rptr = (int64_t*)(final_state.return_back_ptr_at(ret_added));

	permutation.resize((outd_bytes + outr_bytes) / 8, -1);

	uint32_t pinsert_index = 0;

	for(uint32_t i =  0; i < outd_bytes / 8; ++i) {
		for(uint32_t j = 0; j < ind_bytes/8; ++j) {
			if(input_dptr[j] == output_dptr[i]) {
				permutation[pinsert_index] = j;
				break;
			}
		}
		if(permutation[pinsert_index] == -1) {
			for(uint32_t j = 0; j < inr_bytes / 8; ++j) {
				if(input_rptr[j] == output_dptr[i]) {
					permutation[pinsert_index] = j + ind_bytes / 8;
					break;
				}
			}
		}
		++pinsert_index;
	}
	for(uint32_t i = 0; i < outr_bytes / 8; ++i) {
		for(uint32_t j = 0; j < ind_bytes / 8; ++j) {
			if(input_dptr[j] == output_rptr[i]) {
				permutation[pinsert_index] = j;
				break;
			}
		}
		if(permutation[pinsert_index] == -1) {
			for(uint32_t j = 0; j < inr_bytes / 8; ++j) {
				if(input_rptr[j] == output_rptr[i]) {
					permutation[pinsert_index] = j + ind_bytes / 8;
					break;
				}
			}
		}
		++pinsert_index;
	}
	while(!permutation.empty() && permutation.back() == -1)
		permutation.pop_back();

	return permutation;
}

inline void apply_stack_description(std::span<int32_t const> desc, state_stack& ts, std::vector<int32_t> const& param_perm, environment& env) { // ret: true if function matched
	// stack matching
	int32_t match_position = 0;
	std::vector<int32_t> type_subs;
	std::vector<int64_t> params;

	auto const ssize = int32_t(ts.main_size());
	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		assert(consumed_stack_cells < ssize);
		auto match_result = fill_in_variable_types(ts.main_type_back(consumed_stack_cells), desc.subspan(match_position), type_subs, env);
		assert(match_result.match_result);
		match_position += match_result.match_end;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
	}
	//int32_t last_output_stack = match_position;
	++match_position; // skip -1

	// return stack matching
	auto const rsize = int32_t(ts.return_size());
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		assert(consumed_rstack_cells < rsize);
		auto match_result = fill_in_variable_types(ts.return_type_back(consumed_rstack_cells), desc.subspan(match_position), type_subs, env);
		assert(match_result.match_result);
		match_position += match_result.match_end;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	{
		auto ind_bytes = ts.main_byte_back_at(consumed_stack_cells);
		auto inr_bytes = ts.return_byte_back_at(consumed_rstack_cells);
		int64_t* input_dptr = (int64_t*)(ts.main_back_ptr_at(consumed_stack_cells));
		int64_t* input_rptr = (int64_t*)(ts.return_back_ptr_at(consumed_rstack_cells));
		params.insert(params.end(), input_dptr, input_dptr + ind_bytes / 8);
		params.insert(params.end(), input_rptr, input_rptr + inr_bytes / 8);
	}

	// drop consumed types
	ts.resize(ssize - consumed_stack_cells, rsize - consumed_rstack_cells);

	int32_t outd_bytes = 0;
	int32_t outr_bytes = 0;
	int32_t outm_slots = 0;
	int32_t outr_slots = 0;

	// add returned stack types
	while(first_output_stack < int32_t(desc.size()) && desc[first_output_stack] != -1) {
		auto result = resolve_span_type(desc.subspan(first_output_stack), type_subs, env);
		first_output_stack += result.end_match_pos;
		if(env.mode != fif_mode::compiling_bytecode)
			ts.push_back_main(vsize_obj(result.type, env.dict.type_array[result.type].cell_size * 8));
		else
			ts.push_back_main(vsize_obj(result.type, 0));
		outd_bytes += env.dict.type_array[result.type].cell_size * 8;
		++outm_slots;
	}

	// output ret stack
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto result = resolve_span_type(desc.subspan(match_position), type_subs, env);
		match_position += result.end_match_pos;
		if(env.mode != fif_mode::compiling_bytecode)
			ts.push_back_return(vsize_obj(result.type, env.dict.type_array[result.type].cell_size * 8));
		else
			ts.push_back_main(vsize_obj(result.type, 0));
		outr_bytes += env.dict.type_array[result.type].cell_size * 8;
		++outr_slots;
	}

	if(env.mode != fif_mode::compiling_bytecode) {
		int64_t* output_dptr = (int64_t*)(ts.main_back_ptr_at(outm_slots));
		int64_t* output_rptr = (int64_t*)(ts.return_back_ptr_at(outr_slots));

		int32_t pinsert_index = 0;
		for(int32_t i = 0; i < outd_bytes / 8; ++i) {
			if(pinsert_index >= int32_t(param_perm.size()) || param_perm[pinsert_index] == -1) {
				output_dptr[i] = env.new_ident();
			} else {
				output_dptr[i] = params[param_perm[pinsert_index]];
			}
			++pinsert_index;
		}
		for(int32_t i = 0; i < outr_bytes / 8; ++i) {
			if(pinsert_index >= int32_t(param_perm.size()) || param_perm[pinsert_index] == -1) {
				output_rptr[i] = env.new_ident();
			} else {
				output_rptr[i] = params[param_perm[pinsert_index]];
			}
			++pinsert_index;
		}
	}
}


struct stack_consumption {
	int32_t stack;
	int32_t rstack;
};
inline stack_consumption get_stack_consumption(int32_t word, int32_t alternative, environment& env) {
	int32_t count_desc = 0;
	int32_t desc_offset = 0;
	if(std::holds_alternative<interpreted_word_instance>(env.dict.all_instances[alternative])) {
		count_desc = std::get<interpreted_word_instance>(env.dict.all_instances[alternative]).stack_types_count;
		desc_offset = std::get<interpreted_word_instance>(env.dict.all_instances[alternative]).stack_types_start;
	} else if(std::holds_alternative<compiled_word_instance>(env.dict.all_instances[alternative])) {
		count_desc = std::get<compiled_word_instance>(env.dict.all_instances[alternative]).stack_types_count;
		desc_offset = std::get<compiled_word_instance>(env.dict.all_instances[alternative]).stack_types_start;
	}

	std::span<const int> desc = std::span<const int>(env.dict.all_stack_types.data() + desc_offset, size_t(count_desc));

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
	}

	++match_position; // skip -1

	// return stack matching
	
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++consumed_rstack_cells;
	}

	return stack_consumption{ consumed_stack_cells, consumed_rstack_cells };
}

struct word_match_result {
	bool matched = false;
	int32_t stack_consumed = 0;
	int32_t ret_stack_consumed = 0;
	int32_t word_index = 0;
	int32_t substitution_version = 0;
};

inline word_match_result match_word(word const& w, state_stack& ts, std::vector<word_types> const& all_instances, std::vector<int32_t> const& all_stack_types, environment& env) {
	std::vector<int32_t> type_subs;
	for(uint32_t i = 0; i < w.instances.size(); ++i) {
		if(std::holds_alternative<interpreted_word_instance>(all_instances[w.instances[i]])) {
			interpreted_word_instance const& s = std::get<interpreted_word_instance>(all_instances[w.instances[i]]);
			auto mr = match_stack_description(std::span<int32_t const>(all_stack_types.data() + s.stack_types_start, size_t(s.stack_types_count)), ts, env, type_subs);
			if(mr.matched) {
				return word_match_result{ mr.matched, mr.stack_consumed, mr.ret_stack_consumed, w.instances[i], 0 };
			}
			type_subs.clear();
		} else if(std::holds_alternative<compiled_word_instance>(all_instances[w.instances[i]])) {
			compiled_word_instance const& s = std::get<compiled_word_instance>(all_instances[w.instances[i]]);
			auto mr = match_stack_description(std::span<int32_t const>(all_stack_types.data() + s.stack_types_start, size_t(s.stack_types_count)), ts, env, type_subs);
			if(mr.matched) {
				return word_match_result{ mr.matched, mr.stack_consumed, mr.ret_stack_consumed, w.instances[i], 0 };
			}
			type_subs.clear();
		}
	}
	return word_match_result{ false, 0, 0, 0, 0 };
}

inline word_match_result get_basic_type_match(int32_t word_index, state_stack& current_type_state, environment& env, std::vector<int32_t>& specialize_t_subs, bool ignore_specializations);

inline void execute_fif_word(state_stack& ss, uint16_t* ptr, environment& env) {
	enter_function_call(nullptr, env);
	while(ptr) {
		auto fn = env.dict.builtin_array[*ptr];
		ptr = fn(ss, ptr  + 1, &env);
#ifdef HEAP_CHECKS
		assert(_CrtCheckMemory());
#endif
	}
}

inline void execute_fif_word(interpreted_word_instance& wi, state_stack& ss, environment& env) {
	uint16_t* ptr = wi.compiled_offset >= 0 ? env.compiled_bytes.memory_at(wi.compiled_offset) : nullptr;
	if(ptr)
		execute_fif_word(ss, ptr, env);
}
inline void execute_fif_word(compiled_word_instance& wi, state_stack& ss, environment& env) {
	env.dict.builtin_array[wi.implementation_index](ss, nullptr, &env);
}


inline uint16_t* immediate_i32(state_stack& s, uint16_t* p, environment*) {
	int32_t data = *((int32_t*)p);
	s.push_back_main<int32_t>(fif_i32, data);
	return p + 2;
}
inline uint16_t* immediate_f32(state_stack& s, uint16_t* p, environment*) {
	float data = *((float*)p);
	s.push_back_main<float>(fif_f32, data);
	return p + 2;
}
inline uint16_t* immediate_bool(state_stack& s, uint16_t* p, environment*) {
	bool data = *p != 0;
	s.push_back_main<bool>(fif_bool, data != 0);
	return p + 1;
}
inline uint16_t* dup_cimple(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	auto sz = s.main_byte_back_at(1);
	auto dat = s.main_back_ptr_at(1);
	auto t = s.main_type_back(0);
	s.push_back_main(t, sz, dat);
	return p;
}
inline uint16_t* dup(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		c_append(e->compiler_stack.back()->bytecode_compilation_progress(), e->dict.get_builtin(dup_cimple));
	}
	s.mark_used_from_main(2);

	auto sz = s.main_byte_back_at(1);
	auto dat = s.main_back_ptr_at(1);
	auto t = s.main_type_back(0);
	s.push_back_main(t, sz, dat);

	return p;
}
inline uint16_t* f_init_copy(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p;

	s.mark_used_from_main(2);
	return p;
}
inline uint16_t* do_local_ind_copy(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	int32_t offset = *(p);
	auto source_type = s.main_type_back(0);

	auto dest = e->interpreter_stack_space.get() + e->frame_offset + offset;
	auto source_ptr = s.main_back_ptr_at(1);
	unsigned char* content_ptr = nullptr;
	memcpy(&content_ptr, source_ptr, sizeof(void*));
	memcpy(dest, content_ptr, size_t(e->dict.type_array[source_type].byte_size));
	s.push_back_main(source_type, uint32_t(sizeof(void*)), (unsigned char*)(&dest));

	return p + 1;
}
inline uint16_t* f_copy(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p;
	auto t = s.main_type_back(0);
	if(e->dict.type_array[t].is_memory_type() == false) {
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		execute_fif_word(fif::parse_result{ "init-copy", false }, *e, false);
		return p;
	}

	if(e->mode == fif::fif_mode::compiling_bytecode) {
		auto offset_pos = e->lexical_stack.back().allocated_bytes;
		e->lexical_stack.back().allocated_bytes += e->dict.type_array[t].byte_size;
		e->compiler_stack.back()->increase_frame_size(e->lexical_stack.back().allocated_bytes);

		c_append(e->compiler_stack.back()->bytecode_compilation_progress(), e->dict.get_builtin(do_local_ind_copy));
		c_append(e->compiler_stack.back()->bytecode_compilation_progress(), uint16_t(offset_pos));

		s.push_back_main(vsize_obj(t, 0));
	} else if(e->mode == fif_mode::interpreting) {
		// TODO: interpreter alloca
		assert(false);
	} else if(e->mode == fif::fif_mode::compiling_llvm) {
		auto ltype = llvm_type(t, *e);
		auto new_expr = e->compiler_stack.back()->build_alloca(ltype);
		auto source_o = s.popr_main();

		LLVMBuildMemCpy(e->llvm_builder, new_expr, 1, source_o.as<LLVMValueRef>(), 1, LLVMSizeOf(ltype));

		s.push_back_main(source_o);
		s.push_back_main(vsize_obj(t, new_expr, vsize_obj::by_value{ }));
	} else {
		s.mark_used_from_main(2);
		auto t = s.main_type_back(0);
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}

	execute_fif_word(fif::parse_result{ "init-copy", false }, *e, false);
	

	return p;
}
inline uint16_t* drop_cimple(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	s.pop_main();
	return p;
}
inline uint16_t* drop(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		c_append(e->compiler_stack.back()->bytecode_compilation_progress(), e->dict.get_builtin(drop_cimple));
		s.pop_main();
	} else if(e->mode == fif::fif_mode::compiling_llvm) {
		s.pop_main();
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
	}
	return p;
}
inline uint16_t* nop1(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		s.mark_used_from_main(1);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p;
}
inline void do_immediate_i32(state_stack& s, int32_t value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(e->mode == fif_mode::compiling_bytecode) {
		c_append(compile_bytes, e->dict.get_builtin(immediate_i32));
		c_append(compile_bytes, value);
		s.push_back_main(vsize_obj(fif_i32, 0));
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true);
		s.push_back_main<LLVMValueRef>(fif_i32, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main<int32_t>(fif_i32, value);
	} else if(typechecking_mode(e->mode)) {
#ifdef USE_LLVM
		s.push_back_main<int64_t>(fif_i32, int64_t(LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true)));
#else
		s.push_back_main<int64_t>(fif_f32, e->new_ident());
#endif
	}
}
inline uint16_t* immediate_type(state_stack& s, uint16_t* p, environment* e) {
	int32_t data = *((int32_t*)p);
	if(data >= 0 && e->dict.type_array[data].ntt_base_type != -1) {
		s.push_back_main(vsize_obj(e->dict.type_array[data].ntt_base_type, e->dict.type_array[e->dict.type_array[data].ntt_base_type].byte_size, (unsigned char*)( &(e->dict.type_array[data].ntt_data))));
	} else {
		s.push_back_main<int32_t>(fif_type, data);
	}
	return p + 2;
}
inline void do_immediate_type(state_stack& s, int32_t value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(e->mode == fif_mode::compiling_bytecode) {
		c_append(compile_bytes, e->dict.get_builtin(immediate_type));
		c_append(compile_bytes, value);
		s.push_back_main(vsize_obj(fif_type, 0));
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(value >= 0 && e->dict.type_array[value].ntt_base_type != -1) {
			LLVMValueRef val = nullptr;
			switch(e->dict.type_array[value].ntt_base_type) {
				case fif_i32:
					val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_f32:
				{
					float fval = 0.0f;
					memcpy(&fval, &(e->dict.type_array[value].ntt_data), 4);
					val = LLVMConstReal(LLVMFloatTypeInContext(e->llvm_context), fval);
					break;
				}
				case fif_bool:
					val = LLVMConstInt(LLVMInt1TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_type:
					val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_i64:
					val = LLVMConstInt(LLVMInt64TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_f64:
				{
					double fval = 0.0f;
					memcpy(&fval, &(e->dict.type_array[value].ntt_data), 8);
					val = LLVMConstReal(LLVMDoubleTypeInContext(e->llvm_context), fval);
					break;
				}
				case fif_u32:
					val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_u64:
					val = LLVMConstInt(LLVMInt64TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_i16:
					val = LLVMConstInt(LLVMInt16TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_u16:
					val = LLVMConstInt(LLVMInt16TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_i8:
					val = LLVMConstInt(LLVMInt8TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				case fif_u8:
					val = LLVMConstInt(LLVMInt8TypeInContext(e->llvm_context), uint64_t(e->dict.type_array[value].ntt_data), false); break;
				default:
					assert(false);
					break;
			}
			s.push_back_main<LLVMValueRef>(e->dict.type_array[value].ntt_base_type, val);
		} else {
			LLVMValueRef val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true);
			s.push_back_main<LLVMValueRef>(fif_type, val);
		}
#endif
	} else if(e->mode == fif_mode::interpreting) {
		if(value >= 0 && e->dict.type_array[value].ntt_base_type != -1) {
			s.push_back_main(vsize_obj(e->dict.type_array[value].ntt_base_type, e->dict.type_array[e->dict.type_array[value].ntt_base_type].byte_size, (unsigned char*)(&(e->dict.type_array[value].ntt_data))));
		} else {
			s.push_back_main<int32_t>(fif_type, value);
		}
	} else if(typechecking_mode(e->mode)) {
		if(value >= 0 && e->dict.type_array[value].ntt_base_type != -1) {
			s.push_back_main<int64_t>(e->dict.type_array[value].ntt_base_type, int64_t(&(e->dict.type_array[value])));
		} else {
			s.push_back_main<int64_t>(fif_type, int64_t(&(e->dict.type_array[value])));
		}
	}
}
inline void do_immediate_bool(state_stack& s, bool value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(e->mode == fif_mode::compiling_bytecode) {
		c_append(compile_bytes, e->dict.get_builtin(immediate_bool));
		c_append(compile_bytes, uint16_t(value ? 1 : 0));
		s.push_back_main(vsize_obj(fif_bool, 0));
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstInt(LLVMInt1TypeInContext(e->llvm_context), uint32_t(value), true);
		s.push_back_main(fif_bool, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main<bool>(fif_bool, value);
	} else if(typechecking_mode(e->mode)) {
#ifdef USE_LLVM
		s.push_back_main<int64_t>(fif_bool, int64_t(LLVMConstInt(LLVMInt1TypeInContext(e->llvm_context), uint32_t(value), true)));
#else
		s.push_back_main<int64_t>(fif_bool, e->new_ident());
#endif
	}
}
inline void do_immediate_f32(state_stack& s, float value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(e->mode == fif_mode::compiling_bytecode) {
		c_append(compile_bytes, e->dict.get_builtin(immediate_f32));
		c_append(compile_bytes, value);
		s.push_back_main(vsize_obj(fif_f32, 0));
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstReal(LLVMFloatTypeInContext(e->llvm_context), value);
		s.push_back_main(fif_f32, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main<float>(fif_f32, value);
	} else if(typechecking_mode(e->mode)) {
#ifdef USE_LLVM
		s.push_back_main<int64_t>(fif_f32, int64_t(LLVMConstReal(LLVMFloatTypeInContext(e->llvm_context), value)));
#else
		s.push_back_main<int64_t>(fif_f32, e->new_ident());
#endif
	}
}
inline uint16_t* function_return(state_stack& s, uint16_t* p, environment* e) {
	if(e->interpreter_call_stack.empty())
		return nullptr;
	else
		return leave_function_call(*e);
}

inline bool trivial_drop(int32_t type, environment& env) {
	auto& t = env.dict.type_array[type];
	if(t.is_memory_type() || t.is_pointer())
		return true;
	if(t.is_array()) {
		auto atype = env.dict.all_stack_types[t.decomposed_types_start + 1];
		return trivial_drop(atype, env);
	} else if(t.is_struct()) {
		std::vector<int32_t> called_tsub_types;
		auto drop_index = env.dict.words.find(std::string("drop"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(drop_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(env.dict.word_array[match.substitution_version].treat_as_base == false) {
			if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(drop)) {
				return true;
			}
			return false;
		}
		for(int32_t j = 1; j < t.decomposed_types_count - t.non_member_types; ++j) {
			auto st = env.dict.all_stack_types[t.decomposed_types_start + j];
			if(!trivial_drop(st, env))
				return false;
		}
		return true;
	} else {
		std::vector<int32_t> called_tsub_types;
		auto drop_index = env.dict.words.find(std::string("drop"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(drop_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(drop)) {
				return true;
		}
		return false;
	}
}
inline bool trivial_dup(int32_t type, environment& env) {
	auto& t = env.dict.type_array[type];
	if(t.is_memory_type() || t.is_pointer())
		return true;
	if(t.is_array()) {
		auto atype = env.dict.all_stack_types[t.decomposed_types_start + 1];
		return trivial_dup(atype, env);
	} else if(t.is_struct()) {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("dup"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(env.dict.word_array[match.substitution_version].treat_as_base == false) {
			if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(dup)) {
				return true;
			}
			return false;
		}
		for(int32_t j = 1; j < t.decomposed_types_count - t.non_member_types; ++j) {
			auto st = env.dict.all_stack_types[t.decomposed_types_start + j];
			if(!trivial_dup(st, env))
				return false;
		}
		return true;
	} else {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("dup"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(dup)) {
			return true;
		}
		return false;
	}
}
inline bool trivial_copy(int32_t type, environment& env) {
	auto& t = env.dict.type_array[type];
	if(t.is_memory_type() || t.is_pointer())
		return true;
	if(t.is_array()) {
		auto atype = env.dict.all_stack_types[t.decomposed_types_start + 1];
		return trivial_copy(atype, env);
	} else if(t.is_struct()) {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("init-copy"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(env.dict.word_array[match.substitution_version].treat_as_base == false) {
			if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(f_init_copy)) {
				return true;
			}
			return false;
		}
		for(int32_t j = 1; j < t.decomposed_types_count - t.non_member_types; ++j) {
			auto st = env.dict.all_stack_types[t.decomposed_types_start + j];
			if(!trivial_copy(st, env))
				return false;
		}
		return true;
	} else {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("init-copy"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(f_init_copy)) {
			return true;
		}
		return false;
	}
}
inline bool trivial_init(int32_t type, environment& env) {
	auto& t = env.dict.type_array[type];
	if(t.is_pointer())
		return true;
	if(t.is_array()) {
		auto atype = env.dict.all_stack_types[t.decomposed_types_start + 1];
		return trivial_init(atype, env);
	} else if(t.is_struct()) {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("init"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(env.dict.word_array[match.substitution_version].treat_as_base == false) {
			if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(nop1)) {
				return true;
			}
			return false;
		}
		for(int32_t j = 1; j < t.decomposed_types_count - t.non_member_types; ++j) {
			auto st = env.dict.all_stack_types[t.decomposed_types_start + j];
			if(!trivial_init(st, env))
				return false;
		}
		return true;
	} else {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("init"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(nop1)) {
			return true;
		}
		return false;
	}
}
inline bool trivial_finish(int32_t type, environment& env) {
	auto& t = env.dict.type_array[type];
	if(t.is_pointer()) {
		if(type == fif_opaque_ptr)
			return true;
		auto ptr_to_type = env.dict.all_stack_types[env.dict.type_array[type].decomposed_types_start + 1];
		return trivial_drop(ptr_to_type, env);
	} else if(t.is_array()) {
		auto atype = env.dict.all_stack_types[t.decomposed_types_start + 1];
		return trivial_finish(atype, env);
	} else if(t.is_struct()) {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("finish"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(env.dict.word_array[match.substitution_version].treat_as_base == false) {
			if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(nop1)) {
				return true;
			}
			return false;
		}
		for(int32_t j = 1; j < t.decomposed_types_count - t.non_member_types; ++j) {
			auto st = env.dict.all_stack_types[t.decomposed_types_start + j];
			if(!trivial_finish(st, env))
				return false;
		}
		return true;
	} else {
		std::vector<int32_t> called_tsub_types;
		auto w_index = env.dict.words.find(std::string("finish"))->second;
		state_stack temp;
		temp.push_back_main(vsize_obj(type, 0));
		auto match = get_basic_type_match(w_index, temp, env, called_tsub_types, false);

		if(match.matched == false)
			return false;
		if(match.word_index != -1 && std::holds_alternative<compiled_word_instance>(env.dict.all_instances[match.word_index]) && std::get<compiled_word_instance>(env.dict.all_instances[match.word_index]).implementation_index == env.dict.get_builtin(nop1)) {
			return true;
		}
		return false;
	}
}



#ifdef USE_LLVM
struct function_type_info {
	LLVMTypeRef fn_type;
	LLVMTypeRef out_ptr_type;
	uint32_t ret_param_index;
};
inline function_type_info llvm_function_type_from_desc(environment& env, std::span<int32_t const> desc, std::vector<int32_t> const& param_perm) {
	std::vector<LLVMTypeRef> parameter_dgroup;
	std::vector<LLVMTypeRef> parameter_rgroup;
	std::vector<LLVMTypeRef> return_group;
	std::vector<LLVMTypeRef> temp_buffer;

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	//int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		temp_buffer.clear();
		enum_llvm_type(desc[match_position], temp_buffer, env);
		parameter_dgroup.insert(parameter_dgroup.begin(), temp_buffer.begin(), temp_buffer.end());
		++match_position;
	}

	++match_position; // skip -1

	// output stack
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
	}
	++match_position; // skip -1

	// return stack matching
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		temp_buffer.clear();
		enum_llvm_type(desc[match_position], temp_buffer, env);
		parameter_rgroup.insert(parameter_rgroup.begin(), temp_buffer.begin(), temp_buffer.end());
		++match_position;
	}

	++match_position; // skip -1

	// output ret stack
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
	}

	assert(return_group.size() >= param_perm.size());

	parameter_dgroup.insert(parameter_dgroup.end(), parameter_rgroup.begin(), parameter_rgroup.end());
	for(auto pi = param_perm.size(); pi-- > 0; ) {
		if(param_perm[pi] != -1) {
			return_group.erase(return_group.begin() + pi);
		}
	}
	LLVMTypeRef ret_type = nullptr;
	LLVMTypeRef extra_ret_stype = nullptr;
	if(return_group.size() == 0) {
		ret_type = LLVMVoidTypeInContext(env.llvm_context);
	} else if(return_group.size() == 1) {
		ret_type = return_group[0];
	} else {
		for(auto& t : return_group) {
			if(t == llvm_type(fif_bool, env)) {
				t = LLVMInt8TypeInContext(env.llvm_context);
			}
		}
		ret_type = LLVMStructTypeInContext(env.llvm_context, return_group.data(), 2, true);
		if(return_group.size() >= 3) {
			parameter_dgroup.push_back(LLVMPointerTypeInContext(env.llvm_context, 0));
			if(return_group.size() == 3) {
				extra_ret_stype = return_group[2];
			} else if(return_group.size() > 3) {
				extra_ret_stype = LLVMStructTypeInContext(env.llvm_context, return_group.data() + 2, uint32_t(return_group.size() - 2), true);
			}
		}
	}
	return function_type_info{ LLVMFunctionType(ret_type, parameter_dgroup.data(), uint32_t(parameter_dgroup.size()), false), extra_ret_stype, uint32_t(parameter_dgroup.size() - 1) };
}

inline LLVMValueRef llvm_make_function_parameters(environment& env, LLVMValueRef fn, bool return_parameter, state_stack& ws, std::span<int32_t const> desc) {

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	int32_t parameter_count = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++parameter_count;
	}

	++match_position; // skip -1

	// output stack
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t rparameter_count = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		++rparameter_count;
	}

	++match_position; // skip -1

	// output ret stack
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}

	auto ind_bytes = ws.main_byte_back_at(parameter_count);
	auto inr_bytes = ws.return_byte_back_at(rparameter_count);

	LLVMValueRef* input_dptr = (LLVMValueRef*)(ws.main_back_ptr_at(parameter_count));
	LLVMValueRef* input_rptr = (LLVMValueRef*)(ws.return_back_ptr_at(rparameter_count));

	uint32_t pindex = 0;
	for(uint32_t i = 0; i < ind_bytes / 8; ++i) {
		input_dptr[i] = LLVMGetParam(fn, uint32_t(pindex));
		++pindex;
	}
	for(uint32_t i = 0; i < inr_bytes / 8; ++i) {
		input_rptr[i] = LLVMGetParam(fn, uint32_t(pindex));
		++pindex;
	}

	ws.trim_to(size_t(parameter_count), size_t(rparameter_count));

	if(return_parameter) {
		return LLVMGetParam(fn, uint32_t(pindex));
	} else {
		return nullptr;
	}
}

struct brief_fn_return {
	LLVMTypeRef composite_type = nullptr;
	int32_t num_stack_values;
	int32_t num_rstack_values;
	bool is_struct_type;
};

inline brief_fn_return llvm_function_return_type_from_desc(environment& env, std::span<int32_t const> desc, std::vector<int32_t> const& param_perm) {
	std::vector<LLVMTypeRef> return_group;

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	//int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	//int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}

	++match_position; // skip -1

	int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
		++ret_added;
	}

	assert(return_group.size() >= param_perm.size());
	for(auto pi = param_perm.size(); pi-- > 0; ) {
		if(param_perm[pi] != -1) {
			return_group.erase(return_group.begin() + pi);
		}
	}

	LLVMTypeRef ret_type = nullptr;
	if(return_group.size() == 0) {
		// ret_type = LLVMVoidTypeInContext(env.llvm_context);
	} else if(return_group.size() == 1) {
		ret_type = return_group[0];
	} else {
		for(auto& t : return_group) {
			if(t == llvm_type(fif_bool, env)) {
				t = LLVMInt8TypeInContext(env.llvm_context);
			}
		}
		ret_type = LLVMStructTypeInContext(env.llvm_context, return_group.data(), uint32_t(return_group.size()), true);
	}
	return brief_fn_return{ ret_type , output_stack_types, ret_added, return_group.size()  > 1};
}

inline void llvm_make_function_return(environment& env, LLVMValueRef return_param, std::span<int32_t const> desc, std::vector<int32_t> const& param_perm) {
	std::vector<LLVMTypeRef> return_group;
	std::vector<LLVMValueRef> returns_vals;

	auto* ws = env.compiler_stack.back()->working_state();

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}

	++match_position; // skip -1

	// output stack
	int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
	}

	++match_position; // skip -1

	int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
		++ret_added;
	}

	auto outd_bytes = ws->main_byte_back_at(output_stack_types);
	auto outr_bytes = ws->return_byte_back_at(ret_added);

	LLVMValueRef* output_dptr = (LLVMValueRef*)(ws->main_back_ptr_at(output_stack_types));
	LLVMValueRef* output_rptr = (LLVMValueRef*)(ws->return_back_ptr_at(ret_added));
	returns_vals.insert(returns_vals.end(), output_dptr, output_dptr + outd_bytes / 8);
	returns_vals.insert(returns_vals.end(), output_rptr, output_rptr + outr_bytes / 8);

	assert(return_group.size() >= param_perm.size());
	assert(return_group.size() == returns_vals.size());

	for(auto pi = param_perm.size(); pi-- > 0; ) {
		if(param_perm[pi] != -1) {
			return_group.erase(return_group.begin() + pi);
			returns_vals.erase(returns_vals.begin() + pi);
		}
	}

	LLVMTypeRef ret_type = nullptr;
	LLVMTypeRef extra_ret_stype = nullptr;

	if(return_group.size() == 0) {
		
	} else if(return_group.size() == 1) {
		ret_type = return_group[0];
	} else {
		for(auto& t : return_group) {
			if(t == llvm_type(fif_bool, env)) {
				t = LLVMInt8TypeInContext(env.llvm_context);
			}
		}
		ret_type = LLVMStructTypeInContext(env.llvm_context, return_group.data(), 2, true);
		if(return_group.size() > 3) {
			extra_ret_stype = LLVMStructTypeInContext(env.llvm_context, return_group.data() + 2, uint32_t(return_group.size() - 2), true);
		}
	}

	if(ret_type == nullptr) {
		LLVMBuildRetVoid(env.llvm_builder);
		return;
	}
	if(return_group.size() == 1) {
		LLVMBuildRet(env.llvm_builder, returns_vals[0]);
		return;
	}

	auto rstruct = LLVMGetUndef(ret_type);
	uint32_t insert_index = 0;

	for(uint32_t i = 0; i < 2; ++i) {
		assert(return_group[i] == LLVMTypeOf(returns_vals[i]));
		if(LLVMTypeOf(returns_vals[i]) == llvm_type(fif_bool, env)) {
			rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, LLVMBuildZExt(env.llvm_builder, returns_vals[i], LLVMInt8TypeInContext(env.llvm_context), ""), i, "");
		} else {
			rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, returns_vals[i], i, "");
		}
	}
	for(uint32_t i = 2; i < returns_vals.size(); ++i) {
		auto ptr = returns_vals.size() > 3 ? LLVMBuildStructGEP2(env.llvm_builder, extra_ret_stype, return_param, uint32_t(i - 2), "") : return_param;
		if(LLVMTypeOf(returns_vals[i]) == llvm_type(fif_bool, env)) {
			LLVMBuildStore(env.llvm_builder, LLVMBuildZExt(env.llvm_builder, returns_vals[i], LLVMInt8TypeInContext(env.llvm_context), ""), ptr);
		} else {
			LLVMBuildStore(env.llvm_builder, returns_vals[i], ptr);
		}
	}

	LLVMBuildRet(env.llvm_builder, rstruct);
	return;
}

inline void llvm_make_function_call(environment& env, interpreted_word_instance& wi, std::span<int32_t const> desc) {
	state_stack& ts = *(env.compiler_stack.back()->working_state());

	// stack matching
	int32_t match_position = 0;
	std::vector<int32_t> type_subs;
	std::vector<LLVMValueRef> params;
	std::vector<LLVMTypeRef> return_group;

	auto const ssize = int32_t(ts.main_size());
	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		assert(consumed_stack_cells < ssize);
		auto match_result = fill_in_variable_types(ts.main_type_back(consumed_stack_cells), desc.subspan(match_position), type_subs, env);
		assert(match_result.match_result);
		match_position += match_result.match_end;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		match_position += next_encoded_stack_type(desc.subspan(match_position));
	}
	//int32_t last_output_stack = match_position;
	++match_position; // skip -1

	// return stack matching
	auto const rsize = int32_t(ts.return_size());
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		assert(consumed_rstack_cells < rsize);
		auto match_result = fill_in_variable_types(ts.return_type_back(consumed_rstack_cells), desc.subspan(match_position), type_subs, env);
		assert(match_result.match_result);
		match_position += match_result.match_end;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	auto ret_stack_start = match_position;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		enum_llvm_type(desc[match_position], return_group, env);
		++match_position;
	}


	{
		auto ind_bytes = ts.main_byte_back_at(consumed_stack_cells);
		auto inr_bytes = ts.return_byte_back_at(consumed_rstack_cells);

		LLVMValueRef* input_dptr = (LLVMValueRef*)(ts.main_back_ptr_at(consumed_stack_cells));
		LLVMValueRef* input_rptr = (LLVMValueRef*)(ts.return_back_ptr_at(consumed_rstack_cells));

		params.insert(params.end(), input_dptr, input_dptr + ind_bytes / 8);
		params.insert(params.end(), input_rptr, input_rptr + inr_bytes / 8);
	}

	// drop consumed types
	ts.resize(size_t(ssize - consumed_stack_cells), size_t(rsize - consumed_rstack_cells));

	assert(wi.llvm_function);
	auto ftype_info = llvm_function_type_from_desc(env, desc, wi.llvm_parameter_permutation);
	LLVMValueRef return_alloc = nullptr;
	if(ftype_info.out_ptr_type) {
		return_alloc = env.compiler_stack.back()->build_alloca(ftype_info.out_ptr_type);
		params.push_back(return_alloc);
	}
	auto retvalue = LLVMBuildCall2(env.llvm_builder, ftype_info.fn_type, wi.llvm_function, params.data(), uint32_t(params.size()), "");
	LLVMSetInstructionCallConv(retvalue, LLVMCallConv::LLVMFastCallConv);

	int32_t outd_bytes = 0;
	int32_t outr_bytes = 0;
	int32_t outm_slots = 0;
	int32_t outr_slots = 0;

	// add returned stack types
	while(first_output_stack < int32_t(desc.size()) && desc[first_output_stack] != -1) {
		auto result = resolve_span_type(desc.subspan(first_output_stack), type_subs, env);
		first_output_stack += result.end_match_pos;
		ts.push_back_main(vsize_obj(result.type, env.dict.type_array[result.type].cell_size * 8));
		outd_bytes += env.dict.type_array[result.type].cell_size * 8;
		++outm_slots;
	}

	// output ret stack
	// add new types
	//int32_t ret_added = 0;
	while(ret_stack_start < int32_t(desc.size()) && desc[ret_stack_start] != -1) {
		auto result = resolve_span_type(desc.subspan(ret_stack_start), type_subs, env);
		ret_stack_start += result.end_match_pos;
		ts.push_back_return(vsize_obj(result.type, env.dict.type_array[result.type].cell_size * 8));
		outr_bytes += env.dict.type_array[result.type].cell_size * 8;
		++outr_slots;
	}

	LLVMValueRef* output_dptr = (LLVMValueRef*)(ts.main_back_ptr_at(outm_slots));
	LLVMValueRef* output_rptr = (LLVMValueRef*)(ts.return_back_ptr_at(outr_slots));

	for(auto pi = wi.llvm_parameter_permutation.size(); pi-- > 0; ) {
		if(wi.llvm_parameter_permutation[pi] != -1) {
			return_group.erase(return_group.begin() + pi);
		}
	}

	bool multiple_returns = return_group.size() > 1;

	int32_t pinsert_index = 0;
	uint32_t grabbed_o_index = 0;

	for(int32_t i = 0; i < outd_bytes / 8; ++i) {
		if(pinsert_index >= int32_t(wi.llvm_parameter_permutation.size()) || wi.llvm_parameter_permutation[pinsert_index] == -1) {
			if(multiple_returns) {
				if(grabbed_o_index < 2) {
					if(return_group[grabbed_o_index] == llvm_type(fif_bool, env)) {
						output_dptr[i] = LLVMBuildTrunc(env.llvm_builder, LLVMBuildExtractValue(env.llvm_builder, retvalue, grabbed_o_index, ""), LLVMInt1TypeInContext(env.llvm_context), "");
					} else {
						output_dptr[i] = LLVMBuildExtractValue(env.llvm_builder, retvalue, grabbed_o_index, "");
					}
				} else {
					LLVMValueRef load_ptr = nullptr;
					if(return_group.size() == 3) {
						load_ptr = return_alloc;
					} else {
						load_ptr = LLVMBuildStructGEP2(env.llvm_builder, ftype_info.out_ptr_type, return_alloc, uint32_t(grabbed_o_index - 2), "");
					}
					if(return_group[grabbed_o_index] == llvm_type(fif_bool, env)) {
						output_dptr[i] = LLVMBuildTrunc(env.llvm_builder, LLVMBuildLoad2(env.llvm_builder, LLVMInt8TypeInContext(env.llvm_context), load_ptr, ""), LLVMInt1TypeInContext(env.llvm_context), "");
					} else {
						output_dptr[i] = LLVMBuildLoad2(env.llvm_builder, return_group[grabbed_o_index], load_ptr, "");
					}
				}
			} else {
				output_dptr[i] = retvalue;
			}
			++grabbed_o_index;
		} else {
			output_dptr[i] = params[wi.llvm_parameter_permutation[pinsert_index]];
		}
		++pinsert_index;
	}
	for(int32_t i = 0; i < outr_bytes / 8; ++i) {
		if(pinsert_index >= int32_t(wi.llvm_parameter_permutation.size()) || wi.llvm_parameter_permutation[pinsert_index] == -1) {
			if(multiple_returns) {
				if(grabbed_o_index < 2) {
					if(return_group[grabbed_o_index] == llvm_type(fif_bool, env)) {
						output_rptr[i] = LLVMBuildTrunc(env.llvm_builder, LLVMBuildExtractValue(env.llvm_builder, retvalue, grabbed_o_index, ""), LLVMInt1TypeInContext(env.llvm_context), "");
					} else {
						output_rptr[i] = LLVMBuildExtractValue(env.llvm_builder, retvalue, grabbed_o_index, "");
					}
				} else {
					LLVMValueRef load_ptr = nullptr;
					if(return_group.size() == 3) {
						load_ptr = return_alloc;
					} else {
						load_ptr = LLVMBuildStructGEP2(env.llvm_builder, ftype_info.out_ptr_type, load_ptr, uint32_t(grabbed_o_index - 2), "");
					}
					if(return_group[grabbed_o_index] == llvm_type(fif_bool, env)) {
						output_rptr[i] = LLVMBuildTrunc(env.llvm_builder, LLVMBuildLoad2(env.llvm_builder, LLVMInt8TypeInContext(env.llvm_context), load_ptr, ""), LLVMInt1TypeInContext(env.llvm_context), "");
					} else {
						output_rptr[i] = LLVMBuildLoad2(env.llvm_builder, return_group[grabbed_o_index], load_ptr, "");
					}
				}
			} else {
				output_rptr[i] = retvalue;
			}
			++grabbed_o_index;
		} else {
			output_rptr[i] = params[wi.llvm_parameter_permutation[pinsert_index]];
		}
		++pinsert_index;
	}
}
#endif

inline std::vector<int32_t> expand_stack_description(state_stack& initial_stack_state, std::span<const int32_t> desc, int32_t stack_consumed, int32_t rstack_consumed);
inline bool compare_stack_description(std::span<const int32_t> a, std::span<const int32_t> b);


inline void load_from_llvm_pointer(int32_t struct_type, state_stack& ws, LLVMValueRef ptr_expression, environment& env) {
#ifdef USE_LLVM
	switch(struct_type) {
		case fif_i32:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_f32:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_bool:
		{
			auto expr = LLVMBuildTrunc(env.llvm_builder, LLVMBuildLoad2(env.llvm_builder, LLVMInt8TypeInContext(env.llvm_context), ptr_expression, ""), LLVMInt1TypeInContext(env.llvm_context), "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_type:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_i64:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_f64:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_u32:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_u64:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_i16:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_u16:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_i8:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type,expr);
			break;
		}
		case fif_u8:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_nil:
			break;
		case fif_ptr:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_opaque_ptr:
		{
			auto expr = LLVMBuildLoad2(env.llvm_builder, llvm_type(struct_type, env), ptr_expression, "");
			ws.push_back_main(struct_type, expr);
			break;
		}
		case fif_struct:
			break;
		case fif_anon_struct:
			break;
		case fif_stack_token:
			break;
		case fif_array:
			break;
		default:
			if(env.dict.type_array[struct_type].is_struct_template()) {
				break;
			} else if(env.dict.type_array[struct_type].stateless()) {
				break;
			} else if(env.dict.type_array[struct_type].is_struct()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					auto expr = LLVMBuildLoad2(env.llvm_builder, LLVMPointerTypeInContext(env.llvm_context, 0), ptr_expression, "");
					ws.push_back_main(struct_type, expr);
					break;
				} else {
					auto current_ds_size = ws.main_size();
					auto current_ds_byte = ws.main_byte_size();

					auto ltype = llvm_type(struct_type, env);
					for(int32_t j = 1; j < env.dict.type_array[struct_type].decomposed_types_count - env.dict.type_array[struct_type].non_member_types; ++j) {
						auto st = env.dict.all_stack_types[env.dict.type_array[struct_type].decomposed_types_start + j];
						if(env.dict.type_array[st].stateless() == false) {
							// recurse
							auto sub_ptr = LLVMBuildStructGEP2(env.llvm_builder, ltype, ptr_expression, uint32_t(j - 1), "");
							load_from_llvm_pointer(st, ws, sub_ptr, env);
						}
					}

					assert(size_t(env.dict.type_array[struct_type].cell_size * 8) == ws.main_byte_size() - current_ds_byte);
					vsize_obj condensed_struct(struct_type, uint32_t(env.dict.type_array[struct_type].cell_size * 8), ws.main_ptr_at(current_ds_size));
					ws.resize(current_ds_size, ws.return_size());
					ws.push_back_main(condensed_struct);
				}
			} else if(env.dict.type_array[struct_type].is_pointer()) {
				auto expr = LLVMBuildLoad2(env.llvm_builder, LLVMPointerTypeInContext(env.llvm_context, 0), ptr_expression, "");
				ws.push_back_main(struct_type, expr);
				break;
			} else if(env.dict.type_array[struct_type].is_array()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					auto expr = LLVMBuildLoad2(env.llvm_builder, LLVMPointerTypeInContext(env.llvm_context, 0), ptr_expression, "");
					ws.push_back_main(struct_type, expr);
					break;
				} else {
					assert(false);
				}
			} else {
				assert(false);
			}
	}
#endif
}

inline void store_to_llvm_pointer(int32_t struct_type, state_stack& ws, LLVMValueRef ptr_expression, environment& env) {
#ifdef USE_LLVM
	switch(struct_type) {
		case fif_i32:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_f32:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_bool:
		{
			auto extended = LLVMBuildZExt(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), LLVMInt8TypeInContext(env.llvm_context), "");
			LLVMBuildStore(env.llvm_builder, extended, ptr_expression);
			break;
		}
		case fif_type:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_i64:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_f64:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_u32:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_u64:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_i16:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_u16:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_i8:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_u8:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_nil:
			break;
		case fif_ptr:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_opaque_ptr:
		{
			LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
			break;
		}
		case fif_struct:
			break;
		case fif_anon_struct:
			break;
		case fif_stack_token:
			break;
		case fif_array:
			break;
		default:
			if(env.dict.type_array[struct_type].is_struct_template()) {
				break;
			} else if(env.dict.type_array[struct_type].stateless()) {
				break;
			} else if(env.dict.type_array[struct_type].is_struct()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
					break;
				} else {
					auto current_ds_size = ws.main_size();
					auto current_ds_byte = ws.main_byte_size();

					auto ltype = llvm_type(struct_type, env);
					auto svalue = ws.popr_main();
					int32_t consumed_cells = 0;

					for(int32_t j = 1; j < env.dict.type_array[struct_type].decomposed_types_count - env.dict.type_array[struct_type].non_member_types; ++j) {
						auto st = env.dict.all_stack_types[env.dict.type_array[struct_type].decomposed_types_start + j];
						if(env.dict.type_array[st].stateless() == false) {
							// recurse
							ws.push_back_main(vsize_obj(st, env.dict.type_array[st].cell_size * 8, svalue.data() + consumed_cells * 8));
							auto sub_ptr = LLVMBuildStructGEP2(env.llvm_builder, ltype, ptr_expression, uint32_t(j - 1), "");
							store_to_llvm_pointer(st, ws, sub_ptr, env);

							consumed_cells += env.dict.type_array[st].cell_size;
						}
					}
				}
			} else if(env.dict.type_array[struct_type].is_pointer()) {
				LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
				break;
			} else if(env.dict.type_array[struct_type].is_array()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
					break;
				} else {
					assert(false);
				}
			} else {
				assert(false);
			}
	}
#endif
}

inline void store_difference_to_llvm_pointer(int32_t struct_type, state_stack& ws, vsize_obj original, LLVMValueRef ptr_expression, environment& env) {
#ifdef USE_LLVM
	switch(struct_type) {
		case fif_i32:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_f32:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_bool:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>()) {
				auto extended = LLVMBuildZExt(env.llvm_builder, ex, LLVMInt8TypeInContext(env.llvm_context), "");
				LLVMBuildStore(env.llvm_builder, extended, ptr_expression);
			}
			break;
		}
		case fif_type:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_i64:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_f64:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_u32:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_u64:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_i16:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_u16:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_i8:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_u8:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_nil:
			break;
		case fif_ptr:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_opaque_ptr:
		{
			if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
				LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
			break;
		}
		case fif_struct:
			break;
		case fif_anon_struct:
			break;
		case fif_stack_token:
			break;
		case fif_array:
			break;
		default:
			if(env.dict.type_array[struct_type].is_struct_template()) {
				break;
			} else if(env.dict.type_array[struct_type].stateless()) {
				break;
			} else if(env.dict.type_array[struct_type].is_struct()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
						LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
					break;
				} else {
					auto current_ds_size = ws.main_size();
					auto current_ds_byte = ws.main_byte_size();

					auto ltype = llvm_type(struct_type, env);
					auto svalue = ws.popr_main();
					int32_t consumed_cells = 0;

					for(int32_t j = 1; j < env.dict.type_array[struct_type].decomposed_types_count - env.dict.type_array[struct_type].non_member_types; ++j) {
						auto st = env.dict.all_stack_types[env.dict.type_array[struct_type].decomposed_types_start + j];
						if(env.dict.type_array[st].stateless() == false) {
							// recurse
							ws.push_back_main(vsize_obj(st, env.dict.type_array[st].cell_size * 8, svalue.data() + consumed_cells * 8));
							vsize_obj old_subobj(st, env.dict.type_array[st].cell_size * 8, original.data() + consumed_cells * 8);
							auto sub_ptr = LLVMBuildStructGEP2(env.llvm_builder, ltype, ptr_expression, uint32_t(j - 1), "");
							store_difference_to_llvm_pointer(st, ws, old_subobj, sub_ptr, env);

							consumed_cells += env.dict.type_array[st].cell_size;
						}
					}
				}
			} else if(env.dict.type_array[struct_type].is_pointer()) {
				if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
					LLVMBuildStore(env.llvm_builder, ws.popr_main().as<LLVMValueRef>(), ptr_expression);
				break;
			} else if(env.dict.type_array[struct_type].is_array()) {
				if(env.dict.type_array[struct_type].is_memory_type()) {
					if(auto ex = ws.popr_main().as<LLVMValueRef>(); ex != original.as<LLVMValueRef>())
						LLVMBuildStore(env.llvm_builder, ex, ptr_expression);
					break;
				} else {
					assert(false);
				}
			} else {
				assert(false);
			}
	}
#endif
}

inline uint16_t* call_function(state_stack& s, uint16_t* p, environment* env) {
	uint16_t* new_addr = env->compiled_bytes.memory_at(*((int32_t*)p));
	enter_function_call(p + 2, *env);
	return new_addr;
}
inline uint16_t* tail_call_function(state_stack& s, uint16_t* p, environment* env) {
	uint16_t* new_addr = env->compiled_bytes.memory_at(*((int32_t*)p));
	auto retval = leave_function_call(*env);
	enter_function_call(retval, *env);
	return new_addr;
}
inline uint16_t* enter_function(state_stack& s, uint16_t* p, environment* env) {
	env->frame_offset -= *p;
	return p + 1;
}

class mode_switch_scope : public opaque_compiler_data {
public:
	opaque_compiler_data* interpreted_link = nullptr;
	fif_mode entry_mode;
	fif_mode running_mode;

	mode_switch_scope(opaque_compiler_data* p, environment& env, fif_mode running_mode) : opaque_compiler_data(p), running_mode(running_mode) {
		[&]() {
			for(auto i = env.compiler_stack.size(); i-- > 0; ) {
				if(env.compiler_stack[i]->get_type() == control_structure::mode_switch) {
					mode_switch_scope* p = static_cast<mode_switch_scope*>(env.compiler_stack[i].get());
					if(p->running_mode == running_mode) {
						++i;
						for(; i < env.compiler_stack.size(); ++i) {
							if(env.compiler_stack[i]->get_type() == control_structure::mode_switch) {
								interpreted_link = env.compiler_stack[i - 1].get();
								return;
							}
						}
						interpreted_link = env.compiler_stack[env.compiler_stack.size() - 1].get();
						return;
					}
				}
			}
		}();
		entry_mode = env.mode;
		env.mode = running_mode;
	}

	virtual control_structure get_type()override {
		return control_structure::mode_switch;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record() override {
		return parent ? parent->typecheck_record() : nullptr;
	}
	virtual state_stack* working_state() override {
		return interpreted_link ? interpreted_link->working_state() : nullptr;
	}

	virtual void set_working_state(state_stack&& p) override {
		if(interpreted_link)
			interpreted_link->set_working_state(std::move(p));
	}
	virtual bool finish(environment& env)override {
		if(env.mode != fif_mode::error) {
			env.mode = entry_mode;
		}
		return true;
	}
	virtual LLVMValueRef build_alloca(LLVMTypeRef type) override {
		return interpreted_link ? interpreted_link->build_alloca(type) : nullptr;
	}
};
inline void switch_compiler_stack_mode(environment& env, fif_mode new_mode) {
	auto m = std::make_unique<mode_switch_scope>(env.compiler_stack.back().get(), env, new_mode);
	env.compiler_stack.emplace_back(std::move(m));
}
inline void restore_compiler_stack_mode(environment& env) {
	if(env.compiler_stack.empty() || env.compiler_stack.back()->get_type() != control_structure::mode_switch) {
		env.report_error("attempt to switch mode back revealed an unbalanced compiler stack");
		env.mode = fif_mode::error;
		return;
	}
	env.compiler_stack.back()->finish(env);
	env.compiler_stack.pop_back();
}

class outer_interpreter : public opaque_compiler_data {
public:
	state_stack interpreter_state;
	std::vector<unsigned char> lvar_store;

	outer_interpreter() : opaque_compiler_data(nullptr) {
	}
	virtual control_structure get_type() override {
		return control_structure::none;
	}
	virtual state_stack* working_state()override {
		return &interpreter_state;
	}
	virtual void set_working_state(state_stack&& p) override {
		interpreter_state = std::move(p);
	}
	virtual bool finish(environment& env)override {
		return true;
	}
};
class typecheck3_record_holder : public opaque_compiler_data {
public:
	ankerl::unordered_dense::map<uint64_t, typecheck_3_record> tr;

	typecheck3_record_holder(opaque_compiler_data* p, environment& env) : opaque_compiler_data(p) {
	}

	virtual control_structure get_type() override {
		return control_structure::none;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record()override {
		return &tr;
	}
	virtual bool finish(environment& env)override {
		return true;
	}
};


inline std::vector<int32_t> expand_stack_description(state_stack& initial_stack_state, std::span<const int32_t> desc, int32_t stack_consumed, int32_t rstack_consumed) {
	int32_t match_position = 0;
	// stack matching

	std::vector<int32_t> result;

	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		++consumed_stack_cells;
	}
	if(consumed_stack_cells < stack_consumed) {
		for(int32_t i = consumed_stack_cells; i < stack_consumed; ++i) {
			result.push_back(initial_stack_state.main_type_back(i));
		}
	}

	if(consumed_stack_cells < stack_consumed || (match_position < int32_t(desc.size()) && desc[match_position] == -1)) {
		result.push_back(-1);
	}
	++match_position; // skip -1

	if(consumed_stack_cells < stack_consumed) { // add additional touched inputs to the end
		for(int32_t i = stack_consumed - 1; i >= consumed_stack_cells; --i) {
			result.push_back(initial_stack_state.main_type_back(i));
		}
	}

	int32_t first_output_stack = match_position;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
	}

	if(rstack_consumed > 0 || (match_position < int32_t(desc.size()) && desc[match_position] == -1)) {
		result.push_back(-1);
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		++consumed_rstack_cells;
	}
	if(consumed_rstack_cells < rstack_consumed) {
		for(int32_t i = consumed_rstack_cells; i < rstack_consumed; ++i) {
			result.push_back(initial_stack_state.return_type_back(i));
		}
	}

	if(consumed_rstack_cells < rstack_consumed || (match_position < int32_t(desc.size()) && desc[match_position] == -1)) {
		result.push_back(-1);
	}
	++match_position; // skip -1

	if(consumed_rstack_cells < rstack_consumed) { // add additional touched inputs to the end
		for(int32_t i = rstack_consumed - 1; i >= consumed_rstack_cells; --i) {
			result.push_back(initial_stack_state.return_type_back(i));
		}
	}

	// add new types
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
	}

	return result;
}

inline bool compare_stack_description(std::span<const int32_t> a, std::span<const int32_t> b) {
	if(a.size() != b.size())
		return false;
	for(auto i = a.size(); i-- > 0; ) {
		if(a[i] != b[i])
			return false;
	}
	return true;
}

inline bool stack_types_match(state_stack& a, state_stack& b) {
	if(a.main_size() != b.main_size() || a.return_size() != b.return_size()) {
		return false;
	}
	for(size_t i = a.main_size();  i-- > 0; ) {
		if(a.main_type(i) != b.main_type(i))
			return false;
	}
	for(size_t i = a.return_size();  i-- > 0; ) {
		if(a.return_type(i) != b.return_type(i))
			return false;
	}
	return true;
}

inline uint16_t* conditional_jump(state_stack& s, uint16_t* p, environment* env) {
	auto value = s.popr_main().as<bool>();
	if(!value)
		return p + *((int32_t*)p);
	else
		return p + 2;
}
inline uint16_t* reverse_conditional_jump(state_stack& s, uint16_t* p, environment* env) {
	auto value = s.popr_main().as<bool>();
	if(value)
		return p + *((int32_t*)p);
	else
		return p + 2;
}
inline uint16_t* unconditional_jump(state_stack& s, uint16_t* p, environment* env) {
	return p + *((int32_t*)p);
}

struct branch_source {
	state_stack stack;
	std::vector<int64_t> locals_state;
	LLVMBasicBlockRef from_block = nullptr;
	LLVMValueRef conditional_expression = nullptr;
	LLVMBasicBlockRef untaken_block = nullptr;
	size_t jump_bytecode_pos = 0;
	bool jump_if_true = false;
	bool unconditional_jump = false;
	bool write_conditional = false;
	bool speculative_branch = false;
};

enum class add_branch_result {
	ok,
	incompatible
};

struct branch_target {
	enum class node_source : int32_t {
		data_stack, return_stack, locals
	};
	struct pending_phi_node {
		LLVMValueRef vref = nullptr;
		node_source source = node_source::data_stack;
		int32_t index = 0;
	};
	std::vector< branch_source> branches_to_here;
	std::vector<pending_phi_node> phi_nodes;

	size_t bytecode_location = 0;
	LLVMBasicBlockRef block_location = nullptr;

	bool is_concrete = false;

	inline add_branch_result add_concrete_branch(branch_source&& new_branch, environment& env);
	inline add_branch_result add_cb_with_exit_pad(branch_source&& new_branch, int32_t lexical_depth, environment& env);
	inline void materialize(environment& env); // make blocks, phi nodes
	inline void finalize(environment& env); // write incoming links
};

inline add_branch_result branch_target::add_concrete_branch(branch_source&& new_branch, environment& env) {
	if(branches_to_here.size() > 0 && !stack_types_match(branches_to_here[0].stack, new_branch.stack)) {
		return add_branch_result::incompatible;
	}

	if(env.mode == fif_mode::compiling_bytecode) {
		if(new_branch.write_conditional) {
			auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();

			c_append(bcode, env.dict.get_builtin(new_branch.unconditional_jump ? unconditional_jump : (new_branch.jump_if_true ? reverse_conditional_jump : conditional_jump)));
			new_branch.jump_bytecode_pos = bcode->size();
			c_append(bcode, int32_t(0));
		}
		new_branch.speculative_branch = false;
	} else if(env.mode == fif_mode::compiling_llvm) {
		// nothing to do
		new_branch.speculative_branch = false;
	} else {
		new_branch.speculative_branch = true;
	}

	branches_to_here.emplace_back(std::move(new_branch));
	return add_branch_result::ok;
}
inline add_branch_result branch_target::add_cb_with_exit_pad(branch_source&& new_branch, int32_t lexical_depth, environment& env) {
	if(branches_to_here.size() > 0 && !stack_types_match(branches_to_here[0].stack, new_branch.stack)) {
		return add_branch_result::incompatible;
	}

	auto burrow_down_delete = [&]() { 
		for(auto j = env.lexical_stack.size(); j-- > size_t(lexical_depth); ) {
			lexical_free_scope(env.lexical_stack[j], env);
		}
	};

	if(env.mode == fif_mode::compiling_bytecode) {
		new_branch.speculative_branch = false;
	} else if(env.mode == fif_mode::compiling_llvm) {
		new_branch.speculative_branch = false;
	} else {
		new_branch.speculative_branch = true;
	}

	assert(new_branch.write_conditional);
	if(env.mode == fif_mode::compiling_bytecode) {
		auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();

		if(new_branch.unconditional_jump) {
			burrow_down_delete();

			c_append(bcode, env.dict.get_builtin(unconditional_jump));
			new_branch.jump_bytecode_pos = bcode->size();
			c_append(bcode, int32_t(0));
		} else {
			size_t forward_jump_pos = 0;
			c_append(bcode, env.dict.get_builtin(!new_branch.jump_if_true ? reverse_conditional_jump : conditional_jump));
			forward_jump_pos = bcode->size();
			c_append(bcode, int32_t(0));
			
			burrow_down_delete();

			c_append(bcode, env.dict.get_builtin(unconditional_jump));
			new_branch.jump_bytecode_pos = bcode->size();
			c_append(bcode, int32_t(0));

			auto bytecode_location = bcode->size();
			*(int32_t*)(bcode->data() + forward_jump_pos) = int32_t(bytecode_location - forward_jump_pos);
		}
	} else if(env.mode == fif_mode::compiling_llvm) {
		auto in_fn = env.compiler_stack.back()->llvm_function();

		auto continuation_branch = new_branch.untaken_block;
		auto exit_block = LLVMCreateBasicBlockInContext(env.llvm_context, "exit-pad");
		LLVMAppendExistingBasicBlock(in_fn, exit_block);

		if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
			auto old_block = *pb;
			*pb = exit_block;

			LLVMPositionBuilderAtEnd(env.llvm_builder, exit_block);
			burrow_down_delete();

			LLVMPositionBuilderAtEnd(env.llvm_builder, old_block);
			if(new_branch.untaken_block) { // conditional jump
				assert(new_branch.conditional_expression);
				if(new_branch.jump_if_true)
					LLVMBuildCondBr(env.llvm_builder, new_branch.conditional_expression, exit_block, new_branch.untaken_block);
				else
					LLVMBuildCondBr(env.llvm_builder, new_branch.conditional_expression, new_branch.untaken_block, exit_block);
			} else {  // unconditional jump
				LLVMBuildBr(env.llvm_builder, exit_block);
			}

			new_branch.from_block = exit_block;
			new_branch.conditional_expression = nullptr;
			new_branch.unconditional_jump = true;
			new_branch.untaken_block = nullptr;
			new_branch.write_conditional = true;

			if(continuation_branch) {
				*pb = continuation_branch;
				LLVMPositionBuilderAtEnd(env.llvm_builder, continuation_branch);
			} else {
				LLVMPositionBuilderAtEnd(env.llvm_builder, exit_block);
			}
		}
	}
	

	branches_to_here.emplace_back(std::move(new_branch));
	return add_branch_result::ok;
}

inline void branch_target::materialize(environment& env) {
	if(env.mode == fif_mode::compiling_bytecode) {
		auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();
		bytecode_location = bcode->size();
	} else if(typechecking_mode(env.mode) && !skip_compilation(env.mode)) {
		if(branches_to_here.size() > 0) {
			auto new_state = branches_to_here[0].stack;
			auto bsize = new_state.main_byte_size();
			int64_t* ptr = (int64_t*)(new_state.main_ptr_at(0));

			for(uint32_t i = 0; i < bsize / 8; ++i) {
				bool mismatch = false;
				auto base_expr = ptr[i];
				for(auto& b : branches_to_here) {
					int64_t* optr = (int64_t*)(b.stack.main_ptr_at(0));
					if(base_expr != optr[i]) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					ptr[i] = env.new_ident();
				}
			}

			auto rbsize = new_state.return_byte_size();
			int64_t* rptr = (int64_t*)(new_state.return_ptr_at(0));

			for(uint32_t i = 0; i < rbsize / 8; ++i) {
				bool mismatch = false;
				auto base_expr = rptr[i];
				for(auto& b : branches_to_here) {
					int64_t* optr = (int64_t*)(b.stack.return_ptr_at(0));
					if(base_expr != optr[i]) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					rptr[i] = env.new_ident();
				}
			}

			auto lsz = env.tc_local_variables.size();;
			int32_t i = 0;
			for(; i < int32_t(lsz); ++i) {
				bool mismatch = false;
				auto base_expr = i < branches_to_here[0].locals_state.size() ? branches_to_here[0].locals_state[i] : 0;
				for(auto& b : branches_to_here) {
					if(i < b.locals_state.size() && b.locals_state[i] != base_expr) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					env.tc_local_variables [i] = env.new_ident();
				} else if(i < branches_to_here[0].locals_state.size()) {
					env.tc_local_variables[i] = branches_to_here[0].locals_state[i];
				}
			}
			env.compiler_stack.back()->set_working_state(std::move(new_state));
		}
	} else if(env.mode == fif_mode::compiling_llvm && branches_to_here.size() > 0) {
		branch_source* s = nullptr;
		for(auto& b : branches_to_here) {
			if(b.speculative_branch == false) {
				s = &b;
				break;
			}
		}

		auto in_fn = env.compiler_stack.back()->llvm_function();
		block_location = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
		LLVMAppendExistingBasicBlock(in_fn, block_location);

		assert(s);
		if(auto pb = env.compiler_stack.back()->llvm_block(); pb && s) {
			*pb = block_location;
			LLVMPositionBuilderAtEnd(env.llvm_builder, block_location);
			auto new_state = s->stack;
			
			auto bsize = new_state.main_byte_size();
			LLVMValueRef* ptr = (LLVMValueRef*)(new_state.main_ptr_at(0));

			for(uint32_t i = 0; i < bsize / 8; ++i) {
				bool mismatch = false;
				auto base_expr = ptr[i];
				for(auto& b : branches_to_here) {
					LLVMValueRef* optr = (LLVMValueRef*)(b.stack.main_ptr_at(0));
					if(base_expr != optr[i]) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					auto node = LLVMBuildPhi(env.llvm_builder, LLVMTypeOf(ptr[i]), "auto-d-phi");
					phi_nodes.push_back(branch_target::pending_phi_node{ node, branch_target::node_source::data_stack, int32_t(i) });
					ptr[i] = node;
				}
			}

			auto rbsize = new_state.return_byte_size();
			LLVMValueRef* rptr = (LLVMValueRef*)(new_state.return_ptr_at(0));
			for(uint32_t i = 0; i < rbsize / 8; ++i) {
				bool mismatch = false;
				auto base_expr = rptr[i];
				for(auto& b : branches_to_here) {
					LLVMValueRef* optr = (LLVMValueRef*)(b.stack.return_ptr_at(0));
					if(base_expr != optr[i]) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					auto node = LLVMBuildPhi(env.llvm_builder, LLVMTypeOf(rptr[i]), "auto-r-phi");
					phi_nodes.push_back(branch_target::pending_phi_node{ node, branch_target::node_source::return_stack, int32_t(i) });
					rptr[i] = node;
				}
			}

			auto lsz = env.tc_local_variables.size();;
			uint32_t i = 0;
			for(; i < uint32_t(lsz); ++i) {
				bool mismatch = false;
				auto base_expr = i < s->locals_state.size() ? (LLVMValueRef)(s->locals_state[i]) : 0;
				for(auto& b : branches_to_here) {
					if(i < b.locals_state.size() && (LLVMValueRef)(b.locals_state[i]) != base_expr) {
						mismatch = true;
						break;
					}
				}
				if(mismatch) {
					auto node = LLVMBuildPhi(env.llvm_builder, LLVMTypeOf((LLVMValueRef)(env.tc_local_variables[i])), "auto-l-phi");
					phi_nodes.push_back(branch_target::pending_phi_node{ node, branch_target::node_source::locals, int32_t(i) });
					env.tc_local_variables[i] = (int64_t)(node);
				} else if(i < s->locals_state.size()) {
					env.tc_local_variables[i] = (int64_t)(s->locals_state[i]);
				}
			}
			env.compiler_stack.back()->set_working_state(std::move(new_state));
		}
	}
	is_concrete = true;
}
inline void branch_target::finalize(environment& env) {
	if(base_mode(env.mode) == fif_mode::compiling_bytecode) {
		//fill in jumps
		auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();
		assert(bcode);
		for(auto& br : branches_to_here) {
			if(br.write_conditional && !br.speculative_branch) {
				*(int32_t*)(bcode->data() + br.jump_bytecode_pos) = int32_t(bytecode_location) - int32_t(br.jump_bytecode_pos);
			}
		}
	} else if(base_mode(env.mode) == fif_mode::compiling_llvm && block_location) {
		if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
			auto current_location = *pb;

			// write to other blocks
			// write conditional jumps
			for(auto& br : branches_to_here) {
				if(br.write_conditional && !br.speculative_branch) {
					LLVMPositionBuilderAtEnd(env.llvm_builder, br.from_block);
					if(br.untaken_block) { // conditional jump
						assert(br.conditional_expression);
						if(br.jump_if_true)
							LLVMBuildCondBr(env.llvm_builder, br.conditional_expression, block_location, br.untaken_block);
						else
							LLVMBuildCondBr(env.llvm_builder, br.conditional_expression, br.untaken_block, block_location);
					} else {  // unconditional jump
						LLVMBuildBr(env.llvm_builder, block_location);
					}
				}
			}

			// write phi edges
			LLVMPositionBuilderAtEnd(env.llvm_builder, block_location);

			std::vector<LLVMValueRef> inc_vals;
			std::vector<LLVMBasicBlockRef> inc_blocks;

			inc_vals.reserve(branches_to_here.size());
			for(auto& br : branches_to_here) {
				if(!br.speculative_branch)
					inc_blocks.push_back(br.from_block);
			}
			for(auto& pn : phi_nodes) {
				switch(pn.source) {
					case branch_target::node_source::data_stack:
						inc_vals.clear();
						for(auto& br : branches_to_here) {
							if(!br.speculative_branch) {
								LLVMValueRef* ptr = (LLVMValueRef*)(br.stack.main_ptr_at(0));
								inc_vals.push_back(ptr[pn.index]);
							}
						}
						LLVMAddIncoming(pn.vref, inc_vals.data(), inc_blocks.data(), uint32_t(inc_blocks.size()));
						break;
					case branch_target::node_source::return_stack:
						inc_vals.clear();
						for(auto& br : branches_to_here) {
							if(!br.speculative_branch) {
								LLVMValueRef* ptr = (LLVMValueRef*)(br.stack.return_ptr_at(0));
								inc_vals.push_back(ptr[pn.index]);
							}
						}
						LLVMAddIncoming(pn.vref, inc_vals.data(), inc_blocks.data(), uint32_t(inc_blocks.size()));
						break;
					case branch_target::node_source::locals:
						inc_vals.clear();
						for(auto& br : branches_to_here) {
							if(!br.speculative_branch) {
								if(size_t(pn.index) < br.locals_state.size()) {
									inc_vals.push_back((LLVMValueRef)(br.locals_state[pn.index]));
								} else {
									inc_vals.push_back(LLVMGetPoison(LLVMTypeOf(pn.vref)));
								}
							}
						}
						LLVMAddIncoming(pn.vref, inc_vals.data(), inc_blocks.data(), uint32_t(inc_blocks.size()));
						break;
					default:
						break;
				}
			}

			LLVMPositionBuilderAtEnd(env.llvm_builder, current_location);
		}
	}
}

class function_scope : public opaque_compiler_data {
public:
	branch_target fn_exit;

	state_stack initial_state;
	state_stack iworking_state;
	std::vector<uint16_t> compiled_bytes;
	std::vector<int32_t> type_subs;
	std::vector< int64_t> saved_locals;
	LLVMValueRef llvm_fn = nullptr;
	LLVMBasicBlockRef current_block = nullptr;
	LLVMBasicBlockRef alloca_block = nullptr;
	LLVMBasicBlockRef first_real_block = nullptr;
	LLVMValueRef return_parameter = nullptr;

	environment& env;
	size_t locals_size_position = 0;
	int32_t for_word = -1;
	int32_t for_instance = -1;
	int32_t max_locals_size = 0;
	int32_t entry_lex_depth = 0;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);
	bool intepreter_skip_body = false;

	function_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state, int32_t for_word, int32_t for_instance) : opaque_compiler_data(p), env(e), for_word(for_word), for_instance(for_instance) {

		initial_state = entry_state.new_copy();
		iworking_state = entry_state.new_copy();
		entry_mode = env.mode;

		if(for_word == -1) {
			env.report_error("attempted to compile a function for an anonymous word");
			env.mode = fif_mode::error;
			return;
		}

		saved_locals = env.tc_local_variables;
		env.tc_local_variables.clear();
		entry_lex_depth = int32_t(env.lexical_stack.size());
		env.lexical_stack.emplace_back();
		env.lexical_stack.back().new_top_scope = true;

		if(typechecking_mode(env.mode))
			env.dict.word_array[for_word].being_typechecked = true;
		else if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			assert(for_instance != -1);
			std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).being_compiled = true;
		}
		if(env.mode == fif_mode::compiling_bytecode) {
			{
				auto jump_location = env.compiled_bytes.return_new_memory(3);
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).compiled_offset = jump_location;
				auto jump_ptr = env.compiled_bytes.memory_at(jump_location);
				jump_ptr[0] = env.dict.get_builtin(unconditional_jump);
				jump_ptr[1] = uint16_t(2);
				jump_ptr[2] = 0;
			}
			c_append(&compiled_bytes, env.dict.get_builtin(enter_function));
			locals_size_position = compiled_bytes.size();
			compiled_bytes.push_back(0);
		} else if(env.mode == fif_mode::compiling_llvm) {
			assert(for_instance != -1);
			auto fn_desc = std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_start, std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_count);

			bool has_llvm_return_parameter = false;
#ifdef USE_LLVM
			if(!std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function) {
				auto fn_string_name = word_name_from_id(for_word, env) + "#" + std::to_string(for_instance);
				auto fn_type = llvm_function_type_from_desc(env, fn_desc, std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_parameter_permutation);
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function = LLVMAddFunction(env.llvm_module, fn_string_name.c_str(), fn_type.fn_type);
				if(fn_type.out_ptr_type) {
					auto nocapattribute = LLVMCreateEnumAttribute(env.llvm_context, 23, 1);
					LLVMAddAttributeAtIndex(std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function, 1 + fn_type.ret_param_index, nocapattribute);
					//auto sret_attribute = LLVMCreateTypeAttribute(env.llvm_context, 78, fn_type.out_ptr_type);
					//LLVMAddAttributeAtIndex(std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function, 1 + fn_type.ret_param_index, sret_attribute);
					has_llvm_return_parameter = true;
				}
			}
#endif
			auto compiled_fn = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function;
			llvm_fn = compiled_fn;

#ifdef USE_LLVM
			if(std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).is_imported_function)
				LLVMSetFunctionCallConv(compiled_fn, NATIVE_CC);
			else
				LLVMSetFunctionCallConv(compiled_fn, LLVMCallConv::LLVMFastCallConv);

			LLVMSetLinkage(compiled_fn, LLVMLinkage::LLVMPrivateLinkage);
			alloca_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
			LLVMPositionBuilderAtEnd(env.llvm_builder, alloca_block);
			first_real_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "post_alloca_code");
			LLVMPositionBuilderAtEnd(env.llvm_builder, first_real_block);
			current_block = first_real_block;
			return_parameter = llvm_make_function_parameters(env, compiled_fn, has_llvm_return_parameter, iworking_state, fn_desc);
			initial_state = iworking_state;
#endif
		} else if(typechecking_mode(env.mode)) {
			{
				auto bsize = iworking_state.main_byte_size();
				int64_t* ptr = (int64_t*)(iworking_state.main_ptr_at(0));
				for(size_t i = 0; i < bsize / 8; ++i) {
					ptr[i] = env.new_ident();
				}
			}
			{
				auto bsize = iworking_state.return_byte_size();
				int64_t* ptr = (int64_t*)(iworking_state.return_ptr_at(0));
				for(size_t i = 0; i < bsize / 8; ++i) {
					ptr[i] = env.new_ident();
				}
			}
			iworking_state.min_main_depth = int32_t(iworking_state.main_size());
			iworking_state.min_return_depth = int32_t(iworking_state.return_size());
			initial_state = iworking_state;
		}

	}
	virtual void increase_frame_size(int32_t sz) override {
		max_locals_size = std::max(max_locals_size, sz);
	}
	virtual std::vector<int32_t>* type_substitutions() override {
		return &type_subs;
	}
	virtual control_structure get_type()override {
		return control_structure::function;
	}
	virtual LLVMValueRef llvm_function()override {
		return llvm_fn;
	}
	virtual LLVMBasicBlockRef* llvm_block() override {
		return &current_block;
	}
	virtual int32_t word_id()override {
		return for_word;
	}
	virtual int32_t instance_id()override {
		return for_instance;
	}
	virtual std::vector<uint16_t>* bytecode_compilation_progress()override {
		return &compiled_bytes;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record()override {
		return parent ? parent->typecheck_record() : nullptr;
	}
	virtual state_stack* working_state()override {
		return &iworking_state;
	}
	virtual void set_working_state(state_stack&& p) override {
		iworking_state = std::move(p);
	}
	void add_return() {
		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		if(env.mode == fif_mode::compiling_bytecode) {
			for(auto j = env.lexical_stack.size(); j-- > size_t(entry_lex_depth); ) {
				lexical_free_scope(env.lexical_stack[j], env);
			}

			if(compiled_bytes.size() == env.last_compiled_call.offset + 3 && env.last_compiled_call.instance == for_instance) {
				// tail call
				auto high_int = compiled_bytes.back();
				compiled_bytes.pop_back();
				auto low_int = compiled_bytes.back();
				compiled_bytes.pop_back();

				compiled_bytes.pop_back();
				c_append(&compiled_bytes, env.dict.get_builtin(tail_call_function));

				compiled_bytes.push_back(low_int);
				compiled_bytes.push_back(high_int);

				env.mode = surpress_branch(env.mode);
			} else {
				c_append(&compiled_bytes, env.dict.get_builtin(function_return));
			}
		} else if(env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			fn_exit.add_cb_with_exit_pad(branch_source{ env.compiler_stack.back()->working_state()->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, entry_lex_depth, env);
			env.mode = surpress_branch(env.mode);
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			auto wstemp = env.compiler_stack.back()->working_state();
			if(fn_exit.add_cb_with_exit_pad(branch_source{ env.compiler_stack.back()->working_state()->copy(), env.tc_local_variables, nullptr, nullptr, nullptr, 0, false, true, true, false }, entry_lex_depth, env) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			env.mode = surpress_branch(env.mode);
			return;
		} else if(env.mode == fif_mode::interpreting) {
			intepreter_skip_body = true;
			env.mode = surpress_branch(env.mode);
			return;
		}
	}
	virtual LLVMValueRef build_alloca(LLVMTypeRef type) override {
#ifdef USE_LLVM
		if(env.mode == fif_mode::compiling_llvm) {
			auto cb = *(env.compiler_stack.back()->llvm_block());
			LLVMPositionBuilderAtEnd(env.llvm_builder, alloca_block);
			auto v = LLVMBuildAlloca(env.llvm_builder, type, "");
			LLVMPositionBuilderAtEnd(env.llvm_builder, cb);
			return v;
		} else {
			return nullptr;
		}
#else
		return nullptr;
#endif
	}
	virtual bool finish(environment&)override {
		int32_t stack_consumed = int32_t(initial_state.main_size()) - int32_t(iworking_state.min_main_depth);
		int32_t rstack_consumed = int32_t(initial_state.return_size()) - int32_t(iworking_state.min_return_depth);
		int32_t stack_added = int32_t(iworking_state.main_size()) - int32_t(iworking_state.min_main_depth);
		int32_t rstack_added = int32_t(iworking_state.return_size()) - int32_t(iworking_state.min_return_depth);
		assert(stack_added >= 0);
		assert(rstack_added >= 0);
		assert(stack_consumed >= 0);
		assert(rstack_consumed >= 0);

		if(recursive(env.mode)) {
			env.mode = fail_mode(env.mode);
		}


		int32_t non_speculative_branches = 0;
		for(auto& b : fn_exit.branches_to_here) {
			if(b.speculative_branch == false)
				++non_speculative_branches;
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(fn_exit.add_cb_with_exit_pad(branch_source{ iworking_state, env.tc_local_variables, nullptr, nullptr, nullptr, 0, false, true, true, false }, entry_lex_depth, env) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			env.lexical_stack.pop_back();
			assert(env.lexical_stack.size() == size_t(entry_lex_depth));
		} else if(!skip_compilation(env.mode)) {
			if(non_speculative_branches > 0) {
				auto pb = env.compiler_stack.back()->llvm_block();
				fn_exit.add_cb_with_exit_pad(branch_source{ iworking_state, env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, entry_lex_depth, env);
				++non_speculative_branches;

				env.lexical_stack.pop_back();
				assert(env.lexical_stack.size() == size_t(entry_lex_depth));
			} else {
				lexical_end_scope(env);
				assert(env.lexical_stack.size() == size_t(entry_lex_depth));

				if(env.mode == fif_mode::compiling_bytecode && compiled_bytes.size() == env.last_compiled_call.offset + 3 && env.last_compiled_call.instance == for_instance) {
					// tail call
					auto high_int = compiled_bytes.back();
					compiled_bytes.pop_back();
					auto low_int = compiled_bytes.back();
					compiled_bytes.pop_back();

					compiled_bytes.pop_back();
					c_append(&compiled_bytes, env.dict.get_builtin(tail_call_function));

					compiled_bytes.push_back(low_int);
					compiled_bytes.push_back(high_int);
				}
			}
		}

		auto temp_mode = env.mode;
		env.mode = entry_mode;

		fn_exit.materialize(env);
		if(non_speculative_branches > 0) {
			fn_exit.finalize(env);
		}
		env.mode = temp_mode;

		if(condition_mode != fif_mode(0))
			env.mode = merge_modes(env.mode, condition_mode);

		if(typechecking_level(entry_mode) == 1 && !failed(env.mode)) {
			word& w = env.dict.word_array[for_word];

			interpreted_word_instance temp;
			auto& current_stack = iworking_state;

			temp.stack_types_start = int32_t(env.dict.all_stack_types.size());
			{
				for(int32_t i = 0; i < stack_consumed; ++i) {
					env.dict.all_stack_types.push_back(initial_state.main_type_back(i));
				}
			}
			int32_t skipped = 0;
			if(stack_added > 0) {
				env.dict.all_stack_types.push_back(-1);
				for(int32_t i = 1; i <= stack_added; ++i) {
					env.dict.all_stack_types.push_back(current_stack.main_type_back(stack_added - i));
				}
			} else {
				++skipped;
			}
			if(rstack_consumed > 0) {
				for(; skipped > 0; --skipped)
					env.dict.all_stack_types.push_back(-1);
				env.dict.all_stack_types.push_back(-1);
				for(int32_t i = 0; i < rstack_consumed; ++i) {
					env.dict.all_stack_types.push_back(initial_state.return_type_back(i));
				}
			} else {
				++skipped;
			}
			if(rstack_added > 0) {
				for(; skipped > 0; --skipped)
					env.dict.all_stack_types.push_back(-1);
				env.dict.all_stack_types.push_back(-1);
				for(int32_t i = 1; i <= rstack_added; ++i) {
					env.dict.all_stack_types.push_back(current_stack.return_type_back(rstack_added - i));
				}
			}
			temp.stack_types_count = int32_t(env.dict.all_stack_types.size()) - temp.stack_types_start;
			temp.typechecking_level = provisional_success(env.mode) ? 1 : 3;
			if(!provisional_success(env.mode)) {
				temp.llvm_parameter_permutation = make_parameter_permutation(std::span<const int32_t>(env.dict.all_stack_types.data() + temp.stack_types_start, size_t(temp.stack_types_count)), iworking_state, initial_state);
			}

			if(for_instance == -1) {
				w.instances.push_back(int32_t(env.dict.all_instances.size()));
				env.dict.all_instances.push_back(std::move(temp));
			} else if(std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).typechecking_level == 0) {
				env.dict.all_instances[for_instance] = std::move(temp);
			}
			env.dict.word_array[for_word].being_typechecked = false;
		} else if(typechecking_level(entry_mode) == 2 && !failed(env.mode)) {
			if(for_instance == -1) {
				env.report_error("tried to level 2 typecheck a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}
			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			std::span<const int32_t> existing_description = std::span<const int32_t>(env.dict.all_stack_types.data() + wi.stack_types_start, size_t(wi.stack_types_count));
			auto revised_description = expand_stack_description(initial_state, existing_description, stack_consumed, rstack_consumed);

			if(!compare_stack_description(existing_description, std::span<const int32_t>(revised_description.data(), revised_description.size()))) {
				wi.stack_types_start = int32_t(env.dict.all_stack_types.size());
				wi.stack_types_count = int32_t(revised_description.size());
				env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), revised_description.begin(), revised_description.end());
			}

			wi.typechecking_level = std::max(wi.typechecking_level, provisional_success(env.mode) ? int8_t(2) : int8_t(3));
			if(!provisional_success(env.mode)) {
				wi.llvm_parameter_permutation = make_parameter_permutation(revised_description, iworking_state, initial_state);
			}
			env.dict.word_array[for_word].being_typechecked = false;
		} else if(typechecking_level(entry_mode) == 3 && !failed(env.mode)) {
			// no alterations -- handled explicitly by invoking lvl 3
			env.dict.word_array[for_word].being_typechecked = false;
		} else if(base_mode(entry_mode) == fif_mode::compiling_bytecode) {
			if(for_instance == -1) {
				env.report_error("tried to compile a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}
			
			compiled_bytes[locals_size_position] = uint16_t(max_locals_size);
			c_append(&compiled_bytes, env.dict.get_builtin(function_return));
			
			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			
			
			if(wi.compiled_offset != -1) {
				if(env.compiled_bytes.used_mem() == uint32_t(wi.compiled_offset + 3)) {
					env.compiled_bytes.return_new_memory(compiled_bytes.size() - 3);
					auto new_ptr = env.compiled_bytes.memory_at(wi.compiled_offset);
					memcpy(new_ptr, compiled_bytes.data(), compiled_bytes.size() * 2);
				} else {
					auto new_off = env.compiled_bytes.return_new_memory(compiled_bytes.size());
					auto new_ptr = env.compiled_bytes.memory_at(new_off);
					memcpy(new_ptr, compiled_bytes.data(), compiled_bytes.size() * 2);
					auto diff = new_off - wi.compiled_offset;
					auto old_jump = env.compiled_bytes.memory_at(wi.compiled_offset);
					*(int32_t*)(old_jump + 1)= diff;
					wi.compiled_offset = new_off;
				}
			} else {
				auto new_off = env.compiled_bytes.return_new_memory(compiled_bytes.size());
				auto new_ptr = env.compiled_bytes.memory_at(new_off);
				memcpy(new_ptr, compiled_bytes.data(), compiled_bytes.size() * 2);
				wi.compiled_offset = new_off;
			}

			if(for_instance != -1)
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).being_compiled = false;
		} else if(base_mode(entry_mode) == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			if(for_instance == -1) {
				env.report_error("tried to compile a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}

			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			std::span<const int32_t> existing_description = std::span<const int32_t>(env.dict.all_stack_types.data() + wi.stack_types_start, size_t(wi.stack_types_count));

			llvm_make_function_return(env, return_parameter, existing_description, wi.llvm_parameter_permutation);
			wi.llvm_compilation_finished = true;

			LLVMPositionBuilderAtEnd(env.llvm_builder, alloca_block);
			LLVMBuildBr(env.llvm_builder, first_real_block);

			if(LLVMVerifyFunction(wi.llvm_function, LLVMVerifierFailureAction::LLVMPrintMessageAction)) {

				std::string mod_contents = LLVMPrintModuleToString(env.llvm_module);

				char* message = nullptr;
				LLVMVerifyModule(env.llvm_module, LLVMReturnStatusAction, &message);

				env.report_error("LLVM verification of function failed");
				env.mode = fif_mode::error;
				return true;
			}
			if(for_instance != -1)
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).being_compiled = false;
#endif
		} else if(typechecking_mode(entry_mode)) {
			env.dict.word_array[for_word].being_typechecked = false;
		} else {
			env.report_error("reached end of compilation in unexpected mode");
			env.mode = fif_mode::error;
		}
		env.tc_local_variables = saved_locals;
		return true;
	}
};
class conditional_scope : public opaque_compiler_data {
public:
	state_stack initial_state;
	std::vector<int64_t> entry_locals_state;

	branch_target scope_end;
	branch_target else_target;
	environment& env;

	int32_t ds_depth = 0;
	int32_t rs_depth = 0;
	int32_t lexical_depth = 0;

	fif_mode entry_mode;
	fif_mode first_branch_end_mode = fif_mode(0);
	bool interpreter_first_branch = false;
	bool interpreter_second_branch = false;
	bool typechecking_provisional_on_first_branch = false;

	conditional_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : opaque_compiler_data(p), env(e) {
		if(entry_state.main_size() == 0 || entry_state.main_type_back(0) != fif_bool) {
			env.report_error("attempted to start an if without a boolean value on the stack");
			env.mode = fif_mode::error;
		}
		entry_mode = env.mode;
		entry_locals_state = env.tc_local_variables;
		lexical_depth  = int32_t(env.lexical_stack.size());

		if(env.mode == fif_mode::interpreting) {
			if(entry_state.popr_main().as<bool>()) {
				interpreter_first_branch = true;
				interpreter_second_branch = false;
			} else {
				interpreter_first_branch = false;
				interpreter_second_branch = true;
				env.mode = surpress_branch(env.mode);
			}
		} else if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			auto branch_condition = entry_state.popr_main().as<LLVMValueRef>();

			if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
				auto parent_block = *pb;
				
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-first-branch");
				LLVMAppendExistingBasicBlock(env.compiler_stack.back()->llvm_function(), *pb);
				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, parent_block, branch_condition, *pb, 0, false, false, true, false }, env);
			}
#endif
		} else if(env.mode == fif_mode::compiling_bytecode) {
			entry_state.pop_main();
			else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true, false}, env);
		} else if(!skip_compilation(env.mode)) {
			entry_state.pop_main();
			else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true, false }, env);
		}

		if(env.mode != fif_mode::interpreting) {
			lexical_new_scope(false, env);
		}

		ds_depth = int32_t(entry_state.min_main_depth);
		rs_depth = int32_t(entry_state.min_return_depth);

		initial_state = entry_state;
	}
	void and_if() {
		auto wstate = env.compiler_stack.back()->working_state();

		if(env.mode == fif_mode::interpreting) {
			if(wstate->popr_main().as<bool>()) {
				interpreter_first_branch = true;
				interpreter_second_branch = false;
			} else {
				interpreter_first_branch = false;
				interpreter_second_branch = true;
				env.mode = surpress_branch(env.mode);
			}
			return;
		}

		if(else_target.is_concrete) {
			else_target = branch_target{ };
			entry_mode = env.mode;
			entry_locals_state = env.tc_local_variables;

			if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
				auto branch_condition = wstate->popr_main().as<LLVMValueRef>();

				if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
					auto parent_block = *pb;

					*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-first-branch");
					LLVMAppendExistingBasicBlock(env.compiler_stack.back()->llvm_function(), *pb);
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

					else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, parent_block, branch_condition, *pb, 0, false, false, true, false }, env);
				}
#endif
			} else if(env.mode == fif_mode::compiling_bytecode) {
				wstate->pop_main();
				else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true, false }, env);
			} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					env.mode = fail_typechecking(env.mode);
					return;
				}
				wstate->pop_main();
				else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true, false }, env);
			}
		} else {
			if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
				auto pb = env.compiler_stack.back()->llvm_block();
				if(wstate->main_size() == 0) {
					env.report_error("&if with inappropriate value");
					env.mode = fif_mode::error;
					return;
				}
				if(wstate->main_type_back(0) != fif_bool) {
					env.report_error("&if with inappropriate value");
					env.mode = fif_mode::error;
					return;
				}

				auto branch_condition = wstate->popr_main().as<LLVMValueRef>();

				LLVMBasicBlockRef continuation = nullptr;
#ifdef USE_LLVM
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation = LLVMCreateBasicBlockInContext(env.llvm_context, "continuation");
					LLVMAppendExistingBasicBlock(in_fn, continuation);
				}
#endif
				else_target.add_cb_with_exit_pad(branch_source{ wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, branch_condition, continuation, 0, false, false, true, false }, lexical_depth, env);

			} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					env.mode = fail_typechecking(env.mode);
					return;
				}
				wstate->pop_main();
				if(else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true, false }, env) != add_branch_result::ok) {
					env.mode = fail_typechecking(env.mode);
					return;
				}
			}
		}

	}
	void commit_first_branch(environment&) {
		if(else_target.is_concrete) {
			env.report_error("attempted to compile multiple else conditions");
			env.mode = fif_mode::error;
			return;
		}

		if(interpreter_second_branch && !failed(env.mode)) {
			env.mode = entry_mode;
			return;
		} else if(interpreter_first_branch) {
			env.mode = surpress_branch(env.mode);
			return;
		}
		
		auto pb = env.compiler_stack.back()->llvm_block();

		lexical_end_scope(env);
		assert(int32_t(env.lexical_stack.size()) == lexical_depth);
		lexical_new_scope(false, env);

		if(!skip_compilation(env.mode)) {
			auto wstate = env.compiler_stack.back()->working_state();
			ds_depth = std::min(ds_depth, wstate->min_main_depth);
			rs_depth = std::min(rs_depth, wstate->min_return_depth);
			scope_end.add_concrete_branch(branch_source{ *wstate, env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, env);
		}

		env.compiler_stack.back()->set_working_state(initial_state.copy());
		env.tc_local_variables = entry_locals_state;

		if(first_branch_end_mode == fif_mode(0))
			first_branch_end_mode = env.mode;
		else
			first_branch_end_mode = merge_modes(env.mode, first_branch_end_mode);

		env.mode = entry_mode;

		else_target.materialize(env);
		else_target.finalize(env);
	}
	virtual control_structure get_type()override {
		return control_structure::str_if;
	}
	virtual bool finish(environment&)override {
		if(interpreter_second_branch || interpreter_first_branch) {
			env.mode = fif_mode::interpreting;
		}
		if(env.mode == fif_mode::interpreting) {
			return true;
		}

		lexical_end_scope(env);
		assert(int32_t(env.lexical_stack.size()) == lexical_depth);
		
		auto pb = env.compiler_stack.back()->llvm_block();

		if(!else_target.is_concrete) {
			if(!skip_compilation(env.mode)) {
				scope_end.add_concrete_branch(branch_source{ env.compiler_stack.back()->working_state()->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, env);
			}

			if(first_branch_end_mode == fif_mode(0))
				first_branch_end_mode = env.mode;
			else
				first_branch_end_mode = merge_modes(env.mode, first_branch_end_mode);

			env.mode = entry_mode;

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			env.tc_local_variables = entry_locals_state;

			else_target.materialize(env);
			else_target.finalize(env);
		}
		

		if(!skip_compilation(env.mode)) {
			auto branch_valid = scope_end.add_concrete_branch(branch_source{ env.compiler_stack.back()->working_state()->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, env);
			if(branch_valid != add_branch_result::ok) {
				env.mode = fail_mode(env.mode);
			}
		}

		env.mode = merge_modes(env.mode, first_branch_end_mode);
		env.tc_local_variables = entry_locals_state;

		scope_end.materialize(env);
		scope_end.finalize(env);

		auto wstate = env.compiler_stack.back()->working_state();
		wstate->min_main_depth = std::min(wstate->min_main_depth, ds_depth);
		wstate->min_return_depth = std::min(wstate->min_return_depth, rs_depth);

		return true;
	}
};

class while_loop_scope : public opaque_compiler_data {
public:
	state_stack initial_state;
	std::vector<int64_t> entry_locals_state;

	branch_target loop_start;
	branch_target loop_exit;
	environment& env;

	std::string_view entry_source;
	int32_t lex_depth = 0;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);

	bool phi_pass = false;
	bool intepreter_skip_body = false;
	bool saw_conditional = false;


	while_loop_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : opaque_compiler_data(p), env(e) {
		initial_state = entry_state;
		entry_mode = env.mode;
		entry_locals_state = env.tc_local_variables;

		if(!env.source_stack.empty()) {
			entry_source = env.source_stack.back();
		}

		if(env.mode == fif_mode::interpreting) 
			return;
		
		lex_depth = int32_t(env.lexical_stack.size());
		lexical_new_scope(false, env);

		if(!skip_compilation(env.mode)) {
			if(env.mode == fif_mode::compiling_llvm) {
				phi_pass = true;
				env.mode = fif_mode::tc_level_1;
			} else {
				loop_start.add_concrete_branch(branch_source{
					initial_state, env.tc_local_variables, nullptr, nullptr, nullptr,
					0, false, true, true, false }, env);
				loop_start.materialize(env);
			}
		}
	}
	virtual control_structure get_type()override {
		return control_structure::str_while_loop;
	}
	void add_break() {
		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		if(env.mode == fif_mode::interpreting) {
			intepreter_skip_body = true;
			env.mode = surpress_branch(env.mode);
			return;
		}

		auto wstemp = env.compiler_stack.back()->working_state();
		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, lex_depth, env);
			env.mode = surpress_branch(env.mode);
		} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), env.tc_local_variables, nullptr, nullptr, nullptr, 0, false, true, true, false }, lex_depth, env) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			env.mode = surpress_branch(env.mode);
			return;
		}
	}
	void end_condition(environment&) {
		if(saw_conditional) {
			env.report_error("multiple conditions in while loop");
			env.mode = fif_mode::error;
			return;
		}
		saw_conditional = true;

		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			auto wstate = env.compiler_stack.back()->working_state();
			if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
				if(!typechecking_mode(env.mode)) {
					env.report_error("while loop conditional terminated with an inappropriate value");
					env.mode = fif_mode::error;
				} else {
					env.mode = fail_typechecking(env.mode);
				}
				return;
			}
			
			auto branch_condition = wstate->popr_main().as<LLVMValueRef>();

			LLVMBasicBlockRef continuation = nullptr;
#ifdef USE_LLVM
			if(env.mode == fif_mode::compiling_llvm) {
				auto in_fn = env.compiler_stack.back()->llvm_function();
				continuation = LLVMCreateBasicBlockInContext(env.llvm_context, "continuation");
				LLVMAppendExistingBasicBlock(in_fn, continuation);
			}
#endif
			auto bresult = loop_exit.add_cb_with_exit_pad(branch_source{ wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, branch_condition, continuation, 0, false, false, true, false }, lex_depth, env);
		} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			auto wstate = env.compiler_stack.back()->working_state();
			if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
				env.mode = fail_typechecking(env.mode);
				return;
			}
			wstate->pop_main();
			if(loop_exit.add_concrete_branch(branch_source{ wstate->copy(), env.tc_local_variables, nullptr, nullptr, nullptr, 0, false, false, true, false }, env) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			return;
		} else if(env.mode == fif_mode::interpreting) {
			if(env.compiler_stack.back()->working_state()->popr_main().as<bool>() == false) {
				intepreter_skip_body = true;
				env.mode = surpress_branch(env.mode);
			}
			return;
		}
	}
	virtual bool finish(environment&) override {
		if(entry_mode == fif_mode::interpreting && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;
			return true;
		}

		auto wstate = env.compiler_stack.back()->working_state();

		if(env.mode == fif_mode::interpreting) {
			if(!saw_conditional && wstate->main_type_back(0) == fif_bool) {
				if(wstate->popr_main().as<bool>() == false) {
					return true;
				}
			}
			if(!saw_conditional && wstate->main_type_back(0) != fif_bool) {
				env.report_error("while loop terminated with an inappropriate conditional");
				env.mode = fif_mode::error;
				return true;
			}
			saw_conditional = false;
			if(!env.source_stack.empty())
				env.source_stack.back() = entry_source;
			return false;
		}

		auto bcode = parent->bytecode_compilation_progress();

		lexical_end_scope(env);
		assert(lex_depth == int32_t(env.lexical_stack.size()));

		if(typechecking_level(env.mode) == 1 && phi_pass == true) {
			phi_pass = false;
			if(!saw_conditional) {
				wstate->pop_main();
			}

			auto bmatchlb = loop_start.add_concrete_branch(branch_source{
				wstate->copy(), env.tc_local_variables, nullptr, nullptr, nullptr,
				0, false, true, true, false }, env);

			env.mode = entry_mode;
			condition_mode = fif_mode(0);

			if(bmatchlb != add_branch_result::ok) {
				if(!typechecking_mode(env.mode)) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in while loop");
				} else {
					env.mode = fail_typechecking(env.mode);
				}
				return true;
			}

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			env.tc_local_variables = entry_locals_state;
			lexical_new_scope(false, env);

			auto pb = env.compiler_stack.back()->llvm_block();
			auto bmatch = loop_start.add_concrete_branch(branch_source{
				initial_state, env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr,
				0, false, true, true, false }, env);

			if(bmatch != add_branch_result::ok) {
				if(!typechecking_mode(env.mode)) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in while loop");
				} else {
					env.mode = fail_typechecking(env.mode);
				}
				return true;
			} else {
				loop_start.materialize(env);
				saw_conditional = false;
				if(!env.source_stack.empty())
					env.source_stack.back() = entry_source;

				return false;
			}
		} else {
			auto pb = env.compiler_stack.back()->llvm_block();

			if(!saw_conditional && !skip_compilation(env.mode)) {
				saw_conditional = true;
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					if(!typechecking_mode(env.mode)) {
						env.report_error("while loop terminated with an inappropriate conditional");
						env.mode = fif_mode::error;
					} else {
						env.mode = fail_typechecking(env.mode);
					}
					return true;
				}
				auto branch_condition = wstate->popr_main().as<LLVMValueRef>();

				LLVMBasicBlockRef continuation_block = nullptr;
#ifdef USE_LLVM
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation_block = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
					LLVMAppendExistingBasicBlock(in_fn, continuation_block);
				}
#endif
				auto bmatch = loop_exit.add_concrete_branch(branch_source{ wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, branch_condition, continuation_block, 0, false, false, true, false }, env);

				*pb = continuation_block;
				LLVMPositionBuilderAtEnd(env.llvm_builder, continuation_block);

				if(bmatch != add_branch_result::ok) {
					if(!typechecking_mode(env.mode)) {
						env.mode = fif_mode::error;
						env.report_error("branch types incompatible in while loop");
					} else {
						env.mode = fail_typechecking(env.mode);
					}
					return true;
				}
			}

			if(!skip_compilation(env.mode)) {
				auto bmatch = loop_start.add_concrete_branch(branch_source{
					wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr,
					0, false, true, true, false }, env);

				if(bmatch != add_branch_result::ok) {
					if(!typechecking_mode(env.mode)) {
						env.mode = fif_mode::error;
						env.report_error("branch types incompatible in while loop");
					} else {
						env.mode = fail_typechecking(env.mode);
					}
					return true;
				}
			}

			loop_start.finalize(env);

			if(condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);

			env.tc_local_variables = entry_locals_state;

			loop_exit.materialize(env);
			loop_exit.finalize(env);

			return true;
		}
		return true;
	}
};

class do_loop_scope : public opaque_compiler_data {
public:
	state_stack initial_state;
	std::vector< int64_t> entry_locals_state;

	branch_target loop_start;
	branch_target loop_exit;
	environment& env;

	std::string_view entry_source;
	int32_t lex_depth = 0;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);

	bool phi_pass = false;
	bool intepreter_skip_body = false;

	do_loop_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : opaque_compiler_data(p), env(e) {
		initial_state = entry_state;
		entry_mode = env.mode;
		entry_locals_state = env.tc_local_variables;

		if(!env.source_stack.empty()) 
			entry_source = env.source_stack.back();
		
		if(env.mode == fif_mode::interpreting)
			return;
		
		lex_depth = int32_t(env.lexical_stack.size());
		lexical_new_scope(false, env);

		if(!skip_compilation(env.mode)) {
			if(env.mode == fif_mode::compiling_llvm) {
				phi_pass = true;
				env.mode = fif_mode::tc_level_1;
			} else {
				loop_start.add_concrete_branch(branch_source{
					initial_state, env.tc_local_variables, nullptr, nullptr, nullptr,
					0, false, true, true, false }, env);
				loop_start.materialize(env);
			}
		}
	}
	virtual control_structure get_type()override {
		return control_structure::str_do_loop;
	}
	void add_break() {
		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		if(env.mode == fif_mode::interpreting) {
			intepreter_skip_body = true;
			env.mode = surpress_branch(env.mode);
			return;
		}

		auto wstemp = env.compiler_stack.back()->working_state();
		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true, false }, lex_depth, env);
			env.mode = surpress_branch(env.mode);
		} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), env.tc_local_variables, nullptr, nullptr, nullptr, 0, false, true, true, false }, lex_depth, env) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			env.mode = surpress_branch(env.mode);
			return;
		}
	}
	void until_statement(environment&) {
		// does nothing
	}
	virtual bool finish(environment&) override {
		if(entry_mode == fif_mode::interpreting && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;
			return true;
		}

		auto wstate = env.compiler_stack.back()->working_state();
		
		if(env.mode == fif_mode::interpreting) {
			if(wstate->main_size() > 0 && wstate->main_type_back(0) == fif_bool) {
				if(wstate->popr_main().as<bool>()) {
					return true;
				}
			} else {
				env.report_error("do loop terminated with an inappropriate conditional");
				env.mode = fif_mode::error;
				return true;
			}
			if(!env.source_stack.empty())
				env.source_stack.back() = entry_source;
			return false;
		}

		lexical_end_scope(env);
		assert(int32_t(env.lexical_stack.size()) == lex_depth);
		auto bcode = parent->bytecode_compilation_progress();

		if(typechecking_level(env.mode) == 1 && phi_pass == true) {
			phi_pass = false;
			wstate->pop_main();

			auto pb = env.compiler_stack.back()->llvm_block();

			auto bmatchlb = loop_start.add_concrete_branch(branch_source{
				*wstate, env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr,
				0, false, true, true, false }, env);

			env.mode = entry_mode;
			condition_mode = fif_mode(0);

			if(bmatchlb != add_branch_result::ok) {
				if(!typechecking_mode(env.mode)) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in do loop");
				} else {
					env.mode = fail_typechecking(env.mode);
				}
				return true;
			}

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			env.tc_local_variables = entry_locals_state;
			lexical_new_scope(false, env);

			auto bmatch = loop_start.add_concrete_branch(branch_source{
				initial_state, env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr,
				0, false, true, true, false }, env);

			if(bmatch != add_branch_result::ok) {
				if(!typechecking_mode(env.mode)) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in do loop");
				} else {
					env.mode = fail_typechecking(env.mode);
				}
				return true;
			}

			loop_start.materialize(env);

			if(!env.source_stack.empty())
				env.source_stack.back() = entry_source;

			return false;
		} else {
			auto pb = env.compiler_stack.back()->llvm_block();

			if(!skip_compilation(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					if(!typechecking_mode(env.mode)) {
						env.report_error("do loop terminated with an inappropriate conditional");
						env.mode = fif_mode::error;
					} else {
						env.mode = fail_typechecking(env.mode);
					}
					return true;
				}
				auto branch_condition = wstate->popr_main().as<LLVMValueRef>();

				LLVMBasicBlockRef continuation_block = nullptr;
#ifdef USE_LLVM
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation_block = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
					LLVMAppendExistingBasicBlock(in_fn, continuation_block);
				}
#endif
				loop_exit.add_concrete_branch(branch_source{ wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, branch_condition, continuation_block, 0, true, false, true, false }, env);

				*pb = continuation_block;
				LLVMPositionBuilderAtEnd(env.llvm_builder, continuation_block);

				auto bmatch = loop_start.add_concrete_branch(branch_source{
					wstate->copy(), env.tc_local_variables, pb ? *pb : nullptr, nullptr, nullptr,
					0, false, true, true, false }, env);

				if(bmatch != add_branch_result::ok) {
					if(!typechecking_mode(env.mode)) {
						env.mode = fif_mode::error;
						env.report_error("branch types incompatible in do loop");
					} else {
						env.mode = fail_typechecking(env.mode);
					}
					return true;
				}
			}

			loop_start.finalize(env);

			env.tc_local_variables = entry_locals_state;

			loop_exit.materialize(env);
			loop_exit.finalize(env);

			if(condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);

			return true;
		}
		return true;
	}
};

inline constexpr size_t inlining_cells_limit = 512;

inline uint32_t codepoint_from_utf8(char const* start, char const* end) {
	uint8_t byte1 = uint8_t(start + 0 < end ? start[0] : 0);
	uint8_t byte2 = uint8_t(start + 1 < end ? start[1] : 0);
	uint8_t byte3 = uint8_t(start + 2 < end ? start[2] : 0);
	uint8_t byte4 = uint8_t(start + 3 < end ? start[3] : 0);
	if((byte1 & 0x80) == 0) {
		return uint32_t(byte1);
	} else if((byte1 & 0xE0) == 0xC0) {
		return uint32_t(byte2 & 0x3F) | (uint32_t(byte1 & 0x1F) << 6);
	} else  if((byte1 & 0xF0) == 0xE0) {
		return uint32_t(byte3 & 0x3F) | (uint32_t(byte2 & 0x3F) << 6) | (uint32_t(byte1 & 0x0F) << 12);
	} else if((byte1 & 0xF8) == 0xF0) {
		return uint32_t(byte4 & 0x3F) | (uint32_t(byte3 & 0x3F) << 6) | (uint32_t(byte2 & 0x3F) << 12) | (uint32_t(byte1 & 0x07) << 18);
	}
	return 0;
}
inline size_t size_from_utf8(char const* start) {
	uint8_t b = uint8_t(start[0]);
	return ((b & 0x80) == 0) ? 1 : ((b & 0xE0) == 0xC0) ? 2
		: ((b & 0xF0) == 0xE0) ? 3 : ((b & 0xF8) == 0xF0) ? 4
		: 1;
}
inline bool codepoint_is_space(uint32_t c) noexcept {
	return (c == 0x3000 || c == 0x205F || c == 0x202F || c == 0x2029 || c == 0x2028 || c == 0x00A0
		|| c == 0x0085 || c <= 0x0020 || (0x2000 <= c && c <= 0x200A));
}
inline bool codepoint_is_line_break(uint32_t c) noexcept {
	return  c == 0x2029 || c == 0x2028 || c == uint32_t('\n') || c == uint32_t('\r');
}

struct string_start {
	int32_t eq_match = 0;
	bool starts_string = false;
};

inline string_start match_string_start(std::string_view source) {
	if(source.length() < 2)
		return string_start{ 0, false };
	if(source[0] != '<')
		return string_start{ 0, false };
	int32_t eq_count = 0;
	uint32_t pos = 1;
	while(pos < source.length()) {
		if(source[pos] == '<') {
			return string_start{ eq_count, true };
		} else if(source[pos] == '=') {
			++eq_count;
		} else {
			break;
		}
		++pos;
	}
	return string_start{ 0, false };
}
inline bool match_string_end(std::string_view source, int32_t eq_count_in) {
	if(source.length() < 2)
		return false ;
	if(source[0] != '>')
		return  false;
	int32_t eq_count = 0;
	uint32_t pos = 1;
	while(pos < source.length()) {
		if(source[pos] == '>') {
			return eq_count == eq_count_in;
		} else if(source[pos] == '=') {
			++eq_count;
		} else {
			break;
		}
		++pos;
	}
	return false;
}

// string format: <<xxxxx>> or <=<xxxxx>=> or <==<xxxxx>==> etc

inline parse_result read_token(std::string_view& source, environment& env) {
	size_t first_non_space = 0;
	while(first_non_space < source.length()) {
		auto codepoint = codepoint_from_utf8(source.data() + first_non_space, source.data() + source.length());
		if(codepoint_is_space(codepoint) || codepoint_is_line_break(codepoint)) 
			first_non_space += size_from_utf8(source.data() + first_non_space);
		else
			break;
	}

	if(auto sm = match_string_start(source.substr(first_non_space)); sm.starts_string) {
		auto str_start = first_non_space + sm.eq_match + 2;
		auto str_pos = str_start;
		while(str_pos < source.length()) {
			if(match_string_end(source.substr(str_pos), sm.eq_match)) {
				auto result = source.substr(str_start, str_pos - str_start);
				source = source.substr(str_pos + sm.eq_match + 2);
				return parse_result{ result, true };
			}
			str_pos += size_from_utf8(source.data() + str_pos);
		}

		// invalid text, reached the end of source within a string without closing it
		env.report_error("unclosed string constant");
		env.mode = fif_mode::error;
		source = std::string_view{ };
		return parse_result{ std::string_view{ }, false };
	}

	auto word_end = first_non_space;
	while(word_end < source.length()) {
		auto codepoint = codepoint_from_utf8(source.data() + word_end, source.data() + source.length());
		if(codepoint_is_space(codepoint) || codepoint_is_line_break(codepoint))
			break;
		else
			word_end += size_from_utf8(source.data() + word_end);
	}

	auto result = source.substr(first_non_space, word_end - first_non_space);
	source = source.substr(word_end);

	return parse_result{ result, false };

}

inline void execute_fif_word(parse_result word, environment& env, bool ignore_specialization);


inline void run_to_scope_end(environment& env) { // execute/compile source code until current function is completed
	parse_result word;
	auto scope_depth = env.compiler_stack.size();

	if(!env.compiler_stack.empty()) {
		do {
			if(!env.source_stack.empty())
				word = read_token(env.source_stack.back(), env);
			else
				word.content = std::string_view{ };

			if((word.content.length() > 0 || word.is_string) && env.mode != fif_mode::error) {
				execute_fif_word(word, env, false);
			}
		} while((word.content.length() > 0 || word.is_string) && !env.compiler_stack.empty() && env.mode != fif_mode::error);
	}
}

inline void run_to_function_end(environment& env) { // execute/compile source code until current function is completed
	run_to_scope_end(env);
	if(!env.compiler_stack.empty() && env.compiler_stack.back()->finish(env)) {
		env.compiler_stack.pop_back();
	}
}

inline state_stack make_type_checking_stack(state_stack& initial_stack, environment& env) {
	state_stack transformed_copy;
	std::vector<int64_t> scratch;
	for(size_t i = 0; i < initial_stack.main_size(); ++i) {
		scratch.clear();
		auto t = initial_stack.main_type(i);
		for(int32_t j = 0; j < env.dict.type_array[t].cell_size; ++j) {
			scratch.push_back(env.new_ident());
		}
		if(env.dict.type_array[t].cell_size > 0) {
			transformed_copy.push_back_main(vsize_obj(t, uint32_t(env.dict.type_array[t].cell_size * 8), (unsigned char*)(scratch.data())));
		} else {
			transformed_copy.push_back_main(vsize_obj(t, 0));
		}
	}
	for(size_t i = 0; i < initial_stack.return_size(); ++i) {
		scratch.clear();
		auto t = initial_stack.return_type(i);
		for(int32_t j = 0; j < env.dict.type_array[t].cell_size; ++j) {
			scratch.push_back(env.new_ident());
		}
		if(env.dict.type_array[t].cell_size > 0) {
			transformed_copy.push_back_return(vsize_obj(t, uint32_t(env.dict.type_array[t].cell_size * 8), (unsigned char*)(scratch.data())));
		} else {
			transformed_copy.push_back_return(vsize_obj(t, 0));
		}
	}
	transformed_copy.min_main_depth = int32_t(transformed_copy.main_size());
	transformed_copy.min_return_depth = int32_t(transformed_copy.return_size());
	return transformed_copy;
}

inline word_match_result get_basic_type_match(int32_t word_index, state_stack& current_type_state, environment& env, std::vector<int32_t>& specialize_t_subs, bool ignore_specializations) {
	while(word_index != -1) {
		specialize_t_subs.clear();
		bool specialization_matches = [&]() { 
			if(env.dict.word_array[word_index].stack_types_count == 0)
				return true;
			return match_stack_description(std::span<int32_t const>{env.dict.all_stack_types.data() + env.dict.word_array[word_index].stack_types_start, size_t(env.dict.word_array[word_index].stack_types_count)}, current_type_state, env, specialize_t_subs).matched;
		}();
		if(specialization_matches && (ignore_specializations == false || env.dict.word_array[word_index].treat_as_base || env.dict.word_array[word_index].specialization_of == -1)) {
			word_match_result match = match_word(env.dict.word_array[word_index], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);
			int32_t w = word_index;
			match.substitution_version = word_index;

			if(!match.matched) {
				if(failed(env.mode)) {
					// ignore match failure and pass on through
					return word_match_result{ false, 0, 0, 0, 0 };
				} else if(typechecking_level(env.mode) == 1 || typechecking_level(env.mode) == 2) {
					if(env.dict.word_array[w].being_typechecked) { // already being typechecked -- mark as un checkable recursive branch
						env.mode = make_recursive(env.mode);
					} else if(env.dict.word_array[w].source.length() > 0) { // try to typecheck word as level 1
						auto tcstack = make_type_checking_stack(current_type_state, env);

						env.dict.word_array[w].being_typechecked = true;

						switch_compiler_stack_mode(env, fif_mode::tc_level_1);

						env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
						auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, tcstack, w, -1);
						fnscope->type_subs = specialize_t_subs;
						env.compiler_stack.emplace_back(std::move(fnscope));

						run_to_function_end(env);
						env.source_stack.pop_back();

						env.dict.word_array[w].being_typechecked = false;
						match = match_word(env.dict.word_array[w], tcstack, env.dict.all_instances, env.dict.all_stack_types, env);
						match.substitution_version = word_index;

						restore_compiler_stack_mode(env);

						if(match.matched) {
							if(std::get<interpreted_word_instance>(env.dict.all_instances[match.word_index]).typechecking_level < 3) {
								env.mode = make_provisional(env.mode);
							}
							return match;
						} else {
							env.mode = fail_typechecking(env.mode);
							return word_match_result{ false, 0, 0, 0, 0 };
						}
					}
				} else if(env.dict.word_array[w].source.length() > 0) { // either compiling or interpreting or level 3 typecheck-- switch to typechecking to get a type definition
					switch_compiler_stack_mode(env, fif_mode::tc_level_1);
					env.dict.word_array[w].being_typechecked = true;

					auto tcstack = make_type_checking_stack(current_type_state, env);

					env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
					auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, tcstack, w, -1);
					fnscope->type_subs = specialize_t_subs;
					env.compiler_stack.emplace_back(std::move(fnscope));

					run_to_function_end(env);
					env.source_stack.pop_back();

					env.dict.word_array[w].being_typechecked = false;

					restore_compiler_stack_mode(env);

					match = match_word(env.dict.word_array[w], tcstack, env.dict.all_instances, env.dict.all_stack_types, env);
					match.substitution_version = word_index;

					if(!match.matched) {
						env.report_error("typechecking failure for " + word_name_from_id(w, env));
						env.mode = fif_mode::error;
						return word_match_result{ false, 0, 0, 0, 0 };
					}
					return match;
				}
			}

			return match;
		} else {
			word_index = env.dict.word_array[word_index].specialization_of;
		}
	}
	
	return word_match_result{ false, 0, 0, 0, 0 };
}

inline bool fully_typecheck_word(int32_t w, int32_t word_index, interpreted_word_instance& wi, state_stack& current_type_state, environment& env, std::vector<int32_t>& tsubs) {
	if(wi.typechecking_level >= 3)
		return true;

	auto tcstack = make_type_checking_stack(current_type_state, env);
	
	if(wi.typechecking_level == 1) {
		// perform level 2 typechecking

		switch_compiler_stack_mode(env, fif_mode::tc_level_2);

		env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, tcstack, w, word_index);
		fnscope->type_subs = tsubs;
		env.compiler_stack.emplace_back(std::move(fnscope));

		run_to_function_end(env);
		env.source_stack.pop_back();

		bool fc = failed(env.mode);

		restore_compiler_stack_mode(env);

		if(fc) {
			env.report_error("level 2 typecheck failed");
			env.mode = fif_mode::error;
			return false;
		}
	}

	if(wi.typechecking_level == 2) {
		// perform level 3 typechecking

		env.compiler_stack.emplace_back(std::make_unique< typecheck3_record_holder>(env.compiler_stack.back().get(), env));
		auto record_holder = static_cast<typecheck3_record_holder*>(env.compiler_stack.back().get());

		switch_compiler_stack_mode(env, fif_mode::tc_level_3);
		env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, tcstack, w, word_index);
		function_scope* sptr = fnscope.get();
		fnscope->type_subs = tsubs;
		env.compiler_stack.emplace_back(std::move(fnscope));

		run_to_function_end(env);
		
		auto input_stack = std::move(sptr->initial_state);
		auto final_stack = std::move(sptr ->iworking_state);

		env.source_stack.pop_back();

		bool fc = failed(env.mode);

		restore_compiler_stack_mode(env);

		if(fc) {
			env.report_error("level 3 typecheck failed");
			env.mode = fif_mode::error;
			return false;
		}

		wi.typechecking_level = 3;

		if(auto erb = record_holder->tr.find((uint64_t(w) << 32) | uint64_t(word_index)); erb != record_holder->tr.end()) {
			typecheck_3_record r;
			r.stack_height_added_at = int32_t(tcstack.main_size());
			r.rstack_height_added_at = int32_t(tcstack.return_size());

			auto c = get_stack_consumption(w, word_index, env);
			r.stack_consumed = std::max(c.stack, erb->second.stack_consumed);
			r.rstack_consumed = std::max(c.rstack, erb->second.rstack_consumed);

			record_holder->tr.insert_or_assign((uint64_t(w) << 32) | uint64_t(word_index), r);
		} else {
			typecheck_3_record r;
			r.stack_height_added_at = int32_t(tcstack.main_size());
			r.rstack_height_added_at = int32_t(tcstack.return_size());

			auto c = get_stack_consumption(w, word_index, env);
			r.stack_consumed = c.stack;
			r.rstack_consumed = c.rstack;

			record_holder->tr.insert_or_assign((uint64_t(w) << 32) | uint64_t(word_index), r);
		}

		/*
		process results of typechecking
		*/
		auto min_stack_depth = int32_t(tcstack.main_size());
		auto min_rstack_depth = int32_t(tcstack.return_size());
		for(auto& s : record_holder->tr) {
			min_stack_depth = std::min(min_stack_depth, s.second.stack_height_added_at - s.second.stack_consumed);
			min_rstack_depth = std::min(min_rstack_depth, s.second.rstack_height_added_at - s.second.rstack_consumed);
		}

		auto existing_description = std::span<int32_t const>(env.dict.all_stack_types.data() + wi.stack_types_start, wi.stack_types_count);
		auto revised_description = expand_stack_description(tcstack, existing_description, int32_t(tcstack.main_size()) - min_stack_depth, int32_t(tcstack.return_size()) - min_rstack_depth);

		if(!compare_stack_description(existing_description, std::span<int32_t const>(revised_description.data(), revised_description.size()))) {
			wi.stack_types_start = int32_t(env.dict.all_stack_types.size());
			wi.stack_types_count = int32_t(revised_description.size());
			env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), revised_description.begin(), revised_description.end());
		}

		wi.llvm_parameter_permutation = make_parameter_permutation(revised_description, final_stack, input_stack);
		
		env.compiler_stack.pop_back(); // for typecheck3_record_holder
	}

	return true;
}

inline bool compile_word(int32_t w, int32_t word_index, state_stack& state, fif_mode compile_mode, environment& env, std::vector<int32_t>& tsubs) {
	if(!std::holds_alternative<interpreted_word_instance>(env.dict.all_instances[word_index]))
		return true;

	interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[word_index]);
	if(!fully_typecheck_word(w, word_index, wi, state, env, tsubs)) {
		return false;
	}

	switch_compiler_stack_mode(env, compile_mode);

	if(env.mode != fif_mode::compiling_llvm && wi.compiled_offset == -1) {
		// typed but uncompiled word
		if(!std::get<interpreted_word_instance>(env.dict.all_instances[word_index]).being_compiled) {
			env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
			auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, state, w, word_index);
			fnscope->type_subs = tsubs;
			env.compiler_stack.emplace_back(std::move(fnscope));
			run_to_function_end(env);
			env.source_stack.pop_back();
		}
	}

	// case: reached an uncompiled llvm definition
	if(env.mode == fif_mode::compiling_llvm && !wi.llvm_compilation_finished) {
#ifdef USE_LLVM
		// typed but uncompiled word
		if(!std::get<interpreted_word_instance>(env.dict.all_instances[word_index]).being_compiled) {
			LLVMBasicBlockRef stored_block = nullptr;
			if(auto pb = env.compiler_stack.back()->llvm_block(); pb)
				stored_block = *pb;

			
			env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
			auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, state, w, word_index);
			fnscope->type_subs = tsubs;
			env.compiler_stack.emplace_back(std::move(fnscope));
			run_to_function_end(env);
			env.source_stack.pop_back();

			if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
				*pb = stored_block;
				LLVMPositionBuilderAtEnd(env.llvm_builder, stored_block);
			}
			
		}
#endif
	}

	restore_compiler_stack_mode(env);
	return true;
}

inline uint16_t* immediate_mem_var(state_stack& s, uint16_t* p, environment* e) {
	uint16_t index = *(p);
	int32_t type = *(int32_t*)(p + 1);
	auto* l = e->interpreter_stack_space.get() + e->frame_offset + index;
	s.push_back_main(vsize_obj(type, uint32_t(sizeof(void*)), (unsigned char*)(&l)));
	return p + 3;
}

inline dup_evaluation check_dup(int32_t type, fif::environment& env);

inline uint16_t* immediate_let(state_stack& s, uint16_t* p, environment* e) {
	uint16_t index = *(p);
	int32_t type = *(int32_t*)(p + 1);
	auto* l = e->interpreter_stack_space.get() + e->frame_offset + index;
	s.push_back_main(vsize_obj(type, uint32_t(e->dict.type_array[type].byte_size), l));
	return p + 3;
}

inline uint16_t* stash_in_frame(state_stack& s, uint16_t* p, environment* e) {
	auto sz = s.main_byte_back_at(1);
	auto dat = s.main_back_ptr_at(1);
	auto t = s.main_type_back(0);

	auto* base = e->interpreter_stack_space.get() + e->frame_offset;
	auto size_pos = base - 4;
	auto type_pos = base - 8;
	auto data_pos = base - (8 + sz);

	*((uint32_t*)size_pos) = sz;
	*((int32_t*)type_pos) = t;
	memcpy(data_pos, dat, sz);

	s.pop_main();
	return p;
}
inline uint16_t* recover_from_frame(state_stack& s, uint16_t* p, environment* e) {
	auto* base = e->interpreter_stack_space.get() + e->frame_offset;
	auto size_pos = base - 4;
	uint32_t size = *((uint32_t*)size_pos);
	auto type_pos = base - 8;
	int32_t type = *((int32_t*)type_pos);
	auto data_pos = base - (8 + size);

	s.push_back_main(type, size, data_pos);
	return p;
}

inline uint16_t* immediate_global(state_stack& s, uint16_t* p, environment* e) {
	uint16_t gi = *(p);
	
	auto* l = e->globals[gi].bytes.get();
	if(l) {
		if(e->globals[gi].constant) {
			s.push_back_main(vsize_obj(e->globals[gi].type, uint32_t(e->dict.type_array[e->globals[gi].type].byte_size), l));
		} else if(e->dict.type_array[e->globals[gi].type].is_memory_type() == false) {
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), e->globals[gi].type, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);

			s.push_back_main(vsize_obj(mem_type.type, uint32_t(sizeof(void*)), (unsigned char*)(&l)));
		} else {
			s.push_back_main(vsize_obj(e->globals[gi].type, uint32_t(sizeof(void*)), (unsigned char*)(&l)));
		}
	} else {
		e->report_error("unable to find global");
		return nullptr;
	}
	return p + 1;
}

inline void execute_fif_word(parse_result word, environment& env, bool ignore_specializations) {
	auto ws = env.compiler_stack.back()->working_state();
	if(!ws) {
		env.report_error("tried to execute in a scope with no working state");
		env.mode = fif_mode::error;
		return;
	}

	// TODO: string constant case

	auto content_string = std::string{ word.content };

	if(is_integer(word.content.data(), word.content.data() + word.content.length())) {
		do_immediate_i32(*ws, parse_int(word.content), &env);
	} else if(is_fp(word.content.data(), word.content.data() + word.content.length())) {
		do_immediate_f32(*ws, parse_float(word.content), &env);
	} else if(word.content == "true" || word.content == "false") {
		do_immediate_bool(*ws, word.content == "true", &env);
	} else if(auto var = lexical_get_var(content_string, env); env.mode != fif_mode::interpreting && word.is_string == false && var.type != -1) {
		auto vdata = (unsigned char*)(env.tc_local_variables.data() + var.offset);
		if(var.memory_variable) {
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), var.type, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, env);

			if(skip_compilation(env.mode)) {

			} else if(typechecking_mode(env.mode)) {
				ws->push_back_main(vsize_obj(mem_type.type, sizeof(int64_t), vdata));
			} else if(env.mode == fif_mode::compiling_llvm) {
				ws->push_back_main(vsize_obj(mem_type.type, sizeof(LLVMValueRef), vdata));
			} else if(env.mode == fif_mode::interpreting) {
				// ws->push_back_main(vsize_obj(mem_type.type, sizeof(void*), (unsigned char*)(&vdata)));
			} else if(env.mode == fif_mode::compiling_bytecode) {
				auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
				c_append(cbytes, env.dict.get_builtin(immediate_mem_var));
				c_append(cbytes, uint16_t(var.offset)); // index
				c_append(cbytes, int32_t(mem_type.type));

				ws->push_back_main(vsize_obj(mem_type.type, 0));
			}
		} else {
			if(skip_compilation(env.mode)) {

			} else if(typechecking_mode(env.mode)) {
				ws->push_back_main(vsize_obj(var.type, var.size, vdata));
				execute_fif_word(fif::parse_result{ "dup", false }, env, false);

				auto new_copy = ws->popr_main();
				auto old_copy = ws->popr_main();
				if(old_copy.data()) {
					memcpy(vdata, old_copy.data(), std::min(uint32_t(var.size), old_copy.size));
				}
				ws->push_back_main(new_copy);
			} else if(env.mode == fif_mode::compiling_llvm) {
				ws->push_back_main(vsize_obj(var.type, var.size, vdata));
				execute_fif_word(fif::parse_result{ "dup", false }, env, false);

				auto new_copy = ws->popr_main();
				auto old_copy = ws->popr_main();
				if(old_copy.data()) {
					memcpy(vdata, old_copy.data(), std::min(uint32_t(var.size), old_copy.size));
				}
				ws->push_back_main(new_copy);
			} else if(env.mode == fif_mode::interpreting) {

			} else if(env.mode == fif_mode::compiling_bytecode) {
				// implement let
				auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
				c_append(cbytes, env.dict.get_builtin(immediate_let));
				c_append(cbytes, uint16_t(var.offset)); // index
				c_append(cbytes, int32_t(var.type));

				if(trivial_dup(var.type, env) == false) {
					auto duprep = check_dup(var.type, env);
					execute_fif_word(fif::parse_result{ "dup", false }, env, false);
					c_append(cbytes, env.dict.get_builtin(stash_in_frame));

					if(duprep.alters_source) {
						c_append(cbytes, env.dict.get_builtin(do_local_assign));
						c_append(cbytes, uint16_t(var.offset));
					} else {
						c_append(cbytes, env.dict.get_builtin(drop_cimple));
					}
					c_append(cbytes, env.dict.get_builtin(recover_from_frame));
				}
				ws->push_back_main(vsize_obj(var.type, 0));
			}
		}
	} else if(auto varb = env.global_names.find(content_string); word.is_string == false && varb != env.global_names.end()) {
		auto base_type = env.globals[varb->second].type;
		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), env.globals[varb->second].type, -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, env);

		if(skip_compilation(env.mode)) {

		} else if(typechecking_mode(env.mode)) {
			if(env.globals[varb->second].constant) {
				/*
				* TODO: need dup logic
				*/
				ws->push_back_main(vsize_obj(env.globals[varb->second].type, uint32_t(env.dict.type_array[env.globals[varb->second].type].cell_size * 8), (unsigned char*)(env.globals[varb->second].cells.get())));
			} else if(env.dict.type_array[base_type].is_memory_type() == false) {
				ws->push_back_main(vsize_obj(mem_type.type, sizeof(int64_t), (unsigned char*)(env.globals[varb->second].cells.get())));
			} else {
				ws->push_back_main(vsize_obj(base_type, sizeof(int64_t), (unsigned char*)(env.globals[varb->second].cells.get())));
			}
		} else if(env.mode == fif_mode::compiling_llvm) {
			if(env.globals[varb->second].constant) {
				ws->push_back_main(vsize_obj(env.globals[varb->second].type, uint32_t(env.dict.type_array[env.globals[varb->second].type].cell_size * 8), (unsigned char*)(env.globals[varb->second].cells.get())));
			} else  if(env.dict.type_array[base_type].is_memory_type() == false) {
				ws->push_back_main(vsize_obj(mem_type.type, sizeof(int64_t), (unsigned char*)(env.globals[varb->second].cells.get())));
			} else {
				ws->push_back_main(vsize_obj(base_type, sizeof(int64_t), (unsigned char*)(env.globals[varb->second].cells.get())));
			}
		} else if(env.mode == fif_mode::interpreting) {
			if(env.globals[varb->second].constant) {
				ws->push_back_main(vsize_obj(env.globals[varb->second].type, uint32_t(env.dict.type_array[env.globals[varb->second].type].byte_size), (unsigned char*)(env.globals[varb->second].bytes.get())));
			} else if(env.dict.type_array[base_type].is_memory_type() == false) {
				auto ptr = env.globals[varb->second].bytes.get();
				ws->push_back_main(vsize_obj(mem_type.type, sizeof(void*), (unsigned char*)(&ptr)));
			} else {
				auto ptr = env.globals[varb->second].bytes.get();
				ws->push_back_main(vsize_obj(base_type, sizeof(void*), (unsigned char*)(&ptr)));
			}
		} else if(env.mode == fif_mode::compiling_bytecode) {
			auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
			c_append(cbytes, env.dict.get_builtin(immediate_global));
			c_append(cbytes, uint16_t(varb->second));

			if(env.globals[varb->second].constant) {
				ws->push_back_main(vsize_obj(env.globals[varb->second].type, 0));
			} else if(env.dict.type_array[base_type].is_memory_type() == false) {
				auto ptr = env.globals[varb->second].bytes.get();
				ws->push_back_main(vsize_obj(mem_type.type, 0));
			} else {
				auto ptr = env.globals[varb->second].bytes.get();
				ws->push_back_main(vsize_obj(base_type, 0));
			}
		}
	} else if(auto rtype = resolve_type(word.content, env, env.compiler_stack.back()->type_substitutions()); rtype != -1) {
		do_immediate_type(*ws, rtype, &env);
	} else if(auto it = env.dict.words.find(std::string(word.content)); it != env.dict.words.end()) {
		auto w = it->second;
		// execute / compile word

		// IMMEDIATE words with source
		if(env.dict.word_array[w].immediate) {
			if(env.dict.word_array[w].source.length() > 0) {
				env.source_stack.push_back(std::string_view{ env.dict.word_array[w].source });
				switch_compiler_stack_mode(env, fif_mode::interpreting);

				run_to_scope_end(env);

				restore_compiler_stack_mode(env);
				env.source_stack.pop_back(); // remove source replacement

				return;
			}
		}

		std::vector<int32_t> called_tsub_types;
		auto match = get_basic_type_match(w, *ws, env, called_tsub_types, ignore_specializations);
		w = match.substitution_version;

		if(!match.matched) {
			if(failed(env.mode) || skip_compilation(env.mode)) {
				return;
			} else { // critical failure
				env.report_error("could not match word to stack types");
				env.mode = fif_mode::error;
				return;
			}
		}

		word_types* wi = &(env.dict.all_instances[match.word_index]);

		// IMMEDIATE words (source not available)
		if(env.dict.word_array[w].immediate) {
			if(std::holds_alternative<interpreted_word_instance>(*wi)) {
				switch_compiler_stack_mode(env, fif_mode::interpreting);
				execute_fif_word(std::get<interpreted_word_instance>(*wi), *ws, env);
				restore_compiler_stack_mode(env);
			} else if(std::holds_alternative<compiled_word_instance>(*wi)) {
				execute_fif_word(std::get<compiled_word_instance>(*wi), *ws, env);
			}
			return;
		}

		//
		// level 1 typechecking -- should be able to trace at least one path through each word using only known words
		//
		// level 2 typechecking -- should be able to trace through every branch to produce an accurate stack picture for all known words present in the definition
		//

		if(typechecking_level(env.mode) == 1 || typechecking_level(env.mode) == 2) {
			if(std::holds_alternative<interpreted_word_instance>(*wi)) {
				if(!failed(env.mode) && !skip_compilation(env.mode)) {
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count)), *ws, std::get<interpreted_word_instance>(*wi).llvm_parameter_permutation, env);
				}
			} else if(std::holds_alternative<compiled_word_instance>(*wi)) {
				// no special logic, compiled words assumed to always typecheck
				if(!failed(env.mode) && !skip_compilation(env.mode)) {
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(*wi).stack_types_start, size_t(std::get<compiled_word_instance>(*wi).stack_types_count)), *ws, std::get<compiled_word_instance>(*wi).llvm_parameter_permutation, env);
				}
			}
			return;
		}

		//
		// level 3 typechecking -- recursively determine the minimum stack position of all dependencies
		//

		if(typechecking_level(env.mode) == 3) {
			if(std::holds_alternative<interpreted_word_instance>(*wi)) {
				if(!failed(env.mode) && !skip_compilation(env.mode)) {
					// add typecheck info
					if(std::get<interpreted_word_instance>(*wi).typechecking_level < 3) {
						// this word also hasn't been compiled yet

						auto rword = env.compiler_stack.back()->word_id();
						auto rinst = env.compiler_stack.back()->instance_id();
						auto& dep = *(env.compiler_stack.back()->typecheck_record());
						auto existing_record = dep.find((uint64_t(w) << 32) | uint64_t(match.word_index));
						if(existing_record != dep.end()
							&& existing_record->second.stack_height_added_at <= int32_t(ws->main_size())
							&& existing_record->second.rstack_height_added_at <= int32_t(ws->return_size())) {

							// already added all dependencies of this word at this stack height or less
						} else { // word is occurring deeper in the stack or not yet seen
							// recurse and typecheck through it
							// first, make sure that it is typechecked to at least level 2

							if(env.dict.word_array[w].source.length() == 0) {
								env.report_error(std::string("Word ") + std::to_string(w) + " is undefined.");
								env.mode = fif_mode::error;
								return;
							}

							if(std::get<interpreted_word_instance>(*wi).typechecking_level < 2) {
								switch_compiler_stack_mode(env, fif_mode::tc_level_2);
								auto tcstack = make_type_checking_stack(*ws, env);

								env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
								auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, tcstack, w, match.word_index);
								fnscope->type_subs = called_tsub_types;
								env.compiler_stack.emplace_back(std::move(fnscope));

								run_to_function_end(env);
								env.source_stack.pop_back();

								bool fc = failed(env.mode);
								restore_compiler_stack_mode(env);

								if(fc) {
									env.report_error("level 2 typecheck failed");
									env.mode = fif_mode::error;
									return;
								}

								wi = &(env.dict.all_instances[match.word_index]);
							}

							env.mode = fif_mode::tc_level_3;
							if(auto erb = dep.find((uint64_t(w) << 32) | uint64_t(match.word_index)); erb != dep.end()) {
								typecheck_3_record r;
								r.stack_height_added_at = int32_t(ws->main_size());
								r.rstack_height_added_at = int32_t(ws->return_size());

								auto c = get_stack_consumption(w, match.word_index, env);
								r.stack_consumed = std::max(c.stack, erb->second.stack_consumed);
								r.rstack_consumed = std::max(c.rstack, erb->second.rstack_consumed);

								dep.insert_or_assign((uint64_t(w) << 32) | uint64_t(match.word_index), r);
							} else {
								typecheck_3_record r;
								r.stack_height_added_at = int32_t(ws->main_size());
								r.rstack_height_added_at = int32_t(ws->return_size());

								auto c = get_stack_consumption(w, match.word_index, env);
								r.stack_consumed = c.stack;
								r.rstack_consumed = c.rstack;

								dep.insert_or_assign((uint64_t(w) << 32) | uint64_t(match.word_index), r);
							}
						}
					}

					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count)), *ws, std::get<interpreted_word_instance>(*wi).llvm_parameter_permutation, env);
				}
			} else if(std::holds_alternative<compiled_word_instance>(*wi)) {
				if(!failed(env.mode) && !skip_compilation(env.mode)) {
					// no special logic, compiled words assumed to always typecheck
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(*wi).stack_types_start, size_t(std::get<compiled_word_instance>(*wi).stack_types_count)), *ws, std::get<compiled_word_instance>(*wi).llvm_parameter_permutation, env);
				}
			}

			return;
		}

		if(env.mode == fif_mode::interpreting || env.mode == fif_mode::compiling_llvm || env.mode == fif_mode::compiling_bytecode) {
			if(!compile_word(w, match.word_index, *ws, env.mode != fif_mode::compiling_llvm ? fif_mode::compiling_bytecode : fif_mode::compiling_llvm, env, called_tsub_types)) {
				env.report_error("failed to compile word");
				env.mode = fif_mode::error;
				return;
			}
			wi = &(env.dict.all_instances[match.word_index]);

			if(std::holds_alternative<interpreted_word_instance>(*wi)) {
				if(env.mode == fif_mode::interpreting) {
					execute_fif_word(std::get<interpreted_word_instance>(*wi), *ws, env);
				} else if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
					std::span<int32_t const> desc{ env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count) };
					llvm_make_function_call(env, std::get<interpreted_word_instance>(*wi), desc);
#endif
				} else if(env.mode == fif_mode::compiling_bytecode) {
					auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
					if(cbytes) {
						env.last_compiled_call.offset = cbytes->size();
						env.last_compiled_call.instance = [&]() {
							for(auto j = env.compiler_stack.size(); j-- > 0; ) {
								if(env.compiler_stack[j]->get_type() == control_structure::function) {
									auto fs = (function_scope*)(env.compiler_stack[j].get());
									return fs->for_instance;
								}
							}
							return -2;
						}();
						assert(std::get<interpreted_word_instance>(*wi).compiled_offset != -1);

						c_append(cbytes, env.dict.get_builtin(call_function));
						c_append(cbytes, std::get<interpreted_word_instance>(*wi).compiled_offset);
						
						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count)), *ws, std::get<interpreted_word_instance>(*wi).llvm_parameter_permutation, env);
					}
				}
			} else if(std::holds_alternative<compiled_word_instance>(*wi)) {
				if(env.mode == fif_mode::compiling_bytecode) {
					auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
					if(cbytes) {
						c_append(cbytes, std::get<compiled_word_instance>(*wi).implementation_index);

						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(*wi).stack_types_start, size_t(std::get<compiled_word_instance>(*wi).stack_types_count)), *ws, std::get<compiled_word_instance>(*wi).llvm_parameter_permutation, env);
					}
				} else {
					env.dict.builtin_array[std::get<compiled_word_instance>(*wi).implementation_index](*ws, nullptr, &env);
				}
			}
		}
	} else {
		env.report_error(std::string("attempted to execute an unknown word: ") + std::string(word.content));
		env.mode = fif_mode::error;
	}
}

#ifdef USE_LLVM
inline void add_exportable_functions_to_globals(environment& env) {
	if(!env.exported_functions.empty()) {
		auto array_type = LLVMArrayType(LLVMPointerTypeInContext(env.llvm_context, 0), 1);
		auto used_array = LLVMAddGlobal(env.llvm_module, array_type, "llvm.used");
		LLVMSetLinkage(used_array, LLVMLinkage::LLVMAppendingLinkage);
		LLVMSetInitializer(used_array, LLVMConstArray(LLVMPointerTypeInContext(env.llvm_context, 0), env.exported_functions.data(), uint32_t(env.exported_functions.size())));
		LLVMSetSection(used_array, "llvm.metadata");
	}
}

inline LLVMValueRef make_exportable_function(std::string const& export_name, std::string const& word, std::vector<int32_t> param_stack, std::vector<int32_t> return_stack, environment& env) {

	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>());
	env.source_stack.push_back(std::string_view{ });
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().get());
	switch_compiler_stack_mode(env, fif_mode::interpreting);
	static_cast<mode_switch_scope*>(env.compiler_stack.back().get())->interpreted_link = o;

	int32_t w = -1;
	if(auto it = env.dict.words.find(word); it != env.dict.words.end()) {
		w = it->second;
	} else {
		env.report_error("failed to export function (dictionary lookup failed)");
		return nullptr;
	}

	llvm_stack ts;
	for(size_t i = 0; i < param_stack.size(); ++i) {
		ts.push_back_main(vsize_obj(param_stack[i], env.dict.type_array[param_stack[i]].cell_size * 8));
	}
	for(size_t i = 0; i < return_stack.size(); ++i) {
		ts.push_back_return(vsize_obj(return_stack[i], env.dict.type_array[return_stack[i]].cell_size * 8));
	}
	ts.min_main_depth = int32_t(param_stack.size());
	ts.min_return_depth = int32_t(return_stack.size());

	std::vector<int32_t> typevars;
	auto match = get_basic_type_match(w, ts, env, typevars, false);
	w = match.substitution_version;
	if(!match.matched) {
		env.report_error("failed to export function (typematch failed)");
		return nullptr;
	}

	if(!compile_word(w, match.word_index, ts, fif_mode::compiling_llvm, env, typevars)) {
		env.report_error("failed to export function (compilation failed)");
		return nullptr;
	}

	if(!std::holds_alternative<interpreted_word_instance>(env.dict.all_instances[match.word_index])) {
		env.report_error("failed to export function (can't export built-in)");
		return nullptr;
	}

	auto& wi = std::get<interpreted_word_instance>(env.dict.all_instances[match.word_index]);

	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + wi.stack_types_start, wi.stack_types_count);
	auto fn_type = llvm_function_type_from_desc(env, desc, wi.llvm_parameter_permutation);
	auto compiled_fn = LLVMAddFunction(env.llvm_module, export_name.c_str(), fn_type.fn_type);
	if(fn_type.out_ptr_type) {
		auto nocapattribute = LLVMCreateEnumAttribute(env.llvm_context, 23, 1);
		LLVMAddAttributeAtIndex(compiled_fn, 1 + fn_type.ret_param_index, nocapattribute);
		//auto sret_attribute = LLVMCreateTypeAttribute(env.llvm_context, 78, fn_type.out_ptr_type);
		//LLVMAddAttributeAtIndex(compiled_fn, 1 + fn_type.ret_param_index, sret_attribute);
	}

	LLVMSetFunctionCallConv(compiled_fn, NATIVE_CC);
	//LLVMSetLinkage(compiled_fn, LLVMLinkage::LLVMLinkOnceAnyLinkage);
	//LLVMSetVisibility(compiled_fn, LLVMVisibility::LLVMDefaultVisibility);

	auto entry_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
	LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);


	// add body
	std::vector<LLVMValueRef> params;

	int32_t match_position = 0;
	// stack matching

	int32_t added_param = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		for(auto j = env.dict.type_array[desc[match_position]].cell_size; j-- > 0;) {
			params.push_back(LLVMGetParam(compiled_fn, uint32_t(added_param)));
			++added_param;
		}
		++match_position;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		//returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type.llvm_type);
		++match_position;
		//++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		for(auto j = env.dict.type_array[desc[match_position]].cell_size; j-- > 0;) {
			params.push_back(LLVMGetParam(compiled_fn, uint32_t(added_param)));
			++added_param;
		}
		++match_position;
	}

	if(fn_type.out_ptr_type) {
		params.push_back(LLVMGetParam(compiled_fn, uint32_t(added_param)));
		++added_param;
	}

	auto retvalue = LLVMBuildCall2(env.llvm_builder, llvm_function_type_from_desc(env, desc, wi.llvm_parameter_permutation).fn_type, wi.llvm_function, params.data(), uint32_t(params.size()), "");
	LLVMSetInstructionCallConv(retvalue, LLVMCallConv::LLVMFastCallConv);
	auto rsummary = llvm_function_return_type_from_desc(env, desc, wi.llvm_parameter_permutation);

	// make return
	if(rsummary.composite_type == nullptr) {
		LLVMBuildRetVoid(env.llvm_builder);
	} else {
		LLVMBuildRet(env.llvm_builder, retvalue);
	}
	if(LLVMVerifyFunction(compiled_fn, LLVMVerifierFailureAction::LLVMPrintMessageAction)) {

		std::string mod_contents = LLVMPrintModuleToString(env.llvm_module);

		char* message = nullptr;
		LLVMVerifyModule(env.llvm_module, LLVMReturnStatusAction, &message);

		env.report_error("LLVM verification of function failed");
		env.mode = fif_mode::error;
		return nullptr;
	}

	env.exported_functions.push_back(compiled_fn);

	restore_compiler_stack_mode(env);
	env.compiler_stack.pop_back();
	env.source_stack.pop_back();

	return compiled_fn;
}
#endif

inline void run_fif_interpreter(environment& env, std::string_view on_text) {
	env.source_stack.push_back(std::string_view(on_text));
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>());
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().get());

	switch_compiler_stack_mode(env, fif_mode::interpreting);
	mode_switch_scope* m = static_cast<mode_switch_scope*>(env.compiler_stack.back().get());
	m->interpreted_link = o;

	run_to_scope_end(env);
	env.source_stack.pop_back();
	restore_compiler_stack_mode(env);
	env.compiler_stack.pop_back();
}

inline void run_fif_interpreter(environment& env, std::string_view on_text, interpreter_stack& s) {
	env.source_stack.push_back(std::string_view(on_text));
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>());
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().get());
	o->interpreter_state = std::move(s);
	
	switch_compiler_stack_mode(env, fif_mode::interpreting);
	mode_switch_scope* m = static_cast<mode_switch_scope*>(env.compiler_stack.back().get());
	m->interpreted_link = o;

	run_to_scope_end(env);
	env.source_stack.pop_back();
	restore_compiler_stack_mode(env);

	s = std::move(o->interpreter_state);
	env.compiler_stack.pop_back();
}

#ifdef USE_LLVM
inline LLVMErrorRef module_transform(void* Ctx, LLVMModuleRef Mod) {
	LLVMPassBuilderOptionsRef pass_opts = LLVMCreatePassBuilderOptions();
	LLVMPassBuilderOptionsSetLoopInterleaving(pass_opts, true);
	LLVMPassBuilderOptionsSetLoopVectorization(pass_opts, true);
	LLVMPassBuilderOptionsSetSLPVectorization(pass_opts, true);
	LLVMPassBuilderOptionsSetLoopUnrolling(pass_opts, true);
	LLVMPassBuilderOptionsSetForgetAllSCEVInLoopUnroll(pass_opts, false);
	LLVMPassBuilderOptionsSetMergeFunctions(pass_opts, true);
	LLVMErrorRef E = LLVMRunPasses(Mod, "default<O3>", nullptr, pass_opts);
	//LLVMErrorRef E = LLVMRunPasses(Mod, "instcombine", nullptr, pass_opts);
	LLVMDisposePassBuilderOptions(pass_opts);
	if(E) {
		auto msg = LLVMGetErrorMessage(E);
		//std::cout << msg << std::endl;
		LLVMDisposeErrorMessage(msg);
		std::abort();
	}
	return E;
}

inline LLVMErrorRef perform_transform(void* Ctx, LLVMOrcThreadSafeModuleRef* the_module, LLVMOrcMaterializationResponsibilityRef MR) {
	return LLVMOrcThreadSafeModuleWithModuleDo(*the_module, module_transform, Ctx);
}
#endif

inline uint16_t* do_import_call(state_stack& s, uint16_t* p, environment* e) {
	fif_call imm = nullptr;
	memcpy(&imm, p, 8);
	imm(s, p + 4, e);
	return p + 4;
}

inline void add_import(std::string_view name, void* ptr, fif_call interpreter_implementation, std::vector<int32_t> const& params, std::vector<int32_t> const& returns, environment& env) {
	env.imported_functions.push_back(import_item{ std::string(name), ptr });

	std::vector<int32_t> itype_list;
	for(auto j = params.size(); j-->0;) {
		itype_list.push_back(params[j]);
	}
	auto pcount = itype_list.size();
	if(!returns.empty()) {
		itype_list.push_back(-1);
		for(auto r : returns) {
			itype_list.push_back(r);
		}
	}

	int32_t start_types = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), itype_list.begin(), itype_list.end());
	int32_t count_types = int32_t(itype_list.size());

	auto nstr = std::string(name);
	int32_t old_word = -1;
	if(auto it = env.dict.words.find(nstr); it != env.dict.words.end()) {
		old_word = it->second;
	}

	env.dict.words.insert_or_assign(nstr, int32_t(env.dict.word_array.size()));
	env.dict.word_array.emplace_back();
	env.dict.word_array.back().specialization_of = old_word;
	env.dict.word_array.back().stack_types_start = start_types;
	env.dict.word_array.back().stack_types_count = int32_t(pcount);

	auto instance_num = int32_t(env.dict.all_instances.size());
	env.dict.word_array.back().instances.push_back(instance_num);

	interpreted_word_instance wi;
	auto redirect_bytes_off = env.compiled_bytes.return_new_memory(6);
	auto redirect_bytes_ptr = env.compiled_bytes.memory_at(redirect_bytes_off);

	redirect_bytes_ptr[0] = env.dict.get_builtin(do_import_call);
	fif_call imm = interpreter_implementation;
	memcpy(redirect_bytes_ptr + 1, &imm, 8);
	redirect_bytes_ptr[5] = env.dict.get_builtin(function_return);

	wi.llvm_compilation_finished = true;
	wi.stack_types_start = start_types;
	wi.stack_types_count = count_types;
	wi.typechecking_level = 3;
	wi.is_imported_function = true;
	wi.compiled_offset = redirect_bytes_off;

#ifdef USE_LLVM
	auto fn_desc = std::span<int32_t const>(itype_list.begin(), itype_list.end());
	auto fn_type = llvm_function_type_from_desc(env, fn_desc, wi.llvm_parameter_permutation);
	wi.llvm_function = LLVMAddFunction(env.llvm_module, nstr.c_str(), fn_type.fn_type);
	if(fn_type.out_ptr_type) {
		auto nocapattribute = LLVMCreateEnumAttribute(env.llvm_context, 23, 1);
		LLVMAddAttributeAtIndex(wi.llvm_function, 1 + fn_type.ret_param_index, nocapattribute);
		//auto sret_attribute = LLVMCreateTypeAttribute(env.llvm_context, 78, fn_type.out_ptr_type);
		//LLVMAddAttributeAtIndex(wi.llvm_function, 1 + fn_type.ret_param_index, sret_attribute);
	}
	LLVMSetFunctionCallConv(wi.llvm_function, NATIVE_CC);
	LLVMSetLinkage(wi.llvm_function, LLVMLinkage::LLVMExternalLinkage);
#endif

	env.dict.all_instances.emplace_back(std::move(wi));
}

#ifdef USE_LLVM
inline void perform_jit(environment& e) {
	//add_exportable_functions_to_globals(e);

	char* out_message = nullptr;
	auto result = LLVMVerifyModule(e.llvm_module, LLVMVerifierFailureAction::LLVMPrintMessageAction, &out_message);
	if(result) {
		e.report_error(out_message);
		return;
	}
	if(out_message)
		LLVMDisposeMessage(out_message);
	
	LLVMDisposeBuilder(e.llvm_builder);
	e.llvm_builder = nullptr;

	// ORC JIT
	auto jit_builder = LLVMOrcCreateLLJITBuilder();
	assert(jit_builder);

	auto target_machine = LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(e.llvm_target_machine);
	e.llvm_target_machine = nullptr;
	LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(jit_builder, target_machine);

	auto errora = LLVMOrcCreateLLJIT(&e.llvm_jit, jit_builder);
	if(errora) {
		auto msg = LLVMGetErrorMessage(errora);
		e.report_error(msg);
		LLVMDisposeErrorMessage(msg);
		return;
	}

	if(!e.llvm_jit) {
		e.report_error("failed to create jit");
		return;
	}


	LLVMOrcIRTransformLayerRef TL = LLVMOrcLLJITGetIRTransformLayer(e.llvm_jit);
	LLVMOrcIRTransformLayerSetTransform(TL, *perform_transform, nullptr);

	LLVMOrcThreadSafeModuleRef orc_mod = LLVMOrcCreateNewThreadSafeModule(e.llvm_module, e.llvm_ts_context);
	e.llvm_module = nullptr;

	LLVMOrcExecutionSessionRef execution_session = LLVMOrcLLJITGetExecutionSession(e.llvm_jit);
	LLVMOrcJITDylibRef main_dyn_lib = LLVMOrcLLJITGetMainJITDylib(e.llvm_jit);
	if(!main_dyn_lib) {
		e.report_error("failed to get main dylib");
		std::abort();
	}

	// add imports
	std::vector< LLVMOrcCSymbolMapPair> import_symbols;
	for(auto& i : e.imported_functions) {
		auto name = LLVMOrcLLJITMangleAndIntern(e.llvm_jit, i.name.c_str());
		LLVMJITEvaluatedSymbol sym;
		sym.Address = (LLVMOrcExecutorAddress)i.ptr;
		sym.Flags.GenericFlags = LLVMJITSymbolGenericFlags::LLVMJITSymbolGenericFlagsCallable;
		sym.Flags.TargetFlags = 0;
		import_symbols.push_back(LLVMOrcCSymbolMapPair{ name, sym });
	}

	if(import_symbols.size() > 0) {
		auto import_mr = LLVMOrcAbsoluteSymbols(import_symbols.data(), import_symbols.size());
		auto import_result = LLVMOrcJITDylibDefine(main_dyn_lib, import_mr);
		if(import_result) {
			auto msg = LLVMGetErrorMessage(import_result);
			e.report_error(msg);
			LLVMDisposeErrorMessage(msg);
			return;
		}
	}

	auto error = LLVMOrcLLJITAddLLVMIRModule(e.llvm_jit, main_dyn_lib, orc_mod);
	if(error) {
		auto msg = LLVMGetErrorMessage(error);
		e.report_error(msg);
		LLVMDisposeErrorMessage(msg);
		return;
	}
}
#endif


inline dup_evaluation check_dup(int32_t type, fif::environment& env) {
	dup_evaluation result;

	if(env.dict.type_array[type].duptype) {
		result = *(env.dict.type_array[type].duptype);
		return result;
	}

	std::vector<int64_t> initial_cells;
	for(int32_t i = 0; i < env.dict.type_array[type].cell_size; ++i) {
		initial_cells.push_back(-i);
	}
	auto old_mode = env.mode;
	env.mode = fif_mode::tc_level_2;

	auto* ws = env.compiler_stack.back()->working_state();

	ws->push_back_main(vsize_obj(type, uint32_t(env.dict.type_array[type].cell_size * 8), (unsigned char*)(initial_cells.data())));
	execute_fif_word(fif::parse_result{ "dup", false }, env, false);

	auto copy = ws->popr_main();
	auto original = ws->popr_main();
	if(original.type != type) {
		result.alters_source = true;
	} else {
		int64_t* odat = (int64_t*)(original.data());
		for(int32_t i = 0; i < env.dict.type_array[type].cell_size; ++i) {
			if(odat[i] != initial_cells[i]) {
				result.alters_source = true;
				break;
			}
		}
	}
	if(copy.type != type) {
		result.copy_altered = true;
	} else {
		int64_t* odat = (int64_t*)(copy.data());
		for(int32_t i = 0; i < env.dict.type_array[type].cell_size; ++i) {
			if(odat[i] != initial_cells[i]) {
				result.copy_altered = true;
				break;
			}
		}
	}
	env.mode = old_mode;

	env.dict.type_array[type].duptype = result;
	return result;
}


}

#include "fif_language.hpp"
