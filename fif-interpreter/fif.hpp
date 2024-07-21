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

namespace fif {

template<typename T, typename... Args>
int64_t allocate_new_refcounted(Args&&... args) {
	auto bytes = sizeof(T) + 12;
	auto mem = new char[bytes];

	void (*delete_ptr)(T*) = +[](T* v) { v->~T(); };

	memcpy(mem, &delete_ptr, 8);
	auto c = (int32_t*)(mem + 8);
	*c = 1;
	new (mem + 12) T(std::forward<Args>(args)...);
	
	return (int64_t)mem;
}

template<typename T>
T* from_refcounted(int64_t ptr) {
	if(!ptr)
		return nullptr;
	return (T*)(ptr + 12);
}

void release_refcounted(int64_t ptr) {
	if(!ptr)
		return;
	int32_t* count = (int32_t*)(ptr + 8);
	*count = *count - 1;
	if(*count == 0) {
		char* pvalue = (char*)ptr;
		void (*delete_ptr)(void*) = nullptr;
		memcpy(&delete_ptr, pvalue, 8);
		delete_ptr(pvalue + 12);
		delete[] pvalue;
	}
}
void increment_refcounted(int64_t ptr) {
	if(!ptr)
		return;
	int32_t* count = (int32_t*)(ptr + 8);
	*count = *count + 1;
}

enum class stack_type {
	interpreter_stack = 0,
	bytecode_compiler = 1,
	llvm_compiler = 2
};

class state_stack {
protected:
	std::vector<int32_t> main_types;
	std::vector<int32_t> return_types;
public:
	size_t min_main_depth = 0;
	size_t min_return_depth = 0;

	void mark_used_from_main(size_t count) {
		min_main_depth = std::min(std::max(main_types.size(), count) - count, min_main_depth);
	}
	void mark_used_from_return(size_t count) {
		min_return_depth = std::min(std::max(return_types.size(), count) - count, min_return_depth);
	}

	virtual ~state_stack() = default;
	virtual void pop_main() = 0;
	virtual void pop_return() = 0;
	virtual int64_t main_data(size_t index) const = 0;
	virtual int64_t return_data(size_t index) const = 0;
	virtual LLVMValueRef main_ex(size_t index) const = 0;
	virtual LLVMValueRef return_ex(size_t index) const = 0;
	virtual int64_t main_data_back(size_t index) const = 0;
	virtual int64_t return_data_back(size_t index) const = 0;
	virtual LLVMValueRef main_ex_back(size_t index) const = 0;
	virtual LLVMValueRef return_ex_back(size_t index) const = 0;
	virtual void set_main_data(size_t index, int64_t value) = 0;
	virtual void set_return_data(size_t index, int64_t value)  = 0;
	virtual void set_main_ex(size_t index, LLVMValueRef value)  = 0;
	virtual void set_return_ex(size_t index, LLVMValueRef value)  = 0;
	virtual void set_main_data_back(size_t index, int64_t value)  = 0;
	virtual void set_return_data_back(size_t index, int64_t value)  = 0;
	virtual void set_main_ex_back(size_t index, LLVMValueRef value)  = 0;
	virtual void set_return_ex_back(size_t index, LLVMValueRef value)  = 0;
	virtual stack_type get_type() const = 0;
	virtual void move_into(state_stack&& other) = 0;
	virtual void copy_into(state_stack const& other) = 0;
	virtual void push_back_main(int32_t type, int64_t data, LLVMValueRef expr) = 0;
	virtual void push_back_return(int32_t type, int64_t data, LLVMValueRef expr) = 0;
	virtual std::unique_ptr< state_stack> copy() const = 0;
	virtual void resize(size_t main_sz, size_t return_sz) = 0;

	size_t main_size() const {
		return main_types.size();
	}
	size_t return_size()const {
		return return_types.size();
	}
	int32_t main_type(size_t index) const {
		return std::abs(main_types[index]);
	}
	int32_t return_type(size_t index) const {
		return std::abs(return_types[index]);
	}
	int32_t main_type_back(size_t index) const {
		return std::abs(main_types[main_types.size() - (index + 1)]);
	}
	int32_t return_type_back(size_t index) const {
		return std::abs(return_types[return_types.size() - (index + 1)]);
	}
	void set_main_type(size_t index, int32_t type) {
		main_types[index] = type;
		min_main_depth = std::min(min_main_depth, index);
	}
	void set_return_type(size_t index, int32_t type) {
		return_types[index] = type;
		min_return_depth = std::min(min_return_depth, index);
	}
	void set_main_type_back(size_t index, int32_t type) {
		main_types[main_types.size() - (index + 1)] = type;
		min_main_depth = std::min(min_main_depth, main_types.size() - (index + 1));
	}
	void set_return_type_back(size_t index, int32_t type) {
		return_types[return_types.size() - (index + 1)] = type;
		min_return_depth = std::min(min_return_depth, return_types.size() - (index + 1));
	}
};

class llvm_stack : public state_stack {
public:
	std::vector<LLVMValueRef> main_exs;
	std::vector<LLVMValueRef> return_exs;

	virtual ~llvm_stack() {
	}
	virtual void pop_main() {
		if(main_exs.empty())
			return;
		main_exs.pop_back();
		main_types.pop_back();
		min_main_depth = std::min(min_main_depth, main_types.size());
	}
	virtual void pop_return() {
		if(return_exs.empty())
			return;
		return_exs.pop_back();
		return_types.pop_back();
		min_return_depth = std::min(min_return_depth, return_types.size());
	}
	virtual void resize(size_t main_sz, size_t return_sz) {
		min_return_depth = std::min(min_return_depth, return_sz);
		min_main_depth = std::min(min_main_depth, main_sz);
		return_types.resize(return_sz);
		return_exs.resize(return_sz);
		main_types.resize(main_sz);
		main_exs.resize(main_sz);
	}
	virtual int64_t main_data(size_t index) const {
		return 0;
	}
	virtual int64_t return_data(size_t index) const {
		return 0;
	}
	virtual LLVMValueRef main_ex(size_t index) const {
		return main_exs[index];
	}
	virtual LLVMValueRef return_ex(size_t index) const {
		return return_exs[index];
	}
	virtual int64_t main_data_back(size_t index)const {
		return 0;
	}
	virtual int64_t return_data_back(size_t index)const {
		return 0;
	}
	virtual LLVMValueRef main_ex_back(size_t index)const {
		return main_exs[main_exs.size() - (index + 1)];
	}
	virtual LLVMValueRef return_ex_back(size_t index) const {
		return return_exs[return_exs.size() - (index + 1)];
	}
	virtual void set_main_data(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_data(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_ex(size_t index, LLVMValueRef value) {
		main_exs[index] = value;
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_ex(size_t index, LLVMValueRef value) {
		return_exs[index] = value;
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_data_back(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, main_types.size() - (index + 1));
	}
	virtual void set_return_data_back(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, return_types.size() - (index + 1));
	}
	virtual void set_main_ex_back(size_t index, LLVMValueRef value) {
		main_exs[main_exs.size() - (index + 1)] = value;
		min_main_depth = std::min(min_main_depth, main_types.size() - (index + 1));
	}
	virtual void set_return_ex_back(size_t index, LLVMValueRef value) {
		return_exs[return_exs.size() - (index + 1)] = value;
		min_return_depth = std::min(min_return_depth, return_types.size() - (index + 1));
	}
	virtual stack_type get_type() const {
		return stack_type::llvm_compiler;
	}
	virtual void move_into(state_stack&& other) {
		if(other.get_type() != stack_type::llvm_compiler)
			std::abort();

		llvm_stack&& o = static_cast<llvm_stack&&>(other);
		main_exs = std::move(o.main_exs);
		return_exs = std::move(o.return_exs);
		main_types = std::move(o.main_types);
		return_types = std::move(o.return_types);
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);
	}
	virtual void copy_into(state_stack const& other) {
		if(other.get_type() != stack_type::llvm_compiler)
			std::abort();

		llvm_stack const& o = static_cast<llvm_stack const&>(other);
		main_exs = o.main_exs;
		return_exs = o.return_exs;
		main_types = o.main_types;
		return_types = o.return_types;
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);
	}
	virtual void push_back_main(int32_t type, int64_t data, LLVMValueRef expr) {
		main_exs.push_back(expr);
		main_types.push_back(type);
	}
	virtual void push_back_return(int32_t type, int64_t data, LLVMValueRef expr) {
		return_exs.push_back(expr);
		return_types.push_back(type);
	}
	virtual std::unique_ptr<state_stack> copy() const {
		auto temp_new = std::make_unique<llvm_stack>(*this);
		temp_new->min_main_depth = main_types.size();
		temp_new->min_return_depth = return_types.size();
		return temp_new;
	}
};

class type_stack : public state_stack {
public:

	virtual ~type_stack() {
	}
	virtual void pop_main() {
		if(main_types.empty())
			return;
		main_types.pop_back();
		min_main_depth = std::min(min_main_depth, main_types.size());
	}
	virtual void pop_return() {
		if(return_types.empty())
			return;
		return_types.pop_back();
		min_return_depth = std::min(min_return_depth, return_types.size());
	}
	virtual void resize(size_t main_sz, size_t return_sz) {
		min_return_depth = std::min(min_return_depth, return_sz);
		min_main_depth = std::min(min_main_depth, main_sz);
		return_types.resize(return_sz);
		main_types.resize(main_sz);
	}
	virtual int64_t main_data(size_t index) const {
		return 0;
	}
	virtual int64_t return_data(size_t index) const {
		return 0;
	}
	virtual LLVMValueRef main_ex(size_t index) const {
		return nullptr;
	}
	virtual LLVMValueRef return_ex(size_t index) const {
		return nullptr;
	}
	virtual int64_t main_data_back(size_t index)const {
		return 0;
	}
	virtual int64_t return_data_back(size_t index)const {
		return 0;
	}
	virtual LLVMValueRef main_ex_back(size_t index)const {
		return nullptr;
	}
	virtual LLVMValueRef return_ex_back(size_t index) const {
		return nullptr;
	}
	virtual stack_type get_type() const {
		return stack_type::bytecode_compiler;
	}
	virtual void set_main_data(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_data(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_ex(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_ex(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_data_back(size_t index, int64_t value) {
		min_main_depth = std::min(min_main_depth, main_types.size() - (index + 1));
	}
	virtual void set_return_data_back(size_t index, int64_t value) {
		min_return_depth = std::min(min_return_depth, return_types.size() - (index + 1));
	}
	virtual void set_main_ex_back(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, main_types.size() - (index + 1));
	}
	virtual void set_return_ex_back(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, return_types.size() - (index + 1));
	}
	virtual void move_into(state_stack&& other) {
		if(other.get_type() != stack_type::bytecode_compiler)
			std::abort();

		type_stack&& o = static_cast<type_stack&&>(other);
		main_types = std::move(o.main_types);
		return_types = std::move(o.return_types);
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);
	}
	virtual void copy_into(state_stack const& other) {
		if(other.get_type() != stack_type::bytecode_compiler)
			std::abort();

		type_stack const& o = static_cast<type_stack const&>(other);
		main_types = o.main_types;
		return_types = o.return_types;
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);
	}
	virtual void push_back_main(int32_t type, int64_t data, LLVMValueRef expr) {
		main_types.push_back(type);
	}
	virtual void push_back_return(int32_t type, int64_t data, LLVMValueRef expr) {
		return_types.push_back(type);
	}
	virtual std::unique_ptr<state_stack> copy() const {
		auto temp_new = std::make_unique<type_stack>(*this);
		temp_new->min_main_depth = main_types.size();
		temp_new->min_return_depth = return_types.size();
		return temp_new;
	}
};


class environment;
struct type;

using fif_call = int32_t * (*)(state_stack&, int32_t*, environment*);

struct interpreted_word_instance {
	LLVMValueRef llvm_function = nullptr;
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
	int32_t compiled_offset = -1;
	int32_t compiled_size = 0;
	int32_t typechecking_level = 0;
	bool llvm_compilation_finished = false;
};

struct compiled_word_instance {
	fif_call implementation = nullptr;
	int32_t stack_types_start = 0;
	int32_t stack_types_count = 0;
};

using word_types = std::variant<interpreted_word_instance, compiled_word_instance>;

struct word {
	std::vector<int32_t> instances;
	std::string source;
	bool immediate =  false;
	bool being_compiled = false;
	bool being_typechecked = false;
};

inline LLVMValueRef empty_type_fn(LLVMValueRef r, int32_t type, environment*) {
	return r;
}

using llvm_zero_expr = LLVMValueRef (*)(LLVMContextRef);
using interpreted_new = int64_t(*)();

struct type {
	fif_call do_constructor = nullptr;
	fif_call do_destructor = nullptr;
	fif_call do_dup = nullptr;
	fif_call do_copy = nullptr;

	LLVMTypeRef llvm_type = nullptr;
	llvm_zero_expr zero_constant = nullptr;
	interpreted_new stack_value = nullptr;

	int32_t decomposed_types_start = 0;
	int32_t decomposed_types_count = 0;

	bool refcounted_type = false;
};

constexpr inline int32_t fif_i32 = 0;
constexpr inline int32_t fif_f32 = 1;
constexpr inline int32_t fif_bool = 2;
constexpr inline int32_t fif_type = 3;
constexpr inline int32_t fif_i64 = 4;
constexpr inline int32_t fif_f64 = 5;
constexpr inline int32_t fif_ui32 = 6;
constexpr inline int32_t fif_ui64 = 7;
constexpr inline int32_t fif_i16 = 8;
constexpr inline int32_t fif_ui16 = 9;
constexpr inline int32_t fif_i8 = 10;
constexpr inline int32_t fif_ui8 = 11;

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

