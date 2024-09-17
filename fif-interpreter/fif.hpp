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

class single_allocation_2xi32_2xi64_vector {
protected:
	unsigned char* allocation = nullptr;
	int32_t first_capacity = 0;
	int32_t second_capacity = 0;
	int32_t first_size = 0;
	int32_t second_size = 0;

	int32_t* first_i32() const {
		return (int32_t*)allocation;
	}
	int32_t* second_i32() const {
		return (int32_t*)(allocation + (sizeof(int32_t) + sizeof(int64_t)) * first_capacity);
	}
	int64_t* first_i64() const {
		return (int64_t*)(allocation + sizeof(int32_t) * first_capacity);
	}
	int64_t* second_i64() const {
		return (int64_t*)(allocation + (sizeof(int32_t) + sizeof(int64_t)) * first_capacity + sizeof(int64_t) * second_capacity);
	}
public:
	single_allocation_2xi32_2xi64_vector() noexcept { }
	single_allocation_2xi32_2xi64_vector(single_allocation_2xi32_2xi64_vector const& o) noexcept : first_capacity(o.first_capacity), second_capacity(o.second_capacity), first_size(o.first_size), second_size(o.second_size) {
		allocation = new unsigned char[o.first_capacity * (sizeof(int32_t) + sizeof(int64_t)) + o.second_capacity * (sizeof(int32_t) + sizeof(int64_t))];
		memcpy(allocation, o.allocation, o.first_capacity * (sizeof(int32_t) + sizeof(int64_t)) + o.second_capacity * (sizeof(int32_t) + sizeof(int64_t)));
	}
	single_allocation_2xi32_2xi64_vector(single_allocation_2xi32_2xi64_vector&& o) noexcept : allocation(o.allocation), first_capacity(o.first_capacity), second_capacity(o.second_capacity), first_size(o.first_size), second_size(o.second_size) {
		o.allocation = nullptr;
		o.first_capacity = 0;
		o.second_capacity = 0;
		o.first_size = 0;
		o.second_size = 0;
	}
	~single_allocation_2xi32_2xi64_vector() noexcept {
		delete[] allocation;
	}

	single_allocation_2xi32_2xi64_vector& operator=(single_allocation_2xi32_2xi64_vector const& o) noexcept {
		first_capacity = o.first_capacity;
		second_capacity = o.second_capacity;
		first_size = o.first_size;
		second_size = o.second_size;
		
		allocation = new unsigned char[o.first_capacity * (sizeof(int32_t) + sizeof(int64_t)) + o.second_capacity * (sizeof(int32_t) + sizeof(int64_t))];
		memcpy(allocation, o.allocation, o.first_capacity * (sizeof(int32_t) + sizeof(int64_t)) + o.second_capacity * (sizeof(int32_t) + sizeof(int64_t)));

		return *this;
	}
	single_allocation_2xi32_2xi64_vector& operator=(single_allocation_2xi32_2xi64_vector&& o) noexcept {
		allocation = o.allocation;
		first_capacity = o.first_capacity;
		second_capacity = o.second_capacity;
		first_size = o.first_size;
		second_size = o.second_size;

		o.allocation = nullptr;
		o.first_capacity = 0;
		o.second_capacity = 0;
		o.first_size = 0;
		o.second_size = 0;

		return *this;
	}

	int32_t& fi32(int64_t offset) const {
		assert(0 <= offset && offset < first_size);
		return *(first_i32() + offset);
	}
	int32_t& si32(int64_t offset) const {
		assert(0 <= offset && offset < second_size);
		return *(second_i32() + offset);
	}
	int64_t& fi64(int64_t offset) const {
		assert(0 <= offset && offset < first_size);
		return *(first_i64() + offset);
	}
	int64_t& si64(int64_t offset) const {
		assert(0 <= offset && offset < second_size);
		return *(second_i64() + offset);
	}
	void clear_first() {
		first_size = 0;
	}
	void clear_second() {
		second_size = 0;
	}
	int32_t size_first() const {
		return first_size;
	}
	int32_t size_second() const {
		return second_size;
	}
	void pop_first() {
		first_size = std::max(0, first_size - 1);
	}
	void pop_second() {
		second_size = std::max(0, second_size - 1);
	}
	void push_first(int32_t a, int64_t b) {
		if(first_size >= first_capacity) {
			auto temp = std::move(*this);

			auto new_cap = std::max(temp.first_capacity * 2, 8);
			allocation = new unsigned char[new_cap * (sizeof(int32_t) + sizeof(int64_t)) + std::max(8, temp.second_capacity) * (sizeof(int32_t) + sizeof(int64_t))];
			first_capacity = new_cap;
			second_capacity = std::max(8, temp.second_capacity);
			first_size = temp.first_size;
			second_size = temp.second_size;

			memcpy(first_i32(), temp.first_i32(), sizeof(int32_t) * first_size);
			memcpy(first_i64(), temp.first_i64(), sizeof(int64_t) * first_size);
			memcpy(second_i32(), temp.second_i32(), sizeof(int32_t) * second_size);
			memcpy(second_i64(), temp.second_i64(), sizeof(int64_t) * second_size);
		}
		++first_size;
		fi32(first_size - 1) = a;
		fi64(first_size - 1) = b;
	}
	void push_second(int32_t a, int64_t b) {
		if(second_size >= second_capacity) {
			auto temp = std::move(*this);

			auto new_cap = std::max(temp.second_capacity * 2, 8);
			allocation = new unsigned char[std::max(8, temp.first_capacity)  * (sizeof(int32_t) + sizeof(int64_t)) + new_cap * (sizeof(int32_t) + sizeof(int64_t))];
			first_capacity = std::max(8, temp.first_capacity);
			second_capacity = new_cap;
			first_size = temp.first_size;
			second_size = temp.second_size;

			memcpy(first_i32(), temp.first_i32(), sizeof(int32_t) * first_size);
			memcpy(first_i64(), temp.first_i64(), sizeof(int64_t) * first_size);
			memcpy(second_i32(), temp.second_i32(), sizeof(int32_t) * second_size);
			memcpy(second_i64(), temp.second_i64(), sizeof(int64_t) * second_size);
		}
		++second_size;
		si32(second_size - 1) = a;
		si64(second_size - 1) = b;
	}
	void resize(int32_t fs, int32_t ss) {
		if(fs > first_capacity || ss > second_capacity) {
			auto temp = std::move(*this);

			auto new_fcap = fs > temp.first_capacity ? std::max(temp.first_capacity * 2, 8) : std::max(8, temp.first_capacity);
			auto new_scap = ss > temp.second_capacity ? std::max(temp.second_capacity * 2, 8) : std::max(8, temp.second_capacity);
			allocation = new unsigned char[new_fcap * (sizeof(int32_t) + sizeof(int64_t)) + new_scap * (sizeof(int32_t) + sizeof(int64_t))];
			first_capacity = new_fcap;
			second_capacity = new_scap;
			first_size = temp.first_size;
			second_size = temp.second_size;

			memcpy(first_i32(), temp.first_i32(), sizeof(int32_t) * first_size);
			memcpy(first_i64(), temp.first_i64(), sizeof(int64_t) * first_size);
			memcpy(second_i32(), temp.second_i32(), sizeof(int32_t) * second_size);
			memcpy(second_i64(), temp.second_i64(), sizeof(int64_t) * second_size);
		}
		if(fs > first_size) { // zero mem
			memset(first_i32() + first_size, 0, sizeof(int32_t) * (fs - first_size));
			memset(first_i64() + first_size, 0, sizeof(int64_t) * (fs - first_size));
		}
		if(ss > second_size) { // zero mem
			memset(second_i32() + second_size, 0, sizeof(int32_t) * (ss - second_size));
			memset(second_i64() + second_size, 0, sizeof(int64_t) * (ss - second_size));
		}
		first_size = fs;
		second_size = ss;
	}
	void trim_to(int32_t fs, int32_t ss) {
		fs = std::min(fs, first_size);
		ss = std::min(ss, second_size);
		if(fs != first_size) {
			std::memmove(first_i32(), first_i32() + (first_size - fs), fs * sizeof(int32_t));
			std::memmove(first_i64(), first_i64() + (first_size - fs), fs * sizeof(int64_t));
			first_size = fs;
		}
		if(ss != second_size) {
			std::memmove(second_i32(), second_i32() + (second_size - ss), ss * sizeof(int32_t));
			std::memmove(second_i64(), second_i64() + (second_size - ss), ss * sizeof(int64_t));
			second_size = ss;
		}
	}
};

class state_stack {
protected:
	single_allocation_2xi32_2xi64_vector contents;
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
	virtual void pop_return() {
		contents.pop_second();
		min_return_depth = std::min(min_return_depth, contents.size_second());
	}
	int64_t main_data(size_t index) const {
		return contents.fi64(int64_t(index));
	}
	int64_t return_data(size_t index) const {
		return contents.si64(int64_t(index));
	}
	LLVMValueRef main_ex(size_t index) const {
		auto v = contents.fi64(int64_t(index));
		if((v & 1) == 0)
			return (LLVMValueRef)v;
		else
			return nullptr;
	}
	LLVMValueRef return_ex(size_t index) const {
		auto v = contents.si64(int64_t(index));
		if((v & 1) == 0)
			return (LLVMValueRef)v;
		else
			return nullptr;
	}
	int64_t main_data_back(size_t index) const {
		return contents.fi64(contents.size_first() - (1 + int64_t(index)));
	}
	int64_t return_data_back(size_t index) const {
		return contents.si64(contents.size_second() - (1 + int64_t(index)));
	}
	LLVMValueRef main_ex_back(size_t index) const {
		auto v = contents.fi64(contents.size_first() - (1 + int64_t(index)));
		if((v & 1) == 0)
			return (LLVMValueRef)v;
		else
			return nullptr;
	}
	LLVMValueRef return_ex_back(size_t index) const {
		auto v = contents.si64(contents.size_second() - (1 + int64_t(index)));
		if((v & 1) == 0)
			return (LLVMValueRef)v;
		else
			return nullptr;
	}
	void set_main_data(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, int32_t(index));
		contents.fi64(int32_t(index)) = value;
	}
	void set_return_data(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, int32_t(index));
		contents.si64(int32_t(index)) = value;
	}
	void set_main_ex(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, int32_t(index));
		contents.fi64(int32_t(index)) = (int64_t)value;
	}
	void set_return_ex(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, int32_t(index));
		contents.si64(int32_t(index)) = (int64_t)value;
	}
	void set_main_data_back(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, contents.size_first() - int32_t(index + 1));
		contents.fi64(contents.size_first() - (1 + int64_t(index))) = value;
	}
	void set_return_data_back(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, contents.size_second() - int32_t(index + 1));
		contents.si64(contents.size_second() - (1 + int64_t(index))) = value;
	}
	void set_main_ex_back(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, contents.size_first() - int32_t(index + 1));
		contents.fi64(contents.size_first() - (1 + int64_t(index))) = (int64_t)value;
	}
	void set_return_ex_back(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, contents.size_second() - int32_t(index + 1));
		contents.si64(contents.size_second() - (1 + int64_t(index))) = (int64_t)value;
	}
	void move_into(state_stack&& other) {
		*this = std::move(other);
	}
	void copy_into(state_stack const& other) {
		*this = other;
	}
	void push_back_main(int32_t t, int64_t data, LLVMValueRef expr) {
		if(expr)
			contents.push_first(t, (int64_t)expr);
		else
			contents.push_first(t, data);
	}
	virtual void push_back_return(int32_t t, int64_t data, LLVMValueRef expr) {
		if(expr)
			contents.push_second(t, (int64_t)expr);
		else
			contents.push_second(t, data);
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
	int32_t main_type(size_t index) const {
		return contents.fi32(int32_t(index));
	}
	int32_t return_type(size_t index) const {
		return contents.si32(int32_t(index));
	}
	int32_t main_type_back(size_t index) const {
		return contents.fi32(contents.size_first() - int32_t(1 + index));
	}
	int32_t return_type_back(size_t index) const {
		return contents.si32(contents.size_second() - int32_t(1 + index));
	}
	void set_main_type(size_t index, int32_t t) {
		contents.fi32(int32_t(index)) = t;
		min_main_depth = std::min(min_main_depth, int32_t(index));
	}
	void set_return_type(size_t index, int32_t t) {
		contents.si32(int32_t(index)) = t;
		min_return_depth = std::min(min_return_depth, int32_t(index));
	}
	void set_main_type_back(size_t index, int32_t t) {
		contents.fi32(contents.size_first() - int32_t(1 + index)) = t;
		min_main_depth = std::min(min_main_depth, contents.size_first() - int32_t(index + 1));
	}
	void set_return_type_back(size_t index, int32_t t) {
		contents.si32(contents.size_second() - int32_t(1 + index)) = t;
		min_return_depth = std::min(min_return_depth, contents.size_second() - int32_t(index + 1));
	}
};

using llvm_stack = state_stack;
using type_stack = state_stack;
using interpreter_stack = state_stack;

class environment;
struct type;

using fif_call = int32_t * (*)(state_stack&, int32_t*, environment*);

struct interpreted_word_instance {
	std::vector<int32_t> compiled_bytecode;
	std::vector<int32_t> llvm_parameter_permutation;
	LLVMValueRef llvm_function = nullptr;
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
	int32_t typechecking_level = 0;
	bool being_compiled = false;
	bool llvm_compilation_finished = false;
	bool is_imported_function = false;
};

struct compiled_word_instance {
	std::vector<int32_t> llvm_parameter_permutation;
	fif_call implementation = nullptr;
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
};

using word_types = std::variant<interpreted_word_instance, compiled_word_instance>;

struct word {
	std::vector<int32_t> instances;
	std::string source;
	int32_t specialization_of = -1;
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
	bool treat_as_base = false;
	bool immediate =  false;
	bool being_typechecked = false;
};

inline LLVMValueRef empty_type_fn(LLVMValueRef r, int32_t type, environment*) {
	return r;
}

#ifdef USE_LLVM
using llvm_zero_expr = LLVMValueRef (*)(LLVMContextRef, int32_t t, environment*);
#endif
using interpreter_zero_expr = int64_t(*)(int32_t t, environment*);
using interpreted_new = int64_t(*)();

