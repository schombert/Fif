#pragma once
#include "fif_basic_types.hpp"

namespace fif {


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


struct type_match_result {
	bool matched = false;
	int32_t stack_consumed = 0;
	int32_t ret_stack_consumed = 0;
};
struct type_match {
	int32_t type = 0;
	uint32_t end_match_pos = 0;
};
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

	if(cells_count == 0)
		env.dict.type_array.back().flags |= type::FLAG_STATELESS;

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_STRUCT;
	env.dict.type_array.back().in_module = env.dict.type_array[template_base].in_module;
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
	if(auto it = env.dict.types.find(std::string(name)); it != env.dict.types.end()) {
		env.dict.type_array.back().next_type_with_name = it->second;
		env.dict.types.insert_or_assign(std::string(name), new_type);
	} else {
		env.dict.types.insert_or_assign(std::string(name), new_type);
	}

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
	env.dict.type_array.back().in_module = env.module_stack.empty() ? -1 : env.module_stack.back();

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
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("copy", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 dup");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("dup", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map1 init");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("init", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map0 drop");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("drop", int32_t(env.dict.word_array.size() - 1));
	}

	uint32_t index = 0;
	auto desc = subtypes;
	while(desc.size() > 0) {
		auto next = next_encoded_stack_type(desc);

		{
			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types;
			env.dict.word_array.back().stack_types_count = end_zero - start_types;
			env.dict.word_array.back().treat_as_base = true;
			env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
			bury_word(std::string(".") + std::string{ member_names[index] }, int32_t(env.dict.word_array.size() - 1));
		}
		{
			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract-copy ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types;
			env.dict.word_array.back().stack_types_count = end_zero - start_types;
			env.dict.word_array.back().treat_as_base = true;
			env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
			bury_word(std::string(".") + std::string{ member_names[index] } + "@", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.insert ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;
			env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
			bury_word(std::string(".") + std::string{ member_names[index] } + "!", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(fif_ptr);
			env.dict.all_stack_types.push_back(std::numeric_limits<int32_t>::max());
			env.dict.all_stack_types.push_back(new_type);
			add_child_types();
			env.dict.all_stack_types.push_back(-1);
			int32_t end_types_i = int32_t(env.dict.all_stack_types.size());

			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.gep ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types_i;
			env.dict.word_array.back().stack_types_count = end_types_i - start_types_i;
			env.dict.word_array.back().treat_as_base = true;
			env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
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
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("copy", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map2 dup");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		bury_word("dup", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("struct-map1 init");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
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
			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types;
			env.dict.word_array.back().stack_types_count = end_zero - start_types;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index), int32_t(env.dict.word_array.size() - 1));
		}
		{
			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.extract-copy ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types;
			env.dict.word_array.back().stack_types_count = end_zero - start_types;
			env.dict.word_array.back().treat_as_base = true;

			bury_word(std::string(".m") + std::to_string(index) + "@", int32_t(env.dict.word_array.size() - 1));
		}
		{
			int32_t start_types_i = int32_t(env.dict.all_stack_types.size());
			env.dict.all_stack_types.push_back(new_type);
			for(uint32_t j = 0; j < next; ++j)
				env.dict.all_stack_types.push_back(desc[j]);
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

bool is_memory_type_recursive(std::span<const int32_t> type_desc, environment& env) {
	if(type_desc.size() == 0)
		return false;
	if(type_desc.size() == 1)
		return is_memory_type_recursive(type_desc[0], env);
	if(type_desc[0] == fif_nil)
		return false;
	if(env.dict.type_array[type_desc[0]].is_memory_type())
		return true;
	if(type_desc[0] != fif_ptr || type_desc.size() < 3)
		return false;
	return is_memory_type_recursive(type_desc.subspan(2), env);
}

inline int32_t instantiate_templated_m_struct_full(int32_t template_base, std::vector<int32_t> const& final_subtype_list, environment& env, bool skip_check = false) {
	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

	if(!skip_check) {
		uint32_t match_pos = 0;
		uint32_t mem_count = 0;
		std::vector<int32_t> type_subs_out;
		while(match_pos < desc.size()) {
			if(mem_count > final_subtype_list.size()) {
				env.report_error("attempted to instantiate an m-struct template with the wrong number of members");
				env.mode = fif_mode::error;
				return -1;
			}

			auto mr = fill_in_variable_types(final_subtype_list[mem_count], desc.subspan(match_pos), type_subs_out, env);
			if(!mr.match_result) {
				env.report_error("attempted to instantiate an m-struct template with types that do not match its definition");
				env.mode = fif_mode::error;
				return -1;
			}

			match_pos += next_encoded_stack_type(desc.subspan(match_pos));
			++mem_count;
		}
		if(mem_count < final_subtype_list.size()) {
			env.report_error("attempted to instantiate an m-struct template with the wrong number of members");
			env.mode = fif_mode::error;
			return -1;
		}
	}

	for(auto t : final_subtype_list) {
		if(t < 0) {
			env.report_error("attempted to instantiate an m-struct template with too few type parameters");
			env.mode = fif_mode::error;
			return -1;
		}
		if(env.dict.type_array[t].ntt_base_type != -1) {
			env.report_error("attempted to instantiate an m-struct template member type with a non-type template parameter");
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

	int32_t byte_count = 0;
	for(uint32_t j = 0; j < (final_subtype_list.size() - size_t(env.dict.type_array[template_base].non_member_types)); ++j) {
		if(env.dict.type_array[final_subtype_list[j]].stateless() == false) {
			byte_count += env.dict.type_array[final_subtype_list[j]].byte_size;
		}
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(final_subtype_list.size() + 1);
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());

	if(byte_count == 0)
		env.dict.type_array.back().flags |= type::FLAG_STATELESS;

	env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
	env.dict.type_array.back().flags |= type::FLAG_STRUCT;
	env.dict.type_array.back().flags |= type::FLAG_MEMORY_TYPE;
	env.dict.type_array.back().in_module = env.dict.type_array[template_base].in_module;
	env.dict.type_array.back().non_member_types = env.dict.type_array[template_base].non_member_types;
	env.dict.type_array.back().type_slots = 0;
	env.dict.type_array.back().cell_size = (byte_count == 0 ? 0 : 1);
	env.dict.type_array.back().byte_size = byte_count;
	env.dict.all_stack_types.push_back(template_base);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), final_subtype_list.begin(), final_subtype_list.end());

	return new_type;
}

inline int32_t instantiate_templated_m_struct(int32_t template_base, std::vector<int32_t> const& subtypes, environment& env) {
	std::vector<int32_t> final_subtype_list;

	auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[template_base].decomposed_types_start, size_t(env.dict.type_array[template_base].decomposed_types_count));

	uint32_t match_pos = 0;
	while(match_pos < desc.size()) {
		auto mresult = resolve_span_type(desc.subspan(match_pos), subtypes, env);
		match_pos += mresult.end_match_pos;
		final_subtype_list.push_back(mresult.type);
	}

	return instantiate_templated_m_struct_full(template_base, final_subtype_list, env, true);
}

inline int32_t make_m_struct_type(std::string_view name, std::span<int32_t const> subtypes, std::vector<std::string_view> const& member_names, environment& env, int32_t template_types, int32_t extra_count) {

	int32_t new_type = int32_t(env.dict.type_array.size());
	env.dict.type_array.emplace_back();
	if(auto it = env.dict.types.find(std::string(name)); it != env.dict.types.end()) {
		env.dict.type_array.back().next_type_with_name = it->second;
		env.dict.types.insert_or_assign(std::string(name), new_type);
	} else {
		env.dict.types.insert_or_assign(std::string(name), new_type);
	}

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
				byte_count += env.dict.type_array[subtypes[j]].byte_size;
			}
		}

		env.dict.type_array.back().flags &= ~(type::FLAG_TEMPLATE);
		env.dict.type_array.back().flags |= type::FLAG_STRUCT;
		env.dict.type_array.back().flags |= type::FLAG_MEMORY_TYPE;

		if(byte_count == 0) {
			env.dict.type_array.back().flags |= type::FLAG_STATELESS;
		}
	} else {
		env.dict.type_array.back().non_member_types = extra_count;
		env.dict.type_array.back().flags |= type::FLAG_TEMPLATE;
		env.dict.type_array.back().flags |= type::FLAG_MEMORY_TYPE;
	}