	void ready_llvm_types(LLVMContextRef llvm_context) {
		type_array[fif_i32].llvm_type = LLVMInt32TypeInContext(llvm_context);
		type_array[fif_i32].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt32TypeInContext(c), 0, true);
		};
		type_array[fif_f32].llvm_type = LLVMFloatTypeInContext(llvm_context);
		type_array[fif_f32].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstReal(LLVMFloatTypeInContext(c), 0.0);
		};
		type_array[fif_i64].llvm_type = LLVMInt64TypeInContext(llvm_context);
		type_array[fif_i64].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt64TypeInContext(c), 0, true);
		};
		type_array[fif_f64].llvm_type = LLVMDoubleTypeInContext(llvm_context);
		type_array[fif_f64].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstReal(LLVMDoubleTypeInContext(c), 0.0);
		};
		type_array[fif_bool].llvm_type =  LLVMInt1TypeInContext(llvm_context);
		type_array[fif_bool].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt1TypeInContext(c), 0, true);
		};
		type_array[fif_ui32].llvm_type = LLVMInt32TypeInContext(llvm_context);
		type_array[fif_ui32].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt32TypeInContext(c), 0, false);
		};
		type_array[fif_ui64].llvm_type = LLVMInt64TypeInContext(llvm_context);
		type_array[fif_ui64].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt64TypeInContext(c), 0, false);
		};
		type_array[fif_ui16].llvm_type = LLVMInt16TypeInContext(llvm_context);
		type_array[fif_ui16].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt16TypeInContext(c), 0, false);
		};
		type_array[fif_ui8].llvm_type = LLVMInt8TypeInContext(llvm_context);
		type_array[fif_ui8].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt8TypeInContext(c), 0, false);
		};
		type_array[fif_i16].llvm_type = LLVMInt16TypeInContext(llvm_context);
		type_array[fif_i16].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt16TypeInContext(c), 0, true);
		};
		type_array[fif_i8].llvm_type = LLVMInt8TypeInContext(llvm_context);
		type_array[fif_i8].zero_constant = +[](LLVMContextRef c) {
			return LLVMConstInt(LLVMInt8TypeInContext(c), 0, true);
		};
	}
	dictionary() {
		types.insert_or_assign(std::string("nil"), -1);
		types.insert_or_assign(std::string("i32"), fif_i32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("f32"), fif_f32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("bool"), fif_bool);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("type"), fif_type);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("i64"), fif_i64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("f64"), fif_f64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("ui32"), fif_ui32);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("ui64"), fif_ui64);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("i16"), fif_i16);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("ui16"), fif_i16);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("i8"), fif_i8);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
		types.insert_or_assign(std::string("ui8"), fif_ui8);
		type_array.push_back(type{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, false });
	}
};


enum class control_structure : uint8_t {
	none, function, str_if, str_while_loop, str_do_loop, mode_switch
};

class compiler_state;

struct typecheck_3_record {
	int32_t stack_height_added_at = 0;
	int32_t rstack_height_added_at = 0;
	int32_t stack_consumed = 0;
	int32_t rstack_consumed = 0;
};

class opaque_compiler_data {
protected:
	opaque_compiler_data* parent = nullptr;
public:
	opaque_compiler_data(opaque_compiler_data* parent) : parent(parent) { }

	virtual ~opaque_compiler_data() = default;
	virtual control_structure get_type() {
		return parent ? parent->get_type() : control_structure::none;
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
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		if(parent) 
			parent->set_working_state(std::move(p));
	}
	virtual std::string_view* working_input() {
		return parent ? parent->working_input() : nullptr;
	}
	virtual bool finish(environment& env) {
		return true;
	}
};

class compiler_state {
public:
	std::unique_ptr<opaque_compiler_data> data;
};

enum class fif_mode {
	interpreting = 0,
	compiling_bytecode = 1,
	compiling_llvm = 2,
	terminated = 3,
	error = 4,
	typechecking_lvl_1 = 5,
	typechecking_lvl_2 = 6,
	typechecking_lvl_3 = 7,
	typechecking_lvl1_failed = 8,
	typechecking_lvl2_failed = 9,
	typechecking_lvl3_failed = 10
};

inline bool non_immediate_mode(fif_mode m) {
	return m != fif_mode::error && m != fif_mode::terminated && m != fif_mode::interpreting;
}
inline bool typechecking_mode(fif_mode m) {
	return m == fif_mode::typechecking_lvl_1 || m == fif_mode::typechecking_lvl_2 || m == fif_mode::typechecking_lvl_3
		|| m == fif_mode::typechecking_lvl1_failed || m == fif_mode::typechecking_lvl2_failed || m == fif_mode::typechecking_lvl3_failed;
}
inline bool typechecking_failed(fif_mode m) {
	return m == fif_mode::typechecking_lvl1_failed || m == fif_mode::typechecking_lvl2_failed || m == fif_mode::typechecking_lvl3_failed;
}
inline fif_mode reset_typechecking(fif_mode m) {
	if(m == fif_mode::typechecking_lvl1_failed)
		return fif_mode::typechecking_lvl_1;
	return m;
}
inline fif_mode fail_typechecking(fif_mode m) {
	if(m == fif_mode::typechecking_lvl_1)
		return fif_mode::typechecking_lvl1_failed;
	if(m == fif_mode::typechecking_lvl_2)
		return fif_mode::typechecking_lvl2_failed;
	if(m == fif_mode::typechecking_lvl_3)
		return fif_mode::typechecking_lvl3_failed;
	return m;
}

class environment {
public:
	std::vector<LLVMValueRef> exported_functions;
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

	dictionary dict;

	std::vector<compiler_state> compiler_stack;

	fif_mode mode = fif_mode::interpreting;
	std::function<void(std::string_view)> report_error = [](std::string_view) { std::abort(); };

	environment() {
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
	}

	~environment() {
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
	}
};


class interpreter_stack : public state_stack {
public:
	std::vector<int64_t> main_datas;
	std::vector<int64_t> return_datas;
	environment& env;

	interpreter_stack(environment& env) : env(env) { }

	virtual ~interpreter_stack() {
		for(auto i = main_datas.size(); i-- > 0;) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				release_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > 0;) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				release_refcounted(return_datas[i]);
		}
	}
	virtual void pop_main() {
		if(main_datas.empty())
			return;

		if(env.dict.type_array[main_types.back()].refcounted_type)
			release_refcounted(main_datas.back());
		main_datas.pop_back();
		main_types.pop_back();

		min_main_depth = std::min(min_main_depth, main_types.size());
	}
	virtual void pop_return() {
		if(return_datas.empty())
			return;

		if(env.dict.type_array[return_types.back()].refcounted_type)
			release_refcounted(return_datas.back());
		return_datas.pop_back();
		return_types.pop_back();

		min_return_depth = std::min(min_return_depth, return_types.size());
	}
	virtual void resize(size_t main_sz, size_t return_sz) {
		min_return_depth = std::min(min_return_depth, return_sz);
		min_main_depth = std::min(min_main_depth, main_sz);

		for(auto i = main_datas.size(); i-- > main_sz; ) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				release_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > return_sz; ) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				release_refcounted(return_datas[i]);
		}

		return_types.resize(return_sz);
		return_datas.resize(return_sz);
		main_types.resize(main_sz);
		main_datas.resize(main_sz);
	}
	virtual int64_t main_data(size_t index) const {
		return main_datas[index];
	}
	virtual int64_t return_data(size_t index) const {
		return return_datas[index];
	}
	virtual LLVMValueRef main_ex(size_t index) const {
		return nullptr;
	}
	virtual LLVMValueRef return_ex(size_t index) const {
		return nullptr;
	}
	virtual int64_t main_data_back(size_t index)const {
		return main_datas[main_datas.size() - (index + 1)];
	}
	virtual int64_t return_data_back(size_t index)const {
		return return_datas[return_datas.size() - (index + 1)];
	}
	virtual LLVMValueRef main_ex_back(size_t index)const {
		return nullptr;
	}
	virtual LLVMValueRef return_ex_back(size_t index) const {
		return nullptr;
	}
	virtual void set_main_data(size_t index, int64_t value) {
		if(env.dict.type_array[main_type(index)].refcounted_type) {
			release_refcounted(main_datas[index]);
			increment_refcounted(value);
		}
		main_datas[index] = value;
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_data(size_t index, int64_t value) {
		if(env.dict.type_array[return_type(index)].refcounted_type) {
			release_refcounted(return_datas[index]);
			increment_refcounted(value);
		}
		return_datas[index] = value;
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_ex(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, index);
	}
	virtual void set_return_ex(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, index);
	}
	virtual void set_main_data_back(size_t index, int64_t value) {
		if(env.dict.type_array[main_type_back(index)].refcounted_type) {
			release_refcounted(main_datas[main_datas.size() - (index + 1)]);
			increment_refcounted(value);
		}
		main_datas[main_datas.size() - (index + 1)] = value;
		min_main_depth = std::min(min_main_depth, main_datas.size() - (index + 1));
	}
	virtual void set_return_data_back(size_t index, int64_t value) {
		if(env.dict.type_array[return_type_back(index)].refcounted_type) {
			release_refcounted(return_datas[return_datas.size() - (index + 1)]);
			increment_refcounted(value);
		}
		return_datas[return_datas.size() - (index + 1)] = value;
		min_return_depth = std::min(min_return_depth, return_datas.size() - (index + 1));
	}
	virtual void set_main_ex_back(size_t index, LLVMValueRef value) {
		min_main_depth = std::min(min_main_depth, main_datas.size() - (index + 1));
	}
	virtual void set_return_ex_back(size_t index, LLVMValueRef value) {
		min_return_depth = std::min(min_return_depth, return_datas.size() - (index + 1));
	}
	virtual stack_type get_type() const {
		return stack_type::interpreter_stack;
	}
	virtual void move_into(state_stack&& other) {
		if(other.get_type() != stack_type::interpreter_stack)
			std::abort();

		for(auto i = main_datas.size(); i-- > 0;) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				release_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > 0;) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				release_refcounted(return_datas[i]);
		}

		interpreter_stack&& o = static_cast<interpreter_stack&&>(other);
		main_datas = std::move(o.main_datas);
		return_datas = std::move(o.return_datas);
		main_types = std::move(o.main_types);
		return_types = std::move(o.return_types);
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);
		o.main_datas.clear();
		o.return_datas.clear();
		o.main_types.clear();
		o.return_types.clear();
	}
	virtual void copy_into(state_stack const& other) {
		if(other.get_type() != stack_type::interpreter_stack)
			std::abort();

		for(auto i = main_datas.size(); i-- > 0;) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				release_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > 0;) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				release_refcounted(return_datas[i]);
		}

		interpreter_stack const& o = static_cast<interpreter_stack const&>(other);
		main_datas = o.main_datas;
		return_datas = o.return_datas;
		main_types = o.main_types;
		return_types = o.return_types;
		min_main_depth = std::min(min_main_depth, o.min_main_depth);
		min_return_depth = std::min(min_return_depth, o.min_return_depth);

		for(auto i = main_datas.size(); i-- > 0;) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				increment_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > 0;) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				increment_refcounted(return_datas[i]);
		}
	}
	virtual void push_back_main(int32_t type, int64_t data, LLVMValueRef expr) {
		if(env.dict.type_array[type].refcounted_type)
			increment_refcounted(data);
		main_datas.push_back(data);
		main_types.push_back(type);
	}
	virtual void push_back_return(int32_t type, int64_t data, LLVMValueRef expr) {
		if(env.dict.type_array[type].refcounted_type)
			increment_refcounted(data);
		return_datas.push_back(data);
		return_types.push_back(type);
	}
	virtual std::unique_ptr<state_stack> copy() const {
		for(auto i = main_datas.size(); i-- > 0;) {
			if(env.dict.type_array[main_types[i]].refcounted_type)
				increment_refcounted(main_datas[i]);
		}
		for(auto i = return_datas.size(); i-- > 0;) {
			if(env.dict.type_array[return_types[i]].refcounted_type)
				increment_refcounted(return_datas[i]);
		}

		auto temp_new = std::make_unique< interpreter_stack>(*this);
		temp_new->min_main_depth = main_types.size();
		temp_new->min_return_depth = return_types.size();
		return temp_new;
	}
};

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