struct type {
	static constexpr uint32_t FLAG_REFCOUNTED = 0x00000001;
	static constexpr uint32_t FLAG_SINGLE_MEMBER = 0x00000002;
	static constexpr uint32_t FLAG_TEMPLATE = 0x00000004;

#ifdef USE_LLVM
	LLVMTypeRef llvm_type = nullptr;
	llvm_zero_expr zero_constant = nullptr;
#else
	void* llvm_type = nullptr;
	void* zero_constant = nullptr;
#endif
	interpreter_zero_expr interpreter_zero = nullptr;

	int32_t type_slots = 0;
	int32_t non_member_types = 0;
	int32_t decomposed_types_start = 0;
	int32_t decomposed_types_count = 0;

	uint32_t flags = 0;

	bool refcounted_type() const {
		return (flags & FLAG_REFCOUNTED) != 0;
	}
	bool single_member_struct() const {
		return (flags & FLAG_SINGLE_MEMBER) != 0;
	}
	bool is_struct_template() const {
		return (flags & FLAG_TEMPLATE) != 0;
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

class environment;

class dictionary {
public:
	ankerl::unordered_dense::map<std::string, int32_t> words;
	ankerl::unordered_dense::map<std::string, int32_t> types;
	std::vector<word> word_array;
	std::vector<type> type_array;
	std::vector<word_types> all_instances;
	std::vector<int32_t> all_compiled;
	std::vector<int32_t> all_stack_types;

#ifdef USE_LLVM
	void ready_llvm_types(LLVMContextRef llvm_context) {
		type_array[fif_i32].llvm_type = LLVMInt32TypeInContext(llvm_context);
		type_array[fif_i32].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt32TypeInContext(c), 0, true);
		};
		type_array[fif_type].llvm_type = LLVMInt32TypeInContext(llvm_context);
		type_array[fif_type].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt32TypeInContext(c), 0, true);
		};
		type_array[fif_f32].llvm_type = LLVMFloatTypeInContext(llvm_context);
		type_array[fif_f32].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstReal(LLVMFloatTypeInContext(c), 0.0);
		};
		type_array[fif_i64].llvm_type = LLVMInt64TypeInContext(llvm_context);
		type_array[fif_i64].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt64TypeInContext(c), 0, true);
		};
		type_array[fif_f64].llvm_type = LLVMDoubleTypeInContext(llvm_context);
		type_array[fif_f64].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstReal(LLVMDoubleTypeInContext(c), 0.0);
		};
		type_array[fif_bool].llvm_type =  LLVMInt1TypeInContext(llvm_context);
		type_array[fif_bool].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt1TypeInContext(c), 0, true);
		};
		type_array[fif_u32].llvm_type = LLVMInt32TypeInContext(llvm_context);
		type_array[fif_u32].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt32TypeInContext(c), 0, false);
		};
		type_array[fif_u64].llvm_type = LLVMInt64TypeInContext(llvm_context);
		type_array[fif_u64].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt64TypeInContext(c), 0, false);
		};
		type_array[fif_u16].llvm_type = LLVMInt16TypeInContext(llvm_context);
		type_array[fif_u16].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt16TypeInContext(c), 0, false);
		};
		type_array[fif_u8].llvm_type = LLVMInt8TypeInContext(llvm_context);
		type_array[fif_u8].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt8TypeInContext(c), 0, false);
		};
		type_array[fif_i16].llvm_type = LLVMInt16TypeInContext(llvm_context);
		type_array[fif_i16].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt16TypeInContext(c), 0, true);
		};
		type_array[fif_i8].llvm_type = LLVMInt8TypeInContext(llvm_context);
		type_array[fif_i8].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMConstInt(LLVMInt8TypeInContext(c), 0, true);
		};
		type_array[fif_nil].llvm_type = LLVMVoidTypeInContext(llvm_context);
		type_array[fif_nil].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMGetUndef(LLVMVoidTypeInContext(c));
		};
		type_array[fif_ptr].llvm_type = LLVMPointerTypeInContext(llvm_context, 0);
		type_array[fif_ptr].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMGetUndef(LLVMPointerTypeInContext(c, 0));
		};
		type_array[fif_opaque_ptr].llvm_type = LLVMPointerTypeInContext(llvm_context, 0);
		type_array[fif_opaque_ptr].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMGetUndef(LLVMPointerTypeInContext(c, 0));
		};
		type_array[fif_struct].llvm_type = LLVMVoidTypeInContext(llvm_context);
		type_array[fif_struct].zero_constant = +[](LLVMContextRef c, int32_t t, environment*) {
			return LLVMGetUndef(LLVMVoidTypeInContext(c));
		};
	}
#endif
	dictionary() {
		types.insert_or_assign(std::string("i32"), fif_i32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("f32"), fif_f32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("bool"), fif_bool);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("type"), fif_type);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("i64"), fif_i64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("f64"), fif_f64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("u32"), fif_u32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("u64"), fif_u64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("i16"), fif_i16);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("u16"), fif_u16);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("i8"), fif_i8);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("u8"), fif_u8);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("nil"), fif_nil);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("ptr"), fif_ptr);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 1, 0, 0, 0 });
		types.insert_or_assign(std::string("opaque_ptr"), fif_nil);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
		types.insert_or_assign(std::string("struct"), fif_struct);
		type_array.push_back(type{ nullptr, nullptr, nullptr, 0, 0, 0, 0 });
	}
};


enum class control_structure : uint8_t {
	none, function, str_if, str_while_loop, str_do_loop, mode_switch, globals, lexical_scope, rt_function
};

class compiler_state;

struct typecheck_3_record {
	int32_t stack_height_added_at = 0;
	int32_t rstack_height_added_at = 0;
	int32_t stack_consumed = 0;
	int32_t rstack_consumed = 0;
};

struct internal_lvar_data {
	union {
		int64_t data = 0; // only in interpreter mode
		LLVMValueRef expression; // only in llvm mode
	};
	int32_t type = 0;
	bool is_stack_variable = false;

	internal_lvar_data() noexcept {
		memset(this, 0, sizeof(internal_lvar_data));
	}
	internal_lvar_data(internal_lvar_data const& o) noexcept {
		memcpy(this, &o, sizeof(internal_lvar_data));
	}
	internal_lvar_data(internal_lvar_data&& o) noexcept {
		memcpy(this, &o, sizeof(internal_lvar_data));
	}
	internal_lvar_data& operator=(internal_lvar_data const& o) noexcept {
		memcpy(this, &o, sizeof(internal_lvar_data));
		return *this;
	}
	internal_lvar_data& operator=(internal_lvar_data&& o) noexcept {
		memcpy(this, &o, sizeof(internal_lvar_data));
		return *this;
	}
};
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
	virtual std::vector<int32_t>* bytecode_compilation_progress() {
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
	virtual int32_t get_var(std::string const& name) {
		return parent ? parent->get_var(name) : -1;
	}
	virtual int32_t create_var(std::string const& name, int32_t type) {
		return parent ? parent->create_var(name, type) : -1;
	}
	virtual int32_t create_let(std::string const& name, int32_t type, int64_t data, LLVMValueRef expression) {
		return parent ? parent->create_let(name, type, data, expression) : -1;
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) {
		return parent ? parent->re_let(index, type, data, expression) : true;
	}
	virtual std::vector<int32_t>* type_substitutions() {
		return parent ? parent->type_substitutions() : nullptr;
	}
	virtual internal_lvar_data* get_lvar_storage(int32_t offset) {
		return parent ? parent->get_lvar_storage(offset) : nullptr;
	}
	virtual void resize_lvar_storage(int32_t sz) {
		if(parent) parent->resize_lvar_storage(sz);
	}
	virtual std::vector< internal_lvar_data> copy_lvar_storage() {
		return parent ? parent->copy_lvar_storage() : std::vector< internal_lvar_data>{ };
	}
	virtual void set_lvar_storage(std::vector< internal_lvar_data> const& v) {
		if(parent) parent->set_lvar_storage(v);
	}
	virtual int32_t size_lvar_storage() {
		return parent ? parent->size_lvar_storage() : 0;
	}
	virtual void delete_locals() const { }
};


class compiler_state {
public:
	std::unique_ptr<opaque_compiler_data> data;
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

	std::vector<std::unique_ptr<opaque_compiler_data>> compiler_stack;
	std::vector<std::string_view> source_stack;
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

class compiler_globals_layer : public opaque_compiler_data {
public:
	ankerl::unordered_dense::map<std::string, std::unique_ptr<internal_lvar_data>> global_vars;
	environment& env;
	compiler_globals_layer(opaque_compiler_data* p, environment& env) : opaque_compiler_data(p), env(env) {
	}

	virtual control_structure get_type() override {
		return control_structure::globals;
	}

	internal_lvar_data* get_global_var(std::string const& name) {
		if(auto it = global_vars.find(name); it != global_vars.end()) {
			return it->second.get();
		}
		return nullptr;
	}
	internal_lvar_data* create_global_var(std::string const& name, int32_t type) {
		if(auto it = global_vars.find(name); it != global_vars.end()) {
			if(it->second->type == type)
				return it->second.get();
			else
				return nullptr;
		}
		auto added = global_vars.insert_or_assign(name, std::make_unique<internal_lvar_data>());
		added.first->second->type = type;
		added.first->second->data = 0;
#ifdef USE_LLVM
		added.first->second->expression = LLVMAddGlobal(env.llvm_module, env.dict.type_array[type].llvm_type, name.c_str());
		LLVMSetInitializer(added.first->second->expression, env.dict.type_array[type].zero_constant(env.llvm_context, type, &env));
#endif
		return added.first->second.get();
	}
};

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

	dict.ready_llvm_types(llvm_context);
#endif

	compiler_stack.push_back(std::make_unique<compiler_globals_layer>(nullptr, *this));
}

inline std::string word_name_from_id(int32_t w, environment const& e) {
	for(auto& p : e.dict.words) {
		if(p.second == w)
			return p.first;
	}
	return "@unknown (" + std::to_string(w)  + ")";
}

inline int32_t* illegal_interpretation(state_stack& s, int32_t* p, environment* e) {
	e->report_error("attempted to perform an illegal operation under interpretation");
	e->mode = fif_mode::error;
	return nullptr;
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

#ifdef USE_LLVM
inline LLVMValueRef struct_zero_constant(LLVMContextRef c, int32_t t, environment* e) {
	std::vector<LLVMValueRef> zvals;
	for(int32_t j = 1; j < e->dict.type_array[t].decomposed_types_count; ++j) {
		auto st = e->dict.all_stack_types[e->dict.type_array[t].decomposed_types_start + j];
		zvals.push_back(e->dict.type_array[st].zero_constant(c, st, e));
	}
	return LLVMConstNamedStruct(e->dict.type_array[t].llvm_type, zvals.data(), uint32_t(zvals.size()));
}
#endif

inline int32_t interepreter_size(int32_t type, environment& env) {
	if(env.dict.type_array[type].decomposed_types_count == 0)
		return 8;
	auto main_type = env.dict.all_stack_types[env.dict.type_array[type].decomposed_types_start];
	return 8 * (env.dict.type_array[type].decomposed_types_count - 1 - env.dict.type_array[main_type].non_member_types);
}

inline int32_t struct_child_count(int32_t type, environment& env) {
	if(env.dict.type_array[type].decomposed_types_count == 0)
		return 0;
	auto main_type = env.dict.all_stack_types[env.dict.type_array[type].decomposed_types_start];
	return (env.dict.type_array[type].decomposed_types_count - 1 - env.dict.type_array[main_type].non_member_types);
}

inline int64_t interpreter_struct_zero_constant(int32_t t, environment* e) {
	if(e->dict.type_array[t].single_member_struct()) {
		auto child_index = e->dict.type_array[t].decomposed_types_start + 1 + 0;
		auto child_type = e->dict.all_stack_types[child_index];
		if(e->dict.type_array[child_type].interpreter_zero)
			return e->dict.type_array[child_type].interpreter_zero(child_type, e);
		else
			return 0;
	} else {
		auto bytes = interepreter_size(t, *e);
		auto mem = (char*)malloc(bytes);
		memset(mem, 0, bytes);
		return (int64_t)mem;
	}
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

inline int32_t instantiate_templated_struct_full(int32_t template_base, std::vector<int32_t> const& final_subtype_list, environment& env) {

	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

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

	std::vector<LLVMTypeRef> ctypes;
	for(uint32_t j = 0; j < final_subtype_list.size(); ++j) {
		ctypes.push_back(env.dict.type_array[final_subtype_list[j]].llvm_type);
	}
	for(int32_t j = 0; j < env.dict.type_array[template_base].non_member_types; ++j)
		ctypes.pop_back();

	if(ctypes.size() != 1) {
		std::string autoname = "struct#" + std::to_string(env.dict.type_array.size());
#ifdef USE_LLVM
		auto ty = LLVMStructCreateNamed(env.llvm_context, autoname.c_str());
		LLVMStructSetBody(ty, ctypes.data(), uint32_t(ctypes.size()), false);
		env.dict.type_array.back().llvm_type = ty;
		env.dict.type_array.back().zero_constant = struct_zero_constant;
#endif
		env.dict.type_array.back().interpreter_zero = interpreter_struct_zero_constant;
	} else {
#ifdef USE_LLVM
		env.dict.type_array.back().llvm_type = ctypes[0];
		env.dict.type_array.back().zero_constant = env.dict.type_array[final_subtype_list[0]].zero_constant;
#endif
		env.dict.type_array.back().interpreter_zero = env.dict.type_array[final_subtype_list[0]].interpreter_zero;
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(final_subtype_list.size() + 1);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	
	if(ctypes.size() == 1)
		env.dict.type_array.back().flags |= type::FLAG_SINGLE_MEMBER;

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_REFCOUNTED;
	env.dict.type_array.back().type_slots = 0;
	env.dict.all_stack_types.push_back(template_base);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), final_subtype_list.begin(), final_subtype_list.end());

	return new_type;
}

inline int32_t instantiate_templated_struct(int32_t template_base, std::vector<int32_t> const& subtypes, environment& env) {
	std::vector<int32_t> final_subtype_list;
	final_subtype_list.push_back(template_base);

	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

	uint32_t match_pos = 0;
	while(match_pos < desc.size()) {
		auto mresult = resolve_span_type(desc.subspan(match_pos), subtypes, env);
		match_pos += mresult.end_match_pos;
		final_subtype_list.push_back(mresult.type);
	}

	for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
		if(env.dict.type_array[i].decomposed_types_count == int32_t(final_subtype_list.size())) {
			auto ta_start = env.dict.type_array[i].decomposed_types_start;
			bool match = true;

			for(uint32_t j = 0; j < final_subtype_list.size(); ++j) {
				if(env.dict.all_stack_types[ta_start + j] != final_subtype_list[j]) {
					match = false;
					break;
				}
			}

			if(match)
				return  int32_t(i);
		}
	}

	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();

	std::vector<LLVMTypeRef> ctypes;
	for(uint32_t j = 1; j < final_subtype_list.size(); ++j) {
		ctypes.push_back(env.dict.type_array[final_subtype_list[j]].llvm_type);
	}
	for(int32_t j = 0; j < env.dict.type_array[template_base].non_member_types; ++j)
		ctypes.pop_back();

	if(ctypes.size() != 1) {
		std::string autoname = "struct#" + std::to_string(env.dict.type_array.size());
#ifdef USE_LLVM
		auto ty = LLVMStructCreateNamed(env.llvm_context, autoname.c_str());
		LLVMStructSetBody(ty, ctypes.data(), uint32_t(ctypes.size()), false);
		env.dict.type_array.back().llvm_type = ty;
		env.dict.type_array.back().zero_constant = struct_zero_constant;
#endif
		env.dict.type_array.back().interpreter_zero = interpreter_struct_zero_constant;
	} else {
#ifdef USE_LLVM
		env.dict.type_array.back().llvm_type = ctypes[0];
		env.dict.type_array.back().zero_constant = env.dict.type_array[final_subtype_list[0]].zero_constant;
#endif
		env.dict.type_array.back().interpreter_zero = env.dict.type_array[final_subtype_list[0]].interpreter_zero;
	}
	env.dict.type_array.back().decomposed_types_count = uint32_t(final_subtype_list.size());
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_REFCOUNTED;
	if(ctypes.size() == 1)
		env.dict.type_array.back().flags |= type::FLAG_SINGLE_MEMBER;

	env.dict.type_array.back().type_slots = 0;
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), final_subtype_list.begin(), final_subtype_list.end());

	return new_type;
}