	env.dict.type_array.back().decomposed_types_count = uint32_t(subtypes.size() + (template_types + extra_count == 0 ? 1 : extra_count));
	env.dict.type_array.back().decomposed_types_start = uint32_t(env.dict.all_stack_types.size());
	env.dict.type_array.back().cell_size = (byte_count == 0 ? 0 : 1);
	env.dict.type_array.back().byte_size = byte_count;
	env.dict.type_array.back().in_module = env.module_stack.empty() ? -1 : env.module_stack.back();

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
		env.dict.all_stack_types.push_back(fif_memory_struct);
	env.dict.all_stack_types.insert(env.dict.all_stack_types.end(), subtypes.begin(), subtypes.end());
	for(int32_t e = 0; e < extra_count; ++e) {
		env.dict.all_stack_types.push_back(-(template_types + e + 2));
	}

	int32_t pre_start_types = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);
	add_child_types();
	int32_t start_types = int32_t(env.dict.all_stack_types.size());
	env.dict.all_stack_types.push_back(new_type);
	add_child_types();
	int32_t end_zero = int32_t(env.dict.all_stack_types.size());

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
		env.dict.word_array.back().source = std::string("m-struct-mapC init-copy");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - pre_start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("init-copy", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("m-struct-map1 init");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("init", int32_t(env.dict.word_array.size() - 1));
	}
	{
		env.dict.word_array.emplace_back();
		env.dict.word_array.back().source = std::string("m-struct-map0 finish");
		env.dict.word_array.back().stack_types_start = start_types;
		env.dict.word_array.back().stack_types_count = end_zero - start_types;
		env.dict.word_array.back().treat_as_base = true;
		env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
		bury_word("finish", int32_t(env.dict.word_array.size() - 1));
	}

	uint32_t index = 0;
	auto desc = subtypes;

	while(desc.size() > 0) {
		auto next = next_encoded_stack_type(desc);
		{
			env.dict.word_array.emplace_back();
			env.dict.word_array.back().source = std::string("forth.m-gep ") + std::to_string(index);
			env.dict.word_array.back().stack_types_start = start_types;
			env.dict.word_array.back().stack_types_count = end_zero - start_types;
			env.dict.word_array.back().treat_as_base = true;
			env.dict.word_array.back().in_module = env.dict.type_array.back().in_module;
			bury_word(std::string(".") + std::string{ member_names[index] }, int32_t(env.dict.word_array.size() - 1));
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
	if(to_type != -1)
		env.dict.type_array.back().in_module = env.dict.type_array[to_type].in_module;

	env.dict.all_stack_types.push_back(fif_ptr);
	env.dict.all_stack_types.push_back(to_type);

	return new_type;
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

inline bool name_reachable(std::string_view full_name, int32_t search_module_context, int32_t name_context, environment& env);
inline std::string_view get_base_name(std::string_view full_name);
inline bool name_reachable(std::string_view full_name, int32_t name_context, environment& env) {
	return name_reachable(full_name, env.function_compilation_stack.empty() ? (env.module_stack.empty() ? -1 : env.module_stack.back()) : env.function_compilation_stack.back().for_module, name_context, env);
}

inline int32_t find_type_by_name(std::string_view full_name, environment& env) {
	auto it = env.dict.types.find(std::string(get_base_name(full_name)));
	if(it == env.dict.types.end())
		return -1;
	int32_t ttype = it->second;
	while(ttype != -1) {
		if(name_reachable(full_name, env.dict.type_array[ttype].in_module, env))
			return ttype;
		ttype = env.dict.type_array[ttype].next_type_with_name;
	}
	return ttype;
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
	} else if(auto ftype = find_type_by_name(text.substr(0, mt_end), env); ftype != -1) {
		if(mt_end >= text.size() || text[mt_end] == ',' || text[mt_end] == ')') {	// case: plain type
			return type_match{ ftype, mt_end };
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

		if(ftype == fif_anon_struct) {
			if(auto m = find_existing_type_match(ftype, subtypes, env); m != -1)
				return type_match{ m, mt_end };
			return type_match{ make_anon_struct_type(subtypes, env), mt_end };
		} else if(env.dict.type_array[ftype].type_slots != int32_t(subtypes.size())) {
			env.report_error("attempted to instantiate a type with the wrong number of parameters");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(env.dict.type_array[ftype].is_struct_template()) {
			if(env.dict.type_array[ftype].is_memory_type())
				return type_match{ instantiate_templated_m_struct(ftype, subtypes, env), mt_end };
			else
				return type_match{ instantiate_templated_struct(ftype, subtypes, env), mt_end };
		} else if(ftype == fif_ptr && env.dict.type_array[subtypes[0]].ntt_base_type != -1) {
			env.report_error("attempted to instantiate a pointer to a non-type");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(ftype == fif_array && env.dict.type_array[subtypes[1]].ntt_base_type != fif_i32) {
			env.report_error("attempted to instantiate an array with a non-integral size");
			env.mode = fif_mode::error;
			return type_match{ -1, mt_end };
		} else if(ftype == fif_array) {
			if(auto m = find_existing_type_match(ftype, subtypes, env); m != -1)
				return type_match{ m, mt_end };
			auto array_size = std::max(int64_t(0), env.dict.type_array[subtypes[1]].ntt_data);
			return type_match{ make_array_type(subtypes[0], size_t(array_size), subtypes[1], env), mt_end };
		} else if(ftype == fif_ptr) {
			if(auto m = find_existing_type_match(ftype, subtypes, env); m != -1)
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
			r.type_array.push_back(-(v + 2));
			r.end_match_pos = mt_end;
			return r;
		}
	}
	if(mt_end >= text.size() || text[mt_end] == ',' || text[mt_end] == ')') { // plain type
		r.type_array.push_back(resolve_type(text.substr(0, mt_end), env, nullptr));
		r.end_match_pos = mt_end;
		return r;
	} else if(auto ftype = find_type_by_name(text.substr(0, mt_end), env); ftype != -1) {
		r.type_array.push_back(ftype);
		//followed by type list
		++mt_end;
		r.type_array.push_back(std::numeric_limits<int32_t>::max());
		if((env.dict.type_array[ftype].flags & type::FLAG_TEMPLATE) != 0) {
			std::vector<type_span_gen_result> sub_matches;
			while(mt_end < text.size() && text[mt_end] != ')') {
				auto sub_match = internal_generate_type(text.substr(mt_end), env);
				r.max_variable = std::max(r.max_variable, sub_match.max_variable);
				sub_matches.push_back(std::move(sub_match));
				mt_end += sub_match.end_match_pos;
				if(mt_end < text.size() && text[mt_end] == ',')
					++mt_end;
			}
			auto desc = std::span<int32_t const>(env.dict.all_stack_types.data() + env.dict.type_array[ftype].decomposed_types_start, size_t(env.dict.type_array[ftype].decomposed_types_count));
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
		if(env.dict.type_array[base_type].is_memory_type())
			return type_match{ instantiate_templated_m_struct_full(base_type, subtypes, env), mt_end };
		else
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

inline word_match_result get_basic_type_match(std::string_view word_name, int32_t word_index, state_stack& current_type_state, environment& env, std::vector<int32_t>& specialize_t_subs, bool ignore_specializations, bool ignore_tc_results = false);

inline std::string_view get_base_name(std::string_view full_name) {
	if(full_name.size() == 0)
		return full_name;

	int32_t item_name_start = 0;
	do {
		if(full_name[item_name_start] == '.')
			break;
		if(full_name.substr(item_name_start).find_first_of('.') == std::string::npos)
			break;
		while(full_name[item_name_start] != '.') {
			++item_name_start;
		}
		++item_name_start;
		if(size_t(item_name_start) >= full_name.size())
			return std::string_view{ };
	} while(true);

	// item_name_start == index of first character in item name
	return full_name.substr(item_name_start);
}

inline bool name_reachable(std::string_view full_name, int32_t search_module_context, int32_t name_context, environment& env) {
	if(full_name.size() == 0)
		return false;

	if(name_context == -1) // everything in global is accessible
		return true;

	int32_t item_name_start = 0;
	do {
		if(full_name[item_name_start] == '.')
			break;
		if(full_name.substr(item_name_start).find_first_of('.') == std::string::npos)
			break;
		while(full_name[item_name_start] != '.') {
			++item_name_start;
		}
		++item_name_start;
		if(size_t(item_name_start) >= full_name.size()) {
			break;
		}
	} while(true);
	
	// item_name_start == index of first character in item name
	item_name_start--;

	while(item_name_start > 0) {
		int32_t dot_pos = item_name_start - 1;
		while(dot_pos > 0 && full_name[dot_pos] != '.') {
			--dot_pos;
		}
		auto parent_mod_string = (full_name[dot_pos] == '.' ? full_name.substr(dot_pos + 1, item_name_start - (dot_pos + 1)) : full_name.substr(dot_pos, item_name_start - dot_pos));
		if(name_context != -1 && parent_mod_string == "global" && env.dict.modules[name_context].submodule_of == -1) {
			name_context = -1;
		} else if(name_context != -1 && env.dict.modules[name_context].name == parent_mod_string) {
			name_context = env.dict.modules[name_context].submodule_of;
		} else {
			return false;
		}
		item_name_start = dot_pos;
	}

	if(name_context == -1)
		return true;

	while(search_module_context != -1) {
		if(name_context == search_module_context)
			return true;
		for(auto mimports : env.dict.modules[search_module_context].module_search_path) {
			if(mimports == name_context)
				return true;
		}
		search_module_context = env.dict.modules[search_module_context].submodule_of;
	}
	
	return false;
}


}