inline type_match internal_resolve_type(std::string_view text, environment& env) {
	uint32_t mt_end = 0;
	for(; mt_end < text.size(); ++mt_end) {
		if(text[mt_end] == '(' || text[mt_end] == ')' || text[mt_end] == ',')
			break;
	}
	if(auto it = env.dict.types.find(std::string(text.substr(0, mt_end))); it != env.dict.types.end()) {
		if(mt_end >= text.size() || text[mt_end] == ',' ||  text[mt_end] == ')') {	// case: plain type
			return type_match{ it->second, mt_end };
		}
		//followed by type list
		++mt_end;
		std::vector<int32_t> subtypes;
		while(mt_end < text.size() && text[mt_end] != ')') {
			auto sub_match = internal_resolve_type(text.substr(mt_end), env);
			subtypes.push_back(sub_match.type);
			mt_end += sub_match.end_match_pos;
			if(mt_end < text.size() && text[mt_end] == ',')
				++mt_end;
		}
		if(mt_end < text.size() && text[mt_end] == ')')
			++mt_end;

		for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
			if(env.dict.type_array[i].decomposed_types_count == 1 + subtypes.size()) {
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
		env.dict.type_array.back().do_constructor = env.dict.type_array[it->second].do_constructor;
		env.dict.type_array.back().do_copy = env.dict.type_array[it->second].do_copy;
		env.dict.type_array.back().do_destructor = env.dict.type_array[it->second].do_destructor;
		env.dict.type_array.back().do_dup = env.dict.type_array[it->second].do_dup;
		env.dict.type_array.back().llvm_type = env.dict.type_array[it->second].llvm_type;
		env.dict.type_array.back().decomposed_types_count = uint32_t(1 + subtypes.size());
		env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
		env.dict.type_array.back().refcounted_type = env.dict.type_array[it->second].refcounted_type;
		env.dict.type_array.back().stack_value = env.dict.type_array.back().stack_value;
		env.dict.type_array.back().zero_constant = env.dict.type_array.back().zero_constant;

		env.dict.all_stack_types.push_back(it->second);
		for(auto t : subtypes) {
			env.dict.all_stack_types.push_back(t);
		}

		return type_match{ new_type, mt_end };
	}

	return type_match{ -1, mt_end };
}

inline int32_t resolve_type(std::string_view text, environment& env) {
	return internal_resolve_type(text, env).type;
}

inline type_match resolve_span_type(std::span<int32_t const> tlist, std::vector<int32_t> const& type_subs, environment& env) {

	auto make_sub = [&](int32_t type_in) {
		if(type_in < -1) {
			auto slot = -(type_in + 2);
			if(slot < type_subs.size())
				return type_subs[slot];
			return -1;
		}
		return type_in;
	};

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

	for(uint32_t i = 0; i < env.dict.type_array.size(); ++i) {
		if(env.dict.type_array[i].decomposed_types_count == 1 + subtypes.size()) {
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

	env.dict.type_array.back().do_constructor = env.dict.type_array[base_type].do_constructor;
	env.dict.type_array.back().do_copy = env.dict.type_array[base_type].do_copy;
	env.dict.type_array.back().do_destructor = env.dict.type_array[base_type].do_destructor;
	env.dict.type_array.back().do_dup = env.dict.type_array[base_type].do_dup;
	env.dict.type_array.back().llvm_type = env.dict.type_array[base_type].llvm_type;
	env.dict.type_array.back().decomposed_types_count = uint32_t(1 + subtypes.size());
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	env.dict.type_array.back().refcounted_type = env.dict.type_array[base_type].refcounted_type;
	env.dict.type_array.back().stack_value = env.dict.type_array.back().stack_value;
	env.dict.type_array.back().zero_constant = env.dict.type_array.back().zero_constant;
	env.dict.all_stack_types.push_back(base_type);
	for(auto t : subtypes) {
		env.dict.all_stack_types.push_back(t);
	}

	return type_match{ new_type, mt_end };
}

struct variable_match_result {
	bool match_result = false;
	uint32_t match_end = 0;
};
inline variable_match_result fill_in_variable_types(int32_t source_type, std::span<int32_t const> match_span, std::vector<int32_t>& type_subs, environment& env) { 
	if(match_span.size() == 0)
		return variable_match_result{ true, 0 };

	auto match_to_slot = [&](int32_t matching_type, int32_t matching_against) { 
		if(matching_type < -1) {
			auto slot = -(matching_type + 2);
			if(slot >= type_subs.size()) {
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
		result |= sub_result.match_result;
		++dest_offset;
	}
	if(dest_offset < destructured_source.size())
		result |= false;
	if(sub_offset < match_span.size() && match_span[sub_offset] == -1)
		++sub_offset;

	return variable_match_result{ result, sub_offset };
}
inline uint32_t next_encoded_stack_type(std::span<int32_t const> desc) {
	if(desc.size() == 0)
		return 0;
	if(desc.size() == 1)
		return 1;
	if(desc[1] != std::numeric_limits<int32_t>::max())
		return 1;
	uint32_t i = 2;
	while( i < desc.size() && desc[i] != -1) {
		i += next_encoded_stack_type(desc.subspan(i));
	}
	if(i < desc.size() && desc[i] == -1)
		++i;
	return i;
}

inline type_match_result match_stack_description(std::span<int32_t const> desc, state_stack& ts, environment& env) { // ret: true if function matched
	int32_t match_position = 0;
	// stack matching

	std::vector<int32_t> type_subs;

	auto const ssize = ts.main_size();
	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		if(match_position >= ssize)
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
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++output_stack_types;
	}
	//int32_t last_output_stack = match_position;
	++match_position; // skip -1

	// return stack matching
	auto const rsize = ts.return_size();
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
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
	int32_t ret_added = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		auto result = resolve_span_type(desc.subspan(match_position), type_subs, env);
		match_position += result.end_match_pos;
		++ret_added;
	}

	return type_match_result{ true, consumed_stack_cells, consumed_rstack_cells };
}


inline void apply_stack_description(std::span<int32_t const> desc, state_stack& ts, environment& env) { // ret: true if function matched
	int32_t match_position = 0;
	// stack matching

	std::vector<int32_t> type_subs;

	auto const ssize = ts.main_size();
	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		if(match_position >= ssize)
			return;
		auto match_result = fill_in_variable_types(ts.main_type_back(consumed_stack_cells), desc.subspan(match_position), type_subs, env);
		if(!match_result.match_result)
			return;
		match_position += match_result.match_end;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++output_stack_types;
	}
	//int32_t last_output_stack = match_position;
	++match_position; // skip -1

	// return stack matching
	auto const rsize = ts.return_size();
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		if(consumed_rstack_cells >= rsize)
			return;
		auto match_result = fill_in_variable_types(ts.return_type_back(consumed_rstack_cells), desc.subspan(match_position), type_subs, env);
		if(!match_result.match_result)
			return;
		match_position += match_result.match_end;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// drop consumed types
	ts.resize(ssize - consumed_stack_cells, rsize - consumed_rstack_cells);

	// add returned stack types
	while(first_output_stack < desc.size() && desc[first_output_stack] != -1) {
		auto result = resolve_span_type(desc.subspan(first_output_stack), type_subs, env);
		first_output_stack += result.end_match_pos;
		ts.push_back_main(result.type, 0, nullptr);
	}

	// output ret stack
	// add new types
	int32_t ret_added = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		auto result = resolve_span_type(desc.subspan(match_position), type_subs, env);
		ts.push_back_return(result.type, 0, nullptr);
		match_position += result.end_match_pos;
		++ret_added;
	}
}


struct stack_consumption {
	int32_t stack;
	int32_t rstack;
};
stack_consumption get_stack_consumption(int32_t word, int32_t alternative, environment& env) {
	int32_t count_desc;
	int32_t desc_offset;
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
	while(match_position < desc.size() && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// skip over output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		match_position += next_encoded_stack_type(desc.subspan(match_position));
		++output_stack_types;
	}

	++match_position; // skip -1

	// return stack matching
	
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
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
};

inline word_match_result match_word(word const& w, state_stack& ts, std::vector<word_types> const& all_instances, std::vector<int32_t> const& all_stack_types, environment& env) {
	for(uint32_t i = 0; i < w.instances.size(); ++i) {
		if(std::holds_alternative<interpreted_word_instance>(all_instances[w.instances[i]])) {
			interpreted_word_instance const& s = std::get<interpreted_word_instance>(all_instances[w.instances[i]]);
			auto mr = match_stack_description(std::span<int32_t const>(all_stack_types.data() + s.stack_types_start, size_t(s.stack_types_count)), ts, env);
			if(mr.matched) {
				return word_match_result{ mr.matched, mr.stack_consumed, mr.ret_stack_consumed, w.instances[i] };
			}
		} else if(std::holds_alternative<compiled_word_instance>(all_instances[w.instances[i]])) {
			compiled_word_instance const& s = std::get<compiled_word_instance>(all_instances[w.instances[i]]);
			auto mr = match_stack_description(std::span<int32_t const>(all_stack_types.data() + s.stack_types_start, size_t(s.stack_types_count)), ts, env);
			if(mr.matched) {
				return word_match_result{ mr.matched, mr.stack_consumed, mr.ret_stack_consumed, w.instances[i] };
			}
		}
	}
	return word_match_result{ false, 0, 0, 0 };
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
	int32_t* ptr = env.dict.all_compiled.data() + wi.compiled_offset;
	while(ptr) {
		fif_call fn = nullptr;
		memcpy(&fn, ptr, 8);
		ptr = fn(ss, ptr, &env);
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
	}
}

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

bool float_from_chars(char const* start, char const* end, float& float_out) { // returns true on success
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
	if(typechecking_failed(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back().data->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_i32;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(value);
	}
	LLVMValueRef val = nullptr;
	if(e->mode == fif_mode::compiling_llvm) {
		val = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), uint32_t(value), true);
	}
	s.push_back_main(fif_i32, value, val);
}
inline void do_immediate_bool(state_stack& s, bool value, environment* e) {
	if(typechecking_failed(e->mode))
		return;

	auto compile_bytes = e->compiler_stack.back().data->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_bool;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t(value));
	}
	LLVMValueRef val = nullptr;
	if(e->mode == fif_mode::compiling_llvm) {
		val = LLVMConstInt(LLVMInt1TypeInContext(e->llvm_context), uint32_t(value), true);
	}
	s.push_back_main(fif_bool, int64_t(value), val);
}
inline void do_immediate_f32(state_stack& s, float value, environment* e) {
	if(typechecking_failed(e->mode))
		return;

	int32_t v4 = 0;
	int64_t v8 = 0;
	memcpy(&v4, &value, 4);
	memcpy(&v8, &value, 4);

	auto compile_bytes = e->compiler_stack.back().data->bytecode_compilation_progress();
	if(compile_bytes && e->mode == fif_mode::compiling_bytecode) {
		fif_call imm = immediate_i32;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(v4);
	}
	LLVMValueRef val = nullptr;
	if(e->mode == fif_mode::compiling_llvm) {
		val = LLVMConstReal(LLVMFloatTypeInContext(e->llvm_context), value);
	}
	s.push_back_main(fif_i32, v8, val);
}
inline int32_t* call_function(state_stack& s, int32_t* p, environment* env) {
	auto ptr = env->dict.all_compiled.data() + *(p + 2);
	execute_fif_word(s, ptr, *env);
	return p + 3;
}
inline int32_t* call_function_indirect(state_stack& s, int32_t* p, environment* env) {
	auto instance = *(p + 2);
	execute_fif_word(std::get<interpreted_word_instance>(env->dict.all_instances[instance]), s, *env);
	return p + 3;
}
inline int32_t* function_return(state_stack& s, int32_t* p, environment*) {
	return nullptr;
}
inline int32_t* type_construction(state_stack& s, int32_t* p, environment* e) {
	auto type = *(p + 2);
	auto is_refcounted = e->dict.type_array[type].refcounted_type;
	s.push_back_main(is_refcounted ? -type : type, is_refcounted ? e->dict.type_array[type].stack_value() : 0, nullptr);
	return p + 3;
}
inline void do_construct_type(state_stack& s, int32_t type, environment* env) {
	if(typechecking_failed(env->mode))
		return;

	auto compile_bytes = env->compiler_stack.back().data->bytecode_compilation_progress();
	if(compile_bytes && env->mode == fif_mode::compiling_bytecode) {
		fif_call imm = type_construction;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(type);
	}
	LLVMValueRef val = nullptr;
	if(env->mode == fif_mode::compiling_llvm) {
		if(env->dict.type_array[type].zero_constant) {
			val = env->dict.type_array[type].zero_constant(env->llvm_context);
		} else {
			env->report_error("attempted to compile a type without an llvm representation");
			env->mode = fif_mode::error;
			return;
		}
	}
	s.push_back_main(type, 0, val);
	if(env->dict.type_array[type].do_constructor) {
		env->dict.type_array[type].do_constructor(s, nullptr, env);
	}
}

inline LLVMTypeRef llvm_function_type_from_desc(environment& env, std::span<int32_t const> desc) {
	std::vector<LLVMTypeRef> parameter_group;
	std::vector<LLVMTypeRef> returns_group;

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	int32_t ret_added = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
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
	return LLVMFunctionType(ret_type, parameter_group.data(), uint32_t(parameter_group.size()), false);
}

inline void llvm_make_function_parameters(environment& env, LLVMValueRef fn, state_stack& ws, std::span<int32_t const> desc) {

	/*
	* NOTE: function assumes that description is fully resolved
	*/

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		ws.set_main_ex_back(consumed_stack_cells, LLVMGetParam(fn, uint32_t(consumed_stack_cells)));
		++match_position;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		ws.set_return_ex_back(consumed_rstack_cells, LLVMGetParam(fn, uint32_t(consumed_stack_cells + consumed_stack_cells)));
		++match_position;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	int32_t ret_added = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		++match_position;
		++ret_added;
	}
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

	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
		++match_position;
		++consumed_rstack_cells;
	}

	++match_position; // skip -1

	// output ret stack
	int32_t ret_added = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
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

inline void llvm_make_function_return(environment& env, std::span<int32_t const> desc) {
	auto rsummary = llvm_function_return_type_from_desc( env, desc);
	
	if(rsummary.composite_type == nullptr) {
		LLVMBuildRetVoid(env.llvm_builder);
		return;
	}
	if(rsummary.is_struct_type == false) {
		if(rsummary.num_stack_values == 0) {
			LLVMBuildRet(env.llvm_builder, env.compiler_stack.back().data->working_state()->return_ex_back(0));
			return;
		} else if(rsummary.num_rstack_values == 0) {
			LLVMBuildRet(env.llvm_builder, env.compiler_stack.back().data->working_state()->main_ex_back(0));
			return;
		} else {
			assert(false);
		}
	}

	auto rstruct = LLVMGetUndef(rsummary.composite_type);
	uint32_t insert_index = 0;

	for(int32_t i = rsummary.num_stack_values; i > 0; --i) {
		rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, env.compiler_stack.back().data->working_state()->main_ex_back(i), insert_index, "");
		++insert_index;
	}
	for(int32_t i = rsummary.num_rstack_values; i > 0;  --i) {
		rstruct = LLVMBuildInsertValue(env.llvm_builder, rstruct, env.compiler_stack.back().data->working_state()->return_ex_back(i), insert_index, "");
		++insert_index;
	}
	LLVMBuildRet(env.llvm_builder, rstruct);
}

inline void llvm_make_function_call(environment& env, LLVMValueRef fn, std::span<int32_t const> desc) {
	std::vector<LLVMValueRef> params;
	
	{
		int32_t match_position = 0;
		// stack matching

		int32_t consumed_stack_cells = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			params.push_back(env.compiler_stack.back().data->working_state()->main_ex_back(0));
			env.compiler_stack.back().data->working_state()->pop_main();
			++match_position;
			++consumed_stack_cells;
		}

		++match_position; // skip -1

		// output stack
		int32_t first_output_stack = match_position;
		int32_t output_stack_types = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			//returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type.llvm_type);
			++match_position;
			++output_stack_types;
		}
		++match_position; // skip -1

		// return stack matching
		int32_t consumed_rstack_cells = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			params.push_back(env.compiler_stack.back().data->working_state()->return_ex_back(0));
			env.compiler_stack.back().data->working_state()->pop_return();
			++match_position;
			++consumed_rstack_cells;
		}
	}
	auto retvalue = LLVMBuildCall2(env.llvm_builder, llvm_function_type_from_desc(env, desc), fn, params.data(), uint32_t(params.size()), "");
	auto rsummary = llvm_function_return_type_from_desc(env, desc);

	if(rsummary.composite_type == nullptr) {
		return;
	}

	{
		uint32_t extract_index = 0;
		int32_t match_position = 0;
		// stack matching

		int32_t consumed_stack_cells = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
			++match_position;
			++consumed_stack_cells;
		}

		++match_position; // skip -1

		// output stack
		int32_t first_output_stack = match_position;
		int32_t output_stack_types = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			if(rsummary.is_struct_type == false) { // single return value
				env.compiler_stack.back().data->working_state()->push_back_main(desc[match_position], 0, retvalue);
			} else {
				env.compiler_stack.back().data->working_state()->push_back_main(desc[match_position], 0, LLVMBuildExtractValue(env.llvm_builder, retvalue, extract_index, ""));
			}
			++extract_index;
			++match_position;
			++output_stack_types;
		}
		++match_position; // skip -1

		// return stack matching
		int32_t consumed_rstack_cells = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			//parameter_group.push_back(env.dict.type_array[desc[match_position]].llvm_type);
			++match_position;
			++consumed_rstack_cells;
		}

		++match_position; // skip -1

		// output ret stack
		int32_t ret_added = 0;
		while(match_position < desc.size() && desc[match_position] != -1) {
			if(rsummary.is_struct_type == false) { // single return value
				env.compiler_stack.back().data->working_state()->push_back_return(desc[match_position], 0, retvalue);
			} else {
				env.compiler_stack.back().data->working_state()->push_back_return(desc[match_position], 0, LLVMBuildExtractValue(env.llvm_builder, retvalue, extract_index, ""));
			}
			++extract_index;
			++match_position;
			++ret_added;
		}
	}
}