inline int32_t make_struct_type(std::string_view name, std::span<int32_t const> subtypes, std::vector<std::string_view> const& member_names, environment& env, int32_t template_types, int32_t extra_count) {

	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();
	env.dict.types.insert_or_assign(std::string(name), new_type);

	if(template_types == 0 && extra_count == 0) {
		std::vector<LLVMTypeRef> ctypes;
		for(uint32_t j = 0; j < subtypes.size(); ++j) {
			ctypes.push_back(env.dict.type_array[subtypes[j]].llvm_type);
		}
		if(ctypes.size() != 1) {
			std::string autoname = "struct#" + std::to_string(env.dict.type_array.size());
#ifdef USE_LLVM
			auto ty = LLVMStructCreateNamed(env.llvm_context, autoname.c_str());
			LLVMStructSetBody(ty, ctypes.data(), uint32_t(ctypes.size()), false);
			env.dict.type_array.back().llvm_type = ty;
			env.dict.type_array.back().zero_constant = struct_zero_constant;
#endif
			env.dict.type_array.back().interpreter_zero = interpreter_struct_zero_constant;
		} else {
#ifdef USE_LLVM
			env.dict.type_array.back().llvm_type = ctypes[0];
			env.dict.type_array.back().zero_constant = env.dict.type_array[subtypes[0]].zero_constant;
#endif
			env.dict.type_array.back().interpreter_zero = env.dict.type_array[subtypes[0]].interpreter_zero;
		}
	} else {
#ifdef USE_LLVM
		env.dict.type_array.back().llvm_type = LLVMVoidTypeInContext(env.llvm_context);
		env.dict.type_array.back().zero_constant = struct_zero_constant;
#endif
		env.dict.type_array.back().interpreter_zero = interpreter_struct_zero_constant;
		env.dict.type_array.back().non_member_types = extra_count;
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(subtypes.size() + (template_types + extra_count == 0 ? 1 : extra_count));
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());

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


	if(template_types + extra_count > 0)
		env.dict.type_array.back().flags |= type::FLAG_TEMPLATE;
	else
		env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);

	if(subtypes.size() == 1)
		env.dict.type_array.back().flags |= type::FLAG_SINGLE_MEMBER;

	env.dict.type_array.back().flags |= type::FLAG_REFCOUNTED;
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
	if(text.substr(0, mt_end) == "struct") { // prevent "generic" struct matching
		return type_match{ -1, uint32_t(text.length()) };
	}
	if(auto it = env.dict.types.find(std::string(text.substr(0, mt_end))); it != env.dict.types.end()) {
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

		if(env.dict.type_array[it->second].type_slots != int32_t(subtypes.size())) {
			env.report_error("attempted to instantiate a type with the wrong number of parameters");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(env.dict.type_array[it->second].is_struct_template()) {
			return type_match{ instantiate_templated_struct(it->second, subtypes, env), mt_end };
		} else {
			for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
				if(env.dict.type_array[i].decomposed_types_count == int32_t(1 + subtypes.size())) {
					auto ta_start = env.dict.type_array[i].decomposed_types_start;
					if(env.dict.all_stack_types[ta_start] == it->second) {
						bool match = true;
						for(uint32_t j = 0; j < subtypes.size(); ++j) {
							if(env.dict.all_stack_types[ta_start + 1 + j] != subtypes[j]) {
								match = false;
								break;
							}
						}

						if(match) {
							return type_match{ int32_t(i), mt_end };
						}
					}
				}
			}

			int32_t new_type = int32_t(env.dict.type_array.size());
			env.dict.type_array.emplace_back();
			env.dict.type_array.back().llvm_type = env.dict.type_array[it->second].llvm_type;
			env.dict.type_array.back().decomposed_types_count = uint32_t(1 + subtypes.size());
			env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
			env.dict.type_array.back().flags = env.dict.type_array[it->second].flags;
			env.dict.type_array.back().zero_constant = env.dict.type_array[it->second].zero_constant;
			env.dict.type_array.back().type_slots = 0;

			env.dict.all_stack_types.push_back(it->second);
			for(auto t : subtypes) {
				env.dict.all_stack_types.push_back(t);
			}

			return type_match{ new_type, mt_end };
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

	
	 if(base_type == fif_ptr && !subtypes.empty() && subtypes[0] == fif_nil) {
		return type_match{ fif_opaque_ptr, mt_end };
	} else if(env.dict.type_array[base_type].is_struct_template()) {
		return type_match{ instantiate_templated_struct_full(base_type, subtypes, env), mt_end };
	} else {
		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].decomposed_types_count == int32_t(1 + subtypes.size())) {
				auto ta_start = env.dict.type_array[i].decomposed_types_start;
				if(env.dict.all_stack_types[ta_start] == base_type) {
					bool match = true;
					for(uint32_t j = 0; j < subtypes.size(); ++j) {
						if(env.dict.all_stack_types[ta_start + 1 + j] != subtypes[j]) {
							match = false;
							break;
						}
					}

					if(match) {
						return type_match{ int32_t(i), mt_end };
					}
				}
			}
		}

		int32_t new_type = int32_t(env.dict.type_array.size());
		env.dict.type_array.emplace_back();

		env.dict.type_array.back().llvm_type = env.dict.type_array[base_type].llvm_type;
		env.dict.type_array.back().decomposed_types_count = uint32_t(1 + subtypes.size());
		env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
		env.dict.type_array.back().zero_constant = env.dict.type_array[base_type].zero_constant;
		env.dict.type_array.back().flags = env.dict.type_array[base_type].flags;
		env.dict.type_array.back().type_slots = 0;

		env.dict.all_stack_types.push_back(base_type);
		for(auto t : subtypes) {
			env.dict.all_stack_types.push_back(t);
		}

		return type_match{ new_type, mt_end };
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


inline void apply_stack_description(std::span<int32_t const> desc, state_stack& ts, std::vector<int32_t> const& param_perm, environment& env) { // ret: true if function matched
	
	// stack matching

	
	int32_t match_position = 0;
	std::vector<int32_t> type_subs;
	std::vector<int64_t> params;

	auto const ssize = int32_t(ts.main_size());
	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		if(consumed_stack_cells >= ssize)
			return;
		auto match_result = fill_in_variable_types(ts.main_type_back(consumed_stack_cells), desc.subspan(match_position), type_subs, env);
		params.push_back(ts.main_data_back(consumed_stack_cells));
		if(!match_result.match_result)
			return;
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
			return;
		auto match_result = fill_in_variable_types(ts.return_type_back(consumed_rstack_cells), desc.subspan(match_position), type_subs, env);
		params.push_back(ts.return_data_back(consumed_rstack_cells));
		if(!match_result.match_result)
			return;
		match_position += match_result.match_end;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// drop consumed types
	std::vector<int32_t> perm_data;
	perm_data.resize(param_perm.size());
	ts.resize(ssize - consumed_stack_cells, rsize - consumed_rstack_cells);

	size_t output_counter = 0;

	// add returned stack types
	while(first_output_stack < int32_t(desc.size()) && desc[first_output_stack] != -1) {
		auto result = resolve_span_type(desc.subspan(first_output_stack), type_subs, env);
		first_output_stack += result.end_match_pos;
		if(output_counter < param_perm.size() && param_perm[output_counter] != -1) {
			ts.push_back_main(result.type, params[param_perm[output_counter]], nullptr);
		} else {
			ts.push_back_main(result.type, env.new_ident(), nullptr);
		}
		++output_counter;
	}

	// output ret stack
	// add new types
	//int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto result = resolve_span_type(desc.subspan(match_position), type_subs, env);
		if(output_counter < param_perm.size() && param_perm[output_counter] != -1) {
			ts.push_back_return(result.type, params[param_perm[output_counter]], nullptr);
		} else {
			ts.push_back_return(result.type, env.new_ident(), nullptr);
		}
		match_position += result.end_match_pos;
		++output_counter;
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
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		//++output_stack_types;
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

inline std::string_view read_word(std::string_view source, uint32_t& read_position) {
	uint32_t first_non_space = read_position;
	while(first_non_space < source.length() && (source[first_non_space] == ' ' || source[first_non_space] == '\t'))
		++first_non_space;

	auto word_end = first_non_space;
	while(word_end < source.length() && source[word_end] != ' ' && source[word_end] != '\t')
		++word_end;

	read_position = word_end;

	return source.substr(first_non_space, word_end - first_non_space);
}

inline void execute_fif_word(interpreted_word_instance& wi, state_stack& ss, environment& env) {
	int32_t* ptr = wi.compiled_bytecode.data();
	while(ptr) {
		fif_call fn = nullptr;
		memcpy(&fn, ptr, 8);
		ptr = fn(ss, ptr, &env);
#ifdef HEAP_CHECKS
		assert(_CrtCheckMemory());
#endif
	}
}
inline void execute_fif_word(compiled_word_instance& wi, state_stack& ss, environment& env) {
	wi.implementation(ss, nullptr, &env);
}
inline void execute_fif_word(state_stack& ss, int32_t* ptr, environment& env) {
	while(ptr) {
		fif_call fn = nullptr;
		memcpy(&fn, ptr, 8);
		ptr = fn(ss, ptr, &env);
#ifdef HEAP_CHECKS
		assert(_CrtCheckMemory());
#endif
	}
}

inline int32_t* immediate_i32(state_stack& s, int32_t* p, environment*) {
	int32_t data = 0;
	memcpy(&data, p + 2, 4);
	s.push_back_main(fif_i32, data, nullptr);
	return p + 3;
}
inline int32_t* immediate_f32(state_stack& s, int32_t* p, environment*) {
	int64_t data = 0;
	memcpy(&data, p + 2, 4);
	s.push_back_main(fif_f32, data, nullptr);
	return p + 3;
}
inline int32_t* immediate_bool(state_stack& s, int32_t* p, environment*) {
	int32_t data = 0;
	memcpy(&data, p + 2, 4);
	s.push_back_main(fif_bool, data, nullptr);
	return p + 3;
}
inline void do_immediate_i32(state_stack& s, int32_t value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_i32;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(value);
		s.push_back_main(fif_i32, 0, nullptr);
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true);
		s.push_back_main(fif_i32, 0, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main(fif_i32, value, nullptr);
	} else if(typechecking_mode(e->mode)) {
		s.push_back_main(fif_i32, e->new_ident(), nullptr);
	}
}
inline int32_t* immediate_type(state_stack& s, int32_t* p, environment*) {
	int32_t data = 0;
	memcpy(&data, p + 2, 4);
	s.push_back_main(fif_type, data, nullptr);
	return p + 3;
}
inline void do_immediate_type(state_stack& s, int32_t value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_type;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(value);
		s.push_back_main(fif_type, 0, nullptr);
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true);
		s.push_back_main(fif_type, 0, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main(fif_type, value, nullptr);
	} else if(typechecking_mode(e->mode)) {
		s.push_back_main(fif_type, e->new_ident(), nullptr);
	}
}
inline void do_immediate_bool(state_stack& s, bool value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_bool;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t(value));
		s.push_back_main(fif_bool, 0, nullptr);
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstInt(LLVMInt1TypeInContext(e->llvm_context), uint32_t(value), true);
		s.push_back_main(fif_bool, 0, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main(fif_bool, int64_t(value), nullptr);
	} else if(typechecking_mode(e->mode)) {
		s.push_back_main(fif_bool, e->new_ident(), nullptr);
	}
}
inline void do_immediate_f32(state_stack& s, float value, environment* e) {
	if(skip_compilation(e->mode))
		return;

	int32_t v4 = 0;
	int64_t v8 = 0;
	memcpy(&v4, &value, 4);
	memcpy(&v8, &value, 4);

	auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_f32;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(v4);
		s.push_back_main(fif_f32, 0, nullptr);
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMValueRef val = LLVMConstReal(LLVMFloatTypeInContext(e->llvm_context), value);
		s.push_back_main(fif_f32, 0, val);
#endif
	} else if(e->mode == fif_mode::interpreting) {
		s.push_back_main(fif_f32, v8, nullptr);
	} else if(typechecking_mode(e->mode)) {
		s.push_back_main(fif_f32, e->new_ident(), nullptr);
	}
}
inline int32_t* function_return(state_stack& s, int32_t* p, environment*) {
	return nullptr;
}

