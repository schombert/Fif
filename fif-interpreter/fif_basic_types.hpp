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
#include "fif_basic_types.hpp"


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
	struct by_value {
	};

	unsigned char internal_buffer[byte_size] = { 0 };
	int32_t type = 0;
	uint32_t size = 0;

	vsize_obj() noexcept {
	}
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
	unsigned char* data() noexcept {
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
		return (type_size*)(allocation + (sizeof(type_size) * first_tcapacity) + first_bcapacity);
	}
	unsigned char* first_b() const {
		return (allocation + sizeof(type_size) * first_tcapacity);
	}
	unsigned char* second_b() const {
		return (allocation + (sizeof(type_size) * first_tcapacity) + first_bcapacity + sizeof(type_size) * second_tcapacity);
	}
public:
	single_allocation_2xvsize_vector() noexcept {
	}
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
	bool immediate = false;
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
constexpr inline int32_t fif_memory_struct = 19;

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

		//types.insert_or_assign(std::string("llvm.m-struct"), fif_memory_struct);
		type_array.emplace_back();
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

	opaque_compiler_data(opaque_compiler_data* parent) : parent(parent) {
	}

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
		return parent ? parent->build_alloca(type) : nullptr;
	}
	virtual void increase_frame_size(int32_t sz) {
		if(parent) parent->increase_frame_size(sz);
	}
};

struct import_item {
	std::string name;
	void* ptr = nullptr;
};

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
	lexical_scope(int32_t a, bool b) noexcept : allocated_bytes(a), allow_shadowing(b), new_top_scope(false) {
	}
	lexical_scope(int32_t a, bool b, bool c) noexcept : allocated_bytes(a), allow_shadowing(b), new_top_scope(c) {
	}
};

struct function_call {
	uint16_t* return_address = nullptr;
	int32_t return_local_size = 0;
};


enum class fif_mode : uint16_t {
	primary_fn_mask = 0x007,
	interpreting = 0x001,
	compiling_bytecode = 0x002,
	compiling_llvm = 0x003,
	terminated = 0x004,
	error_base = 0x005,
	error = 0x01D,
	typechecking = 0x006,
	compilation_masked = 0x010,
	failure = 0x008,
	typecheck_provisional = 0x020,
	typecheck_recursion = 0x040,
	tc_level_mask = 0xF00,
	tc_level_1 = 0x106,
	tc_level_2 = 0x206,
	tc_level_3 = 0x306
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

}