inline std::vector<int32_t> expand_stack_description(state_stack& initial_stack_state, std::span<const int32_t> desc, int32_t stack_consumed, int32_t rstack_consumed);
inline bool compare_stack_description(std::span<const int32_t> a, std::span<const int32_t> b);

class function_scope : public opaque_compiler_data {
public:
	std::unique_ptr<state_stack> initial_state;
	std::unique_ptr<state_stack> iworking_state;
	std::vector<int32_t> compiled_bytes;
	std::string_view working_source;
	LLVMValueRef llvm_fn = nullptr;
	LLVMBasicBlockRef current_block = nullptr;
	int32_t for_word = -1;
	int32_t for_instance = -1;

	function_scope(opaque_compiler_data* parent, environment& env, state_stack& entry_state, std::string_view working_source, int32_t for_word, int32_t for_instance) : opaque_compiler_data(parent) , working_source(working_source), for_word(for_word), for_instance(for_instance) {
		
		initial_state = entry_state.copy();
		iworking_state = entry_state.copy();

		if(for_word == -1) {
			env.report_error("attempted to compile a function for an anonymous word");
			env.mode = fif_mode::error;
			return;
		}

		
			if(typechecking_mode(env.mode))
				env.dict.word_array[for_word].being_typechecked = true;
			else if(env.mode == fif_mode::compiling_bytecode || env.mode == fif_mode::compiling_llvm)
				env.dict.word_array[for_word].being_compiled = true;

			if(env.mode == fif_mode::compiling_llvm) {
				assert(for_instance != -1);
				auto fn_desc = std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_start, std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).stack_types_count);

				if(!std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function) {
					auto fn_string_name = word_name_from_id(for_word, env) + "#" + std::to_string(for_instance);
					auto fn_type = llvm_function_type_from_desc(env, fn_desc);
					std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function = LLVMAddFunction(env.llvm_module, fn_string_name.c_str(), fn_type);
				}
				auto compiled_fn = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).llvm_function;
				llvm_fn = compiled_fn;

				// for unknown reasons, this calling convention breaks things
				//LLVMSetFunctionCallConv(compiled_fn, LLVMCallConv::LLVMFastCallConv);

				LLVMSetLinkage(compiled_fn, LLVMLinkage::LLVMPrivateLinkage);
				auto entry_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
				LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);
				llvm_make_function_parameters(env, compiled_fn, *iworking_state, fn_desc);

				current_block = entry_block;
			}
		
	}

	virtual control_structure get_type() {
		return control_structure::function;
	}
	virtual LLVMValueRef llvm_function() {
		return llvm_fn;
	}
	virtual LLVMBasicBlockRef* llvm_block() {
		return &current_block;
	}
	virtual int32_t word_id() {
		return for_word;
	}
	virtual int32_t instance_id() {
		return for_instance;
	}
	virtual std::vector<int32_t>* bytecode_compilation_progress() {
		return &compiled_bytes;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record() {
		return parent ? parent->typecheck_record() : nullptr;
	}
	virtual state_stack* working_state() {
		return iworking_state.get();
	}
	virtual std::string_view* working_input() {
		return &working_source;
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		iworking_state = std::move(p);
	}
	virtual bool finish(environment& env) {
		int32_t stack_consumed = int32_t(initial_state->main_size()) - int32_t(iworking_state->min_main_depth);
		int32_t rstack_consumed = int32_t(initial_state->return_size()) - int32_t(iworking_state->min_return_depth);
		int32_t stack_added = int32_t(iworking_state->main_size()) - int32_t(iworking_state->min_main_depth);
		int32_t rstack_added = int32_t(iworking_state->return_size()) - int32_t(iworking_state->min_return_depth);
		assert(stack_added >= 0);
		assert(rstack_added >= 0);
		assert(stack_consumed >= 0);
		assert(rstack_consumed >= 0);

		if(env.mode == fif_mode::typechecking_lvl_1) {
			word& w = env.dict.word_array[for_word];

			interpreted_word_instance temp;
			auto& current_stack = *iworking_state;

			temp.stack_types_start = int32_t(env.dict.all_stack_types.size());
			{
				for(int32_t i = 0; i < stack_consumed; ++i) {
					env.dict.all_stack_types.push_back(initial_state->main_type_back(i));
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
					env.dict.all_stack_types.push_back(initial_state->return_type_back(i));
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
			temp.typechecking_level = 1;

			if(for_instance == -1) {
				w.instances.push_back(int32_t(env.dict.all_instances.size()));
				env.dict.all_instances.push_back(std::move(temp));
			} else if(std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]).typechecking_level == 0) {
				env.dict.all_instances[for_instance] = std::move(temp);
			}

			env.dict.word_array[for_word].being_typechecked = false;
		} else if(env.mode == fif_mode::typechecking_lvl_2) {
			if(for_instance == -1) {
				env.report_error("tried to level 2 typecheck a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}
			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			std::span<const int32_t> existing_description = std::span<const int32_t>(env.dict.all_stack_types.data() + wi.stack_types_start, size_t(wi.stack_types_count));
			auto revised_description = expand_stack_description(*initial_state, existing_description, stack_consumed, rstack_consumed);

			if(!compare_stack_description(existing_description, std::span<const int32_t>(revised_description.data(), revised_description.size()))) {
				wi.stack_types_start = int32_t(env.dict.all_stack_types.size());
				wi.stack_types_count = int32_t(revised_description.size());
				env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), revised_description.begin(), revised_description.end());
			}

			wi.typechecking_level = std::max(wi.typechecking_level, 2);
			env.dict.word_array[for_word].being_typechecked = false;
		} else if(env.mode == fif_mode::typechecking_lvl_3) {
			// no alterations -- handled explicitly by invoking lvl 3
			env.dict.word_array[for_word].being_typechecked = false;
		} else if(env.mode == fif_mode::compiling_bytecode) {
			if(for_instance == -1) {
				env.report_error("tried to compile a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}

			fif_call imm = function_return;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compiled_bytes.push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compiled_bytes.push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));

			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);

			wi.compiled_offset = int32_t(env.dict.all_compiled.size());
			wi.compiled_size = int32_t(compiled_bytes.size() - 2);
			env.dict.all_compiled.insert(env.dict.all_compiled.end(), compiled_bytes.begin(), compiled_bytes.end());

			env.dict.word_array[for_word].being_compiled = false;
		} else if(env.mode == fif_mode::compiling_llvm) {
			if(for_instance == -1) {
				env.report_error("tried to compile a word without an instance");
				env.mode = fif_mode::error;
				return true;
			}

			interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[for_instance]);
			std::span<const int32_t> existing_description = std::span<const int32_t>(env.dict.all_stack_types.data() + wi.stack_types_start, size_t(wi.stack_types_count));
			llvm_make_function_return(env, existing_description);
			wi.llvm_compilation_finished = true;

			if(LLVMVerifyFunction(wi.llvm_function, LLVMVerifierFailureAction::LLVMPrintMessageAction)) {
				env.report_error("LLVM verification of function failed");
				env.mode = fif_mode::error;
				return true;
			}

			env.dict.word_array[for_word].being_compiled = false;
		} else if(typechecking_failed(env.mode)) {
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

class mode_switch_scope : public opaque_compiler_data {
public:
	opaque_compiler_data* interpreted_link = nullptr;
	fif_mode entry_mode;
	fif_mode running_mode;

	mode_switch_scope(opaque_compiler_data* parent, environment& env, fif_mode running_mode) : opaque_compiler_data(parent), running_mode(running_mode) {
		[&]() {
			for(auto i = env.compiler_stack.size(); i-- > 0; ) {
				if(env.compiler_stack[i].data->get_type() == control_structure::mode_switch) {
					mode_switch_scope* p = static_cast<mode_switch_scope*>(env.compiler_stack[i].data.get());
					if(p->running_mode == running_mode) {
						++i;
						for(; i < env.compiler_stack.size(); ++i) {
							if(env.compiler_stack[i].data->get_type() == control_structure::mode_switch) {
								interpreted_link = env.compiler_stack[i - 1].data.get();
								return;
							}
						}
						interpreted_link = env.compiler_stack[env.compiler_stack.size() - 1].data.get();
						return;
					}
				}
			}
		}();
		entry_mode = env.mode;
		env.mode = running_mode;
	}

	virtual control_structure get_type() {
		return control_structure::mode_switch;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record() {
		return parent ? parent->typecheck_record() : nullptr;
	}
	virtual state_stack* working_state() {
		return interpreted_link ? interpreted_link->working_state() : nullptr;
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		if(interpreted_link)
			interpreted_link->set_working_state(std::move(p));
	}
	virtual bool finish(environment& env) {
		if(env.mode != fif_mode::error) {
			env.mode = entry_mode;
		}
		return true;
	}
};
inline void switch_compiler_stack_mode(environment& env, fif_mode new_mode) {
	auto m = std::make_unique<mode_switch_scope>(env.compiler_stack.back().data.get(), env, new_mode);
	env.compiler_stack.emplace_back(std::move(m));
}
inline void restore_compiler_stack_mode(environment& env) {
	if(env.compiler_stack.empty() || env.compiler_stack.back().data->get_type() != control_structure::mode_switch) {
		env.report_error("attempt to switch mode back revealed an unbalanced compiler stack");
	}
	env.compiler_stack.back().data->finish(env);
	env.compiler_stack.pop_back();
}
class outer_interpreter : public opaque_compiler_data {
public:
	std::unique_ptr<state_stack> interpreter_state;
	std::string_view source;

	outer_interpreter(environment& env, std::string_view source) : opaque_compiler_data(nullptr), interpreter_state(std::make_unique<interpreter_stack>(env)), source(source) {
	}
	virtual control_structure get_type() {
		return control_structure::none;
	}
	virtual state_stack* working_state() {
		return interpreter_state.get();
	}
	virtual std::string_view* working_input() {
		return &source;
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		interpreter_state = std::move(p);
	}
	virtual bool finish(environment& env) {
		return true;
	}
};
class replace_source_scope : public opaque_compiler_data {
public:
	std::string_view replaced_text;

	replace_source_scope(opaque_compiler_data* parent, std::string_view replaced_text) : opaque_compiler_data(parent), replaced_text(replaced_text) {
	}
	virtual control_structure get_type() {
		return control_structure::none;
	}
	virtual std::string_view* working_input() {
		return &replaced_text;
	}
	virtual bool finish(environment& env) {
		return true;
	}
};
class typecheck3_record_holder : public opaque_compiler_data {
public:
	ankerl::unordered_dense::map<uint64_t, typecheck_3_record> tr;

	typecheck3_record_holder(environment& env) : opaque_compiler_data(nullptr) {
	}

	virtual control_structure get_type() {
		return control_structure::none;
	}
	virtual ankerl::unordered_dense::map<uint64_t, typecheck_3_record>* typecheck_record() {
		return &tr;
	}
	virtual bool finish(environment& env) {
		return true;
	}
};


inline std::vector<int32_t> expand_stack_description(state_stack& initial_stack_state, std::span<const int32_t> desc, int32_t stack_consumed, int32_t rstack_consumed) {
	int32_t match_position = 0;
	// stack matching

	std::vector<int32_t> result;

	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		++consumed_stack_cells;
	}
	if(consumed_stack_cells < stack_consumed) {
		for(int32_t i = consumed_stack_cells; consumed_stack_cells < stack_consumed; ++i) {
			result.push_back(initial_stack_state.main_type_back(i));
		}
	}

	if(consumed_stack_cells < stack_consumed || (match_position < desc.size() && desc[match_position] == -1)) {
		result.push_back(-1);
	}
	++match_position; // skip -1

	if(consumed_stack_cells < stack_consumed) { // add additional touched inputs to the end
		for(int32_t i = stack_consumed - 1; i >= consumed_stack_cells; --i) {
			result.push_back(initial_stack_state.main_type_back(i));
		}
	}

	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		++output_stack_types;
	}

	if(rstack_consumed > 0 || (match_position < desc.size() && desc[match_position] == -1)) {
		result.push_back(-1);
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		auto match_size = next_encoded_stack_type(desc.subspan(match_position));
		result.insert(result.end(), desc.data() + match_position, desc.data() + match_position + match_size);
		match_position += match_size;
		++consumed_rstack_cells;
	}
	if(consumed_rstack_cells < rstack_consumed) {
		for(int32_t i = consumed_rstack_cells; consumed_rstack_cells < rstack_consumed; ++i) {
			result.push_back(initial_stack_state.return_type_back(i));
		}
	}

	if(consumed_rstack_cells < rstack_consumed || (match_position < desc.size() && desc[match_position] == -1)) {
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
	while(match_position < desc.size() && desc[match_position] != -1) {
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

class conditional_scope : public opaque_compiler_data {
public:
	std::unique_ptr<state_stack> initial_state;
	std::unique_ptr<state_stack> iworking_state;
	std::unique_ptr<state_stack> first_branch_state;
	LLVMValueRef branch_condition = nullptr;
	LLVMBasicBlockRef parent_block = nullptr;
	LLVMBasicBlockRef true_block = nullptr;
	bool interpreter_skipped_first_branch = false;
	bool interpreter_skipped_second_branch = false;
	bool typechecking_failed_on_first_branch = false;
	bool initial_typechecking_failed = false;
	size_t bytecode_first_branch_point = 0;
	size_t bytecode_second_branch_point = 0;

	conditional_scope(opaque_compiler_data* parent, environment& env, state_stack& entry_state) : opaque_compiler_data(parent) {	
		if(entry_state.main_size() == 0 || entry_state.main_type_back(0) != fif_bool) {
			env.report_error("attempted to start an if without a boolean value on the stack");
			env.mode = fif_mode::error;
		}
		if(env.mode == fif_mode::interpreting) {
			if(entry_state.main_data_back(0) != 0) {
				interpreter_skipped_second_branch = true;
			} else {
				interpreter_skipped_first_branch = true;
				env.mode = fif_mode::typechecking_lvl1_failed;
				entry_state.pop_main();
			}
		}
		if(env.mode == fif_mode::compiling_llvm) {
			branch_condition = entry_state.main_ex_back(0);
			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				parent_block = *pb;
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-first-branch");
				LLVMAppendExistingBasicBlock(env.compiler_stack.back().data->llvm_function(), *pb);
				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);
			}
		}
		if(env.mode == fif_mode::compiling_bytecode) {
			auto bcode = parent->bytecode_compilation_progress();

			fif_call imm = conditional_jump;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			bytecode_first_branch_point = bcode->size();
			bcode->push_back(0);
		}
		initial_typechecking_failed = typechecking_failed(env.mode);
		if(!typechecking_failed(env.mode))
			entry_state.pop_main();

		initial_state = entry_state.copy();
		initial_state->min_main_depth = entry_state.min_main_depth;
		initial_state->min_return_depth = entry_state.min_return_depth;

		iworking_state = entry_state.copy();
	}
	void commit_first_branch(environment& env) {
		if(first_branch_state) {
			env.report_error("attempted to compile multiple else conditions");
			env.mode = fif_mode::error;
			return;
		}

		if(interpreter_skipped_first_branch) {
			env.mode = fif_mode::interpreting;
		} else if(interpreter_skipped_second_branch) {
			env.mode = fif_mode::typechecking_lvl1_failed;
		} else { // not interpreted mode
			first_branch_state = std::move(iworking_state);
			iworking_state = initial_state->copy();

			if(env.mode == fif_mode::compiling_llvm) {
				if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
					true_block = *pb;
					*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "if-else-branch");
					LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);
				}
			}
			if(env.mode == fif_mode::compiling_bytecode) {
				auto bcode = parent->bytecode_compilation_progress();

				fif_call imm = unconditional_jump;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				bytecode_second_branch_point = bcode->size();
				bcode->push_back(0);
				(*bcode)[bytecode_first_branch_point] = int32_t(bcode->size() - (bytecode_first_branch_point - 2));
			}
			if(typechecking_failed(env.mode)) {
				typechecking_failed_on_first_branch = true;
				env.mode = reset_typechecking(env.mode);
			}
		}
	}
	virtual control_structure get_type() {
		return control_structure::str_if;
	}
	virtual state_stack* working_state() {
		return iworking_state.get();
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		iworking_state = std::move(p);
	}
	virtual bool finish(environment& env) {
		if(interpreter_skipped_second_branch) {
			env.mode = fif_mode::interpreting;
		}
		if(env.mode == fif_mode::interpreting) {
			parent->set_working_state(std::move(iworking_state));
			return true;
		}

		bool final_types_match = true;
		if(first_branch_state) {
			if(!initial_typechecking_failed && !typechecking_failed_on_first_branch && !typechecking_failed(env.mode)) {
				final_types_match = stack_types_match(*first_branch_state, *iworking_state);
			}
		} else {
			if(!typechecking_failed(env.mode))
				final_types_match = stack_types_match(*initial_state , *iworking_state);
		}

		if(!final_types_match) {
			if(typechecking_mode(env.mode)) {
				env.mode = fail_typechecking(env.mode);
				return true;
			} else {
				env.report_error("inconsistent stack types over a conditional branch");
				env.mode = fif_mode::error;
				return true;
			}
		}

		if(typechecking_mode(env.mode)) {
			if(typechecking_failed_on_first_branch && typechecking_failed(env.mode))
				return true; // failed to typecheck on both branches
			if(initial_typechecking_failed) {
				env.mode = fail_typechecking(env.mode);
				return true;
			}
		}

		if(env.mode == fif_mode::compiling_bytecode) {
			auto bcode = parent->bytecode_compilation_progress();
			if(!first_branch_state)
				(*bcode)[bytecode_first_branch_point] = int32_t(bcode->size() - (bytecode_first_branch_point - 2));
			else
				(*bcode)[bytecode_second_branch_point] = int32_t(bcode->size() - (bytecode_second_branch_point - 2));
		}

		if(env.mode == fif_mode::compiling_llvm) {
			auto pb = env.compiler_stack.back().data->llvm_block();
			auto current_block = *pb;

			if(first_branch_state) {
				LLVMPositionBuilderAtEnd(env.llvm_builder, parent_block);
				LLVMBuildCondBr(env.llvm_builder, branch_condition, true_block, current_block);
				auto in_fn = env.compiler_stack.back().data->llvm_function();

				auto post_if = LLVMCreateBasicBlockInContext(env.llvm_context, "then-branch");
				*pb = post_if;

				LLVMAppendExistingBasicBlock(in_fn, post_if);

				LLVMPositionBuilderAtEnd(env.llvm_builder, current_block);
				LLVMBuildBr(env.llvm_builder, post_if);
				LLVMPositionBuilderAtEnd(env.llvm_builder, true_block);
				LLVMBuildBr(env.llvm_builder, post_if);

				/*
				* build phi nodes
				*/

				LLVMPositionBuilderAtEnd(env.llvm_builder, post_if);

				for(auto i = iworking_state->main_size(); i-- > 0; ) {
					if(iworking_state->main_ex(i) != first_branch_state->main_ex(i)) {
						auto phi_ref = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->main_type(i)].llvm_type, "if-phi");
						LLVMValueRef inc_vals[2] = { iworking_state->main_ex(i), first_branch_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { current_block, true_block };
						LLVMAddIncoming(phi_ref, inc_vals, inc_blocks, 2);
						iworking_state->set_main_ex(i, phi_ref);
					} else {
						// leave working state as is
					}
				}
				for(auto i = iworking_state->return_size(); i-- > 0; ) {
					if(iworking_state->return_ex(i) != first_branch_state->return_ex(i)) {
						auto phi_ref = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->return_type(i)].llvm_type, "if-phi");
						LLVMValueRef inc_vals[2] = { iworking_state->return_ex(i), first_branch_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { current_block, true_block };
						LLVMAddIncoming(phi_ref, inc_vals, inc_blocks, 2);
						iworking_state->set_return_ex(i, phi_ref);
					} else {
						// leave working state as is
					}
				}
			} else {
				auto in_fn = env.compiler_stack.back().data->llvm_function();
				auto post_if = LLVMCreateBasicBlockInContext(env.llvm_context, "then-branch");
				*pb = post_if;

				LLVMAppendExistingBasicBlock(in_fn, post_if);

				LLVMPositionBuilderAtEnd(env.llvm_builder, parent_block);
				LLVMBuildCondBr(env.llvm_builder, branch_condition, current_block, post_if);

				LLVMPositionBuilderAtEnd(env.llvm_builder, current_block);
				LLVMBuildBr(env.llvm_builder, post_if);

				/*
				* build phi nodes
				*/

				LLVMPositionBuilderAtEnd(env.llvm_builder, post_if);

				for(auto i = iworking_state->main_size(); i-- > 0; ) {
					if(iworking_state->main_ex(i) != initial_state->main_ex(i)) {
						auto phi_ref = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->main_type(i)].llvm_type, "if-phi");
						LLVMValueRef inc_vals[2] = { iworking_state->main_ex(i), initial_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { current_block, parent_block };
						LLVMAddIncoming(phi_ref, inc_vals, inc_blocks, 2);
						iworking_state->set_main_ex(i, phi_ref);
					} else {
						// leave working state as is
					}
				}
				for(auto i = iworking_state->return_size(); i-- > 0; ) {
					if(iworking_state->return_ex(i) != initial_state->return_ex(i)) {
						auto phi_ref = LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->return_type(i)].llvm_type, "if-phi");
						LLVMValueRef inc_vals[2] = { iworking_state->return_ex(i), initial_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { current_block, parent_block };
						LLVMAddIncoming(phi_ref, inc_vals, inc_blocks, 2);
						iworking_state->set_return_ex(i, phi_ref);
					} else {
						// leave working state as is
					}
				}
			}
		}

		// branch reconciliation
		if(first_branch_state && !initial_typechecking_failed && !typechecking_failed_on_first_branch && typechecking_failed(env.mode)) {
			// case: second branch failed to typecheck, but first branch passed
			iworking_state = std::move(first_branch_state);
			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);

		} else if(first_branch_state && !initial_typechecking_failed && typechecking_failed_on_first_branch &&!typechecking_failed(env.mode)) {
			// case: first branch failed to typecheck, but second branch passed
			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);

		} else if(first_branch_state && !initial_typechecking_failed && !typechecking_failed_on_first_branch && !typechecking_failed(env.mode)) {
			// case both branches typechecked
			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, first_branch_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, first_branch_state->min_return_depth);
			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
		} else if(first_branch_state && !initial_typechecking_failed && typechecking_failed_on_first_branch && typechecking_failed(env.mode)) {
			// case both branches failed to typechecked
			// don't set working state
			return true;

		} else if(!first_branch_state && typechecking_failed(env.mode) && !initial_typechecking_failed) {
			// case: single branch failed to typecheck
			iworking_state = std::move(initial_state);

		} else if(!first_branch_state && !initial_typechecking_failed && !typechecking_failed(env.mode)) {
			// case: single branch typechecked
			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);

		} else if(initial_typechecking_failed) {
			// case: typechecking failed going into the conditional
			// dont set working state
			env.mode = fail_typechecking(env.mode);
			return true;

		}

		parent->set_working_state(std::move(iworking_state));
		env.mode = reset_typechecking(env.mode);
		return true;
	}
};