struct parse_result {
	std::string_view content;
	bool is_string = false;
};

inline void execute_fif_word(parse_result word, environment& env, bool ignore_specializations);

#ifdef USE_LLVM
inline LLVMTypeRef llvm_function_type_from_desc(environment& env, std::span<int32_t const> desc) {
	std::vector<LLVMTypeRef> parameter_group;
	std::vector<LLVMTypeRef> returns_group;

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	//int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	//int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	//int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++ret_added;
	}

	LLVMTypeRef ret_type = nullptr;
	if(returns_group.size() == 0) {
		ret_type = LLVMVoidTypeInContext(env.llvm_context);
	} else if(returns_group.size() == 1) {
		ret_type = returns_group[0];
	} else {
		ret_type = LLVMStructTypeInContext(env.llvm_context, returns_group.data(), uint32_t(returns_group.size()), false);
	}
	return LLVMFunctionType(ret_type, parameter_group.data(), uint32_t(parameter_group.size()), false);
}

inline void llvm_make_function_parameters(environment& env, LLVMValueRef fn, state_stack& ws, std::span<int32_t const> desc, std::vector<LLVMValueRef>& initial_params) {

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto ex = LLVMGetParam(fn, uint32_t(consumed_stack_cells));
		initial_params.push_back(ex);
		ws.set_main_ex_back(consumed_stack_cells, ex);
		++match_position;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		//++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto ex = LLVMGetParam(fn, uint32_t(consumed_stack_cells + consumed_rstack_cells));
		initial_params.push_back(ex);
		ws.set_return_ex_back(consumed_rstack_cells, ex);
		++match_position;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	//int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		++match_position;
		//++ret_added;
	}

	ws.trim_to(size_t(consumed_stack_cells), size_t(consumed_rstack_cells));
}

struct brief_fn_return {
	LLVMTypeRef composite_type = nullptr;
	int32_t num_stack_values;
	int32_t num_rstack_values;
	bool is_struct_type;
};

inline brief_fn_return llvm_function_return_type_from_desc(environment& env, std::span<int32_t const> desc) {
	std::vector<LLVMTypeRef> returns_group;

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	//int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	//int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		//++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	int32_t ret_added = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++ret_added;
	}

	LLVMTypeRef ret_type = nullptr;
	if(returns_group.size() == 0) {
		// leave as nullptr
	} else if(returns_group.size() == 1) {
		ret_type = returns_group[0];
	} else {
		ret_type = LLVMStructTypeInContext(env.llvm_context, returns_group.data(), uint32_t(returns_group.size()), false);
	}
	return brief_fn_return{ ret_type , output_stack_types, ret_added, returns_group.size()  > 1};
}

inline std::vector<int32_t> llvm_make_function_return(environment& env, std::span<int32_t const> desc, std::vector<LLVMValueRef> const& initial_parameters) {
	std::vector<int32_t> permutation;
	
	auto rsummary = llvm_function_return_type_from_desc( env, desc);
	permutation.resize(rsummary.num_stack_values + rsummary.num_rstack_values, -1);

	uint32_t pinsert_index = 0;

	for(int32_t i = rsummary.num_stack_values - 1; i >= 0; --i) {
		for(int32_t j = 0; uint32_t(j) < initial_parameters.size(); ++j) {
			if(env.compiler_stack.back()->working_state()->main_size() > uint32_t(j) && initial_parameters[j] == env.compiler_stack.back()->working_state()->main_ex_back(i)) {
				permutation[pinsert_index] = j;
				break;
			}
		}
		++pinsert_index;
	}
	for(int32_t i = rsummary.num_rstack_values - 1; i >= 0; --i) {
		for(int32_t j = 0; uint32_t(j) < initial_parameters.size(); ++j) {
			if(env.compiler_stack.back()->working_state()->return_size() > uint32_t(j) && initial_parameters[j] == env.compiler_stack.back()->working_state()->return_ex_back(i)) {
				permutation[pinsert_index] = j;
				break;
			}
		}
		++pinsert_index;
	}

	if(rsummary.composite_type == nullptr) {
		LLVMBuildRetVoid(env.llvm_builder);
		return permutation;
	}
	if(rsummary.is_struct_type == false) {
		if(rsummary.num_stack_values == 0) {
			LLVMBuildRet(env.llvm_builder, env.compiler_stack.back()->working_state()->return_ex_back(0));
			return permutation;
		} else if(rsummary.num_rstack_values == 0) {
			LLVMBuildRet(env.llvm_builder, env.compiler_stack.back()->working_state()->main_ex_back(0));
			return permutation;
		} else {
			assert(false);
		}
	}

	auto rstruct = LLVMGetUndef(rsummary.composite_type);
	uint32_t insert_index = 0;

	for(int32_t i = rsummary.num_stack_values - 1; i >= 0; --i) {
		rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, env.compiler_stack.back()->working_state()->main_ex_back(i), insert_index, "");
		++insert_index;
	}
	for(int32_t i = rsummary.num_rstack_values - 1; i >= 0;  --i) {
		rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, env.compiler_stack.back()->working_state()->return_ex_back(i), insert_index, "");
		++insert_index;
	}
	LLVMBuildRet(env.llvm_builder, rstruct);
	// LLVMBuildAggregateRet(env.llvm_builder, LLVMValueRef * RetVals, unsigned N);

	return permutation;
}

inline void llvm_make_function_call(environment& env, interpreted_word_instance& wi, std::span<int32_t const> desc) {
	std::vector<LLVMValueRef> params;
	{
		int32_t match_position = 0;
		// stack matching

		//int32_t consumed_stack_cells = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			params.push_back(env.compiler_stack.back()->working_state()->main_ex_back(0));
			env.compiler_stack.back()->working_state()->pop_main();
			++match_position;
			//++consumed_stack_cells;
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
		//int32_t consumed_rstack_cells = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			params.push_back(env.compiler_stack.back()->working_state()->return_ex_back(0));
			env.compiler_stack.back()->working_state()->pop_return();
			++match_position;
			//++consumed_rstack_cells;
		}
	}

	assert(wi.llvm_function);
	auto retvalue = LLVMBuildCall2(env.llvm_builder, llvm_function_type_from_desc(env, desc), wi.llvm_function, params.data(), uint32_t(params.size()), "");
	LLVMSetInstructionCallConv(retvalue, LLVMCallConv::LLVMFastCallConv);
	auto rsummary = llvm_function_return_type_from_desc(env, desc);

	if(rsummary.composite_type == nullptr) {
		return;
	}

	{
		uint32_t extract_index = 0;
		int32_t match_position = 0;
		// stack matching

		//int32_t consumed_stack_cells = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
			++match_position;
			//++consumed_stack_cells;
		}

		++match_position; // skip -1

		// output stack
		int32_t first_output_stack = match_position;
		//int32_t output_stack_types = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			if(extract_index < wi.llvm_parameter_permutation.size() && wi.llvm_parameter_permutation[extract_index] != -1) {
				env.compiler_stack.back()->working_state()->push_back_main(desc[match_position], 0, params[wi.llvm_parameter_permutation[extract_index]]);
			} else if(rsummary.is_struct_type == false) { // single return value
				env.compiler_stack.back()->working_state()->push_back_main(desc[match_position], 0, retvalue);
			} else {
				env.compiler_stack.back()->working_state()->push_back_main(desc[match_position], 0, LLVMBuildExtractValue(env.llvm_builder, retvalue, extract_index, ""));
			}
			++extract_index;
			++match_position;
			//++output_stack_types;
		}
		++match_position; // skip -1

		// return stack matching
		//int32_t consumed_rstack_cells = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
			++match_position;
			//++consumed_rstack_cells;
		}

		++match_position; // skip -1

		// output ret stack
		//int32_t ret_added = 0;
		while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
			if(extract_index < wi.llvm_parameter_permutation.size() && wi.llvm_parameter_permutation[extract_index] != -1) {
				env.compiler_stack.back()->working_state()->push_back_main(desc[match_position], 0, params[wi.llvm_parameter_permutation[extract_index]]);
			} else if(rsummary.is_struct_type == false) { // single return value
				env.compiler_stack.back()->working_state()->push_back_return(desc[match_position], 0, retvalue);
			} else {
				env.compiler_stack.back()->working_state()->push_back_return(desc[match_position], 0, LLVMBuildExtractValue(env.llvm_builder, retvalue, extract_index, ""));
			}
			++extract_index;
			++match_position;
			//++ret_added;
		}
	}
}
#endif

inline std::vector<int32_t> expand_stack_description(state_stack& initial_stack_state, std::span<const int32_t> desc, int32_t stack_consumed, int32_t rstack_consumed);
inline bool compare_stack_description(std::span<const int32_t> a, std::span<const int32_t> b);

class locals_holder : public opaque_compiler_data {
public:
	ankerl::unordered_dense::map<std::string, int32_t> vars;
	environment& env;
	const bool llvm_mode;
	int32_t initial_bank_offset = 0;

	locals_holder(opaque_compiler_data* p, environment& env) : opaque_compiler_data(p), env(env), llvm_mode(env.mode == fif_mode::compiling_llvm) {
		initial_bank_offset = env.compiler_stack.back()->size_lvar_storage();
	}

	virtual int32_t get_var(std::string const& name)override {
		if(auto it = vars.find(name); it != vars.end()) {
			return it->second;
		}
		return parent ? parent->get_var(name) : -1;
	}
	virtual int32_t create_var(std::string const& name, int32_t type) override {
		if(auto it = vars.find(name); it != vars.end()) {
			return -1;
		}
		
		auto new_pos = env.compiler_stack.back()->size_lvar_storage();
		env.compiler_stack.back()->resize_lvar_storage(new_pos + 1);
		auto data_ptr = env.compiler_stack.back()->get_lvar_storage(new_pos);

		vars.insert_or_assign(name, new_pos);
		data_ptr->is_stack_variable = true;
		data_ptr->type = type;
#ifdef USE_LLVM
		data_ptr->expression = llvm_mode ? LLVMBuildAlloca(env.llvm_builder, env.dict.type_array[type].llvm_type, name.c_str()) : nullptr;
#endif
		return new_pos;
	}
	virtual int32_t create_let(std::string const& name, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(auto it = vars.find(name); it != vars.end()) {
			return -1;
		}

		auto new_pos = env.compiler_stack.back()->size_lvar_storage();
		env.compiler_stack.back()->resize_lvar_storage(new_pos + 1);
		auto data_ptr = env.compiler_stack.back()->get_lvar_storage(new_pos);

		vars.insert_or_assign(name, new_pos);
		data_ptr->is_stack_variable = false;
		data_ptr->type = type;
		if(!expression)
			data_ptr->data = data;
		else
			data_ptr->expression = expression;

		return new_pos;
	}

	virtual void delete_locals() const override {
		auto* ws = env.compiler_stack.back()->working_state();
		for(auto& l : vars) {
			auto data_ptr = env.compiler_stack.back()->get_lvar_storage(l.second);
			if(env.mode == fif_mode::compiling_bytecode) {
				if(env.dict.type_array[data_ptr->type].flags != 0) {
					if(data_ptr->is_stack_variable) {
						ws->push_back_main(data_ptr->type, data_ptr->data, nullptr);
						execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					} else {
						ws->push_back_main(data_ptr->type, data_ptr->data, nullptr);
						execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					}
				}
			} else if(env.mode == fif_mode::compiling_llvm) {
				if(env.dict.type_array[data_ptr->type].flags != 0) {
					if(data_ptr->is_stack_variable) {
#ifdef USE_LLVM
						auto iresult = LLVMBuildLoad2(env.llvm_builder, env.dict.type_array[data_ptr->type].llvm_type, data_ptr->expression, "");
#else
						void* iresult = nullptr;
#endif
						ws->push_back_main(data_ptr->type, 0, iresult);
						execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					} else {
						ws->push_back_main(data_ptr->type, 0, data_ptr->expression);
						execute_fif_word(fif::parse_result{ "drop", false }, env, false);
					}
				}
			}
		}
	}

	void release_locals() {
		delete_locals();
		vars.clear();
		env.compiler_stack.back()->resize_lvar_storage(initial_bank_offset);
	}
};


inline int32_t* enter_function(state_stack& s, int32_t* p, environment* env);

class lexical_scope : public locals_holder {
public:
	lexical_scope(opaque_compiler_data* p, environment& e) : locals_holder(p, e) {

	}

	virtual control_structure get_type()override {
		return control_structure::lexical_scope;
	}

	virtual bool finish(environment&)override {
		if(typechecking_mode(env.mode)) {
			return true;
		}
		if(!skip_compilation(env.mode))
			release_locals();

		return true;
	}
};

class runtime_function_scope : public locals_holder {
public:
	std::vector< internal_lvar_data> lvar_store;

	runtime_function_scope(opaque_compiler_data* p, environment& e) : locals_holder(p, e) {
	}
	virtual bool finish(environment&) override {
		release_locals();
		return true;
	}
	virtual control_structure get_type() override {
		return control_structure::rt_function;
	}
	virtual internal_lvar_data* get_lvar_storage(int32_t offset) override {
		return &(lvar_store[offset]);
	}
	virtual void resize_lvar_storage(int32_t sz) override {
		if(sz > 0 && uint32_t(sz) > lvar_store.size())
			lvar_store.resize(uint32_t(sz));
	}
	virtual std::vector< internal_lvar_data> copy_lvar_storage() override {
		return lvar_store;
	}
	virtual void set_lvar_storage(std::vector< internal_lvar_data> const& v) override {
		for(auto sz = v.size(); sz-->0;)
			lvar_store[sz] = v[sz];
	}
	virtual int32_t size_lvar_storage() override {
		return int32_t(lvar_store.size());
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_store[index].type != type)
			return false;

