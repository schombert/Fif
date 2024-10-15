#pragma once
#include "fif_basic_types.hpp"

namespace fif {

inline bool typechecking_mode(fif_mode m);
inline fif_mode base_mode(fif_mode m);
inline bool skip_compilation(fif_mode m);
inline dup_evaluation check_dup(int32_t type, fif::environment& env);
inline uint16_t* stash_in_frame(state_stack& s, uint16_t* p, environment* e);
inline uint16_t* drop_cimple(fif::state_stack& s, uint16_t* p, fif::environment* e);
inline uint16_t* recover_from_frame(state_stack& s, uint16_t* p, environment* e);

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

inline uint16_t* immediate_mem_var(state_stack& s, uint16_t* p, environment* e) {
	uint16_t index = *(p);
	int32_t type = *(int32_t*)(p + 1);
	auto* l = e->interpreter_stack_space.get() + e->frame_offset + index;
	s.push_back_main(vsize_obj(type, uint32_t(sizeof(void*)), (unsigned char*)(&l)));
	return p + 3;
}

inline uint16_t* immediate_let(state_stack& s, uint16_t* p, environment* e) {
	uint16_t index = *(p);
	int32_t type = *(int32_t*)(p + 1);
	auto* l = e->interpreter_stack_space.get() + e->frame_offset + index;
	s.push_back_main(vsize_obj(type, uint32_t(e->dict.type_array[type].byte_size), l));
	return p + 3;
}

void push_lexical_variable(fif::lvar_description const& var, state_stack& ws, environment& env) {
	auto vdata = (unsigned char*)(env.tc_local_variables.data() + var.offset);
	if(var.memory_variable) {
		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), var.type, -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, env);

		if(skip_compilation(env.mode)) {

		} else if(typechecking_mode(env.mode)) {
			ws.push_back_main(vsize_obj(mem_type.type, sizeof(int64_t), vdata));
		} else if(env.mode == fif_mode::compiling_llvm) {
			ws.push_back_main(vsize_obj(mem_type.type, sizeof(LLVMValueRef), vdata));
		} else if(env.mode == fif_mode::interpreting) {
			// ws->push_back_main(vsize_obj(mem_type.type, sizeof(void*), (unsigned char*)(&vdata)));
		} else if(env.mode == fif_mode::compiling_bytecode) {
			auto cbytes = env.compiler_stack.back()->bytecode_compilation_progress();
			c_append(cbytes, env.dict.get_builtin(immediate_mem_var));
			c_append(cbytes, uint16_t(var.offset)); // index
			c_append(cbytes, int32_t(mem_type.type));

			ws.push_back_main(vsize_obj(mem_type.type, 0));
		}
	} else {
		if(skip_compilation(env.mode)) {

		} else if(typechecking_mode(env.mode)) {
			ws.push_back_main(vsize_obj(var.type, var.size, vdata));
			execute_fif_word(fif::parse_result{ "dup", false }, env, false);

			auto new_copy = ws.popr_main();
			auto old_copy = ws.popr_main();
			if(old_copy.data()) {
				memcpy(vdata, old_copy.data(), std::min(uint32_t(var.size), old_copy.size));
			}
			ws.push_back_main(new_copy);
		} else if(env.mode == fif_mode::compiling_llvm) {
			ws.push_back_main(vsize_obj(var.type, var.size, vdata));
			execute_fif_word(fif::parse_result{ "dup", false }, env, false);

			auto new_copy = ws.popr_main();
			auto old_copy = ws.popr_main();
			if(old_copy.data()) {
				memcpy(vdata, old_copy.data(), std::min(uint32_t(var.size), old_copy.size));
			}
			ws.push_back_main(new_copy);
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
			ws.push_back_main(vsize_obj(var.type, 0));
		}
	}
}

}