class while_loop_scope : public opaque_compiler_data {
public:
	std::unique_ptr<state_stack> initial_state;
	std::unique_ptr<state_stack> iworking_state;
	std::unique_ptr<state_stack> loop_entry_copy;

	std::string_view entry_source;
	std::string_view iworking_source;
	size_t loop_entry_point = 0;
	size_t end_of_loop_branch = 0;
	bool phi_pass = false;
	bool intepreter_skip_body = false;
	bool saw_conditional = false;
	bool initial_typechecking_failed = false;
	LLVMValueRef conditional_expr = nullptr;
	LLVMBasicBlockRef pre_block = nullptr;
	LLVMBasicBlockRef entry_block = nullptr;
	LLVMBasicBlockRef body_block = nullptr;
	std::vector<LLVMValueRef> phinodes;

	while_loop_scope(opaque_compiler_data* parent, environment& env, state_stack& entry_state) : opaque_compiler_data(parent) {
		initial_state = entry_state.copy();
		iworking_state = entry_state.copy();
		initial_state->min_main_depth = entry_state.min_main_depth;
		initial_state->min_return_depth = entry_state.min_return_depth;
		initial_typechecking_failed = typechecking_failed(env.mode);

		auto source = parent->working_input();
		if(source) {
			entry_source = *source;
			iworking_source = *source;
		}
		if(env.mode == fif_mode::compiling_bytecode) {
			auto bcode = parent->bytecode_compilation_progress();
			loop_entry_point = bcode->size();
		}
		if(env.mode == fif_mode::compiling_llvm) {
			phi_pass = true;
			env.mode = fif_mode::typechecking_lvl_2;
			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				pre_block = *pb;
			}
		}
	}

	virtual std::string_view* working_input() {
		return &iworking_source;
	}
	virtual control_structure get_type() {
		return control_structure::str_while_loop;
	}
	virtual state_stack* working_state() {
		return iworking_state.get();
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		iworking_state = std::move(p);
	}
	void end_condition(environment& env) {
		if(saw_conditional) {
			env.report_error("multiple conditions in while loop");
			env.mode = fif_mode::error;
			return;
		}
		saw_conditional = true;

		if(!typechecking_failed(env.mode)) {
			if(iworking_state->main_type_back(0) != fif_bool) {
				if(!typechecking_mode(env.mode)) {
					env.report_error("while loop conditional expression did not produce a boolean");
					env.mode = fif_mode::error;
					return;
				} else {
					env.mode = fail_typechecking(env.mode);
					return;
				}
			}

			if(env.mode == fif_mode::interpreting) {
				if(iworking_state->main_data_back(0) == 0) {
					intepreter_skip_body = true;
					env.mode = fif_mode::typechecking_lvl1_failed;
				}
				iworking_state->pop_main();
			} else if(env.mode == fif_mode::compiling_bytecode) {
				auto bcode = parent->bytecode_compilation_progress();
				fif_call imm = conditional_jump;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				end_of_loop_branch = bcode->size();
				bcode->push_back(0);
				iworking_state->pop_main();
			} else if(env.mode == fif_mode::compiling_llvm) {
				if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
					conditional_expr = iworking_state->main_ex_back(0);
					iworking_state->pop_main();

					*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "loop_body");
					body_block = *pb;
					LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);
				}
			} else if(typechecking_mode(env.mode)) {
				iworking_state->pop_main();
				if(!stack_types_match(*initial_state, *iworking_state)) {
					env.mode = fail_typechecking(env.mode);
				}
			}
		}
	}
	virtual bool finish(environment& env) {
		if(env.mode == fif_mode::typechecking_lvl1_failed && intepreter_skip_body) {
			env.mode = fif_mode::interpreting;
			
			if(auto source = parent->working_input();  source) 
				*source = iworking_source;
			
			parent->set_working_state(std::move(iworking_state));
			return true;
		}

		if(env.mode == fif_mode::interpreting) {
			if(!saw_conditional && iworking_state->main_type_back(0) == fif_bool) {
				if(iworking_state->main_data_back(0) == 0) {
					iworking_state->pop_main();
					parent->set_working_state(std::move(iworking_state));
					if(auto source = parent->working_input();  source)
						*source = iworking_source;
					return true;
				}
				iworking_state->pop_main();
			}
			saw_conditional = false;
			iworking_source = entry_source;
			return false;
		}

		if(env.mode == fif_mode::compiling_bytecode) {
			if(!saw_conditional) {
				if(iworking_state->main_type_back(0) == fif_bool) {
					iworking_state->pop_main();
				} else {
					env.report_error("while loop terminated with an appropriate conditional");
					env.mode = fif_mode::error;
					return true;
				}

				auto bcode = parent->bytecode_compilation_progress();
				fif_call imm = reverse_conditional_jump;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				end_of_loop_branch = bcode->size();
				bcode->push_back(int32_t(int64_t(loop_entry_point) - int64_t(bcode->size() - 2)));
			}

			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.report_error("while loop had a net stack effect");
				env.mode = fif_mode::error;
				return true;
			}

			if(saw_conditional) {
				auto bcode = parent->bytecode_compilation_progress();
				fif_call imm = unconditional_jump;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				bcode->push_back(int32_t(int64_t(loop_entry_point) - int64_t(bcode->size() - 2)));
				(*bcode)[end_of_loop_branch] = int32_t(bcode->size() - (end_of_loop_branch - 2));;
			}

			if(auto source = parent->working_input();  source)
				*source = iworking_source;

			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
			parent->set_working_state(std::move(iworking_state));

			return true;
		} else if(env.mode == fif_mode::compiling_llvm) {
			if(!saw_conditional) {
				if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
					conditional_expr = iworking_state->main_ex_back(0);
					iworking_state->pop_main();

					*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "loop_body");
					body_block = *pb;
					LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);
				}
			}

			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.report_error("while loop had a net stack effect");
				env.mode = fif_mode::error;
				return true;
			}

			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				LLVMBuildBr(env.llvm_builder, entry_block);

				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "post_loop");
				LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);

				LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);
				LLVMBuildCondBr(env.llvm_builder, conditional_expr, body_block, *pb);

				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				/*
				set phi inputs in entry block
				*/
				uint32_t node_index = 0;
				for(auto i = iworking_state->main_size(); i-- > iworking_state->min_main_depth; ) {
					if(phinodes[node_index] == iworking_state->main_ex(i)) { // phi node unchanged
						LLVMValueRef inc_vals[1] = { initial_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[1] = { pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 1);
					} else { 
						LLVMValueRef inc_vals[2] = { iworking_state->main_ex(i), initial_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { body_block, pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 2);
					}
					++node_index;
				}
				for(auto i = iworking_state->return_size(); i-- > iworking_state->min_return_depth; ) {
					if(phinodes[node_index] == iworking_state->return_ex(i)) { // phi node unchanged
						LLVMValueRef inc_vals[1] = { initial_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[1] = { pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 1);
					} else {
						LLVMValueRef inc_vals[2] = { iworking_state->return_ex(i), initial_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { body_block, pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 2);
					}
					++node_index;
				}
			}

			if(auto source = parent->working_input();  source)
				*source = iworking_source;

			loop_entry_copy->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			loop_entry_copy->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
			parent->set_working_state(std::move(loop_entry_copy));

			return true;
		} else if(env.mode == fif_mode::typechecking_lvl_2 && phi_pass == true) {
			phi_pass = false;
			if(!saw_conditional) {
				iworking_state->pop_main();
			}


			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "while_entry");
				entry_block = *pb;
				LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);

				LLVMBuildBr(env.llvm_builder, entry_block);
				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				/*
				build entry phi_nodes
				*/
				for(auto i = iworking_state->main_size(); i-- > iworking_state->min_main_depth; ) {
					phinodes.push_back(LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->main_type(i)].llvm_type, ""));
					iworking_state->set_main_ex(i, phinodes.back());
				}
				for(auto i = iworking_state->return_size(); i-- > iworking_state->min_return_depth; ) {
					phinodes.push_back(LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->return_type(i)].llvm_type, ""));
					iworking_state->set_return_ex(i, phinodes.back());
				}
			}

			loop_entry_copy = iworking_state->copy();
			saw_conditional = false;
			env.mode = fif_mode::compiling_llvm;
			iworking_source = entry_source;
			return false;
		} else if(typechecking_mode(env.mode)) {
			if(!saw_conditional) {
				if(iworking_state->main_type_back(0) != fif_bool) {
					env.mode = reset_typechecking(env.mode);
					if(initial_typechecking_failed || env.mode != fif_mode::typechecking_lvl_1)
						env.mode = fail_typechecking(env.mode);

					if(auto source = parent->working_input();  source)
						*source = iworking_source;
					return true;
				}
				iworking_state->pop_main();
			}
			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.mode = reset_typechecking(env.mode);
				if(initial_typechecking_failed || env.mode != fif_mode::typechecking_lvl_1)
					env.mode = fail_typechecking(env.mode);

				if(auto source = parent->working_input();  source)
					*source = iworking_source;
				return true;
			}
			if(!typechecking_failed(env.mode)) {
				iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
				iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
				parent->set_working_state(std::move(iworking_state));
			} else if(env.mode == fif_mode::typechecking_lvl1_failed) {
				env.mode = reset_typechecking(env.mode);
			}
			if(auto source = parent->working_input();  source)
				*source = iworking_source;
			return true;
		}
		return true;
	}
};