		if(!expression)
			lvar_store[index].data = data;
		else
			lvar_store[index].expression = expression;
		return true;
	}
};
inline int32_t* call_function(state_stack& s, int32_t* p, environment* env) {
	int32_t* jump_target = nullptr;
	memcpy(&jump_target, p + 2, 8);
	env->compiler_stack.push_back(std::make_unique<runtime_function_scope>(env->compiler_stack.back().get(), *env));
	execute_fif_word(s, jump_target, *env);
	env->compiler_stack.back()->finish(*env);
	env->compiler_stack.pop_back();
	return p + 4;
}
inline int32_t* call_function_indirect(state_stack& s, int32_t* p, environment* env) {
	auto instance = *(p + 2);
	env->compiler_stack.push_back(std::make_unique<runtime_function_scope>(env->compiler_stack.back().get(), *env));
	execute_fif_word(std::get<interpreted_word_instance>(env->dict.all_instances[instance]), s, *env);
	env->compiler_stack.back()->finish(*env);
	env->compiler_stack.pop_back();
	return p + 3;
}
inline int32_t* enter_function(state_stack& s, int32_t* p, environment* env) {
	env->compiler_stack.back()->resize_lvar_storage(*(p + 2));
	return p + 3;
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
	virtual int32_t get_var(std::string const& name) override {
		return interpreted_link ? interpreted_link->get_var(name) : -1;
	}
	virtual int32_t create_var(std::string const& name, int32_t type) override {
		return interpreted_link ? interpreted_link->create_var(name, type) : -1;
	}
	virtual int32_t create_let(std::string const& name, int32_t type, int64_t data, LLVMValueRef expression)override {
		return interpreted_link ? interpreted_link->create_let(name, type, data, expression) : -1;
	}
	virtual internal_lvar_data* get_lvar_storage(int32_t offset) {
		return interpreted_link ? interpreted_link->get_lvar_storage(offset) : nullptr;
	}
	virtual void resize_lvar_storage(int32_t sz) {
		if(interpreted_link) interpreted_link->resize_lvar_storage(sz);
	}
	virtual std::vector< internal_lvar_data> copy_lvar_storage() {
		return interpreted_link ? interpreted_link->copy_lvar_storage() : std::vector< internal_lvar_data>{ };
	}
	virtual void set_lvar_storage(std::vector< internal_lvar_data> const& v) override {
		if(interpreted_link) interpreted_link->set_lvar_storage(v);
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
class outer_interpreter : public locals_holder {
public:
	state_stack interpreter_state;
	std::vector< internal_lvar_data> lvar_store;

	outer_interpreter(environment& env) : locals_holder(nullptr, env) {
	}

	virtual internal_lvar_data* get_lvar_storage(int32_t offset) override {
		return &(lvar_store[offset]);
	}
	virtual void resize_lvar_storage(int32_t sz) override {
		if(sz > 0)
			lvar_store.resize(uint32_t(sz));
	}
	virtual std::vector< internal_lvar_data> copy_lvar_storage() override {
		return lvar_store;
	}
	virtual int32_t size_lvar_storage() override {
		return int32_t(lvar_store.size());
	}

	virtual int32_t create_var(std::string const& name, int32_t type) override {
		return -1;
	}

	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_store[index].type != type)
			return false;
		if(!expression)
			lvar_store[index].data = data;
		else
			lvar_store[index].expression = expression;
		return true;
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
	//int32_t output_stack_types = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		//++output_stack_types;
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
	int32_t ret_added = 0;
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

inline int32_t* conditional_jump(state_stack& s, int32_t* p, environment* env) {
	auto value = s.main_data_back(0);
	s.pop_main();
	if(value == 0)
		return p + *(p + 2);
	else
		return p + 3;
}
inline int32_t* reverse_conditional_jump(state_stack& s, int32_t* p, environment* env) {
	auto value = s.main_data_back(0);
	s.pop_main();
	if(value != 0)
		return p + *(p + 2);
	else
		return p + 3;
}
inline int32_t* unconditional_jump(state_stack& s, int32_t* p, environment* env) {
	return p + *(p + 2);
}

struct branch_source {
	state_stack stack;
	std::vector< internal_lvar_data> locals_state;
	LLVMBasicBlockRef from_block = nullptr;
	LLVMValueRef conditional_expression = nullptr;
	LLVMBasicBlockRef untaken_block = nullptr;
	size_t jump_bytecode_pos = 0;
	bool jump_if_true = false;
	bool unconditional_jump = false;
	bool write_conditional = false;
};

enum class add_branch_result {
	ok,
	incompatible
};

struct branch_target {
	std::vector< branch_source> branches_to_here;
	std::vector<LLVMValueRef> phi_nodes;
	std::vector<bool> altered_locals;
	std::optional<state_stack> types_on_entry;

	size_t bytecode_location = 0;
	LLVMBasicBlockRef block_location = nullptr;

	bool is_concrete = false;

	inline add_branch_result add_concrete_branch(branch_source&& new_branch, std::vector<bool> const& locals_altered, environment& env);
	inline add_branch_result add_cb_with_exit_pad(branch_source&& new_branch, locals_holder const& loc, std::vector<bool> const& locals_altered, environment& env);
	inline add_branch_result add_speculative_branch(state_stack& entry_state, std::vector<bool> const& locals_altered);
	inline void materialize(environment& env); // make blocks, phi nodes
	inline void finalize(environment& env); // write incoming links
};

inline add_branch_result branch_target::add_concrete_branch(branch_source&& new_branch, std::vector<bool> const& locals_altered, environment& env) {
	assert(!branches_to_here.empty() || !types_on_entry || (types_on_entry->main_size() == new_branch.stack.main_size() && types_on_entry->return_size() == new_branch.stack.return_size()));

	if(!types_on_entry || branches_to_here.empty()) {
		types_on_entry = new_branch.stack;
	} else {
		if(!stack_types_match(*types_on_entry, new_branch.stack)) {
			return add_branch_result::incompatible;
		}
		types_on_entry->min_main_depth = std::min(types_on_entry->min_main_depth, new_branch.stack.min_main_depth);
		types_on_entry->min_return_depth = std::min(types_on_entry->min_return_depth, new_branch.stack.min_return_depth);
	}
	if(is_concrete) {
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(i > altered_locals.size())
				return add_branch_result::incompatible;
			if(locals_altered[i] && !altered_locals[i])
				return add_branch_result::incompatible;
		}
	} else {
		if(altered_locals.size() < locals_altered.size())
			altered_locals.resize(locals_altered.size());
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(locals_altered[i])
				altered_locals[i] = true;
		}
	}

	if(env.mode == fif_mode::compiling_bytecode) {
		if(new_branch.write_conditional) {
			auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();

			fif_call imm = new_branch.unconditional_jump ? unconditional_jump : (new_branch.jump_if_true ? reverse_conditional_jump : conditional_jump);
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			new_branch.jump_bytecode_pos = bcode->size();
			bcode->push_back(0);
		}
	} else if(env.mode == fif_mode::compiling_llvm) {
		// nothing to do
	}

	branches_to_here.emplace_back(std::move(new_branch));
	return add_branch_result::ok;
}
inline add_branch_result branch_target::add_cb_with_exit_pad(branch_source&& new_branch, locals_holder const& loc, std::vector<bool> const& locals_altered, environment& env) {
	assert(!branches_to_here.empty() || !types_on_entry || (types_on_entry->main_size() == new_branch.stack.main_size() && types_on_entry->return_size() == new_branch.stack.return_size()));

	auto burrow_down_delete = [&]() { 
		auto* s_top = env.compiler_stack.back().get();
		while(s_top != &loc) {
			s_top->delete_locals();
			if(s_top->get_type() == control_structure::mode_switch) 
				s_top = static_cast<mode_switch_scope*>(s_top)->interpreted_link;
			else
				s_top = s_top->parent;
		}
		loc.delete_locals();
	};

	if(!types_on_entry || branches_to_here.empty()) {
		types_on_entry = new_branch.stack;
	} else {
		if(!stack_types_match(*types_on_entry, new_branch.stack)) {
			return add_branch_result::incompatible;
		}
		types_on_entry->min_main_depth = std::min(types_on_entry->min_main_depth, new_branch.stack.min_main_depth);
		types_on_entry->min_return_depth = std::min(types_on_entry->min_return_depth, new_branch.stack.min_return_depth);
	}
	if(is_concrete) {
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(i > altered_locals.size())
				return add_branch_result::incompatible;
			if(locals_altered[i] && !altered_locals[i])
				return add_branch_result::incompatible;
		}
	} else {

		if(altered_locals.size() < locals_altered.size())
			altered_locals.resize(locals_altered.size());
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(locals_altered[i])
				altered_locals[i] = true;
		}
	}

	if(loc.vars.size() != 0) {
		assert(new_branch.write_conditional);
		if(env.mode == fif_mode::compiling_bytecode) {
			auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();

			if(new_branch.unconditional_jump) {
				burrow_down_delete();

				fif_call imm = unconditional_jump;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				new_branch.jump_bytecode_pos = bcode->size();
				bcode->push_back(0);
			} else {
				size_t forward_jump_pos = 0;

				{
					fif_call imm = !new_branch.jump_if_true ? reverse_conditional_jump : conditional_jump;
					uint64_t imm_bytes = 0;
					memcpy(&imm_bytes, &imm, 8);
					bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
					bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
					forward_jump_pos = bcode->size();
					bcode->push_back(0);
				}
				burrow_down_delete();
				{
					fif_call imm = unconditional_jump;
					uint64_t imm_bytes = 0;
					memcpy(&imm_bytes, &imm, 8);
					bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
					bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
					new_branch.jump_bytecode_pos = bcode->size();
					bcode->push_back(0);
				}
				auto bytecode_location = bcode->size();
				(*bcode)[forward_jump_pos] = int32_t(bytecode_location - (forward_jump_pos - 2));
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
	} else {
		if(env.mode == fif_mode::compiling_bytecode) {
			if(new_branch.write_conditional) {
				auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();

				fif_call imm = new_branch.unconditional_jump ? unconditional_jump : (new_branch.jump_if_true ? reverse_conditional_jump : conditional_jump);
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				new_branch.jump_bytecode_pos = bcode->size();
				bcode->push_back(0);
			}
		} else if(env.mode == fif_mode::compiling_llvm) {
			if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
				if(new_branch.untaken_block) {
					*pb = new_branch.untaken_block;
					LLVMPositionBuilderAtEnd(env.llvm_builder, new_branch.untaken_block);
				} else {
					// nothing
				}
			}
		}
	}

	branches_to_here.emplace_back(std::move(new_branch));
	return add_branch_result::ok;
}
inline add_branch_result branch_target::add_speculative_branch(state_stack& entry_state, std::vector<bool> const& locals_altered) {
	if(!types_on_entry) {
		types_on_entry = entry_state.copy();
	} else {
		if(!stack_types_match(*types_on_entry, entry_state)) {
			return add_branch_result::incompatible;
		}
	}
	if(is_concrete) {
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(i > altered_locals.size())
				return add_branch_result::incompatible;
			if(locals_altered[i] && !altered_locals[i])
				return add_branch_result::incompatible;
		}
	} else {
		if(altered_locals.size() < locals_altered.size())
			altered_locals.resize(locals_altered.size());
		for(auto i = locals_altered.size(); i-- > 0;) {
			if(locals_altered[i])
				altered_locals[i] = true;
		}
	}
	return add_branch_result::ok;
}

inline void branch_target::materialize(environment& env) {
	if(env.mode == fif_mode::compiling_bytecode) {
		auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();
		bytecode_location = bcode->size();
	} else if(env.mode == fif_mode::compiling_llvm) {
		auto in_fn = env.compiler_stack.back()->llvm_function();
		block_location = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
		LLVMAppendExistingBasicBlock(in_fn, block_location);

		if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
			*pb = block_location;
			LLVMPositionBuilderAtEnd(env.llvm_builder, block_location);
			auto new_state = types_on_entry->copy();
			
			for(uint32_t i = 0; i < new_state.main_size(); ++i) {
				auto node = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[new_state.main_type_back(i)].llvm_type, "auto-d-phi");
				phi_nodes.push_back(node);
				new_state.set_main_ex_back(i, node);
			}
			for(uint32_t i = 0; i < new_state.return_size(); ++i) {
				auto node = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[new_state.return_type_back(i)].llvm_type, "auto-r-phi");
				phi_nodes.push_back(node);
				new_state.set_return_ex_back(i, node);
			}
			auto lsz = env.compiler_stack.back()->size_lvar_storage();
			uint32_t i = 0;
			for(; i < altered_locals.size(); ++i) {
				if(altered_locals[i] && i < uint32_t(lsz)) {
					auto node = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[env.compiler_stack.back()->get_lvar_storage(int32_t(i))->type].llvm_type, "auto-l-phi");
					phi_nodes.push_back(node);
					env.compiler_stack.back()->get_lvar_storage(int32_t(i))->expression = node;
				}
			}
			env.compiler_stack.back()->set_working_state(std::move(new_state));
		}
	}
	is_concrete = true;
}
inline void branch_target::finalize(environment& env) {
	if(env.mode == fif_mode::compiling_bytecode) {
		//fill in jumps
		auto bcode = env.compiler_stack.back()->bytecode_compilation_progress();
		assert(bcode);
		for(auto& br : branches_to_here) {
			if(br.write_conditional) {
				(*bcode)[br.jump_bytecode_pos] = int32_t(bytecode_location - (br.jump_bytecode_pos - 2));
			}
		}
	} else if(env.mode == fif_mode::compiling_llvm) {
		if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
			auto current_location = *pb;
			assert(block_location);

			// write to other blocks
			// write conditional jumps
			for(auto& br : branches_to_here) {
				if(br.write_conditional) {
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
				inc_blocks.push_back(br.from_block);
			}
			for(uint32_t i = 0; i < types_on_entry->main_size(); ++i) {
				inc_vals.clear();
				for(auto& br : branches_to_here) {
					inc_vals.push_back(br.stack.main_ex_back(i));
				}
				LLVMAddIncoming(phi_nodes[i], inc_vals.data(), inc_blocks.data(), uint32_t(branches_to_here.size()));
			}
			for(uint32_t i = 0; i < types_on_entry->return_size(); ++i) {
				inc_vals.clear();
				for(auto& br : branches_to_here) {
					inc_vals.push_back(br.stack.return_ex_back(i));
				}
				LLVMAddIncoming(phi_nodes[types_on_entry->main_size() + i], inc_vals.data(), inc_blocks.data(), uint32_t(branches_to_here.size()));
			}
			uint32_t j = 0;
			for(uint32_t i = 0; i < altered_locals.size(); ++i) {
				if(altered_locals[i]) {
					inc_vals.clear();
					for(auto& br : branches_to_here) {
						assert(i < br.locals_state.size());
						inc_vals.push_back(br.locals_state[i].expression);
					}
					LLVMAddIncoming(phi_nodes[types_on_entry->main_size() + types_on_entry->return_size() + j], inc_vals.data(), inc_blocks.data(), uint32_t(branches_to_here.size()));
					++j;
				}
			}

			LLVMPositionBuilderAtEnd(env.llvm_builder, current_location);
		}
	}
}