class do_loop_scope : public opaque_compiler_data {
public:
	std::unique_ptr<state_stack> initial_state;
	std::unique_ptr<state_stack> iworking_state;

	std::string_view entry_source;
	std::string_view iworking_source;
	size_t loop_entry_point = 0;
	size_t end_of_loop_branch = 0;
	bool phi_pass = false;
	bool initial_typechecking_failed = false;
	
	LLVMBasicBlockRef pre_block = nullptr;
	LLVMBasicBlockRef body_block = nullptr;
	std::vector<LLVMValueRef> phinodes;

	do_loop_scope(opaque_compiler_data* parent, environment& env, state_stack& entry_state) : opaque_compiler_data(parent) {
		initial_state = entry_state.copy();
		iworking_state = entry_state.copy();
		initial_state->min_main_depth = entry_state.min_main_depth;
		initial_state->min_return_depth = entry_state.min_return_depth;
		initial_typechecking_failed = typechecking_failed(env.mode);

		auto source = parent->working_input();
		if(source) {
			entry_source = *source;
			iworking_source = *source;
		}
		if(env.mode == fif_mode::compiling_bytecode) {
			auto bcode = parent->bytecode_compilation_progress();
			loop_entry_point = bcode->size();
		}
		if(env.mode == fif_mode::compiling_llvm) {
			phi_pass = true;
			env.mode = fif_mode::typechecking_lvl_2;
			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				pre_block = *pb;
			}
		}
	}

	virtual std::string_view* working_input() {
		return &iworking_source;
	}
	virtual control_structure get_type() {
		return control_structure::str_do_loop;
	}
	virtual state_stack* working_state() {
		return iworking_state.get();
	}
	virtual void set_working_state(std::unique_ptr<state_stack> p) {
		iworking_state = std::move(p);
	}
	void at_until(environment& env) {
		// don't actually need to do anything here ...
	}
	virtual bool finish(environment& env) {
		if(env.mode == fif_mode::interpreting) {
			if(iworking_state->main_type_back(0) == fif_bool) {
				if(iworking_state->main_data_back(0) != 0) {
					iworking_state->pop_main();
					parent->set_working_state(std::move(iworking_state));
					if(auto source = parent->working_input();  source)
						*source = iworking_source;
					return true;
				}
				iworking_state->pop_main();
			}
			iworking_source = entry_source;
			return false;
		}

		if(env.mode == fif_mode::compiling_bytecode) {
			
			if(iworking_state->main_type_back(0) == fif_bool) {
				iworking_state->pop_main();
			} else {
				env.report_error("while loop terminated with an appropriate conditional");
				env.mode = fif_mode::error;
				return true;
			}

			auto bcode = parent->bytecode_compilation_progress();
			fif_call imm = conditional_jump;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			bcode->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			bcode->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			end_of_loop_branch = bcode->size();
			bcode->push_back(int32_t(int64_t(loop_entry_point) - int64_t(bcode->size() - 2)));
			
			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.report_error("do loop had a net stack effect");
				env.mode = fif_mode::error;
				return true;
			}

			if(auto source = parent->working_input();  source)
				*source = iworking_source;

			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
			parent->set_working_state(std::move(iworking_state));

			return true;
		} else if(env.mode == fif_mode::compiling_llvm) {
			
			auto conditional_expr = iworking_state->main_ex_back(0);
			iworking_state->pop_main();


			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.report_error("while loop had a net stack effect");
				env.mode = fif_mode::error;
				return true;
			}

			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				auto in_block = *pb;
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "post_loop");
				LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);

				LLVMPositionBuilderAtEnd(env.llvm_builder, in_block);
				LLVMBuildCondBr(env.llvm_builder, conditional_expr, *pb, body_block);

				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				/*
				set phi inputs in entry block
				*/
				uint32_t node_index = 0;
				for(auto i = iworking_state->main_size(); i-- > iworking_state->min_main_depth; ) {
					if(phinodes[node_index] == iworking_state->main_ex(i)) { // phi node unchanged
						LLVMValueRef inc_vals[1] = { initial_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[1] = { pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 1);
					} else {
						LLVMValueRef inc_vals[2] = { iworking_state->main_ex(i), initial_state->main_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { in_block, pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 2);
					}
					++node_index;
				}
				for(auto i = iworking_state->return_size(); i-- > iworking_state->min_return_depth; ) {
					if(phinodes[node_index] == iworking_state->return_ex(i)) { // phi node unchanged
						LLVMValueRef inc_vals[1] = { initial_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[1] = { pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 1);
					} else {
						LLVMValueRef inc_vals[2] = { iworking_state->return_ex(i), initial_state->return_ex(i) };
						LLVMBasicBlockRef inc_blocks[2] = { in_block, pre_block };
						LLVMAddIncoming(phinodes[node_index], inc_vals, inc_blocks, 2);
					}
					++node_index;
				}
			}

			if(auto source = parent->working_input();  source)
				*source = iworking_source;

			iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
			iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
			parent->set_working_state(std::move(iworking_state));

			return true;
		} else if(env.mode == fif_mode::typechecking_lvl_2 && phi_pass == true) {
			phi_pass = false;

			if(iworking_state->main_type_back(0) != fif_bool) {
				env.mode = fif_mode::error;
				env.report_error("do loop did not finish with a boolean value");
				return true;
			}
			iworking_state->pop_main();

			if(auto pb = env.compiler_stack.back().data->llvm_block(); pb) {
				*pb = LLVMCreateBasicBlockInContext(env.llvm_context, "do_body");
				body_block = *pb;
				LLVMAppendExistingBasicBlock(parent->llvm_function(), *pb);

				LLVMBuildBr(env.llvm_builder, body_block);
				LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);

				/*
				build entry phi_nodes
				*/
				for(auto i = iworking_state->main_size(); i-- > iworking_state->min_main_depth; ) {
					phinodes.push_back(LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->main_type(i)].llvm_type, ""));
					iworking_state->set_main_ex(i, phinodes.back());
				}
				for(auto i = iworking_state->return_size(); i-- > iworking_state->min_return_depth; ) {
					phinodes.push_back(LLVMBuildPhi(env.llvm_builder, env.dict.type_array[iworking_state->return_type(i)].llvm_type, ""));
					iworking_state->set_return_ex(i, phinodes.back());
				}
			}

			env.mode = fif_mode::compiling_llvm;
			iworking_source = entry_source;
			return false;
		} else if(typechecking_mode(env.mode)) {
			if(iworking_state->main_type_back(0) != fif_bool) {
				env.mode = reset_typechecking(env.mode);
				if(initial_typechecking_failed || env.mode != fif_mode::typechecking_lvl_1)
					env.mode = fail_typechecking(env.mode);
				
				if(auto source = parent->working_input();  source)
					*source = iworking_source;
				return true;
			}
			iworking_state->pop_main();

			bool final_types_match = stack_types_match(*initial_state, *iworking_state);
			if(!final_types_match) {
				env.mode = reset_typechecking(env.mode);
				if(initial_typechecking_failed || env.mode != fif_mode::typechecking_lvl_1)
					env.mode = fail_typechecking(env.mode);

				parent->set_working_state(std::move(initial_state));
				if(auto source = parent->working_input();  source)
					*source = iworking_source;
				return true;
			}

			if(!typechecking_failed(env.mode)) {
				iworking_state->min_main_depth = std::min(iworking_state->min_main_depth, initial_state->min_main_depth);
				iworking_state->min_return_depth = std::min(iworking_state->min_return_depth, initial_state->min_return_depth);
				parent->set_working_state(std::move(iworking_state));
			} else if(!initial_typechecking_failed && env.mode == fif_mode::typechecking_lvl_1) {
				env.mode = reset_typechecking(env.mode);
			}

			if(auto source = parent->working_input();  source)
				*source = iworking_source;
			return true;
		}

		return true;
	}
};

inline constexpr size_t inlining_cells_limit = 512;

struct parse_result {
	std::string_view content;
	bool is_string = false;
};

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
bool codepoint_is_space(uint32_t c) noexcept {
	return (c == 0x3000 || c == 0x205F || c == 0x202F || c == 0x2029 || c == 0x2028 || c == 0x00A0
		|| c == 0x0085 || c <= 0x0020 || (0x2000 <= c && c <= 0x200A));
}
bool codepoint_is_line_break(uint32_t c) noexcept {
	return  c == 0x2029 || c == 0x2028 || c == uint32_t('\n') || c == uint32_t('\r');
}

struct string_start {
	int32_t eq_match = 0;
	bool starts_string = false;
};

string_start match_string_start(std::string_view source) {
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
bool match_string_end(std::string_view source, int32_t eq_count_in) {
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

parse_result read_token(std::string_view& source, environment& env) {
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

inline void execute_fif_word(parse_result word, environment& env);


inline void run_to_scope_end(environment& env) { // execute/compile source code until current function is completed
	parse_result word;
	auto* current_end_scope = !env.compiler_stack.empty() ? env.compiler_stack.back().data.get() : nullptr;

	while(!env.compiler_stack.empty()) {
		do {
			auto* source = !env.compiler_stack.empty() ? env.compiler_stack.back().data->working_input() : nullptr;

			if(source)
				word = read_token(*source, env);
			else
				word.content = std::string_view{ };

			if(word.content.length() > 0 && env.mode != fif_mode::error) {
				execute_fif_word(word, env);
			}
		} while(word.content.length() > 0 && !env.compiler_stack.empty());

		if(env.compiler_stack.empty())
			return;

		if(env.compiler_stack.back().data.get() == current_end_scope)
			return;

		if(env.compiler_stack.back().data->finish(env)) {
			env.compiler_stack.pop_back();
		}
	}
}

inline void run_to_function_end(environment& env) { // execute/compile source code until current function is completed
	run_to_scope_end(env);
	if(!env.compiler_stack.empty() && env.compiler_stack.back().data->finish(env)) {
		env.compiler_stack.pop_back();
	}
}



inline word_match_result get_basic_type_match(int32_t word_index, state_stack& current_type_state, environment& env) {
	int32_t w = word_index;

	auto match = match_word(env.dict.word_array[w], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);

	if(!match.matched) {
		if(typechecking_failed(env.mode)) {
			// ignore match failure and pass on through
			return word_match_result{ false, 0, 0, 0 };
		} else if(env.mode == fif_mode::typechecking_lvl_1 || env.mode == fif_mode::typechecking_lvl_2) {
			if(env.dict.word_array[w].being_typechecked) { // already being typechecked -- mark as un checkable recursive branch
				env.mode = fail_typechecking(env.mode);
			} else if(env.dict.word_array[w].source.length() > 0) { // try to typecheck word as level 1
				env.dict.word_array[w].being_typechecked = true;

				switch_compiler_stack_mode(env, fif_mode::typechecking_lvl_1);

				auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, current_type_state, std::string_view(env.dict.word_array[w].source), w, -1);
				env.compiler_stack.emplace_back(std::move(fnscope));

				run_to_function_end(env);

				env.dict.word_array[w].being_typechecked = false;
				match = match_word(env.dict.word_array[w], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);

				restore_compiler_stack_mode(env);

				if(match.matched) {
					std::get<interpreted_word_instance>(env.dict.all_instances[match.word_index]).typechecking_level = 1;
				} else {
					env.mode = fail_typechecking(env.mode);
					return word_match_result{ false, 0, 0, 0 };
				}
			}
		} else if(env.dict.word_array[w].source.length() > 0) { // either compiling or interpreting or level 3 typecheck-- switch to typechecking to get a type definition
			switch_compiler_stack_mode(env, fif_mode::typechecking_lvl_1);
			env.dict.word_array[w].being_typechecked = true;

			auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, current_type_state, std::string_view(env.dict.word_array[w].source), w, -1);
			env.compiler_stack.emplace_back(std::move(fnscope));

			run_to_function_end(env);

			env.dict.word_array[w].being_typechecked = false;

			restore_compiler_stack_mode(env);

			match = match_word(env.dict.word_array[w], current_type_state, env.dict.all_instances, env.dict.all_stack_types, env);
			if(!match.matched) {
				env.report_error("typechecking failure for " + word_name_from_id(w, env));
				env.mode = fif_mode::error;
				return word_match_result{ false, 0, 0, 0 };
			}
		}
	}

	return match;
}

bool fully_typecheck_word(int32_t w, int32_t word_index, interpreted_word_instance& wi, state_stack& current_type_state, environment& env) {
	if(wi.typechecking_level == 1) {
		// perform level 2 typechecking

		switch_compiler_stack_mode(env, fif_mode::typechecking_lvl_2);

		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, current_type_state, std::string_view(env.dict.word_array[w].source), w, word_index);
		env.compiler_stack.emplace_back(std::move(fnscope));

		run_to_function_end(env);

		bool failed = typechecking_failed(env.mode);

		restore_compiler_stack_mode(env);

		if(failed) {
			env.report_error("level 2 typecheck failed");
			env.mode = fif_mode::error;
			return false;
		}

		wi.typechecking_level = 2;
	}

	if(wi.typechecking_level == 2) {
		// perform level 3 typechecking

		env.compiler_stack.emplace_back(std::make_unique< typecheck3_record_holder>(env));
		auto record_holder = static_cast<typecheck3_record_holder*>(env.compiler_stack.back().data.get());

		switch_compiler_stack_mode(env, fif_mode::typechecking_lvl_3);
		auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, current_type_state, std::string_view(env.dict.word_array[w].source), w, word_index);

		env.compiler_stack.emplace_back(std::move(fnscope));

		run_to_function_end(env);

		bool failed = typechecking_failed(env.mode);

		restore_compiler_stack_mode(env);

		if(failed) {
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

inline bool compile_word(int32_t w, int32_t word_index, state_stack& state, fif_mode compile_mode, environment& env) {
	if(!std::holds_alternative<interpreted_word_instance>(env.dict.all_instances[word_index]))
		return true;

	interpreted_word_instance& wi = std::get<interpreted_word_instance>(env.dict.all_instances[word_index]);
	if(!fully_typecheck_word(w, word_index, wi, state, env)) {
		return false;
	}

	switch_compiler_stack_mode(env, compile_mode);

	if(env.mode != fif_mode::compiling_llvm && wi.compiled_offset == -1) {
		// typed but uncompiled word
		if(!env.dict.word_array[w].being_compiled) {
			auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, state, std::string_view(env.dict.word_array[w].source), w, word_index);
			env.compiler_stack.emplace_back(std::move(fnscope));
			run_to_function_end(env);
		}
	}

	// case: reached an uncompiled llvm definition
	if(env.mode == fif_mode::compiling_llvm && !wi.llvm_compilation_finished) {
		// typed but uncompiled word
		if(!env.dict.word_array[w].being_compiled) {
			auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, state, std::string_view(env.dict.word_array[w].source), w, word_index);
			env.compiler_stack.emplace_back(std::move(fnscope));
			run_to_function_end(env);

			if(!env.compiler_stack.empty()) {
				if(auto pb = env.compiler_stack.back().data->llvm_block(); pb)
					LLVMPositionBuilderAtEnd(env.llvm_builder, *pb);
			}
		}
	}

	restore_compiler_stack_mode(env);
	return true;
}

inline void execute_fif_word(parse_result word, environment& env) {
	auto ws = env.compiler_stack.back().data->working_state();
	if(!ws)
		std::abort();

	// TODO: string constant case
	if(word.content.length() > 0 && word.content[0] == '@') {
		do_construct_type(*ws, resolve_type(word.content.substr(1), env), &env);
	} else if(is_integer(word.content.data(), word.content.data() + word.content.length())) {
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
				auto source_layer = std::make_unique<replace_source_scope>(env.compiler_stack.back().data.get(), env.dict.word_array[w].source);
				env.compiler_stack.emplace_back(std::move(source_layer));
				switch_compiler_stack_mode(env, fif_mode::interpreting);

				run_to_scope_end(env);

				restore_compiler_stack_mode(env);
				env.compiler_stack.pop_back(); // remove source replacement

				return;
			}
		}

		auto match = get_basic_type_match(w, *ws, env);

		if(!match.matched) {
			if(typechecking_failed(env.mode)) {
				return;
			} else { // critical failure
				env.report_error("could not match word to stack types");
				env.mode = fif_mode::error;
				return;
			}
		}

		word_types& wi = env.dict.all_instances[match.word_index];

		// IMMEDIATE words (source not available)
		if(env.dict.word_array[w].immediate) {
			if(std::holds_alternative<interpreted_word_instance>(wi)) {
				switch_compiler_stack_mode(env, fif_mode::interpreting);
				execute_fif_word(std::get<interpreted_word_instance>(wi), *ws, env);
				restore_compiler_stack_mode(env);
			} else if(std::holds_alternative<compiled_word_instance>(wi)) {
				execute_fif_word(std::get<compiled_word_instance>(wi), *ws, env);
			}
			return;
		}

		//
		// level 1 typechecking -- should be able to trace at least one path through each word using only known words
		//
		// level 2 typechecking -- should be able to trace through every branch to produce an accurate stack picture for all known words present in the definition
		//

		if(env.mode == fif_mode::typechecking_lvl_1 || env.mode == fif_mode::typechecking_lvl1_failed
			|| env.mode == fif_mode::typechecking_lvl_2 || env.mode == fif_mode::typechecking_lvl2_failed) {

			if(std::holds_alternative<interpreted_word_instance>(wi)) {
				if(!typechecking_failed(env.mode)) {
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(wi).stack_types_start, size_t(std::get<interpreted_word_instance>(wi).stack_types_count)), *ws, env);
				}
			} else if(std::holds_alternative<compiled_word_instance>(wi)) {
				// no special logic, compiled words assumed to always typecheck
				if(!typechecking_failed(env.mode)) {
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(wi).stack_types_start, size_t(std::get<compiled_word_instance>(wi).stack_types_count)), *ws, env);
				}
			}
			return;
		}

		//
		// level 3 typechecking -- recursively determine the minimum stack position of all dependencies
		//

		if(env.mode == fif_mode::typechecking_lvl_3 || env.mode == fif_mode::typechecking_lvl3_failed) {
			if(std::holds_alternative<interpreted_word_instance>(wi)) {
				if(!typechecking_failed(env.mode)) {
					// add typecheck info
					if(std::get<interpreted_word_instance>(wi).typechecking_level < 3) {
						// this word also hasn't been compiled yet

						auto rword = env.compiler_stack.back().data->word_id();
						auto rinst = env.compiler_stack.back().data->instance_id();
						auto& dep = *(env.compiler_stack.back().data->typecheck_record());
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

							if(std::get<interpreted_word_instance>(wi).typechecking_level < 2) {
								switch_compiler_stack_mode(env, fif_mode::typechecking_lvl_2);

								auto fnscope = std::make_unique<function_scope>(env.compiler_stack.back().data.get(), env, *ws, std::string_view(env.dict.word_array[w].source), w, match.word_index);
								env.compiler_stack.emplace_back(std::move(fnscope));

								run_to_function_end(env);
								bool failed = typechecking_failed(env.mode);
								restore_compiler_stack_mode(env);

								if(failed) {
									env.report_error("level 2 typecheck failed");
									env.mode = fif_mode::error;
									return;
								}

								std::get<interpreted_word_instance>(wi).typechecking_level = 2;
							}

							env.mode = fif_mode::typechecking_lvl_3;
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

					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(wi).stack_types_start, size_t(std::get<interpreted_word_instance>(wi).stack_types_count)), *ws, env);
				}
			} else if(std::holds_alternative<compiled_word_instance>(wi)) {
				if(!typechecking_failed(env.mode)) {
					// no special logic, compiled words assumed to always typecheck
					apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(wi).stack_types_start, size_t(std::get<compiled_word_instance>(wi).stack_types_count)), *ws, env);
				}
			}

			return;
		}

		if(env.mode == fif_mode::interpreting || env.mode == fif_mode::compiling_llvm || env.mode == fif_mode::compiling_bytecode) {
			if(!compile_word(w, match.word_index, *ws, env.mode != fif_mode::compiling_llvm ? fif_mode::compiling_bytecode : fif_mode::compiling_llvm, env)) {
				env.report_error("failed to compile word");
				env.mode = fif_mode::error;
				return;
			}

			if(std::holds_alternative<interpreted_word_instance>(wi)) {
				if(env.mode == fif_mode::interpreting) {
					execute_fif_word(std::get<interpreted_word_instance>(wi), *ws, env);
				} else if(env.mode == fif_mode::compiling_llvm) {
					std::span<int32_t const> desc{ env.dict.all_stack_types.data() + std::get<interpreted_word_instance>(wi).stack_types_start, size_t(std::get<interpreted_word_instance>(wi).stack_types_count) };
					llvm_make_function_call(env, std::get<interpreted_word_instance>(wi).llvm_function, desc);
				} else if(env.mode == fif_mode::compiling_bytecode) {
					auto precompiled = std::get<interpreted_word_instance>(wi).compiled_offset;
					auto precompiled_sz = std::get<interpreted_word_instance>(wi).compiled_size;
					auto cbytes = env.compiler_stack.back().data->bytecode_compilation_progress();
					if(cbytes) {
						if(env.dict.word_array[w].being_compiled) {
							fif_call imm = call_function_indirect;
							uint64_t imm_bytes = 0;
							memcpy(&imm_bytes, &imm, 8);
							cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
							cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
							cbytes->push_back(match.word_index);
						} else if(cbytes->size() < inlining_cells_limit) {
							cbytes->insert(cbytes->end(), env.dict.all_compiled.data() + precompiled, env.dict.all_compiled.data() + precompiled + precompiled_sz);
						} else {
							fif_call imm = call_function;
							uint64_t imm_bytes = 0;
							memcpy(&imm_bytes, &imm, 8);
							cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
							cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
							cbytes->push_back(std::get<interpreted_word_instance>(wi).compiled_offset);
						}
						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(wi).stack_types_start, size_t(std::get<compiled_word_instance>(wi).stack_types_count)), *ws, env);
					}
				}
			} else if(std::holds_alternative<compiled_word_instance>(wi)) {
				if(env.mode == fif_mode::compiling_bytecode) {
					auto cbytes = env.compiler_stack.back().data->bytecode_compilation_progress();
					if(cbytes) {
						fif_call imm = std::get<compiled_word_instance>(wi).implementation;
						uint64_t imm_bytes = 0;
						memcpy(&imm_bytes, &imm, 8);
						cbytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
						cbytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));

						apply_stack_description(std::span<int32_t const>(env.dict.all_stack_types.data() + std::get<compiled_word_instance>(wi).stack_types_start, size_t(std::get<compiled_word_instance>(wi).stack_types_count)), *ws, env);
					}
				} else {
					std::get<compiled_word_instance>(wi).implementation(*ws, nullptr, &env);
				}
			}
		}
	} else {
		env.report_error(std::string("attempted to execute an unknown word: ") + std::string(word.content));
		env.mode = fif_mode::error;
	}
}

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

	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env, ""));
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().data.get());
	switch_compiler_stack_mode(env, fif_mode::interpreting);

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
	ts.min_main_depth = param_stack.size();
	ts.min_return_depth = param_stack.size();

	auto match = get_basic_type_match(w, ts, env);
	if(!match.matched) {
		env.report_error("failed to export function (typematch failed)");
		return nullptr;
	}

	if(!compile_word(w, match.word_index, ts, fif_mode::compiling_llvm, env)) {
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

	LLVMSetFunctionCallConv(compiled_fn, LLVMCallConv::LLVMWin64CallConv);
	LLVMSetLinkage(compiled_fn, LLVMLinkage::LLVMLinkOnceAnyLinkage);
	LLVMSetVisibility(compiled_fn, LLVMVisibility::LLVMDefaultVisibility);

	auto entry_block = LLVMAppendBasicBlockInContext(env.llvm_context, compiled_fn, "fn_entry_point");
	LLVMPositionBuilderAtEnd(env.llvm_builder, entry_block);


	// add body
	std::vector<LLVMValueRef> params;

	int32_t match_position = 0;
	// stack matching

	int32_t consumed_stack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		params.push_back(LLVMGetParam(compiled_fn, uint32_t(consumed_stack_cells)));
		++match_position;
		++consumed_stack_cells;
	}

	++match_position; // skip -1

	// output stack
	int32_t first_output_stack = match_position;
	int32_t output_stack_types = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		//returns_group.push_back(env.dict.type_array[desc[match_position]].llvm_type.llvm_type);
		++match_position;
		++output_stack_types;
	}
	++match_position; // skip -1

	// return stack matching
	int32_t consumed_rstack_cells = 0;
	while(match_position < desc.size() && desc[match_position] != -1) {
		params.push_back(LLVMGetParam(compiled_fn, uint32_t(consumed_stack_cells + consumed_rstack_cells)));
		++match_position;
		++consumed_rstack_cells;
	}

	auto retvalue = LLVMBuildCall2(env.llvm_builder, llvm_function_type_from_desc(env, desc), wi.llvm_function, params.data(), uint32_t(params.size()), "");
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

	return compiled_fn;
}