class function_scope : public locals_holder {
public:
	branch_target fn_exit;

	state_stack initial_state;
	state_stack iworking_state;
	std::vector<int32_t> compiled_bytes;
	std::vector<int32_t> type_subs;
	std::vector< LLVMValueRef> initial_parameters;
	std::vector< internal_lvar_data> lvar_store;
	std::vector<bool> lvar_relet;
	LLVMValueRef llvm_fn = nullptr;
	LLVMBasicBlockRef current_block = nullptr;
	size_t locals_size_position = 0;
	int32_t for_word = -1;
	int32_t for_instance = -1;
	int32_t max_locals_size = 0;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);
	bool intepreter_skip_body = false;
	uint8_t exit_point_count = 0;

	function_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state, int32_t for_word, int32_t for_instance) : locals_holder(p, e), for_word(for_word), for_instance(for_instance) {

		initial_state = entry_state.new_copy();
		iworking_state = entry_state.new_copy();
		entry_mode = env.mode;

		if(for_word == -1) {
			env.report_error("attempted to compile a function for an anonymous word");
			env.mode = fif_mode::error;
			return;
		}


		if(typechecking_mode(env.mode))
			env.dict.word_array[for_word].being_typechecked = true;
		else if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			assert(for_instance != -1);
			std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).being_compiled = true;
		}
		if(env.mode == fif_mode::compiling_bytecode) {
			fif_call imm2 = enter_function;
			uint64_t imm2_bytes = 0;
			memcpy(&imm2_bytes, &imm2, 8);
			compiled_bytes.push_back(int32_t(imm2_bytes & 0xFFFFFFFF));
			compiled_bytes.push_back(int32_t((imm2_bytes >> 32) & 0xFFFFFFFF));
			locals_size_position = compiled_bytes.size();
			compiled_bytes.push_back(0);
		}
		if(env.mode == fif_mode::compiling_llvm) {
			assert(for_instance != -1);
			auto fn_desc = std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_start, std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_count);

#ifdef USE_LLVM
			if(!std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function) {
				auto fn_string_name = word_name_from_id(for_word, env) + "#" + std::to_string(for_instance);
				auto fn_type = llvm_function_type_from_desc(env, fn_desc);
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function = LLVMAddFunction(env.llvm_module, fn_string_name.c_str(), fn_type);
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
			auto entry_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
			LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);
			llvm_make_function_parameters(env, compiled_fn, iworking_state, fn_desc, initial_parameters);
			initial_state = iworking_state;
			current_block = entry_block;
#endif
		}

	}
	virtual internal_lvar_data* get_lvar_storage(int32_t offset) override {
		return &(lvar_store[offset]);
	}
	virtual void resize_lvar_storage(int32_t sz) override {
		max_locals_size = std::max(max_locals_size, sz);
		if(sz > 0)
			lvar_store.resize(uint32_t(sz));
	}
	virtual std::vector< internal_lvar_data> copy_lvar_storage() override {
		return lvar_store;
	}
	virtual void set_lvar_storage(std::vector< internal_lvar_data> const& v) override {
		lvar_store = v;
	}
	virtual int32_t size_lvar_storage() override {
		return int32_t(lvar_store.size());
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_store[index].type != type)
			return false;

		if(lvar_relet.size() <= uint32_t(index))
			lvar_relet.resize(uint32_t(index) + 1);
		lvar_relet[index] = true;
		
		if(!expression)
			lvar_store[index].data = data;
		else
			lvar_store[index].expression = expression;
		return true;
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
	virtual std::vector<int32_t>* bytecode_compilation_progress()override {
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

		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			++exit_point_count;
			auto pb = env.compiler_stack.back()->llvm_block();
			fn_exit.add_cb_with_exit_pad(branch_source{ env.compiler_stack.back()->working_state()->copy(), copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, *this, lvar_relet, env);
			env.mode = surpress_branch(env.mode);
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			auto wstemp = env.compiler_stack.back()->working_state();
			if(fn_exit.add_speculative_branch(*wstemp, lvar_relet) != add_branch_result::ok) {
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


		if(entry_mode == fif_mode::interpreting && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;

			release_locals();
			return true;
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(fn_exit.add_speculative_branch(iworking_state, lvar_relet) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
		}

		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			++exit_point_count;
			if(exit_point_count > 1) {
				auto pb = env.compiler_stack.back()->llvm_block();
				fn_exit.add_cb_with_exit_pad(branch_source{ iworking_state, copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, *this, lvar_relet, env);
			}
		}

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

			wi.typechecking_level = std::max(wi.typechecking_level, provisional_success(env.mode) ? 2 : 3);
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

			if(exit_point_count > 1 || (exit_point_count == 1 && env.mode != fif_mode::compiling_bytecode)) {
				fn_exit.materialize(env);
				fn_exit.finalize(env);
			}

			compiled_bytes[locals_size_position] = max_locals_size;

			fif_call imm = function_return;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compiled_bytes.push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compiled_bytes.push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			
			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			wi.compiled_bytecode = std::move(compiled_bytes);

			if(for_instance != -1)
				std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).being_compiled = false;
		} else if(base_mode(entry_mode) == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			if(for_instance == -1) {
				env.report_error("tried to compile a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}

			if(exit_point_count > 1 || (exit_point_count == 1 && env.mode != fif_mode::compiling_llvm)) {
				fn_exit.materialize(env);
				fn_exit.finalize(env);
			}

			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			std::span<const int32_t> existing_description = std::span<const int32_t>(env.dict.all_stack_types.data() + wi.stack_types_start, size_t(wi.stack_types_count));

			if(exit_point_count == 1) {
				wi.llvm_parameter_permutation = llvm_make_function_return(env, existing_description, initial_parameters);
			} else {
				llvm_make_function_return(env, existing_description, initial_parameters);
				wi.llvm_parameter_permutation.clear();
			}
			
			wi.llvm_compilation_finished = true;

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
			return true;
		} else {
			env.report_error("reached end of compilation in unexpected mode");
			env.mode = fif_mode::error;
			return true;
		}
		return true;
	}
};
class conditional_scope : public locals_holder {
public:
	state_stack initial_state;
	std::vector< internal_lvar_data> entry_locals_state;

	std::vector<bool> lvar_relet;

	branch_target scope_end;
	branch_target else_target;

	int32_t ds_depth = 0;
	int32_t rs_depth = 0;

	fif_mode entry_mode;
	fif_mode first_branch_end_mode = fif_mode(0);
	bool interpreter_first_branch = false;
	bool interpreter_second_branch = false;
	bool typechecking_provisional_on_first_branch = false;

	conditional_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : locals_holder(p, e) {	
		if(entry_state.main_size() == 0 || entry_state.main_type_back(0) != fif_bool) {
			env.report_error("attempted to start an if without a boolean value on the stack");
			env.mode = fif_mode::error;
		}
		entry_mode = env.mode;
		entry_locals_state = env.compiler_stack.back()->copy_lvar_storage();
		if(env.mode == fif_mode::interpreting) {
			if(entry_state.main_data_back(0) != 0) {
				interpreter_first_branch = true;
				interpreter_second_branch = false;
			} else {
				interpreter_first_branch = false;
				interpreter_second_branch = true;
				env.mode = surpress_branch(env.mode);
			}
			entry_state.pop_main();
		} else if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			auto branch_condition = entry_state.main_ex_back(0);
			entry_state.pop_main();

			if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
				auto parent_block = *pb;
				
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-first-branch");
				LLVMAppendExistingBasicBlock(env.compiler_stack.back()->llvm_function(), *pb);
				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, parent_block, branch_condition, *pb, 0, false, false, true}, lvar_relet, env);
			}
#endif
		} else if(env.mode == fif_mode::compiling_bytecode) {
			entry_state.pop_main();
			else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true }, lvar_relet, env);
		} else if(!skip_compilation(env.mode)) {
			entry_state.pop_main();
			else_target.add_concrete_branch(branch_source{ entry_state.copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true }, lvar_relet, env);
		}

		ds_depth = int32_t(entry_state.min_main_depth);
		rs_depth = int32_t(entry_state.min_return_depth);

		initial_state = entry_state;
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_relet.size() <= uint32_t(index))
			lvar_relet.resize(uint32_t(index) + 1);
		lvar_relet[index] = true;
		return parent ? parent->re_let(index, type, data, expression) : true;
	}
	void and_if() {
		auto wstate = env.compiler_stack.back()->working_state();

		if(env.mode == fif_mode::interpreting) {
			if(wstate->main_data_back(0) != 0) {
				interpreter_first_branch = true;
				interpreter_second_branch = false;
			} else {
				interpreter_first_branch = false;
				interpreter_second_branch = true;
				release_locals();
				env.mode = surpress_branch(env.mode);
			}
			wstate->pop_main();
			return;
		}

		if(else_target.is_concrete) {
			else_target = branch_target{ };
			entry_mode = env.mode;
			entry_locals_state = env.compiler_stack.back()->copy_lvar_storage();

			if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
				auto branch_condition = wstate->main_ex_back(0);
				wstate->pop_main();

				if(auto pb = env.compiler_stack.back()->llvm_block(); pb) {
					auto parent_block = *pb;

					*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-first-branch");
					LLVMAppendExistingBasicBlock(env.compiler_stack.back()->llvm_function(), *pb);
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

					else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, parent_block, branch_condition, *pb, 0, false, false, true }, lvar_relet, env);
				}
#endif
			} else if(env.mode == fif_mode::compiling_bytecode) {
				wstate->pop_main();
				else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true }, lvar_relet, env);
			} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					env.mode = fail_typechecking(env.mode);
					return;
				}
				wstate->pop_main();
				else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true }, lvar_relet, env);
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

				auto branch_condition = wstate->main_ex_back(0);
				wstate->pop_main();

				LLVMBasicBlockRef continuation = nullptr;
#ifdef USE_LLVM
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation = LLVMCreateBasicBlockInContext(env.llvm_context, "continuation");
					LLVMAppendExistingBasicBlock(in_fn, continuation);
				}
#endif
				else_target.add_cb_with_exit_pad(branch_source{ wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, branch_condition, continuation, 0, false, false, true }, *this, lvar_relet, env);

			} else if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					env.mode = fail_typechecking(env.mode);
					return;
				}
				wstate->pop_main();
				if(else_target.add_concrete_branch(branch_source{ wstate->copy(), { }, nullptr, nullptr, nullptr, 0, false, false, true }, lvar_relet, env) != add_branch_result::ok) {
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
			release_locals();
			env.mode = surpress_branch(env.mode);
			return;
		}
		
		if(!typechecking_mode(env.mode) && !skip_compilation(env.mode)) {
			release_locals();
		}

		auto pb = env.compiler_stack.back()->llvm_block();

		if(!skip_compilation(env.mode)) {
			auto wstate = env.compiler_stack.back()->working_state();
			ds_depth = std::min(ds_depth, wstate->min_main_depth);
			rs_depth = std::min(rs_depth, wstate->min_return_depth);
			scope_end.add_concrete_branch(branch_source{ *wstate, parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, lvar_relet, env);
		}

		env.compiler_stack.back()->set_working_state(initial_state.copy());
		lvar_relet.clear();
		env.compiler_stack.back()->set_lvar_storage(entry_locals_state);

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
		if(!typechecking_mode(env.mode) && !skip_compilation(env.mode)) {
			release_locals();
		}

		if(interpreter_second_branch || interpreter_first_branch) {
			env.mode = fif_mode::interpreting;
		}

		if(env.mode == fif_mode::interpreting) {
			return true;
		}

		auto pb = env.compiler_stack.back()->llvm_block();

		if(!else_target.is_concrete) {
			if(!skip_compilation(env.mode)) {
				scope_end.add_concrete_branch(branch_source{ env.compiler_stack.back()->working_state()->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, lvar_relet, env);
			}

			if(first_branch_end_mode == fif_mode(0))
				first_branch_end_mode = env.mode;
			else
				first_branch_end_mode = merge_modes(env.mode, first_branch_end_mode);

			env.mode = entry_mode;

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			lvar_relet.clear();
			env.compiler_stack.back()->set_lvar_storage(entry_locals_state);

			else_target.materialize(env);
			else_target.finalize(env);
		}
		

		if(!skip_compilation(env.mode)) {
			auto branch_valid = scope_end.add_concrete_branch(branch_source{ env.compiler_stack.back()->working_state()->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, lvar_relet, env);
			if(branch_valid != add_branch_result::ok) {
				env.mode = fail_mode(env.mode);
			}
		}

		env.mode = merge_modes(env.mode, first_branch_end_mode);
		
		scope_end.materialize(env);
		scope_end.finalize(env);

		auto wstate = env.compiler_stack.back()->working_state();
		wstate->min_main_depth = std::min(wstate->min_main_depth, ds_depth);
		wstate->min_return_depth = std::min(wstate->min_return_depth, rs_depth);

		return true;
	}
};

class while_loop_scope : public locals_holder {
public:
	state_stack initial_state;
	std::vector< internal_lvar_data> entry_locals_state;

	std::vector<bool> lvar_relet;

	branch_target loop_start;
	branch_target loop_exit;
	
	std::string_view entry_source;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);

	bool phi_pass = false;
	bool intepreter_skip_body = false;
	bool saw_conditional = false;


	while_loop_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : locals_holder(p, e) {
		initial_state = entry_state;
		entry_mode = env.mode;
		entry_locals_state = env.compiler_stack.back()->copy_lvar_storage();

		if(!env.source_stack.empty()) {
			entry_source = env.source_stack.back();
		}

		if(typechecking_mode(env.mode)) {
			if(failed(env.mode))
				return;
			loop_start.add_speculative_branch(initial_state, lvar_relet);
			return;
		} else if(env.mode == fif_mode::interpreting) {
			return;
		}
		phi_pass = true;
		env.mode = fif_mode::tc_level_2;
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_relet.size() <= uint32_t(index))
			lvar_relet.resize(uint32_t(index) + 1);
		lvar_relet[index] = true;
		return parent ? parent->re_let(index, type, data, expression) : true;
	}
	virtual control_structure get_type()override {
		return control_structure::str_while_loop;
	}
	void add_break() {
		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		auto wstemp = env.compiler_stack.back()->working_state();
		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, *this, lvar_relet, env);
			env.mode = surpress_branch(env.mode);
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(loop_exit.add_speculative_branch(*wstemp, lvar_relet) != add_branch_result::ok) {
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
			if(wstate->main_size() == 0) {
				env.report_error("while loop conditional terminated with an inappropriate value");
				env.mode = fif_mode::error;
				return;
			}
			if(wstate->main_type_back(0) != fif_bool) {
				env.report_error("while loop conditional terminated with an inappropriate value");
				env.mode = fif_mode::error;
				return;
			}
			
			auto branch_condition = wstate->main_ex_back(0);
			wstate->pop_main();

			LLVMBasicBlockRef continuation = nullptr;

			if(env.mode == fif_mode::compiling_llvm) {
				auto in_fn = env.compiler_stack.back()->llvm_function();
				continuation = LLVMCreateBasicBlockInContext(env.llvm_context, "continuation");
				LLVMAppendExistingBasicBlock(in_fn, continuation);
			}

			loop_exit.add_cb_with_exit_pad(branch_source{ wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, branch_condition, continuation, 0, false, false, true }, *this, lvar_relet, env);
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			auto wstate = env.compiler_stack.back()->working_state();
			if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
				env.mode = fail_typechecking(env.mode);
				return;
			}
			wstate->pop_main();
			if(loop_exit.add_speculative_branch(*wstate, lvar_relet) != add_branch_result::ok) {
				env.mode = fail_typechecking(env.mode);
			}
			return;
		} else if(env.mode == fif_mode::interpreting) {
			if(env.compiler_stack.back()->working_state()->main_data_back(0) == 0) {
				intepreter_skip_body = true;
				env.mode = surpress_branch(env.mode);
			}
			env.compiler_stack.back()->working_state()->pop_main();
			return;
		}
	}
	virtual bool finish(environment&) override {
		if(entry_mode == fif_mode::interpreting && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;
			
			release_locals();
			return true;
		}

		auto wstate = env.compiler_stack.back()->working_state();

		if(env.mode == fif_mode::interpreting) {
			if(!saw_conditional && wstate->main_type_back(0) == fif_bool) {
				if(wstate->main_data_back(0) == 0) {
					wstate->pop_main();
					release_locals();
					return true;
				}
				wstate->pop_main();
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

		if(base_mode(env.mode) == fif_mode::compiling_bytecode || base_mode(env.mode) == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();

			if(!saw_conditional && !skip_compilation(env.mode)) {
				saw_conditional = true;
				if(wstate->main_size() == 0) {
					env.report_error("while loop terminated with an inappropriate conditional");
					env.mode = fif_mode::error;
					return true;
				}
				if(wstate->main_type_back(0) != fif_bool) {
					env.report_error("while loop terminated with an inappropriate conditional");
					env.mode = fif_mode::error;
					return true;
				}
				auto branch_condition = wstate->main_ex_back(0);
				wstate->pop_main();

				LLVMBasicBlockRef continuation_block = nullptr;
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation_block = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
					LLVMAppendExistingBasicBlock(in_fn, continuation_block);
				}

				loop_exit.add_cb_with_exit_pad(branch_source{ wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, branch_condition, continuation_block, 0, false, false, true }, *this,  lvar_relet, env);
			}


			if(!skip_compilation(env.mode)) {
				release_locals();
				
				auto bmatch = loop_start.add_concrete_branch(branch_source{
					wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr,
					0, false, true, true },
					lvar_relet, env);

				if(bmatch != add_branch_result::ok) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in while loop");
					return true;
				}
			}

			loop_start.finalize(env);

			loop_exit.materialize(env);
			loop_exit.finalize(env);

			if(condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);

			return true;
		} else if(typechecking_level(env.mode) == 2 && phi_pass == true) {
			phi_pass = false;
			if(!saw_conditional) {
				wstate->pop_main();
			}

			env.mode = entry_mode;
			condition_mode = fif_mode(0);

			loop_start.add_speculative_branch(*wstate, lvar_relet);

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			env.compiler_stack.back()->set_lvar_storage(entry_locals_state);

			auto pb = env.compiler_stack.back()->llvm_block();
			auto bmatch = loop_start.add_concrete_branch(branch_source{
				initial_state, parent->copy_lvar_storage(), pb ? *pb : nullptr,  nullptr,  nullptr,
					0, false, true, true }, 
				lvar_relet, env);
			
			if(bmatch != add_branch_result::ok) {
				env.mode = fif_mode::error;
				env.report_error("branch types incompatible in while loop");
				return true;
			}

			loop_start.materialize(env);

			saw_conditional = false;
			
			if(!env.source_stack.empty())	
				env.source_stack.back() = entry_source;

			return false;
		} else if(typechecking_mode(env.mode)) {
			if(!saw_conditional) {
				if(condition_mode == fif_mode(0))
					condition_mode = env.mode;
				else
					condition_mode = merge_modes(env.mode, condition_mode);

				if(!skip_compilation(env.mode)) {
					if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
						env.mode = fail_typechecking(env.mode);
						return true;
					}
					wstate->pop_main();
					loop_exit.add_speculative_branch(*wstate, lvar_relet);
				}
			}
			
			if(failed(env.mode) || (!recursive(env.mode) && !skip_compilation(env.mode) && loop_start.add_speculative_branch(*wstate, lvar_relet) != add_branch_result::ok)) {
				env.mode = fail_typechecking(env.mode);
				return true;
			}

			if(recursive(env.mode) && condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);

			if(loop_exit.types_on_entry) {
				loop_exit.types_on_entry->min_main_depth = std::min(loop_exit.types_on_entry->min_main_depth, wstate->min_main_depth);
				loop_exit.types_on_entry->min_return_depth = std::min(loop_exit.types_on_entry->min_return_depth, wstate->min_return_depth);
				parent->set_working_state(loop_exit.types_on_entry->copy());
			}
			return true;

		}
		return true;
	}
};

class do_loop_scope : public locals_holder {
public:
	state_stack initial_state;
	std::vector< internal_lvar_data> entry_locals_state;

	std::vector<bool> lvar_relet;

	branch_target loop_start;
	branch_target loop_exit;

	std::string_view entry_source;
	fif_mode entry_mode;
	fif_mode condition_mode = fif_mode(0);

	bool phi_pass = false;
	bool intepreter_skip_body = false;

	do_loop_scope(opaque_compiler_data* p, environment& e, state_stack& entry_state) : locals_holder(p, e) {
		initial_state = entry_state;
		entry_mode = env.mode;
		entry_locals_state = env.compiler_stack.back()->copy_lvar_storage();

		if(!env.source_stack.empty()) {
			entry_source = env.source_stack.back();
		}

		if(typechecking_mode(env.mode)) {
			if(failed(env.mode))
				return;
			loop_start.add_speculative_branch(initial_state, lvar_relet);
			return;
		} else if(env.mode == fif_mode::interpreting) {
			return;
		}
		phi_pass = true;
		env.mode = fif_mode::tc_level_2;
	}
	virtual bool re_let(int32_t index, int32_t type, int64_t data, LLVMValueRef expression) override {
		if(lvar_relet.size() <= uint32_t(index))
			lvar_relet.resize(uint32_t(index) + 1);
		lvar_relet[index] = true;
		return parent ? parent->re_let(index, type, data, expression) : true;
	}
	virtual control_structure get_type()override {
		return control_structure::str_do_loop;
	}
	void add_break() {
		if(condition_mode == fif_mode(0))
			condition_mode = env.mode;
		else
			condition_mode = merge_modes(env.mode, condition_mode);

		auto wstemp = env.compiler_stack.back()->working_state();

		if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();
			loop_exit.add_cb_with_exit_pad(branch_source{ wstemp->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr, 0, false, true, true }, *this, lvar_relet, env);
			env.mode = surpress_branch(env.mode);
		}

		if(!skip_compilation(env.mode) && typechecking_mode(env.mode)) {
			if(loop_exit.add_speculative_branch(*wstemp, lvar_relet) != add_branch_result::ok) {
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
	void until_statement(environment&) {
		// does nothing
	}
	virtual bool finish(environment&) override {
		if(entry_mode == fif_mode::interpreting && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;

			release_locals();
			return true;
		}

		auto wstate = env.compiler_stack.back()->working_state();
		if(env.mode == fif_mode::interpreting) {
			if(wstate->main_size() > 0 && wstate->main_type_back(0) == fif_bool) {
				if(wstate->main_data_back(0) != 0) {
					wstate->pop_main();
					release_locals();
					return true;
				}
				wstate->pop_main();
			} else {
				env.report_error("do loop terminated with an inappropriate conditional");
				env.mode = fif_mode::error;
				return true;
			}
			if(!env.source_stack.empty())
				env.source_stack.back() = entry_source;
			return false;
		}

		auto bcode = parent->bytecode_compilation_progress();

		if(base_mode(env.mode) == fif_mode::compiling_bytecode || base_mode(env.mode) == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back()->llvm_block();

			if(!skip_compilation(env.mode)) {
				if(wstate->main_size() == 0) {
					env.report_error("do loop terminated with an inappropriate conditional");
					env.mode = fif_mode::error;
					return true;
				}
				if(wstate->main_type_back(0) != fif_bool) {
					env.report_error("do loop terminated with an inappropriate conditional");
					env.mode = fif_mode::error;
					return true;
				}
				auto branch_condition = wstate->main_ex_back(0);
				wstate->pop_main();

				LLVMBasicBlockRef continuation_block = nullptr;
				if(env.mode == fif_mode::compiling_llvm) {
					auto in_fn = env.compiler_stack.back()->llvm_function();
					continuation_block = LLVMCreateBasicBlockInContext(env.llvm_context, "branch-target");
					LLVMAppendExistingBasicBlock(in_fn, continuation_block);
				}

				loop_exit.add_cb_with_exit_pad(branch_source{ wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, branch_condition, continuation_block, 0, true, false, true }, *this, lvar_relet, env);


				release_locals();

				auto bmatch = loop_start.add_concrete_branch(branch_source{
					wstate->copy(), parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr,
					0, false, true, true },
					lvar_relet, env);

				if(bmatch != add_branch_result::ok) {
					env.mode = fif_mode::error;
					env.report_error("branch types incompatible in while loop");
					return true;
				}
			}

			loop_start.finalize(env);

			loop_exit.materialize(env);
			loop_exit.finalize(env);

			if(condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);


			return true;
		} else if(typechecking_level(env.mode) == 2 && phi_pass == true) {
			phi_pass = false;
			wstate->pop_main();

			env.mode = entry_mode;
			condition_mode = fif_mode(0);

			loop_start.add_speculative_branch(*wstate, lvar_relet);

			env.compiler_stack.back()->set_working_state(initial_state.copy());
			env.compiler_stack.back()->set_lvar_storage(entry_locals_state);

			auto pb = env.compiler_stack.back()->llvm_block();
			auto bmatch = loop_start.add_concrete_branch(branch_source{
				initial_state, parent->copy_lvar_storage(), pb ? *pb : nullptr, nullptr, nullptr,
				0, false, true, true },
				lvar_relet, env);

			if(bmatch != add_branch_result::ok) {
				env.mode = fif_mode::error;
				env.report_error("branch types incompatible in while loop");
				return true;
			}

			loop_start.materialize(env);

			if(!env.source_stack.empty())
				env.source_stack.back() = entry_source;

			return false;
		} else if(typechecking_mode(env.mode)) {
			if(condition_mode == fif_mode(0))
				condition_mode = env.mode;
			else
				condition_mode = merge_modes(env.mode, condition_mode);

			if(!skip_compilation(env.mode)) {
				if(wstate->main_size() == 0 || wstate->main_type_back(0) != fif_bool) {
					env.mode = fail_typechecking(env.mode);
					return true;
				}
				wstate->pop_main();
				loop_exit.add_speculative_branch(*wstate, lvar_relet);
			}
			
			if(failed(env.mode) || (!recursive(env.mode) && !skip_compilation(env.mode) && loop_start.add_speculative_branch(*wstate, lvar_relet) != add_branch_result::ok)) {
				env.mode = fail_typechecking(env.mode);
				return true;
			}

			if(recursive(env.mode) && condition_mode != fif_mode(0))
				env.mode = merge_modes(env.mode, condition_mode);

			if(loop_exit.types_on_entry) {
				loop_exit.types_on_entry->min_main_depth = std::min(loop_exit.types_on_entry->min_main_depth, wstate->min_main_depth);
				loop_exit.types_on_entry->min_return_depth = std::min(loop_exit.types_on_entry->min_return_depth, wstate->min_return_depth);
				parent->set_working_state(loop_exit.types_on_entry->copy());
			}
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
						env.dict.word_array[w].being_typechecked = true;

						switch_compiler_stack_mode(env, fif_mode::tc_level_1);

						env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
						auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, current_type_state, w, -1);
						fnscope->type_subs = specialize_t_subs;
						env.compiler_stack.emplace_back(std::move(fnscope));

						run_to_function_end(env);
						env.source_stack.pop_back();

						env.dict.word_array[w].being_typechecked = false;
						match = match_word(env.dict.word_array[w], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);
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

					env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
					auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, current_type_state, w, -1);
					fnscope->type_subs = specialize_t_subs;
					env.compiler_stack.emplace_back(std::move(fnscope));

					run_to_function_end(env);
					env.source_stack.pop_back();

					env.dict.word_array[w].being_typechecked = false;

					restore_compiler_stack_mode(env);

					match = match_word(env.dict.word_array[w], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);
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
	if(wi.typechecking_level == 1) {
		// perform level 2 typechecking

		switch_compiler_stack_mode(env, fif_mode::tc_level_2);

		env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, current_type_state, w, word_index);
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
		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, current_type_state, w, word_index);
		fnscope->type_subs = tsubs;
		env.compiler_stack.emplace_back(std::move(fnscope));

		run_to_function_end(env);
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
			r.stack_height_added_at = int32_t(current_type_state.main_size());
			r.rstack_height_added_at = int32_t(current_type_state.return_size());

			auto c = get_stack_consumption(w, word_index, env);
			r.stack_consumed = std::max(c.stack, erb->second.stack_consumed);
			r.rstack_consumed = std::max(c.rstack, erb->second.rstack_consumed);

			record_holder->tr.insert_or_assign((uint64_t(w) << 32) | uint64_t(word_index), r);
		} else {
			typecheck_3_record r;
			r.stack_height_added_at = int32_t(current_type_state.main_size());
			r.rstack_height_added_at = int32_t(current_type_state.return_size());

			auto c = get_stack_consumption(w, word_index, env);
			r.stack_consumed = c.stack;
			r.rstack_consumed = c.rstack;

			record_holder->tr.insert_or_assign((uint64_t(w) << 32) | uint64_t(word_index), r);
		}