inline void run_fif_interpreter(environment& env, std::string on_text) {
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env, std::string_view(on_text)));
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().data.get());

	switch_compiler_stack_mode(env, fif_mode::interpreting);
	mode_switch_scope* m = static_cast<mode_switch_scope*>(env.compiler_stack.back().data.get());
	m->interpreted_link = o;

	run_to_scope_end(env);
	restore_compiler_stack_mode(env);
	env.compiler_stack.pop_back();
}

inline void run_fif_interpreter(environment& env, std::string on_text, interpreter_stack& s) {
	env.compiler_stack.emplace_back(std::make_unique<outer_interpreter>(env, std::string_view(on_text)));
	outer_interpreter* o = static_cast<outer_interpreter*>(env.compiler_stack.back().data.get());
	static_cast<interpreter_stack*>(o->interpreter_state.get())->move_into(std::move(s));
	
	switch_compiler_stack_mode(env, fif_mode::interpreting);
	mode_switch_scope* m = static_cast<mode_switch_scope*>(env.compiler_stack.back().data.get());
	m->interpreted_link = o;

	run_to_scope_end(env);
	restore_compiler_stack_mode(env);

	s.move_into(std::move(*(o->interpreter_state)));
	env.compiler_stack.pop_back();
}

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
		std::cout << msg << std::endl;
		LLVMDisposeErrorMessage(msg);
		std::abort();
	}
	return E;
}

inline LLVMErrorRef perform_transform(void* Ctx, LLVMOrcThreadSafeModuleRef* the_module, LLVMOrcMaterializationResponsibilityRef MR) {
	return LLVMOrcThreadSafeModuleWithModuleDo(*the_module, module_transform, Ctx);
}

void perform_jit(environment& e) {
	add_exportable_functions_to_globals(e);

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
		std::cout << "failed to get main dylib" << std::endl;
		std::abort();
	}

	auto error = LLVMOrcLLJITAddLLVMIRModule(e.llvm_jit, main_dyn_lib, orc_mod);
	if(error) {
		auto msg = LLVMGetErrorMessage(error);
		e.report_error(msg);
		LLVMDisposeErrorMessage(msg);
		return;
	}
}

inline int32_t* colon_definition(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to compile a definition inside a definition");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}
	auto source = e->compiler_stack.back().data->working_input();
	if(!source) {
		e->report_error("attempted to compile a definition without a source");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	auto name_token = fif::read_token(*source, *e);

	auto string_start = source->data();
	while(source->length() > 0) {
		auto t = fif::read_token(*source, *e);
		if(t.is_string == false && t.content == ";") { // end of definition
			auto string_end = t.content.data();

			auto nstr = std::string(name_token.content);
			if(e->dict.words.find(nstr) != e->dict.words.end()) {
				e->report_error("illegal word redefinition");
				e->mode = fif_mode::error;
				return p + 2;
			}

			e->dict.words.insert_or_assign(nstr, int32_t(e->dict.word_array.size()));
			e->dict.word_array.emplace_back();
			e->dict.word_array.back().source = std::string(string_start, string_end);
			
			return p + 2;
		}
	}

	e->report_error("reached the end of a definition source without a ; terminator");
	e->mode = fif::fif_mode::error;
	return p + 2;
}
inline int32_t* i32_add(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto add_result = LLVMBuildAdd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_i32, 0, add_result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_i32, a + b, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_i32, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* f32_add(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto add_result = LLVMBuildAdd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &a, 4);
		memcpy(&fb, &b, 4);
		fa = fa + fb;
		memcpy(&a, &fa, 4);
		s.push_back_main(fif::fif_f32, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, 0, nullptr);
	}
	return p + 2;
}

inline int32_t* dup(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type = s.main_type_back(0);
		auto expr = s.main_ex_back(0);
		s.push_back_main(type, 0, expr);
		if(e->dict.type_array[type].do_dup) {
			e->dict.type_array[type].do_dup(s, p, e);
		}
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
		if(e->dict.type_array[type].do_dup) {
			e->dict.type_array[type].do_dup(s, p, e);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type = s.main_type_back(0);
		s.push_back_main(type, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* copy(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type = s.main_type_back(0);
		auto expr = s.main_ex_back(0);
		s.push_back_main(type, 0, expr);
		if(e->dict.type_array[type].do_copy) {
			e->dict.type_array[type].do_copy(s, p, e);
		}
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
		if(e->dict.type_array[type].do_copy) {
			e->dict.type_array[type].do_copy(s, p, e);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type = s.main_type_back(0);
		s.push_back_main(type, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* drop(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type = s.main_type_back(0);
		if(e->dict.type_array[type].do_destructor) {
			e->dict.type_array[type].do_destructor(s, p, e);
		}
		s.pop_main();
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type = s.main_type_back(0);
		if(e->dict.type_array[type].do_destructor) {
			e->dict.type_array[type].do_destructor(s, p, e);
		}
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
	}
	return p + 2;
}
inline int32_t* fif_swap(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type_a = s.main_type_back(0);
		auto type_b = s.main_type_back(1);
		auto expr_a = s.main_ex_back(0);
		auto expr_b = s.main_ex_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(type_a, 0, expr_a);
		s.push_back_main(type_b, 0, expr_b);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type_a = s.main_type_back(0);
		auto type_b = s.main_type_back(1);
		auto dat_a = s.main_data_back(0);
		auto dat_b = s.main_data_back(1);
		if(e->dict.type_array[type_a].refcounted_type)
			fif::increment_refcounted(dat_a);
		if(e->dict.type_array[type_b].refcounted_type)
			fif::increment_refcounted(dat_b);
		s.pop_main();
		s.pop_main();
		s.push_back_main(type_a, dat_a, nullptr);
		s.push_back_main(type_b, dat_b, nullptr);
		if(e->dict.type_array[type_a].refcounted_type)
			fif::release_refcounted(dat_a);
		if(e->dict.type_array[type_b].refcounted_type)
			fif::release_refcounted(dat_b);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type_a = s.main_type_back(0);
		auto type_b = s.main_type_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(type_a, 0, nullptr);
		s.push_back_main(type_b, 0, nullptr);
	}
	return p + 2;
}


inline int32_t* fif_if(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::conditional_scope>(e->compiler_stack.back().data.get(), *e, s));
	return p + 2;
}
inline int32_t* fif_else(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_if) {
		e->report_error("invalid use of else");
		e->mode = fif::fif_mode::error;
	} else {
		fif::conditional_scope* c = static_cast<fif::conditional_scope*>(e->compiler_stack.back().data.get());
		c->commit_first_branch(*e);
	}
	return p + 2;
}
inline int32_t* fif_then(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_if) {
		e->report_error("invalid use of then/end-if");
		e->mode = fif::fif_mode::error;
	} else {
		if(e->compiler_stack.back().data->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}

inline int32_t* fif_while(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::while_loop_scope>(e->compiler_stack.back().data.get(), *e, s));
	return p + 2;
}
inline int32_t* fif_loop(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_while_loop) {
		e->report_error("invalid use of loop");
		e->mode = fif::fif_mode::error;
	} else {
		fif::while_loop_scope* c = static_cast<fif::while_loop_scope*>(e->compiler_stack.back().data.get());
		c->end_condition(*e);
	}
	return p + 2;
}
inline int32_t* fif_end_while(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_while_loop) {
		e->report_error("invalid use of end-while");
		e->mode = fif::fif_mode::error;
	} else {
		if(e->compiler_stack.back().data->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}
inline int32_t* fif_do(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::do_loop_scope>(e->compiler_stack.back().data.get(), *e, s));
	return p + 2;
}
inline int32_t* fif_until(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_do_loop) {
		e->report_error("invalid use of until");
		e->mode = fif::fif_mode::error;
	} else {
		fif::do_loop_scope* c = static_cast<fif::do_loop_scope*>(e->compiler_stack.back().data.get());
		c->at_until(*e);
	}
	return p + 2;
}
inline int32_t* fif_end_do(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back().data->get_type() != fif::control_structure::str_do_loop) {
		e->report_error("invalid use of end-do");
		e->mode = fif::fif_mode::error;
	} else {
		if(e->compiler_stack.back().data->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}
inline int32_t* from_r(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type_a = s.return_type_back(0);
		auto expr_a = s.return_ex_back(0);
		s.push_back_main(type_a, 0, expr_a);
		s.pop_return();
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type_a = s.return_type_back(0);
		auto dat_a = s.return_data_back(0);
		s.push_back_main(type_a, dat_a, nullptr);
		s.pop_return();
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type_a = s.return_type_back(0);
		s.push_back_main(type_a, 0, nullptr);
		s.pop_return();
	}
	return p + 2;
}
inline int32_t* to_r(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type_a = s.main_type_back(0);
		auto expr_a = s.main_ex_back(0);
		s.push_back_return(type_a, 0, expr_a);
		s.pop_main();
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type_a = s.main_type_back(0);
		auto dat_a = s.main_data_back(0);
		s.push_back_return(type_a, dat_a, nullptr);
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type_a = s.main_type_back(0);
		s.push_back_return(type_a, 0, nullptr);
		s.pop_main();
	}
	return p + 2;
}
inline  int32_t* copy_r(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type_a = s.return_type_back(0);
		auto expr_a = s.return_ex_back(0);
		s.push_back_main(type_a, 0, expr_a);
		if(e->dict.type_array[type_a].do_dup)
			e->dict.type_array[type_a].do_dup(s, p, e);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type_a = s.return_type_back(0);
		auto dat_a = s.return_data_back(0);
		s.push_back_main(type_a, dat_a, nullptr);
		if(e->dict.type_array[type_a].do_dup)
			e->dict.type_array[type_a].do_dup(s, p, e);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		auto type_a = s.return_type_back(0);
		s.push_back_main(type_a, 0, nullptr);
	}
	return p + 2;
}


inline void add_precompiled(fif::environment& env, std::string name, fif::fif_call fn, std::vector<int32_t> types, bool immediate = false) {
	if(auto it = env.dict.words.find(name); it != env.dict.words.end()) {
		env.dict.word_array[it->second].instances.push_back(int32_t(env.dict.all_instances.size()));
		env.dict.word_array[it->second].immediate |= immediate;
	} else {
		env.dict.words.insert_or_assign(name, int32_t(env.dict.word_array.size()));
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().instances.push_back(int32_t(env.dict.all_instances.size()));
		env.dict.word_array.back().immediate = immediate;
	}
	env.dict.all_instances.push_back(fif::compiled_word_instance{ });
	auto& w = std::get< fif::compiled_word_instance>(env.dict.all_instances.back());
	w.implementation = fn;
	w.stack_types_count = int32_t(types.size());
	w.stack_types_start = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), types.begin(), types.end());
}

inline int32_t* i32_lt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b < a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* i32_gt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b > a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* i32_le(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b <= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* i32_ge(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b >= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* i32_eq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntEQ, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b == a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* i32_ne(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntNE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b != a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::typechecking_failed(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, 0, nullptr);
	}
	return p + 2;
}

void initialize_standard_vocab(environment& fif_env) {
	add_precompiled(fif_env, ":", colon_definition, { });
	add_precompiled(fif_env, "+", i32_add, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "+", f32_add, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "<", i32_lt, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", i32_le, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", i32_gt, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", i32_ge, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", i32_eq, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", i32_ne, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "dup", dup, { -2, -1, -2, -2 });
	add_precompiled(fif_env, "copy", copy, { -2, -1, -2, -2 });
	add_precompiled(fif_env, "drop", drop, { -2 });
	add_precompiled(fif_env, "swap", fif_swap, { -2, -3, -1, -2, -3 });
	add_precompiled(fif_env, "if", fif_if, { }, true);
	add_precompiled(fif_env, "else", fif_else, { }, true);
	add_precompiled(fif_env, "then", fif_then, { }, true);
	add_precompiled(fif_env, "end-if", fif_then, { }, true);
	add_precompiled(fif_env, "while", fif_while, { }, true);
	add_precompiled(fif_env, "loop", fif_loop, { }, true);
	add_precompiled(fif_env, "end-while", fif_end_while, { }, true);
	add_precompiled(fif_env, "do", fif_do, { }, true);
	add_precompiled(fif_env, "until", fif_until, { }, true);
	add_precompiled(fif_env, "end-do", fif_end_do, { fif::fif_bool }, true);
	add_precompiled(fif_env, ">r", to_r, { -2, -1, -1, -1, -2 });
	add_precompiled(fif_env, "r>", from_r, { -1, -2, -1, -2 });
	add_precompiled(fif_env, "r@", copy_r, { -1, -2, -1, -2, -1, -2 });
}

}