		/*
		process results of typechecking
		*/
		auto min_stack_depth = int32_t(current_type_state.main_size());
		auto min_rstack_depth = int32_t(current_type_state.return_size());
		for(auto& s : record_holder->tr) {
			min_stack_depth = std::min(min_stack_depth, s.second.stack_height_added_at - s.second.stack_consumed);
			min_rstack_depth = std::min(min_rstack_depth, s.second.rstack_height_added_at - s.second.rstack_consumed);
		}

		auto existing_description = std::span<int32_t const>(env.dict.all_stack_types.data() + wi.stack_types_start, wi.stack_types_count);
		auto revised_description = expand_stack_description(current_type_state, existing_description, int32_t(current_type_state.main_size()) - min_stack_depth, int32_t(current_type_state.return_size()) - min_rstack_depth);

		if(!compare_stack_description(existing_description, std::span<int32_t const>(revised_description.data(), revised_description.size()))) {
			wi.stack_types_start = int32_t(env.dict.all_stack_types.size());
			wi.stack_types_count = int32_t(revised_description.size());
			env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), revised_description.begin(), revised_description.end());
		}

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

	if(env.mode != fif_mode::compiling_llvm && wi.compiled_bytecode.size() == 0) {
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

inline internal_lvar_data* get_global_var(environment& env, std::string const& name) {
	for(auto& p : env.compiler_stack) {
		if(p->get_type() == control_structure::globals)
			return static_cast<compiler_globals_layer*>(p.get())->get_global_var(name);
	}
	return nullptr;
}

inline int32_t* immediate_local(state_stack& s, int32_t* p, environment* e) {
	int32_t index = *(p + 2);
	int32_t type = *(p + 3);
	auto* l = e->compiler_stack.back()->get_lvar_storage(index);
	if(!l) {
		e->report_error("unable to find local var");
		return nullptr;
	}
	s.push_back_main(type, (int64_t)l, nullptr);
	return p + 4;
}

inline int32_t* immediate_let(state_stack& s, int32_t* p, environment* e) {
	auto index = *(p + 2);
	auto ds = e->compiler_stack.back()->get_lvar_storage(index);
	if(!ds) {
		e->report_error("unable to find local let");
		return nullptr;
	}
	s.push_back_main(ds->type, ds->data, nullptr);

	execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
	
	auto type_b = s.main_type_back(0);
	auto dat_b = s.main_data_back(0);
	ds->data = s.main_data_back(1);

	s.pop_main();
	s.pop_main();
	s.push_back_main(type_b, dat_b, nullptr);

	return p + 3;
}

inline int32_t* immediate_global(state_stack& s, int32_t* p, environment* e) {
	char* name = 0;
	memcpy(&name, p + 2, 8);
	auto* v = get_global_var(*e, std::string(name));
	if(!v) {
		e->report_error("unable to find global");
		return nullptr;
	}
	int32_t type = 0;
	memcpy(&type, p + 4, 4);
	s.push_back_main(type, (int64_t)v, nullptr);
	return p + 5;
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

								env.source_stack.push_back(std::string_view(env.dict.word_array[w].source));
								auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().get(), env, *ws, w, match.word_index);
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
					env.compiler_stack.push_back(std::make_unique<runtime_function_scope>(env.compiler_stack.back().get(), env));
					execute_fif_word(std::get<interpreted_word_instance>(*wi), *ws, env);
					env.compiler_stack.back()->finish(env);
					env.compiler_stack.pop_back();
				} else if(env.mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
					std::span<int32_t const> desc{ env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count) };
					llvm_make_function_call(env, std::get<interpreted_word_instance>(*wi), desc);
#endif
				} else if(env.mode == fif_mode::compiling_bytecode) {
					auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
					if(cbytes) {
						if(std::get<interpreted_word_instance>(*wi).being_compiled) {
							fif_call imm = call_function_indirect;
							uint64_t imm_bytes = 0;
							memcpy(&imm_bytes, &imm, 8);
							cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
							cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
							cbytes->push_back(match.word_index);
						} else {
							{
								fif_call imm = call_function;
								uint64_t imm_bytes = 0;
								memcpy(&imm_bytes, &imm, 8);
								cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
								cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
							}
							{
								int32_t* bcode = std::get<interpreted_word_instance>(*wi).compiled_bytecode.data();
								uint64_t imm_bytes = 0;
								memcpy(&imm_bytes, &bcode, 8);
								cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
								cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
							}
						}
						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(*wi).stack_types_start, size_t(std::get<interpreted_word_instance>(*wi).stack_types_count)), *ws, std::get<interpreted_word_instance>(*wi).llvm_parameter_permutation, env);
					}
				}
			} else if(std::holds_alternative<compiled_word_instance>(*wi)) {
				if(env.mode == fif_mode::compiling_bytecode) {
					auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
					if(cbytes) {
						fif_call imm = std::get<compiled_word_instance>(*wi).implementation;
						uint64_t imm_bytes = 0;
						memcpy(&imm_bytes, &imm, 8);
						cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
						cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));

						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(*wi).stack_types_start, size_t(std::get<compiled_word_instance>(*wi).stack_types_count)), *ws, std::get<compiled_word_instance>(*wi).llvm_parameter_permutation, env);
					}
				} else {
					std::get<compiled_word_instance>(*wi).implementation(*ws, nullptr, &env);
				}
			}
		}
	} else if(auto var = env.compiler_stack.back()->get_var(content_string); word.is_string == false && var != -1) {
		auto vdata = env.compiler_stack.back()->get_lvar_storage(var);
		if(vdata->is_stack_variable) {
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), vdata->type, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, env);

			if(skip_compilation(env.mode)) {

			} else if(typechecking_mode(env.mode)) {
				ws->push_back_main(mem_type.type, (int64_t)(vdata), 0);
			} else if(env.mode == fif_mode::compiling_llvm) {
				ws->push_back_main(mem_type.type, 0, vdata->expression);
			} else if(env.mode == fif_mode::interpreting) {
				ws->push_back_main(mem_type.type, (int64_t)(vdata), 0);
			} else if(env.mode == fif_mode::compiling_bytecode) {
				auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
				if(cbytes) {
					fif_call imm = immediate_local;
					uint64_t imm_bytes = 0;
					memcpy(&imm_bytes, &imm, 8);
					cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
					cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));

					cbytes->push_back(var); // index
					cbytes->push_back(mem_type.type);
				}
				ws->push_back_main(mem_type.type, 0, 0);
			}
		} else {
			if(skip_compilation(env.mode)) {

			} else if(typechecking_mode(env.mode)) {
				auto data_in = vdata->data;
				ws->push_back_main(vdata->type, vdata->data, nullptr);

				execute_fif_word(fif::parse_result{ "dup", false }, env, false);
				auto type_b = ws->main_type_back(0);
				auto dat_b = ws->main_data_back(0);
				if(vdata->data != ws->main_data_back(1)) {
					if(!env.compiler_stack.back()->re_let(var, ws->main_type_back(1), ws->main_data_back(1), nullptr))
						env.mode = fail_typechecking(env.mode);
				}

				ws->pop_main();
				ws->pop_main();
				ws->push_back_main(type_b, dat_b, nullptr);
			} else if(env.mode == fif_mode::compiling_llvm) {
				ws->push_back_main(vdata->type, 0, vdata->expression);

				execute_fif_word(fif::parse_result{ "dup", false }, env, false);
				auto type_b = ws->main_type_back(0);
				auto dat_b = ws->main_ex_back(0);
				if(vdata->expression != ws->main_ex_back(1)) {
					if(!env.compiler_stack.back()->re_let(var, ws->main_type_back(1), 0, ws->main_ex_back(1)))
						env.mode = fail_typechecking(env.mode);
				}

				ws->pop_main();
				ws->pop_main();
				ws->push_back_main(type_b, 0, dat_b);
			} else if(env.mode == fif_mode::interpreting) {
				ws->push_back_main(vdata->type, vdata->data, nullptr);

				execute_fif_word(fif::parse_result{ "dup", false }, env, false);
				auto type_b = ws->main_type_back(0);
				auto dat_b = ws->main_data_back(0);
				vdata->data = ws->main_data_back(1);
				
				ws->pop_main();
				ws->pop_main();
				ws->push_back_main(type_b, dat_b, nullptr);
			} else if(env.mode == fif_mode::compiling_bytecode) {
				// implement let
				auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
				if(cbytes) {
					fif_call imm = immediate_let;
					uint64_t imm_bytes = 0;
					memcpy(&imm_bytes, &imm, 8);
					cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
					cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
					cbytes->push_back(var);
				}
				ws->push_back_main(vdata->type, 0, 0);
			}
		}
	} else if(auto varb = get_global_var(env, content_string); word.is_string == false && varb) {
		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), varb->type, -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, env);

		if(skip_compilation(env.mode)) {

		} else if(typechecking_mode(env.mode)) {
			ws->push_back_main(mem_type.type, (int64_t)(varb), 0);
		} else if(env.mode == fif_mode::compiling_llvm) {
			ws->push_back_main(mem_type.type, 0, varb->expression);
		} else if(env.mode == fif_mode::interpreting) {
			ws->push_back_main(mem_type.type, (int64_t)(varb), 0);
		} else if(env.mode == fif_mode::compiling_bytecode) {
			// implement let
			auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
			if(cbytes) {
				fif_call imm = immediate_global;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));

				auto string_constant = env.get_string_constant(word.content);
				char const* cptr = string_constant.data();
				uint64_t let_addr = 0;
				memcpy(&let_addr, &cptr, 8);
				cbytes->push_back(int32_t(let_addr & 0xFFFFFFFF));
				cbytes->push_back(int32_t((let_addr >> 32) & 0xFFFFFFFF));

				cbytes->push_back(mem_type.type);
			}
			ws->push_back_main(mem_type.type, 0, 0);
		}
	} else if(auto rtype = resolve_type(word.content, env, env.compiler_stack.back()->type_substitutions()); rtype != -1) {
		do_immediate_type(*ws, rtype, &env);
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

	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env));
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
	ts.resize(param_stack.size(), return_stack.size());
	for(auto i = param_stack.size(); i-- > 0; ) {
		ts.set_main_type(i, param_stack[i]);
	}
	for(auto i = return_stack.size(); i-- > 0; ) {
		ts.set_return_type(i, return_stack[i]);
	}
	ts.min_main_depth = int32_t(param_stack.size());
	ts.min_return_depth = int32_t(param_stack.size());

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
	auto fn_type = llvm_function_type_from_desc(env, desc);
	auto compiled_fn = LLVMAddFunction(env.llvm_module, export_name.c_str(), fn_type);

	LLVMSetFunctionCallConv(compiled_fn, NATIVE_CC);
	//LLVMSetLinkage(compiled_fn, LLVMLinkage::LLVMLinkOnceAnyLinkage);
	//LLVMSetVisibility(compiled_fn, LLVMVisibility::LLVMDefaultVisibility);

	auto entry_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
	LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);


	// add body
	std::vector<LLVMValueRef> params;

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		params.push_back(LLVMGetParam(compiled_fn, uint32_t(consumed_stack_cells)));
		++match_position;
		++consumed_stack_cells;
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
	int32_t consumed_rstack_cells = 0;
	while(match_position < int32_t(desc.size()) && desc[match_position] != -1) {
		params.push_back(LLVMGetParam(compiled_fn, uint32_t(consumed_stack_cells + consumed_rstack_cells)));
		++match_position;
		++consumed_rstack_cells;
	}

	auto retvalue = LLVMBuildCall2(env.llvm_builder, llvm_function_type_from_desc(env, desc), wi.llvm_function, params.data(), uint32_t(params.size()), "");
	LLVMSetInstructionCallConv(retvalue, LLVMCallConv::LLVMFastCallConv);
	auto rsummary = llvm_function_return_type_from_desc(env, desc);

	// make return
	if(rsummary.composite_type == nullptr) {
		LLVMBuildRetVoid(env.llvm_builder);
	} else {
		LLVMBuildRet(env.llvm_builder, retvalue);
	}
	if(LLVMVerifyFunction(compiled_fn, LLVMVerifierFailureAction::LLVMPrintMessageAction))
		std::abort();

	env.exported_functions.push_back(compiled_fn);

	restore_compiler_stack_mode(env);
	env.compiler_stack.pop_back();
	env.source_stack.pop_back();

	return compiled_fn;
}
#endif

inline void run_fif_interpreter(environment& env, std::string_view on_text) {
	env.source_stack.push_back(std::string_view(on_text));
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env));
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
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env));
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
	{
		fif_call imm = interpreter_implementation;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		wi.compiled_bytecode.push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		wi.compiled_bytecode.push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
	}
	{
		fif_call imm = function_return;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		wi.compiled_bytecode.push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		wi.compiled_bytecode.push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
	}
	wi.llvm_compilation_finished = true;
	wi.stack_types_start = start_types;
	wi.stack_types_count = count_types;
	wi.typechecking_level = 3;
	wi.is_imported_function = true;

#ifdef USE_LLVM
	auto fn_desc = std::span<int32_t const>(itype_list.begin(), itype_list.end());
	auto fn_type = llvm_function_type_from_desc(env, fn_desc);
	wi.llvm_function = LLVMAddFunction(env.llvm_module, nstr.c_str(), fn_type);
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

}

#include "fif_language.hpp"
