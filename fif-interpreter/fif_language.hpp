#pragma once
#include "fif.hpp"

namespace fif {

inline int32_t* colon_definition(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to compile a definition inside a definition");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	if(e->source_stack.empty()) {
		e->report_error("attempted to compile a definition without a source");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	auto name_token = fif::read_token(e->source_stack.back(), *e);

	auto string_start = e->source_stack.back().data();
	while(e->source_stack.back().length() > 0) {
		auto t = fif::read_token(e->source_stack.back(), *e);
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

inline int32_t* colon_specialization(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to compile a definition inside a definition");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	if(e->source_stack.empty()) {
		e->report_error("attempted to compile a definition without a source");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	auto name_token = fif::read_token(e->source_stack.back(), *e);

	std::vector<std::string_view> stack_types;
	while(true) {
		auto next_token = fif::read_token(e->source_stack.back(), *e);
		if(next_token.content.length() == 0 || next_token.content == "s:") {
			break;
		}
		stack_types.push_back(next_token.content);
	}

	std::vector<int32_t> acc_types;
	while(!stack_types.empty()) {
		auto result = internal_generate_type(stack_types.back(), *e);
		if(result.type_array.empty()) {
			e->mode = fif_mode::error;
			e->report_error("unable to resolve type from text");
			return nullptr;
		}
		acc_types.insert(acc_types.end(), result.type_array.begin(), result.type_array.end());
		stack_types.pop_back();
	}
	int32_t start_types = int32_t(e->dict.all_stack_types.size());
	e->dict.all_stack_types.insert(e->dict.all_stack_types.end(), acc_types.begin(), acc_types.end());
	int32_t count_types = int32_t(acc_types.size());

	auto string_start = e->source_stack.back().data();
	while(e->source_stack.back().length() > 0) {
		auto t = fif::read_token(e->source_stack.back(), *e);
		if(t.is_string == false && t.content == ";") { // end of definition
			auto string_end = t.content.data();

			auto nstr = std::string(name_token.content);
			int32_t old_word = -1;
			if(auto it = e->dict.words.find(nstr); it != e->dict.words.end()) {
				old_word = it->second;
			}

			e->dict.words.insert_or_assign(nstr, int32_t(e->dict.word_array.size()));
			e->dict.word_array.emplace_back();
			e->dict.word_array.back().source = std::string(string_start, string_end);
			e->dict.word_array.back().specialization_of = old_word;
			e->dict.word_array.back().stack_types_start = start_types;
			e->dict.word_array.back().stack_types_count = count_types;

			return p + 2;
		}
	}

	e->report_error("reached the end of a definition source without a ; terminator");
	e->mode = fif::fif_mode::error;
	return p + 2;
}

inline int32_t* iadd(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildAdd(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() + a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() + a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() + a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() + a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() + a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() + a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() + a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() + a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() || a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fadd(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFAdd(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(a.type, b.as<float>() + a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(a.type, b.as<double>() + a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* isub(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildSub(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() - a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() - a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() - a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() - a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() - a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() - a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() - a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() - a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fsub(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFSub(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(a.type, b.as<float>() - a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(a.type, b.as<double>() - a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* imul(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildMul(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() * a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() * a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() * a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() * a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() * a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() * a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() * a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() * a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fmul(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFMul(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(a.type, b.as<float>() * a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(a.type, b.as<double>() * a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sidiv(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildSDiv(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() / a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() / a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() / a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() / a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uidiv(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildUDiv(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() / a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() / a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() / a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() / a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fdiv(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFDiv(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(a.type, b.as<float>() / a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(a.type, b.as<double>() / a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* simod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildSRem(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() % a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() % a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() % a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() % a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uimod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildURem(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() % a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() % a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() % a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() % a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* f_mod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFRem(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(a.type, fmodf(b.as<float>(), a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(a.type, fmod(b.as<double>(), a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* dup(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p + 2;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = dup;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	}
	auto back = s.popr_main();
	s.push_back_main(back);
	s.push_back_main(back);
	
	return p + 2;
}
inline int32_t* init(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		s.mark_used_from_main(1);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* drop(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = drop;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
		s.pop_main();
	} else if(e->mode == fif::fif_mode::compiling_llvm) {
		s.pop_main();
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
	}
	return p + 2;
}
inline int32_t* fif_swap(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p + 2;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = fif_swap;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	}
	auto back = s.popr_main();
	auto next = s.popr_main();
	s.push_back_main(back);
	s.push_back_main(next);

	return p + 2;
}


inline int32_t* lex_scope(fif::state_stack& s, int32_t* p, fif::environment* e) {
	lexical_new_scope(true, *e);
	return p + 2;
}
inline int32_t* lex_scope_end(fif::state_stack& s, int32_t* p, fif::environment* e) {
	lexical_end_scope(*e);
	return p + 2;
}

inline int32_t* fif_if(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::conditional_scope>(e->compiler_stack.back().get(), *e, s));
	return p + 2;
}
inline int32_t* fif_else(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_if) {
		e->report_error("invalid use of else");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		fif::conditional_scope* c = static_cast<fif::conditional_scope*>(e->compiler_stack.back().get());
		c->commit_first_branch(*e);
	}
	return p + 2;
}
inline int32_t* fif_and_if(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_if) {
		e->report_error("invalid use of &if");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		fif::conditional_scope* c = static_cast<fif::conditional_scope*>(e->compiler_stack.back().get());
		c->and_if();
	}
	return p + 2;
}
inline int32_t* fif_then(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_if) {
		e->report_error("invalid use of then/end-if");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		if(e->compiler_stack.back()->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}

inline int32_t* fif_while(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::while_loop_scope>(e->compiler_stack.back().get(), *e, s));
	return p + 2;
}
inline int32_t* fif_loop(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_while_loop) {
		e->report_error("invalid use of loop");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		fif::while_loop_scope* c = static_cast<fif::while_loop_scope*>(e->compiler_stack.back().get());
		c->end_condition(*e);
	}
	return p + 2;
}
inline int32_t* fif_end_while(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_while_loop) {
		e->report_error("invalid use of end-while");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		if(e->compiler_stack.back()->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}
inline int32_t* fif_do(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::do_loop_scope>(e->compiler_stack.back().get(), *e, s));
	return p + 2;
}
inline int32_t* fif_until(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_do_loop) {
		e->report_error("invalid use of until");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		fif::do_loop_scope* c = static_cast<fif::do_loop_scope*>(e->compiler_stack.back().get());
		c->until_statement(*e);
	}
	return p + 2;
}
inline int32_t* fif_end_do(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::str_do_loop) {
		e->report_error("invalid use of end-do");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		if(e->compiler_stack.back()->finish(*e))
			e->compiler_stack.pop_back();
	}
	return p + 2;
}
inline int32_t* fif_break(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty()) {
		e->report_error("invalid use of break");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		auto* s_top = e->compiler_stack.back().get();
		while(s_top) {
			if(s_top->get_type() == fif::control_structure::str_do_loop) {
				fif::do_loop_scope* c = static_cast<fif::do_loop_scope*>(s_top);
				c->add_break();
				return p + 2;
			} else if(s_top->get_type() == fif::control_structure::str_while_loop) {
				fif::while_loop_scope* c = static_cast<fif::while_loop_scope*>(s_top);
				c->add_break();
				return p + 2;
			}
			if(s_top->get_type() == control_structure::mode_switch)
				s_top = static_cast<mode_switch_scope*>(s_top)->interpreted_link;
			else
				s_top = s_top->parent;
		}
	}
	e->report_error("break not used within a do or while loop");
	e->mode = fif::fif_mode::error;
	return nullptr;
}
inline int32_t* fif_return(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty()) {
		e->report_error("invalid use of return");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		auto* s_top = e->compiler_stack.back().get();
		while(s_top) {
			if(s_top->get_type() == fif::control_structure::function) {
				fif::function_scope* c = static_cast<fif::function_scope*>(s_top);
				c->add_return();
				return p + 2;
			}
			if(s_top->get_type() == control_structure::mode_switch)
				s_top = static_cast<mode_switch_scope*>(s_top)->interpreted_link;
			else
				s_top = s_top->parent;
		}
	}
	e->report_error("return not used within a function");
	e->mode = fif::fif_mode::error;
	return nullptr;
}
inline int32_t* from_r(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p + 2;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = from_r;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	}
	auto back = s.popr_return();
	s.push_back_main(back);
	return p + 2;
}
inline int32_t* r_at(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p + 2;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = r_at;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
		auto back = s.popr_return();
		s.push_back_return(back);
		s.push_back_main(back);
	} else {
		auto back = s.popr_return();
		s.push_back_main(back);
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_obj = s.popr_main();
		auto old_obj = s.popr_main();
		s.push_back_return(old_obj);
		s.push_back_main(new_obj);
	}
	return p + 2;
}
inline int32_t* to_r(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(skip_compilation(e->mode))
		return p + 2;
	if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = to_r;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	}
	auto back = s.popr_main();
	s.push_back_return(back);
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

inline int32_t* ilt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() < a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() < a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() < a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() < a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, !b.as<bool>() && a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* flt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() < a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() < a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uilt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntULT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() < a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() < a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() < a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() < a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* igt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() > a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() > a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() > a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() > a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, b.as<bool>() && !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fgt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() > a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() > a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uigt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntUGT, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() > a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() > a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() > a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() > a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ile(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() <= a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() <= a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() <= a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() <= a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, !b.as<bool>() || a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uile(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntULE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() <= a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() <= a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() <= a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() <= a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fle(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() <= a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() <= a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ige(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() >= a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() >= a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() >= a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() >= a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, b.as<bool>() || !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uige(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntUGE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() >= a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() >= a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() >= a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() >= a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fge(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() >= a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() >= a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ieq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntEQ, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() == a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() == a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() == a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() == a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() == a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() == a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() == a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() == a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, b.as<bool>() == a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* feq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOEQ, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() == a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() == a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ine(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntNE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, b.as<int32_t>() != a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint32_t>() != a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, b.as<int64_t>() != a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint64_t>() != a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, b.as<int16_t>() != a.as<int16_t>(), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint16_t>() != a.as<uint16_t>(), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, b.as<int8_t>() != a.as<int8_t>(), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, b.as<uint8_t>() != a.as<uint8_t>(), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, b.as<bool>() != a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fne(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealONE, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(fif_bool, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_bool, b.as<float>() != a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_bool, b.as<double>() != a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

inline int32_t* f_select(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto expr = s.popr_main();
		auto tval = s.popr_main();
		auto fval = s.popr_main();
		assert(tval.size == fval.size);
		LLVMValueRef* tvalptr = (LLVMValueRef*)(tval.data());
		LLVMValueRef* fvalptr = (LLVMValueRef*)(fval.data());
		std::vector<LLVMValueRef> trueselects;

		if(e->dict.type_array[tval.type].is_struct() || e->dict.type_array[tval.type].is_array()) {
			std::vector<LLVMValueRef> falseselects;
			for(uint32_t i = 0; i < tval.size / 8; ++i) {
				if(fvalptr[i] != tvalptr[i])
					falseselects.push_back(LLVMBuildSelect(e->llvm_builder, expr.as<LLVMValueRef>(), fvalptr[i], tvalptr[i], ""));
				else
					falseselects.push_back(fvalptr[i]);
			}
			s.push_back_main(vsize_obj(tval.type, tval.size, (unsigned char*)(falseselects.data())));
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}

		for(uint32_t i = 0; i < tval.size / 8; ++i) {
			if(fvalptr[i] != tvalptr[i])
				trueselects.push_back(LLVMBuildSelect(e->llvm_builder, expr.as<LLVMValueRef>(), tvalptr[i], fvalptr[i], ""));
			else
				trueselects.push_back(fvalptr[i]);
		}

		s.push_back_main(vsize_obj(tval.type, tval.size, (unsigned char*)(trueselects.data())));
#endif
	} else if(e->mode == fif::fif_mode::compiling_bytecode) {
		auto expr = s.popr_main();
		auto tval = s.popr_main();
		auto fval = s.popr_main();

		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = f_select;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}

		s.push_back_main(vsize_obj(tval.type, 0));
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto expr = s.popr_main();
		auto tval = s.popr_main();
		auto fval = s.popr_main();

		if(e->dict.type_array[tval.type].is_struct() || e->dict.type_array[tval.type].is_array()) {
			s.push_back_main(expr.as<bool>() ? fval : tval);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}
		s.push_back_main(expr.as<bool>() ? tval : fval);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto expr = s.popr_main();
		auto tval = s.popr_main();
		auto fval = s.popr_main();

		if(tval.type != fval.type || expr.type != fif_bool) {
			e->mode = fail_typechecking(e->mode);
			return nullptr;
		}

		int64_t* tvalptr = (int64_t*)(tval.data());
		int64_t* fvalptr = (int64_t*)(fval.data());
		std::vector<int64_t> trueselects;

		for(uint32_t i = 0; i < tval.size / 8; ++i) {
			if(fvalptr[i] != tvalptr[i])
				trueselects.push_back(e->new_ident());
			else
				trueselects.push_back(fvalptr[i]);
		}

		s.push_back_main(vsize_obj(tval.type, tval.size, (unsigned char*)(trueselects.data())));
	}
	return p + 2;
}

inline int32_t* make_immediate(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		e->report_error("cannot turn a word immediate in compiled code");
		e->mode = fif_mode::error;
		return nullptr;
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.word_array.back().specialization_of == -1) {
			e->dict.word_array.back().immediate = true;
		} else {
			e->report_error("cannot mark a specialized word as immediate");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {

	}
	return p + 2;
}

inline int32_t* open_bracket(fif::state_stack& s, int32_t* p, fif::environment* e) {
	switch_compiler_stack_mode(*e, fif_mode::interpreting);
	return p + 2;
}
inline int32_t* close_bracket(fif::state_stack& s, int32_t* p, fif::environment* e) {
	restore_compiler_stack_mode(*e);
	return p + 2;
}

inline int32_t* impl_heap_allot(fif::state_stack& s, int32_t* p, fif::environment* e) { // must drop contents ?
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto t = s.main_type_back(0);
		auto new_mem = LLVMBuildMalloc(e->llvm_builder, llvm_type(t, *e), "");
		store_to_llvm_pointer(t, s, new_mem, *e);
		if(e->dict.type_array[t].is_memory_type()) {
			s.push_back_main(vsize_obj(t, new_mem, vsize_obj::by_value{ }));
		} else {
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), t, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
			s.push_back_main(vsize_obj(mem_type.type, new_mem, vsize_obj::by_value{ }));
		}
#endif
	} else if(e->mode == fif::fif_mode::compiling_bytecode) {
		if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
			fif_call imm = impl_heap_allot;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
		auto t = s.main_type_back(0);
		if(e->dict.type_array[t].is_memory_type()) {
			s.pop_main();
			s.push_back_main(vsize_obj(t, 0));
		} else {
			s.pop_main();
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), t, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
			s.push_back_main(vsize_obj(mem_type.type, 0));
		}
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto t = s.main_type_back(0);
		auto ptr = malloc(e->dict.type_array[t].byte_size);
		if(e->dict.type_array[t].is_memory_type()) {
			auto val = s.popr_main();
			memcpy(ptr, val.as<unsigned char*>(), e->dict.type_array[t].byte_size);
			s.push_back_main(vsize_obj(t, ptr, vsize_obj::by_value{ }));
		} else {
			auto val = s.popr_main();
			memcpy(ptr, val.data(), e->dict.type_array[t].byte_size);
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), t, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
			s.push_back_main(vsize_obj(mem_type.type, ptr, vsize_obj::by_value{ }));
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		if(e->dict.type_array[t].is_memory_type()) {
			s.pop_main();
			s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
		} else {
			s.pop_main();
			int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), t, -1 };
			std::vector<int32_t> subs;
			auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
			s.push_back_main(vsize_obj(mem_type.type, e->new_ident(), vsize_obj::by_value{ }));
		}
	}
	return p + 2;
}

inline int32_t* impl_heap_free(fif::state_stack& s, int32_t* p, fif::environment* e) { // must drop contents ?
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	if(e->dict.type_array[ptr_type].is_memory_type()) {
		if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			auto v = s.popr_main();
			s.push_back_main(v);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			LLVMBuildFree(e->llvm_builder, v.as<LLVMValueRef>());
#endif
		} else if(e->mode == fif_mode::compiling_bytecode) {
			if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
				fif_call imm = impl_heap_free;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
			s.pop_main();
		} else if(e->mode == fif::fif_mode::interpreting) {
			auto v = s.popr_main();
			s.push_back_main(v);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			free(v.as<void*>());
		} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
			s.pop_main();
		}
	} else if(e->dict.type_array[ptr_type].is_pointer()) {
		int32_t pointer_contents = fif_nil;
		if(ptr_type != fif_opaque_ptr) {
			auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
			pointer_contents = e->dict.all_stack_types[decomp + 1];
		}

		if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
			auto ptr_expr = s.popr_main().as<LLVMValueRef>();
			if(pointer_contents != fif_nil) {
				load_from_llvm_pointer(pointer_contents, s, ptr_expr, *e);
				execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			}
			LLVMBuildFree(e->llvm_builder, ptr_expr);
#endif
		} else if(e->mode == fif_mode::compiling_bytecode) {
			if(auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress(); compile_bytes) {
				fif_call imm = impl_heap_free;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
			s.pop_main();
		} else if(e->mode == fif::fif_mode::interpreting) {
			auto ptr_v = s.popr_main().as<unsigned char*>();
			if(pointer_contents != fif_nil) {
				s.push_back_main(vsize_obj(pointer_contents, e->dict.type_array[pointer_contents].byte_size, ptr_v));
				execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			}
			free(ptr_v);
		} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
			s.pop_main();
		}
	} else {
		if(typechecking_mode(e->mode)) {
			e->mode = fail_typechecking(e->mode);
		}
	}

	return p + 2;
}

inline int32_t* impl_load(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	auto pointer_contents = e->dict.all_stack_types[decomp + 1];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto ptr_expr = s.popr_main().as<LLVMValueRef>();
		load_from_llvm_pointer(pointer_contents, s, ptr_expr, *e);

		if(!e->dict.type_array[pointer_contents].is_memory_type()) {
			auto original_value = s.popr_main();
			s.push_back_main(original_value);

			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);

			auto new_val = s.popr_main();
			store_difference_to_llvm_pointer(pointer_contents, s, original_value, ptr_expr, *e);

			s.push_back_main(new_val);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ptr = s.popr_main().as<unsigned char*>();
		if(!e->dict.type_array[pointer_contents].is_memory_type()) {
			s.push_back_main(vsize_obj(pointer_contents, uint32_t(e->dict.type_array[pointer_contents].byte_size), ptr));

			auto de = check_dup(pointer_contents, *e);
			if(de.alters_source || de.copy_altered || e->dict.type_array[pointer_contents].is_struct()) {
				execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
				auto copy = s.popr_main();
				auto original = s.popr_main();
				if(de.alters_source) {
					memcpy(ptr, original.data(), e->dict.type_array[pointer_contents].byte_size);
				}
				s.push_back_main(copy);
			}
		} else {
			s.push_back_main(vsize_obj(pointer_contents, sizeof(void*), ptr));
		}
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		if(!e->dict.type_array[pointer_contents].is_memory_type()) {
			std::vector<int64_t> ncells;
			for(int32_t i = 0; i < e->dict.type_array[pointer_contents].cell_size; ++i) {
				ncells.push_back(e->new_ident());
			}
			s.push_back_main(vsize_obj(pointer_contents, uint32_t(e->dict.type_array[pointer_contents].cell_size * 8), (unsigned char*)(ncells.data())));
		} else {
			s.push_back_main(vsize_obj(pointer_contents, e->new_ident(), vsize_obj::by_value{ }));
		}
	}
	return p + 2;
}

inline int32_t* impl_load_deallocated(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	auto pointer_contents = e->dict.all_stack_types[decomp + 1];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto ptr_expr = s.popr_main().as<LLVMValueRef>();
		load_from_llvm_pointer(pointer_contents, s, ptr_expr, *e);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ptr = s.popr_main().as<unsigned char*>();
		if(!e->dict.type_array[pointer_contents].is_memory_type()) {
			s.push_back_main(vsize_obj(pointer_contents, uint32_t(e->dict.type_array[pointer_contents].byte_size), ptr));
		} else {
			s.push_back_main(vsize_obj(pointer_contents, sizeof(void*), ptr));
		}
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		if(!e->dict.type_array[pointer_contents].is_memory_type()) {
			std::vector<int64_t> ncells;
			for(int32_t i = 0; i < e->dict.type_array[pointer_contents].cell_size; ++i) {
				ncells.push_back(e->new_ident());
			}
			s.push_back_main(vsize_obj(pointer_contents, uint32_t(e->dict.type_array[pointer_contents].cell_size * 8), (unsigned char*)(ncells.data())));
		} else {
			s.push_back_main(vsize_obj(pointer_contents, e->new_ident(), vsize_obj::by_value{ }));
		}
	}
	return p + 2;
}

inline int32_t* impl_store(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	auto pointer_contents = e->dict.all_stack_types[decomp + 1];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto ptr_expr = s.popr_main().as<LLVMValueRef>();
		if(e->dict.type_array[pointer_contents].flags != 0) {
			load_from_llvm_pointer(pointer_contents, s, ptr_expr, *e);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}
		store_to_llvm_pointer(pointer_contents, s, ptr_expr, *e);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ptr_expr = s.popr_main().as<unsigned char*>();

		if(e->dict.type_array[pointer_contents].flags != 0) {
			s.push_back_main(vsize_obj(pointer_contents, e->dict.type_array[pointer_contents].byte_size, ptr_expr));
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}
		auto new_val = s.popr_main();
		memcpy(ptr_expr, new_val.data(), size_t(new_val.size));
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* impl_uninit_store(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	auto pointer_contents = e->dict.all_stack_types[decomp + 1];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto ptr_expr = s.popr_main().as<LLVMValueRef>();
		store_to_llvm_pointer(pointer_contents, s, ptr_expr, *e);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ptr_expr = s.popr_main().as<unsigned char*>();
		auto new_val = s.popr_main();
		memcpy(ptr_expr, new_val.data(), size_t(new_val.size));
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_pointer_cast(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto t = *(p + 2);
	auto ptr = s.popr_main();
	ptr.type = t;
	s.push_back_main(ptr);
	return p + 3;
}


inline int32_t* pointer_cast(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("pointer cast was unable to read the word describing the pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto ptype = read_token(e->source_stack.back(), *e);
	bool bad_type = ptype.is_string;
	auto resolved_type = resolve_type(ptype.content, *e, e->compiler_stack.back()->type_substitutions());
	if(resolved_type == -1) {
		bad_type = true;
	} else if(resolved_type != fif_opaque_ptr) {
		if(e->dict.type_array[resolved_type].decomposed_types_count == 0) {
			bad_type = true;
		} else if(e->dict.type_array[resolved_type].is_pointer() == false && e->dict.type_array[resolved_type].is_memory_type() == false) {
			bad_type = true;
		}
	}

	if(bad_type) {
		e->report_error("pointer attempted to be cast to a non pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	if(!skip_compilation(e->mode)) {
		auto ptr = s.popr_main();
		ptr.type = resolved_type;
		s.push_back_main(ptr);
	}

	if(e->mode == fif::fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_pointer_cast;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(resolved_type);
		}
	}
	return p + 2;
}

inline int32_t* impl_sizeof(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("sizeof was unable to read the word describing the type");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto ptype = read_token(e->source_stack.back(), *e);
	bool bad_type = ptype.is_string;
	auto resolved_type = resolve_type(ptype.content, *e, e->compiler_stack.back()->type_substitutions());
	if(resolved_type == -1) {
		e->report_error("sizeof given an invalid type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.push_back_main(vsize_obj(fif_i32, LLVMConstTrunc(LLVMSizeOf(llvm_type(resolved_type, *e)), LLVMInt32TypeInContext(e->llvm_context)), vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = immediate_i32;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(e->dict.type_array[resolved_type].byte_size);
		}
		s.push_back_main(vsize_obj(fif_i32, 0));
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.push_back_main(vsize_obj(fif_i32, e->dict.type_array[resolved_type].byte_size, vsize_obj::by_value{ }));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.push_back_main(vsize_obj(fif_i32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

int64_t generic_integer(vsize_obj const& o) {
	switch(o.type) {
		case fif_i8:
			return o.as<int8_t>();
		case fif_u8:
			return o.as<uint8_t>();
		case fif_i16:
			return o.as<int16_t>();
		case fif_u16:
			return o.as<uint16_t>();
		case fif_i32:
			return o.as<int32_t>();
		case fif_u32:
			return o.as<uint32_t>();
		case fif_i64:
			return o.as<int64_t>();
		case fif_u64:
			return int64_t(o.as<uint64_t>());
		default:
			return 0;
	}
}

inline int32_t* impl_index(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto pval = s.popr_main().as<LLVMValueRef>();
		auto index = s.popr_main().as<LLVMValueRef>();
		auto iresult = LLVMBuildInBoundsGEP2(e->llvm_builder, LLVMInt8TypeInContext(e->llvm_context), pval, &index, 1, "");
		s.push_back_main(vsize_obj(fif_opaque_ptr, iresult, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto pval = s.popr_main().as<unsigned char*>();
		auto index = generic_integer(s.popr_main());
		s.push_back_main(vsize_obj(fif_opaque_ptr, pval + index, vsize_obj::by_value{ }));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(fif_opaque_ptr, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

inline int32_t* allocate_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto sz = s.popr_main().as<LLVMValueRef>();
		auto iresult = LLVMBuildArrayMalloc(e->llvm_builder, LLVMInt8TypeInContext(e->llvm_context), sz, "");
		LLVMBuildMemSet(e->llvm_builder, iresult, LLVMConstInt(LLVMInt8TypeInContext(e->llvm_context), 0, false), sz, 1);
		s.push_back_main(vsize_obj(fif_opaque_ptr, iresult, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto sz = generic_integer(s.popr_main());
		auto val = malloc(size_t(sz));
		memset(val, 0, size_t(sz));
		s.push_back_main(vsize_obj(fif_opaque_ptr, val, vsize_obj::by_value{ }));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_opaque_ptr, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

inline int32_t* copy_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto bytes = s.popr_main().as<LLVMValueRef>();
		auto dest_ptr = s.popr_main().as<LLVMValueRef>();
		auto source_ptr = s.popr_main().as<LLVMValueRef>();
		auto res = LLVMBuildMemCpy(e->llvm_builder, dest_ptr, 1, source_ptr, 1, bytes);
		s.push_back_main(vsize_obj(fif_opaque_ptr, dest_ptr, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto bytes = generic_integer(s.popr_main());
		auto dest_ptr = s.popr_main().as<unsigned char*>();
		auto source_ptr = s.popr_main().as<unsigned char*>();
		memcpy(dest_ptr, source_ptr, size_t(bytes));
		s.push_back_main(vsize_obj(fif_opaque_ptr, dest_ptr, vsize_obj::by_value{ }));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		auto dest_ptr = s.popr_main();
		s.pop_main();
		s.push_back_main(dest_ptr);
	}
	return p + 2;
}

inline int32_t* free_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMBuildFree(e->llvm_builder, s.popr_main().as<LLVMValueRef>());
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto* ptr = s.popr_main().as<unsigned char*>();
		free(ptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_local_reassign(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t offset = *(p + 2);
	auto dest = e->interpreter_stack_space.get() + e->frame_offset + offset;
	auto val = s.popr_main();

	s.push_back_main(vsize_obj(val.type, val.size, dest));
	execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

	memcpy(dest, val.data(), size_t(val.size));
	return p + 3;
}
inline int32_t* do_local_assign(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t offset = *(p + 2);
	auto dest = e->interpreter_stack_space.get() + e->frame_offset + offset;
	auto val = s.popr_main();

	memcpy(dest, val.data(), size_t(val.size));
	return p + 3;
}

inline int32_t* create_relet(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("-> was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(typechecking_mode(e->mode) && !skip_compilation(e->mode)) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, true, *e) == -1) {
			e->mode = fail_typechecking(e->mode);
		}
	} else if(e->mode == fif_mode::interpreting) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, true, *e) == -1) {
			e->report_error("could not find a let with given name and type");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, true, *e) == -1) {
			e->report_error("could not find a let with given name and type");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto var = s.popr_main();
		if(auto offset = lexical_create_var(std::string(name.content), var.type, e->dict.type_array[var.type].is_memory_type() ? 8 : e->dict.type_array[var.type].byte_size, var.data(), false, true, *e); offset != -1) {
			auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = do_local_reassign;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(offset);
			}
		} else {
			e->report_error("could not find a let with given name and type");
			e->mode = fif_mode::error;
			return nullptr;
		}
	}

	return p + 2;
}

inline int32_t* create_let(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("let was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(typechecking_mode(e->mode) && !skip_compilation(e->mode)) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, false, *e) == -1) {
			e->mode = fail_typechecking(e->mode);
		}
	} else if(e->mode == fif_mode::interpreting) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, false, *e) == -1) {
			e->report_error("could not make a let with given name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), false, false, *e) == -1) {
			e->report_error("could not make a let with given name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto var = s.popr_main();
		if(auto offset = lexical_create_var(std::string(name.content), var.type, e->dict.type_array[var.type].is_memory_type() ? 8 : e->dict.type_array[var.type].byte_size, var.data(), false, false, *e); offset != -1) {
			auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = do_local_assign;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(offset);
			}
		} else {
			e->report_error("could not make a let with given name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	}

	return p + 2;
}

inline int32_t* create_params(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("( was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	std::vector<parse_result> names;
	auto name = read_token(e->source_stack.back(), *e);
	while(name.content != "" && name.content != ")") {
		names.push_back(name);
		name = read_token(e->source_stack.back(), *e);
	}

	while(!names.empty()) {
		auto n = names.back();
		names.pop_back();

		if(typechecking_mode(e->mode) && !skip_compilation(e->mode)) {
			auto var = s.popr_main();
			if(lexical_create_var(std::string(n.content), var.type, var.size, var.data(), false, false, *e) == -1) {
				e->mode = fail_typechecking(e->mode);
			}
		} else if(e->mode == fif_mode::interpreting) {
			auto var = s.popr_main();
			if(lexical_create_var(std::string(n.content), var.type, var.size, var.data(), false, false, *e) == -1) {
				e->report_error("could not make a let with given name");
				e->mode = fif_mode::error;
				return nullptr;
			}
		} else if(e->mode == fif_mode::compiling_llvm) {
			auto var = s.popr_main();
			if(lexical_create_var(std::string(n.content), var.type, var.size, var.data(), false, false, *e) == -1) {
				e->report_error("could not make a let with given name");
				e->mode = fif_mode::error;
				return nullptr;
			}
		} else if(e->mode == fif_mode::compiling_bytecode) {
			auto var = s.popr_main();
			if(auto offset = lexical_create_var(std::string(n.content), var.type, e->dict.type_array[var.type].is_memory_type() ? 8 : e->dict.type_array[var.type].byte_size, var.data(), false, false, *e); offset != -1) {
				auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
				if(compile_bytes) {
					fif_call imm = do_local_assign;
					uint64_t imm_bytes = 0;
					memcpy(&imm_bytes, &imm, 8);
					compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
					compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
					compile_bytes->push_back(offset);
				}
			} else {
				e->report_error("could not make a let with given name");
				e->mode = fif_mode::error;
				return nullptr;
			}
		}
	}
	return p + 2;
}

inline int32_t* create_var(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("var was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(typechecking_mode(e->mode) && !skip_compilation(e->mode)) {
		auto var = s.popr_main();
		auto new_expr = e->new_ident();
		if(lexical_create_var(std::string(name.content), var.type, 8, (unsigned char*)(&new_expr), true, false, *e) == -1) {
			e->mode = fail_typechecking(e->mode);
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto type = s.main_type_back(0);
		auto new_expr = e->compiler_stack.back()->build_alloca(e->dict.type_array[type].is_memory_type() ? LLVMPointerTypeInContext(e->llvm_context, 0) : llvm_type(type, *e));
		if(lexical_create_var(std::string(name.content), type, 8, (unsigned char*)(&new_expr), true, false, *e) == -1) {
			e->report_error("could not make a var with given name");
			e->mode = fif_mode::error;
			return nullptr;
		} else {
			if(e->dict.type_array[type].is_memory_type()) {
				auto ptr = s.popr_main().as<LLVMValueRef>();
				LLVMBuildStore(e->llvm_builder, ptr, new_expr);
			} else {
				store_to_llvm_pointer(type, s, new_expr, *e);
			}
		}
	} else if(e->mode == fif_mode::interpreting) {
		auto var = s.popr_main();
		if(lexical_create_var(std::string(name.content), var.type, var.size, var.data(), true, false, *e) == -1) {
			e->report_error("could not make a var with given name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto var = s.popr_main();
		if(auto offset = lexical_create_var(std::string(name.content), var.type, e->dict.type_array[var.type].is_memory_type() ? 8 : e->dict.type_array[var.type].byte_size, var.data(), true, false, *e); offset != -1) {
			auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = do_local_assign;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(offset);
			}
		} else {
			e->report_error("could not make a var with given name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	}
	return p + 2;
}

inline int32_t* create_global_impl(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("global was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(skip_compilation(e->mode))
		return p + 2;

	auto type = s.popr_main().as<int32_t>();

	if(typechecking_mode(e->mode)) {
		return p + 2;
	}
	if(e->mode != fif_mode::interpreting) {
		e->report_error("attempted to define a global outside of an immediate mode");
		e->mode = fif_mode::error;
		return nullptr;
	}

	auto strname = std::string(name.content);

	if(e->global_names.find(strname) != e->global_names.end()) {
		e->report_error("duplicate global definition");
		e->mode = fif_mode::error;
		return nullptr;
	}

	e->globals.emplace_back();
	global_item& newitem = e->globals.back();
	newitem.bytes = std::unique_ptr<unsigned char[]>(new unsigned char[e->dict.type_array[type].byte_size]);
	memset(newitem.bytes.get(), 0, size_t(e->dict.type_array[type].byte_size));
	newitem.cells  = std::unique_ptr<LLVMValueRef[]>(new LLVMValueRef[1]);
	auto cp = newitem.cells.get();
	cp[0] = LLVMAddGlobal(e->llvm_module, llvm_type(type, *e), strname.c_str());
	LLVMSetInitializer(cp[0], llvm_zero_constant(type, *e));
	newitem.type = type;
	newitem.constant = false;

	e->global_names.insert_or_assign(strname, int32_t(e->globals.size() - 1));
	return p + 2;
}

inline int32_t* do_fextract(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	auto v = s.popr_main();
	int32_t boffset = 0;
	for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		boffset += e->dict.type_array[c].byte_size;
	}
	auto de = check_dup(child_type, *e);
	s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
	if(de.copy_altered || e->dict.type_array[child_type].is_struct()) {
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();
		s.pop_main();
		s.push_back_main(new_copy);
	}
	s.push_back_main(v);
	execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

	return p + 3;
}

inline int32_t* forth_extract(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].is_struct() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}

		auto v = s.popr_main();
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8));
		
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();
		s.pop_main();
		s.push_back_main(new_copy);
		
		s.push_back_main(v);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			boffset += e->dict.type_array[c].byte_size;
		}
		auto de = check_dup(child_type, *e);

		auto v = s.popr_main();
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
		if(de.copy_altered || e->dict.type_array[child_type].is_struct()) {
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			auto new_copy = s.popr_main();
			s.pop_main();
			s.push_back_main(new_copy);
		}
		s.push_back_main(v);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}

		auto v = s.popr_main();
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8));

		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();
		s.pop_main();
		s.push_back_main(new_copy);
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_fextract;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(index_value);
		}
		s.pop_main();
		s.push_back_main(vsize_obj(child_type, 0));
	}
	return p + 2;
}
inline int32_t* do_fextractc(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	auto v = s.popr_main();
	int32_t boffset = 0;
	for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		boffset += e->dict.type_array[c].byte_size;
	}

	auto de = check_dup(child_type, *e);
	
	if(de.alters_source || de.copy_altered || e->dict.type_array[child_type].is_struct()) {
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();
		if(de.alters_source) {
			auto old_copy = s.popr_main();
			memcpy(v.data() + boffset, old_copy.data(), e->dict.type_array[child_type].byte_size);
		} else {
			s.pop_main();
		}

		s.push_back_main(v);
		s.push_back_main(new_copy);
	} else {
		s.push_back_main(v);
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
	}

	return p + 3;
}

inline int32_t* forth_extract_copy(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].is_struct() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto v = s.popr_main();
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8));
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();
			
		auto old_copy = s.popr_main();
		memcpy(v.data() + coffset * 8, old_copy.data(), e->dict.type_array[child_type].cell_size * 8);
			
		s.push_back_main(v);
		s.push_back_main(new_copy);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.popr_main();
		int32_t boffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			boffset += e->dict.type_array[c].byte_size;
		}

		auto de = check_dup(child_type, *e);

		if(de.alters_source || de.copy_altered || e->dict.type_array[child_type].is_struct()) {
			s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			auto new_copy = s.popr_main();
			if(de.alters_source) {
				auto old_copy = s.popr_main();
				memcpy(v.data() + boffset, old_copy.data(), e->dict.type_array[child_type].byte_size);
			} else {
				s.pop_main();
			}

			s.push_back_main(v);
			s.push_back_main(new_copy);
		} else {
			s.push_back_main(v);
			s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset));
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto v = s.popr_main();
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}
		s.push_back_main(vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8));
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto new_copy = s.popr_main();

		auto old_copy = s.popr_main();
		memcpy(v.data() + coffset * 8, old_copy.data(), e->dict.type_array[child_type].cell_size * 8);

		s.push_back_main(v);
		s.push_back_main(new_copy);
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_fextractc;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(index_value);
		}
		s.mark_used_from_main(1);
		s.push_back_main(vsize_obj(child_type, 0));
	}
	return p + 2;
}
inline int32_t* do_finsert(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	int32_t boffset = 0;
	for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		boffset += e->dict.type_array[c].byte_size;
	}

	auto v = s.popr_main();
	auto oldv = vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset);
	s.push_back_main(oldv);
	execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

	auto newv = s.popr_main();
	memcpy(v.data() + boffset, newv.data(), e->dict.type_array[child_type].byte_size);

	s.push_back_main(v);

	return p + 3;
}

inline int32_t* forth_insert(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].is_struct() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}

		auto v = s.popr_main();
		auto oldv = vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8);
		s.push_back_main(oldv);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

		auto newv = s.popr_main();
		memcpy(v.data() + coffset * 8, newv.data(), e->dict.type_array[child_type].cell_size * 8);

		s.push_back_main(v);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			boffset += e->dict.type_array[c].byte_size;
		}

		auto v = s.popr_main();
		auto oldv = vsize_obj(child_type, e->dict.type_array[child_type].byte_size, v.data() + boffset);
		s.push_back_main(oldv);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

		auto newv = s.popr_main();
		memcpy(v.data() + boffset, newv.data(), e->dict.type_array[child_type].byte_size);

		s.push_back_main(v);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t coffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			coffset += e->dict.type_array[c].cell_size;
		}

		auto v = s.popr_main();
		auto oldv = vsize_obj(child_type, e->dict.type_array[child_type].cell_size * 8, v.data() + coffset * 8);
		s.push_back_main(oldv);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);

		auto newv = s.popr_main();
		memcpy(v.data() + coffset * 8, newv.data(), e->dict.type_array[child_type].cell_size * 8);

		s.push_back_main(v);
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_finsert;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(index_value);
		}
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(stype, 0));
	}
	return p + 2;
}

inline int32_t* do_fgep(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto ptr_type = s.main_type_back(0);

	if(ptr_type == -1) {
		e->report_error("attempted to use a pointer operation on a non-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	if(e->dict.type_array[ptr_type].decomposed_types_count == 0 || e->dict.type_array[ptr_type].is_pointer() == false) {
		e->report_error("attempted to use a struct-pointer operation on a non-struct-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto stype = e->dict.all_stack_types[decomp + 1];

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	int32_t boffset = 0;
	for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		boffset += e->dict.type_array[c].byte_size;
	}

	int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
	std::vector<int32_t> subs;
	auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

	auto ptr = s.popr_main().as<unsigned char*>() + boffset;
	s.push_back_main(vsize_obj(child_ptr_type.type, ptr, vsize_obj::by_value{ }));

	return p + 3;
}

inline int32_t* forth_gep(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto ptr_type = s.main_type_back(0);

	if(ptr_type == -1) {
		e->report_error("attempted to use a pointer operation on a non-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;

	if(e->dict.type_array[ptr_type].decomposed_types_count == 0 || e->dict.type_array[ptr_type].is_pointer() == false) {
		e->report_error("attempted to use a struct-pointer operation on a non-struct-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	auto stype = e->dict.all_stack_types[decomp + 1];

	if(stype == -1 || e->dict.type_array[stype].is_struct() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
	std::vector<int32_t> subs;
	auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t real_index = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			if(e->dict.type_array[c].stateless() == false)
				++real_index;
		}

		int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
		std::vector<int32_t> subs;
		auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

		auto ptr = s.popr_main().as<LLVMValueRef>();
		if(real_index != 0) {
			s.push_back_main(vsize_obj(child_ptr_type.type, LLVMBuildStructGEP2(e->llvm_builder, llvm_type(stype, *e), ptr, uint32_t(real_index), ""), vsize_obj::by_value{ }));
		} else {
			s.push_back_main(vsize_obj(child_ptr_type.type, ptr, vsize_obj::by_value{ }));
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			boffset += e->dict.type_array[c].byte_size;
		}

		int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
		std::vector<int32_t> subs;
		auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

		auto ptr = s.popr_main().as<unsigned char*>() + boffset;
		s.push_back_main(vsize_obj(child_ptr_type.type, ptr, vsize_obj::by_value{ }));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t real_index = 0;
		for(auto i = 1; i < 1 + index_value && i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			if(e->dict.type_array[c].stateless() == false)
				++real_index;
		}

		int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
		std::vector<int32_t> subs;
		auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

		auto ptr = s.popr_main().as<int64_t>();
		if(real_index != 0) {
			s.push_back_main(vsize_obj(child_ptr_type.type, e->new_ident(), vsize_obj::by_value{ }));
		} else {
			s.push_back_main(vsize_obj(child_ptr_type.type, ptr, vsize_obj::by_value{ }));
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_fgep;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(index_value);
		}
		s.pop_main();
		s.push_back_main(vsize_obj(child_ptr_type.type, 0));
	}
	return p + 2;
}

inline int32_t* do_fsmz(fif::state_stack& s, int32_t* p, fif::environment* e) {
	char* command = nullptr;
	memcpy(&command, p + 2, 8);

	auto stype = s.main_type_back(0);

	int32_t boffset = 0;
	auto v = s.popr_main();
	for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		
		s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);

		boffset += e->dict.type_array[c].byte_size;
	}

	return p + 4;
}

inline int32_t* forth_struct_map_zero(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			execute_fif_word(mapped_function, *e, false);

			coffset += e->dict.type_array[c].cell_size;
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
			execute_fif_word(mapped_function, *e, false);

			boffset += e->dict.type_array[c].byte_size;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			{
				fif_call imm = do_fsmz;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
			{
				auto str_const = e->get_string_constant(mapped_function.content).data();
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &str_const, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
		}
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_fsmo(fif::state_stack& s, int32_t* p, fif::environment* e) {
	char* command = nullptr;
	memcpy(&command, p + 2, 8);

	auto stype = s.main_type_back(0);

	int32_t boffset = 0;
	auto v = s.popr_main();
	for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

		s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
		auto new_val = s.popr_main();
		memcpy(v.data() + boffset, new_val.data(), e->dict.type_array[c].byte_size);

		boffset += e->dict.type_array[c].byte_size;
	}

	s.push_back_main(v);

	return p + 4;
}
inline int32_t* forth_struct_map_one(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);
	auto children_count = struct_child_count(stype, *e);


	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();
			memcpy(v.data() + coffset * 8, new_val.data(), e->dict.type_array[c].cell_size * 8);

			coffset += e->dict.type_array[c].cell_size;
		}

		s.push_back_main(v);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();

			if(new_val.type != c) {
				e->mode = fif_mode::error;
				e->report_error("struct-map one did not return the same type");
				return nullptr;
			}

			memcpy(v.data() + boffset, new_val.data(), e->dict.type_array[c].byte_size);
			boffset += e->dict.type_array[c].byte_size;
		}

		s.push_back_main(v);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t coffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();
			if(new_val.type != c) {
				e->mode = fail_typechecking(e->mode);
				return nullptr;
			}

			memcpy(v.data() + coffset * 8, new_val.data(), e->dict.type_array[c].cell_size * 8);
			coffset += e->dict.type_array[c].cell_size;
		}

		s.push_back_main(v);
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			{
				fif_call imm = do_fsmo;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
			{
				auto str_const = e->get_string_constant(mapped_function.content).data();
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &str_const, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
		}
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* do_fsmt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	char* command = nullptr;
	memcpy(&command, p + 2, 8);

	auto stype = s.main_type_back(0);

	int32_t boffset = 0;
	auto v = s.popr_main();
	std::vector<unsigned char> return_data;

	for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

		s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
		auto new_val = s.popr_main();
		auto old_val = s.popr_main();
		memcpy(v.data() + boffset, old_val.data(), e->dict.type_array[c].byte_size);
		return_data.insert(return_data.end(), new_val.data(), new_val.data() + e->dict.type_array[c].byte_size);
		boffset += e->dict.type_array[c].byte_size;
	}

	s.push_back_main(v);
	s.push_back_main(vsize_obj(stype, e->dict.type_array[stype].byte_size, return_data.data()));
	return p + 4;
}
inline int32_t* forth_struct_map_two(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		auto v = s.popr_main();
		std::vector<unsigned char> return_data;

		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();
			auto old_val = s.popr_main();
			memcpy(v.data() + coffset * 8, old_val.data(), e->dict.type_array[c].cell_size * 8);
			return_data.insert(return_data.end(), new_val.data(), new_val.data() + e->dict.type_array[c].cell_size * 8);
			coffset += e->dict.type_array[c].cell_size;
		}

		s.push_back_main(v);
		s.push_back_main(vsize_obj(stype, e->dict.type_array[stype].cell_size * 8, return_data.data()));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		auto v = s.popr_main();
		std::vector<unsigned char> return_data;

		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();
			auto old_val = s.popr_main();
			if(new_val.type != old_val.type || new_val.type != c) {
				e->mode = fif_mode::error;
				e->report_error("struct-map two did not return the same type");
				return nullptr;
			}
			memcpy(v.data() + boffset, old_val.data(), e->dict.type_array[c].byte_size);
			return_data.insert(return_data.end(), new_val.data(), new_val.data() + e->dict.type_array[c].byte_size);
			boffset += e->dict.type_array[c].byte_size;
		}

		s.push_back_main(v);
		s.push_back_main(vsize_obj(stype, e->dict.type_array[stype].byte_size, return_data.data()));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t coffset = 0;
		auto v = s.popr_main();
		std::vector<unsigned char> return_data;

		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];

			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			execute_fif_word(mapped_function, *e, false);
			auto new_val = s.popr_main();
			auto old_val = s.popr_main();
			if(new_val.type != old_val.type || new_val.type != c) {
				e->mode = fail_typechecking(e->mode);
				return nullptr;
			}
			memcpy(v.data() + coffset * 8, old_val.data(), e->dict.type_array[c].cell_size * 8);
			return_data.insert(return_data.end(), new_val.data(), new_val.data() + e->dict.type_array[c].cell_size * 8);
			coffset += e->dict.type_array[c].cell_size;
		}

		s.push_back_main(v);
		s.push_back_main(vsize_obj(stype, e->dict.type_array[stype].cell_size * 8, return_data.data()));
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			{
				fif_call imm = do_fsmt;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
			{
				auto str_const = e->get_string_constant(mapped_function.content).data();
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &str_const, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			}
		}
		s.mark_used_from_main(1);
		s.push_back_main(vsize_obj(stype, 0));
	}
	return p + 2;
}

inline int32_t* memory_type_construction(state_stack& s, int32_t* p, environment* e) {
	auto type = *(p + 2);
	auto offset = *(p + 3);
	if(e->dict.type_array[type].is_memory_type()) {
		auto ptr = e->interpreter_stack_space.get() + e->frame_offset + offset;
		memset(ptr, 0, size_t(e->dict.type_array[type].byte_size));
		s.push_back_main(vsize_obj(type, ptr, vsize_obj::by_value{ }));
	} else {
		assert(false);
	}
	return p + 4;
}
inline int32_t* type_construction(state_stack& s, int32_t* p, environment* e) {
	auto type = *(p + 2);
	if(e->dict.type_array[type].is_memory_type()) {
		assert(false);
	} else {
		std::vector<unsigned char> vals;
		vals.resize(e->dict.type_array[type].byte_size, 0);
		s.push_back_main(vsize_obj(type, e->dict.type_array[type].byte_size, vals.data()));
	}
	return p + 3;
}
inline int32_t* do_make(state_stack& s, int32_t* p, environment* env) {
	if(env->source_stack.empty()) {
		env->report_error("make was unable to read the word describing the type");
		env->mode = fif_mode::error;
		return nullptr;
	}
	auto type = read_token(env->source_stack.back(), *env);

	if(skip_compilation(env->mode))
		return p + 2;

	auto resolved_type = resolve_type(type.content, *env, env->compiler_stack.back()->type_substitutions());

	if(resolved_type == -1) {
		env->report_error("make was unable to resolve the type");
		env->mode = fif_mode::error;
		return nullptr;
	}
	if(env->dict.type_array[resolved_type].ntt_base_type != -1) {
		env->report_error("make was unable to resolve the type");
		env->mode = fif_mode::error;
		return nullptr;
	}
	if(env->dict.type_array[resolved_type].is_memory_type()) {
		if(typechecking_mode(env->mode)) {
			s.push_back_main(vsize_obj(resolved_type, env->new_ident(), vsize_obj::by_value{ }));
		} else if(env->mode == fif_mode::compiling_bytecode) {
			auto offset_pos = env->lexical_stack.back().allocated_bytes;
			env->lexical_stack.back().allocated_bytes += env->dict.type_array[resolved_type].byte_size;
			env->compiler_stack.back()->increase_frame_size(env->lexical_stack.back().allocated_bytes);

			auto compile_bytes = env->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = memory_type_construction;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(resolved_type);
				compile_bytes->push_back(offset_pos);
			}
			s.push_back_main(vsize_obj(resolved_type, 0));
		} else if(env->mode == fif_mode::compiling_llvm) {
			auto new_expr = env->compiler_stack.back()->build_alloca(llvm_type(resolved_type, *env));
			s.push_back_main(vsize_obj(resolved_type, new_expr, vsize_obj::by_value{ }));
		} else if(env->mode == fif_mode::interpreting) {
			env->mode = fif_mode::error;
			env->report_error("cannot construct a memory type during direct interpretation");
			return nullptr;
		}
	} else {
		if(typechecking_mode(env->mode)) {
			if(env->dict.type_array[resolved_type].is_struct()) {
				for(auto i = 1; i < env->dict.type_array[resolved_type].decomposed_types_count - env->dict.type_array[resolved_type].non_member_types; ++i) {
					auto c = env->dict.all_stack_types[env->dict.type_array[resolved_type].decomposed_types_start + i];
					if(env->dict.type_array[c].is_memory_type()) {
						env->mode = fail_typechecking(env->mode);
						return nullptr;
					}
				}
			}

			std::vector<int64_t> vals;
			for(int32_t i = 0; i < env->dict.type_array[resolved_type].cell_size; ++i)
				vals.push_back(env->new_ident());
			s.push_back_main(vsize_obj(resolved_type, env->dict.type_array[resolved_type].cell_size * 8, (unsigned char*)(vals.data())));
		} else if(env->mode == fif_mode::compiling_bytecode) {
			auto compile_bytes = env->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = type_construction;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(resolved_type);
			}
			s.push_back_main(vsize_obj(resolved_type, 0));
		} else if(env->mode == fif_mode::compiling_llvm) {
			std::vector<LLVMValueRef> vals;
			enum_llvm_zero_constant(resolved_type, vals, *env);
			s.push_back_main(vsize_obj(resolved_type, env->dict.type_array[resolved_type].cell_size * 8, (unsigned char*)(vals.data())));
		} else if(env->mode == fif_mode::interpreting) {
			std::vector<unsigned char> vals;
			vals.resize(env->dict.type_array[resolved_type].byte_size, 0);
			s.push_back_main(vsize_obj(resolved_type, env->dict.type_array[resolved_type].byte_size, vals.data()));
		}
	}

	execute_fif_word(fif::parse_result{ "init", false }, *env, false);

	return p + 2;
}

inline int32_t* struct_definition(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to define a struct inside a definition");
		e->mode = fif::fif_mode::error;
		return nullptr;
	}

	if(e->source_stack.empty()) {
		e->report_error("attempted to define a struct without a source");
		e->mode = fif::fif_mode::error;
		return nullptr;
	}

	auto name_token = fif::read_token(e->source_stack.back(), *e);

	if(e->dict.types.find(std::string(name_token.content)) != e->dict.types.end()) {
		e->report_error("attempted to redefine an existing type");
		e->mode = fif::fif_mode::error;
		return nullptr;
	}

	std::vector<int32_t> stack_types;
	std::vector<std::string_view> names;
	int32_t max_variable = -1;
	bool read_extra_count = false;

	while(true) {
		auto next_token = fif::read_token(e->source_stack.back(), *e);
		if(next_token.content.length() == 0 || next_token.content == ";") {
			break;
		}
		if(next_token.content == "$") {
			read_extra_count = true;
			break;
		}

		auto result = internal_generate_type(next_token.content, *e);
		if(result.type_array.empty()) {
			e->mode = fif_mode::error;
			e->report_error("unable to resolve type from text");
			return nullptr;
		}
		stack_types.insert(stack_types.end(), result.type_array.begin(), result.type_array.end());
		max_variable = std::max(max_variable, result.max_variable);

		auto nnext_token = fif::read_token(e->source_stack.back(), *e);
		if(nnext_token.content.length() == 0 || nnext_token.content == ";") {
			e->report_error("struct contained a type without a matching name");
			e->mode = fif::fif_mode::error;
			return nullptr;
		}
		names.push_back(nnext_token.content);
	}

	int32_t extra_count = 0;
	if(read_extra_count) {
		auto next_token = fif::read_token(e->source_stack.back(), *e);
		auto next_next_token = fif::read_token(e->source_stack.back(), *e);
		if(next_next_token.content != ";") {
			e->report_error("struct definition ended incorrectly");
			e->mode = fif::fif_mode::error;
			return nullptr;
		}
		extra_count = parse_int(next_token.content);
	}

	make_struct_type(name_token.content, std::span<int32_t const>{stack_types.begin(), stack_types.end()}, names, * e, max_variable + 1, extra_count);

	return p + 2;
}
inline int32_t* export_definition(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to define an export inside a definition");
		e->mode = fif::fif_mode::error;
		return nullptr;
	}

	if(e->source_stack.empty()) {
		e->report_error("attempted to define an export without a source");
		e->mode = fif::fif_mode::error;
		return nullptr;
	}

	auto name_token = fif::read_token(e->source_stack.back(), *e);

	std::vector<int32_t> stack_types;
	std::vector<std::string_view> names;
	int32_t max_variable = -1;
	bool read_extra_count = false;

	while(true) {
		auto next_token = fif::read_token(e->source_stack.back(), *e);
		if(next_token.content.length() == 0 || next_token.content == ";") {
			break;
		}
		names.push_back(next_token.content);
	}

	auto fn_to_export = names.back();
	names.pop_back();

	for(auto tyn : names) {
		auto result = resolve_type(tyn, *e, nullptr);
		if(result == -1) {
			e->mode = fif_mode::error;
			e->report_error("unable to resolve type from text");
			return nullptr;
		}
		stack_types.push_back(result);
	}
#ifdef USE_LLVM
	make_exportable_function(std::string(name_token.content), std::string(fn_to_export), stack_types, { }, *e);
#endif
	return p + 2;
}
inline int32_t* do_use_base(state_stack& s, int32_t* p, environment* env) {
	if(env->source_stack.empty()) {
		env->report_error("make was unable to read name of word");
		env->mode = fif_mode::error;
		return nullptr;
	}
	auto type = read_token(env->source_stack.back(), *env);

	if(skip_compilation(env->mode))
		return p + 2;

	execute_fif_word(type, *env, true);

	return p + 2;
}

inline int32_t* long_comment(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		return p + 2;
	}

	int32_t depth = 1;
	while(depth > 0) {
		auto token = fif::read_token(e->source_stack.back(), *e);
		if(token.content == "))" && token.is_string == false)
			--depth;
		if(token.content == "((" && token.is_string == false)
			++depth;
		if(token.content == "" && token.is_string == false)
			break;
	}

	return p + 2;
}
inline int32_t* line_comment(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		return p + 2;
	}

	std::string_view& source = e->source_stack.back();
	while(source.length() > 0 && source[0] != '\n' && source[0] != '\r')
		source = source.substr(1);

	return p + 2;
}

inline int32_t* zext_i64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i64, int64_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i32, int32_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i16, int16_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i8, int8_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* zext_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildZExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(uint8_t(a.as<bool>())), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_i64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i64, int64_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i64, int64_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u64, int64_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u64, int64_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u32, int32_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u32, int32_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u32, e->new_ident(), vsize_obj::by_value{ }));
	}

	return p + 2;
}
inline int32_t* sext_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u16, int16_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u16, int16_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sext_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int32_t(a.as<int32_t>())), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int32_t(a.as<uint32_t>())), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int64_t(a.as<int64_t>())), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int64_t(a.as<uint64_t>())), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int16_t(a.as<int16_t>())), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int16_t(a.as<uint16_t>())), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int8_t(a.as<int8_t>())), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u8, int8_t(int8_t(a.as<uint8_t>())), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u8, int8_t(a.as<bool>() ? -1 : 0), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_i1(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt1TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_bool, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<int32_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<uint32_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<int64_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<uint64_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<int16_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<uint16_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<int8_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<uint8_t>() & 0x01), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_bool, bool(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_bool, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* trunc_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* nop1(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		s.mark_used_from_main(1);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* ftrunc(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPTrunc(e->llvm_builder, a.as<LLVMValueRef>(), LLVMFloatTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_f32, a.as<float>(), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fext(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPExt(e->llvm_builder, a.as<LLVMValueRef>(), LLVMDoubleTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_f64, a.as<double>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fti8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToSI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_i8, int8_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sif32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSIToFP(e->llvm_builder, a.as<LLVMValueRef>(), LLVMFloatTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uif32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildUIToFP(e->llvm_builder, a.as<LLVMValueRef>(), LLVMFloatTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_f32, float(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* sif64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildSIToFP(e->llvm_builder, a.as<LLVMValueRef>(), LLVMDoubleTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* uif64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildUIToFP(e->llvm_builder, a.as<LLVMValueRef>(), LLVMDoubleTypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_f64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(fif_f64, double(a.as<bool>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_f64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fti16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToSI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_i16, int16_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fti32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToSI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_i32, int32_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* fti64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToSI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_i64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_i64, int64_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_i64, int64_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_i64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ftui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToUI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt8TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u8, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_u8, uint8_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u8, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ftui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToUI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt16TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u16, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_u16, uint16_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u16, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ftui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToUI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt32TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u32, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_u32, uint32_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u32, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* ftui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto result = LLVMBuildFPToUI(e->llvm_builder, a.as<LLVMValueRef>(), LLVMInt64TypeInContext(e->llvm_context), "");
		s.push_back_main(vsize_obj(fif_u64, result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_f32:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(a.as<float>()), vsize_obj::by_value{ })); break;
			case fif_f64:
				s.push_back_main(vsize_obj(fif_u64, uint64_t(a.as<double>()), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(vsize_obj(fif_u64, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

inline int32_t* bit_and(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildAnd(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() & a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() & a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() & a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() & a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() & a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() & a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() & a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() & a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_or(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildOr(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() | a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() | a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() | a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() | a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() | a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() | a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() | a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() | a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() || a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_xor(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildXor(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() ^ a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() ^ a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() ^ a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() ^ a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() ^ a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() ^ a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() ^ a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() ^ a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() != a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_not(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto add_result = LLVMBuildNot(e->llvm_builder, a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, ~a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, ~a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, ~a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, ~a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type,  int16_t(~a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type,  uint16_t(~a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(~a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(~a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_shl(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildShl(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, b.as<int32_t>() << a.as<int32_t>(), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, b.as<uint32_t>() << a.as<uint32_t>(), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, b.as<int64_t>() << a.as<int64_t>(), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, b.as<uint64_t>() << a.as<uint64_t>(), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() << a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() << a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() << a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() << a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_ashr(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildAShr(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, int32_t(b.as<int32_t>() >> a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, uint32_t(b.as<int32_t>() >> a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, int64_t(b.as<int64_t>() >> a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, uint64_t(b.as<int64_t>() >> a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<int16_t>() >> a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<int16_t>() >> a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<int8_t>() >> a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<int8_t>() >> a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}
inline int32_t* bit_lshr(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto a = s.popr_main();
		auto b = s.popr_main();
		auto add_result = LLVMBuildLShr(e->llvm_builder, b.as<LLVMValueRef>(), a.as<LLVMValueRef>(), "");
		s.push_back_main(vsize_obj(a.type, add_result, vsize_obj::by_value{ }));
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.popr_main();
		auto b = s.popr_main();
		switch(a.type) {
			case fif_i32:
				s.push_back_main(vsize_obj(a.type, int32_t(b.as<uint32_t>() >> a.as<int32_t>()), vsize_obj::by_value{ })); break;
			case fif_u32:
				s.push_back_main(vsize_obj(a.type, uint32_t(b.as<uint32_t>() >> a.as<uint32_t>()), vsize_obj::by_value{ })); break;
			case fif_i64:
				s.push_back_main(vsize_obj(a.type, int64_t(b.as<uint64_t>() >> a.as<int64_t>()), vsize_obj::by_value{ })); break;
			case fif_u64:
				s.push_back_main(vsize_obj(a.type, uint64_t(b.as<uint64_t>() >> a.as<uint64_t>()), vsize_obj::by_value{ })); break;
			case fif_i16:
				s.push_back_main(vsize_obj(a.type, int16_t(b.as<uint16_t>() >> a.as<int16_t>()), vsize_obj::by_value{ })); break;
			case fif_u16:
				s.push_back_main(vsize_obj(a.type, uint16_t(b.as<uint16_t>() >> a.as<uint16_t>()), vsize_obj::by_value{ })); break;
			case fif_i8:
				s.push_back_main(vsize_obj(a.type, int8_t(b.as<uint8_t>() >> a.as<int8_t>()), vsize_obj::by_value{ })); break;
			case fif_u8:
				s.push_back_main(vsize_obj(a.type, uint8_t(b.as<uint8_t>() >> a.as<uint8_t>()), vsize_obj::by_value{ })); break;
			case fif_bool:
				s.push_back_main(vsize_obj(a.type, b.as<bool>() && !a.as<bool>(), vsize_obj::by_value{ })); break;
			default:
				assert(false); break;
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto t = s.main_type_back(0);
		s.pop_main();
		s.pop_main();
		s.push_back_main(vsize_obj(t, e->new_ident(), vsize_obj::by_value{ }));
	}
	return p + 2;
}

inline int32_t* insert_stack_token(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(!fif::skip_compilation(e->mode))
		s.push_back_main(vsize_obj(fif_stack_token, 0));
	if(e->mode == fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = insert_stack_token;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	}
	return p + 2;
}

inline int32_t* memcpy_to_local(state_stack& s, int32_t* p, environment* e) {
	auto offset = *(p + 2);
	auto local_count = *(p + 3);
	auto type = *(p + 4);

	auto bytes_down = s.main_byte_back_at(local_count);
	auto bytes_total = s.main_byte_size();
	auto source_ptr = (s.main_ptr_at(0) + bytes_total) - bytes_down;

	auto ptr = e->interpreter_stack_space.get() + e->frame_offset + offset;
	memcpy(ptr, source_ptr, bytes_total);
	s.push_back_main(vsize_obj(type, ptr, vsize_obj::by_value{ }));
	
	s.resize(size_t(s.main_size() - local_count), s.return_size());

	return p + 5;
}
inline int32_t* arrayify(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	std::vector<int32_t> types;
	int32_t etype = -1;
	types.push_back(fif::fif_array);
	types.push_back(std::numeric_limits<int32_t>::max());

	size_t i = 0;
	int32_t st_depth = 0;
	for(; i < s.main_size(); ++i) {
		s.mark_used_from_main(i + 1);
		if(s.main_type_back(i) == fif::fif_stack_token) {
			break;
		}
		++st_depth;
		if(etype == -1) {
			types.push_back(s.main_type_back(i));
			etype = s.main_type_back(i);
		} else if(etype != s.main_type_back(i)) {
			e->mode = fif_mode::error;
			e->report_error("attempted to make an array of heterogeneous types");
			return p + 2;
		}
	}
	if(i == s.main_size()) {
		if(typechecking_mode(e->mode)) {
			e->mode = fail_typechecking(e->mode);
		} else {
			e->mode = fif_mode::error;
			e->report_error("attempted to make an array without a stack token present");
		}
		return p + 2;
	}
	types.push_back(-1);

	auto resolved_ar_type = resolve_span_type(types, { }, *e);
	if(typechecking_mode(e->mode)) {
		while(s.main_size() > 0) {
			if(s.main_type_back(0) == fif::fif_stack_token) {
				s.pop_main();
				break;
			}
			s.pop_main();
		}
		s.push_back_main(vsize_obj(resolved_ar_type.type, e->new_ident(), vsize_obj::by_value{ }));
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto offset_pos = e->lexical_stack.back().allocated_bytes;
		e->lexical_stack.back().allocated_bytes += e->dict.type_array[resolved_ar_type.type].byte_size;
		e->compiler_stack.back()->increase_frame_size(e->lexical_stack.back().allocated_bytes);

		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = memcpy_to_local;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			
			compile_bytes->push_back(offset_pos);
			compile_bytes->push_back(st_depth + 1);
			compile_bytes->push_back(resolved_ar_type.type);
		}

		while(s.main_size() > 0) {
			if(s.main_type_back(0) == fif::fif_stack_token) {
				s.pop_main();
				break;
			}
			s.pop_main();
		}

		s.push_back_main(vsize_obj(resolved_ar_type.type, 0));
	} else if(e->mode == fif_mode::interpreting) {
		// TODO: interpreter alloca
		assert(false);
	} else if(e->mode == fif_mode::compiling_llvm) {
		if(e->dict.type_array[resolved_ar_type.type].stateless()) {
			s.push_back_main(vsize_obj(resolved_ar_type.type, 0));
		} else {
			auto array_type = llvm_type(resolved_ar_type.type, *e);
			auto member_type = llvm_type(types[2], *e);
			auto new_expr = e->compiler_stack.back()->build_alloca(array_type);
			s.push_back_main(vsize_obj(resolved_ar_type.type, new_expr, vsize_obj::by_value{ }));

			uint32_t i = 0;
			while(s.main_size() > 0) {
				if(s.main_type_back(0) == fif::fif_stack_token) {
					s.pop_main();
					break;
				}
				auto index = LLVMConstInt(LLVMInt32TypeInContext(e->llvm_context), i, false);
				auto store_target = LLVMBuildInBoundsGEP2(e->llvm_builder, array_type, new_expr, &index, 1, "");
				if(e->dict.type_array[types[2]].is_memory_type()) {
					auto source_ptr = s.popr_main().as<LLVMValueRef>();
					LLVMBuildMemCpy(e->llvm_builder, store_target, 1, source_ptr, 1, LLVMSizeOf(member_type));
				} else {
					store_to_llvm_pointer(types[2], s, store_target, *e);
				}
				++i;
				
			}
		}
	}

	return p + 2;
}

inline int32_t* comp_structify(state_stack& s, int32_t* p, environment* e) {
	auto local_count = *(p + 2);
	auto type = *(p + 3);

	auto bytes_down = s.main_byte_back_at(local_count);
	auto bytes_total = s.main_byte_size();
	auto source_ptr = (s.main_ptr_at(0) + bytes_total) - bytes_down;

	vsize_obj newobj(type, bytes_down, source_ptr);

	s.resize(size_t(s.main_size() - local_count), s.return_size());
	s.push_back_main(newobj);

	return p + 4;
}

inline int32_t* structify(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	std::vector<int32_t> types;
	types.push_back(fif::fif_anon_struct);
	types.push_back(std::numeric_limits<int32_t>::max());

	size_t i = 0;
	int32_t st_depth = 0;
	for(; i < s.main_size(); ++i) {
		s.mark_used_from_main(i + 1);
		if(s.main_type_back(i) == fif::fif_stack_token) {
			break;
		}
		++st_depth;
	}
	for(size_t j = i; j-- > 0;) {
		types.push_back(s.main_type_back(j));
	}
	if(i == s.main_size()) {
		if(typechecking_mode(e->mode)) {
			e->mode = fail_typechecking(e->mode);
		} else {
			e->mode = fif_mode::error;
			e->report_error("attempted to make an anonymous struct without a stack token present");
		}
		return p + 2;
	}
	types.push_back(-1);

	auto resolved_struct_type = resolve_span_type(types, { }, *e);
	if(typechecking_mode(e->mode)) {
		auto bytes_down = s.main_byte_back_at(st_depth);
		auto bytes_total = s.main_byte_size();
		auto source_ptr = (s.main_ptr_at(0) + bytes_total) - bytes_down;

		vsize_obj newobj(resolved_struct_type.type, bytes_down, source_ptr);

		s.resize(size_t(s.main_size() - (st_depth + 1)), s.return_size());
		s.push_back_main(newobj);
	} else if(e->mode == fif_mode::compiling_bytecode) {
		while(s.main_size() > 0) {
			if(s.main_type_back(0) == fif::fif_stack_token) {
				s.pop_main();
				break;
			}
			s.pop_main();
		}
		

		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = comp_structify;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(st_depth + 1);
			compile_bytes->push_back(resolved_struct_type.type);
		}

		s.push_back_main(vsize_obj(resolved_struct_type.type,0));
	} else if(e->mode == fif_mode::interpreting) {
		auto bytes_down = s.main_byte_back_at(st_depth);
		auto bytes_total = s.main_byte_size();
		auto source_ptr = (s.main_ptr_at(0) + bytes_total) - bytes_down;

		vsize_obj newobj(resolved_struct_type.type, bytes_down, source_ptr);

		s.resize(size_t(s.main_size() - (st_depth + 1)), s.return_size());
		s.push_back_main(newobj);
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto bytes_down = s.main_byte_back_at(st_depth);
		auto bytes_total = s.main_byte_size();
		auto source_ptr = (s.main_ptr_at(0) + bytes_total) - bytes_down;

		vsize_obj newobj(resolved_struct_type.type, bytes_down, source_ptr);

		s.resize(size_t(s.main_size() - (st_depth + 1)), s.return_size());
		s.push_back_main(newobj);
	}

	return p + 2;
}

inline int32_t* comp_destruct(state_stack& s, int32_t* p, environment* e) {
	auto stype = *(p + 2);

	int32_t boffset = 0;
	auto v = s.popr_main();
	for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
		auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
		s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
		boffset += e->dict.type_array[c].byte_size;
	}
	return p + 3;
}

inline int32_t* de_struct(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	if(s.main_size() == 0) {
		if(typechecking_mode(e->mode)) {
			e->mode = fail_typechecking(e->mode);
		} else {
			e->mode = fif_mode::error;
			e->report_error("attempted to de-struct an empty stack");
		}
		return p + 2;
	}

	auto stype = s.main_type_back(0);
	auto children_count = struct_child_count(stype, *e);

	if(e->dict.type_array[stype].decomposed_types_count == 0 || e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start] != fif_anon_struct) {
		if(typechecking_mode(e->mode)) {
			e->mode = fail_typechecking(e->mode);
		} else {
			e->mode = fif_mode::error;
			e->report_error("attempted to de-struct something other than an anonymous structure");
		}
		return p + 2;
	}

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		int32_t coffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			coffset += e->dict.type_array[c].cell_size;
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		int32_t boffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			s.push_back_main(vsize_obj(c, e->dict.type_array[c].byte_size, v.data() + boffset));
			boffset += e->dict.type_array[c].byte_size;
		}
	} else if(fif::typechecking_mode(e->mode)) {
		int32_t coffset = 0;
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			s.push_back_main(vsize_obj(c, e->dict.type_array[c].cell_size * 8, v.data() + coffset * 8));
			coffset += e->dict.type_array[c].cell_size;
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto v = s.popr_main();
		for(auto i = 1; i < e->dict.type_array[stype].decomposed_types_count - e->dict.type_array[stype].non_member_types; ++i) {
			auto c = e->dict.all_stack_types[e->dict.type_array[stype].decomposed_types_start + i];
			s.push_back_main(vsize_obj(c, 0));
		}

		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = comp_destruct;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(stype);
		}
	}
	return p + 2;
}

inline void initialize_standard_vocab(environment& fif_env) {
	add_precompiled(fif_env, ":", colon_definition, { });
	add_precompiled(fif_env, ":s", colon_specialization, { });
	add_precompiled(fif_env, "((", long_comment, { }, true);
	add_precompiled(fif_env, "--", line_comment, { }, true);

	add_precompiled(fif_env, "+", iadd, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "+", iadd, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "+", fadd, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "+", fadd, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "-", isub, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "-", fsub, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "-", fsub, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "/", sidiv, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "/", fdiv, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "/", fdiv, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "mod", simod, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "mod", f_mod, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "mod", f_mod, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "*", imul, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "*", fmul, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "*", fmul, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "<", ilt, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", ile, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", igt, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", ige, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_i32, fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", ilt, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", ile, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", igt, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", ige, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_i64, fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", ilt, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", ile, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", igt, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", ige, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_i16, fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", ilt, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", ile, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", igt, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", ige, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_i8, fif::fif_i8, -1, fif::fif_bool });

	add_precompiled(fif_env, "<", uilt, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", uile, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", uigt, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", uige, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_u32, fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", uilt, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", uile, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", uigt, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", uige, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_u64, fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", uilt, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", uile, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", uigt, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", uige, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_u16, fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", uilt, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", uile, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", uigt, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", uige, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", ieq, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_u8, fif::fif_u8, -1, fif::fif_bool });

	add_precompiled(fif_env, "<", flt, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", fle, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", fgt, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", fge, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", feq, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", fne, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", flt, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", fle, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", fgt, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", fge, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", feq, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", fne, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });

	add_precompiled(fif_env, "=", ieq, { fif::fif_bool, fif::fif_bool, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", ine, { fif::fif_bool, fif::fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, ">i64", zext_i64, { fif::fif_u64, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", zext_i64, { fif::fif_u32, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", zext_i64, { fif::fif_u16, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", zext_i64, { fif::fif_u8, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", nop1, { fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", sext_i64, { fif::fif_i32, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", sext_i64, { fif::fif_i16, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", sext_i64, { fif::fif_i8, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", zext_i64, { fif::fif_bool, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", fti64, { fif::fif_f32, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", fti64, { fif::fif_f64, -1, fif::fif_i64 });

	add_precompiled(fif_env, ">u64", nop1, { fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u16, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u8, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i64, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i16, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i8, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_bool, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", ftui64, { fif::fif_f32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", ftui64, { fif::fif_f64, -1, fif::fif_u64 });

	add_precompiled(fif_env, ">i32", trunc_i32, { fif::fif_u64, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u16, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u8, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", trunc_i32, { fif::fif_i64, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", nop1, { fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", sext_i32, { fif::fif_i16, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", sext_i32, { fif::fif_i8, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_bool, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", fti32, { fif::fif_f32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", fti32, { fif::fif_f64, -1, fif::fif_i32 });

	add_precompiled(fif_env, ">u32", trunc_ui32, { fif::fif_u64, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", nop1, { fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_u16, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_u8, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", trunc_ui32, { fif::fif_i64, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i16, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i8, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_bool, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", ftui32, { fif::fif_f32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", ftui32, { fif::fif_f64, -1, fif::fif_u32 });

	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_u64, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_u32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_u16, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_u8, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_i64, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_i32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", nop1, { fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", sext_i16, { fif::fif_i8, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_bool, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", fti16, { fif::fif_f32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", fti16, { fif::fif_f64, -1, fif::fif_i16 });

	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_u64, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_u32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", nop1, { fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", zext_ui16, { fif::fif_u8, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_i64, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_i32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", sext_ui16, { fif::fif_i16, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", sext_ui16, { fif::fif_i8, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", zext_ui16, { fif::fif_bool, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", ftui16, { fif::fif_f32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", ftui16, { fif::fif_f64, -1, fif::fif_u16 });

	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u64, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u16, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", zext_i8, { fif::fif_u8, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i64, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i16, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", nop1, { fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", zext_i8, { fif::fif_bool, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", fti8, { fif::fif_f32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", fti8, { fif::fif_f64, -1, fif::fif_i8 });

	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u64, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u16, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", nop1, { fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i64, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i16, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", sext_ui8, { fif::fif_i8, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", zext_ui8, { fif::fif_bool, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", ftui8, { fif::fif_f32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", ftui8, { fif::fif_f64, -1, fif::fif_u8 });

	add_precompiled(fif_env, ">f32", uif32, { fif::fif_u64, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", uif32, { fif::fif_u32, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", uif32, { fif::fif_u16, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", uif32, { fif::fif_u8, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", sif32, { fif::fif_i64, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", sif32, { fif::fif_i32, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", sif32, { fif::fif_i16, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", sif32, { fif::fif_i8, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", nop1, { fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, ">f32", ftrunc, { fif::fif_f64, -1, fif::fif_f32 });

	add_precompiled(fif_env, ">f64", uif64, { fif::fif_u64, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", uif64, { fif::fif_u32, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", uif64, { fif::fif_u16, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", uif64, { fif::fif_u8, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", sif64, { fif::fif_i64, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", sif64, { fif::fif_i32, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", sif64, { fif::fif_i16, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", sif64, { fif::fif_i8, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", fext, { fif::fif_f32, -1, fif::fif_f64 });
	add_precompiled(fif_env, ">f64", nop1, { fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_u64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_u32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_u16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_u8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_i64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_i32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_i16, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", trunc_i1, { fif::fif_i8, -1, fif::fif_bool });
	add_precompiled(fif_env, ">bool", nop1, { fif::fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, "and", bit_and, { fif::fif_u64, fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_u32, fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_u16, fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_u8, fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_i64, fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_i32, fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_i16, fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_i8, fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "and", bit_and, { fif::fif_bool, fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, "or", bit_or, { fif::fif_u64, fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_u32, fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_u16, fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_u8, fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_i64, fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_i32, fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_i16, fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_i8, fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "or", bit_or, { fif::fif_bool, fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_u64, fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_u32, fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_u16, fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_u8, fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_i64, fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_i32, fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_i16, fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_i8, fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "xor", bit_xor, { fif::fif_bool, fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, "not", bit_not, { fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "not", bit_not, { fif::fif_bool, -1, fif::fif_bool });

	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_u64, fif_i32, -1, fif::fif_u64 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_i64, fif_i32, -1, fif::fif_i64 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_u32, fif_i32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_i32, fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_u16, fif_i32, -1, fif::fif_u16 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_i16, fif_i32, -1, fif::fif_i16 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_u8, fif_i32, -1, fif::fif_u8 });
	add_precompiled(fif_env, "shl", bit_shl, { fif::fif_i8, fif_i32, -1, fif::fif_i8 });

	add_precompiled(fif_env, "shr", bit_lshr, { fif::fif_u64, fif_i32, -1, fif::fif_u64 });
	add_precompiled(fif_env, "shr", bit_ashr, { fif::fif_i64, fif_i32, -1, fif::fif_i64 });
	add_precompiled(fif_env, "shr", bit_lshr, { fif::fif_u32, fif_i32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "shr", bit_ashr, { fif::fif_i32, fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "shr", bit_lshr, { fif::fif_u16, fif_i32, -1, fif::fif_u16 });
	add_precompiled(fif_env, "shr", bit_ashr, { fif::fif_i16, fif_i32, -1, fif::fif_i16 });
	add_precompiled(fif_env, "shr", bit_lshr, { fif::fif_u8, fif_i32, -1, fif::fif_u8 });
	add_precompiled(fif_env, "shr", bit_ashr, { fif::fif_i8, fif_i32, -1, fif::fif_i8 });

	add_precompiled(fif_env, "init", init, { }, true);
	add_precompiled(fif_env, "dup", dup, { }, true);
	add_precompiled(fif_env, "copy", dup, { }, true);
	add_precompiled(fif_env, "drop", drop, { }, true);
	add_precompiled(fif_env, "swap", fif_swap, {  }, true);
	
	add_precompiled(fif_env, "lex-scope", lex_scope, { }, true);
	add_precompiled(fif_env, "end-lex", lex_scope_end, { }, true);
	add_precompiled(fif_env, "if", fif_if, { }, true);
	add_precompiled(fif_env, "else", fif_else, { }, true);
	add_precompiled(fif_env, "&if", fif_and_if, { }, true);
	add_precompiled(fif_env, "then", fif_then, { }, true);
	add_precompiled(fif_env, "end-if", fif_then, { }, true);
	add_precompiled(fif_env, "while", fif_while, { }, true);
	add_precompiled(fif_env, "loop", fif_loop, { }, true);
	add_precompiled(fif_env, "end-while", fif_end_while, { }, true);
	add_precompiled(fif_env, "do", fif_do, { }, true);
	add_precompiled(fif_env, "until", fif_until, { }, true);
	add_precompiled(fif_env, "end-do", fif_end_do, { fif::fif_bool }, true);
	add_precompiled(fif_env, "break", fif_break, { }, true);
	add_precompiled(fif_env, "return", fif_return, { }, true);

	add_precompiled(fif_env, ">r", to_r, { -2, -1, -1, -1, -2 }, true);
	add_precompiled(fif_env, "r>", from_r, { -1, -2, -1, -2 }, true);
	add_precompiled(fif_env, "r@", r_at, { -1, -2, -1, -2, -1, -2 }, true);
	add_precompiled(fif_env, "immediate", make_immediate, { });
	add_precompiled(fif_env, "[", open_bracket, { }, true);
	add_precompiled(fif_env, "]", close_bracket, { }, true);

	add_precompiled(fif_env, "ptr-cast", pointer_cast, { fif_opaque_ptr }, true);
	add_precompiled(fif_env, "ptr-cast", pointer_cast, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1 }, true);
	add_precompiled(fif_env, "heap-alloc", impl_heap_allot, { }, true);
	add_precompiled(fif_env, "heap-free", impl_heap_free, { }, true);
	add_precompiled(fif_env, "@", impl_load, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1, -1, -2 });
	add_precompiled(fif_env, "@@", impl_load_deallocated, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1, -1, -2 });
	add_precompiled(fif_env, "!", impl_store, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1, -2 });
	add_precompiled(fif_env, "!!", impl_uninit_store, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1, -2 });
	add_precompiled(fif_env, "sizeof", impl_sizeof, { -1, fif_i32 }, true);
	add_precompiled(fif_env, "buf-free", free_buffer, { fif_opaque_ptr });
	add_precompiled(fif_env, "buf-alloc", allocate_buffer, { fif_i32, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-alloc", allocate_buffer, { fif_i64, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-alloc", allocate_buffer, { fif_i16, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-alloc", allocate_buffer, { fif_i8, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-copy", copy_buffer, { fif_i32, fif_opaque_ptr, fif_opaque_ptr, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-copy", copy_buffer, { fif_i64, fif_opaque_ptr, fif_opaque_ptr, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-copy", copy_buffer, { fif_i16, fif_opaque_ptr, fif_opaque_ptr, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-copy", copy_buffer, { fif_i8, fif_opaque_ptr, fif_opaque_ptr, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-add", impl_index, { fif_opaque_ptr, fif_i32, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-add", impl_index, { fif_opaque_ptr, fif_i64, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-add", impl_index, { fif_opaque_ptr, fif_i16, -1, fif_opaque_ptr });
	add_precompiled(fif_env, "buf-add", impl_index, { fif_opaque_ptr, fif_i8, -1, fif_opaque_ptr });

	add_precompiled(fif_env, "let", create_let, { -2 }, true);
	add_precompiled(fif_env, "->", create_relet, { -2 }, true);
	add_precompiled(fif_env, "var", create_var, { -2 }, true);
	add_precompiled(fif_env, "(", create_params, { }, true);
	add_precompiled(fif_env, "global", create_global_impl, { fif_type });

	add_precompiled(fif_env, "forth.insert", forth_insert, { }, true);
	add_precompiled(fif_env, "forth.extract", forth_extract, { }, true);
	add_precompiled(fif_env, "forth.extract-copy", forth_extract_copy, { }, true);
	add_precompiled(fif_env, "forth.gep", forth_gep, { }, true);
	add_precompiled(fif_env, "struct-map2", forth_struct_map_two, { }, true);
	add_precompiled(fif_env, "struct-map1", forth_struct_map_one, { }, true);
	add_precompiled(fif_env, "struct-map0", forth_struct_map_zero, { }, true);
	add_precompiled(fif_env, "make", do_make, { }, true);
	add_precompiled(fif_env, ":struct", struct_definition, { });
	add_precompiled(fif_env, ":export", export_definition, { });
	add_precompiled(fif_env, "use-base", do_use_base, { }, true);
	add_precompiled(fif_env, "{", insert_stack_token, { }, true);
	add_precompiled(fif_env, "}struct", structify, { }, true);
	add_precompiled(fif_env, "de-struct", de_struct, { }, true);
	add_precompiled(fif_env, "}array", arrayify, { }, true);
	//add_precompiled(fif_env, "forth.array-load", forth_array_load, { }, true);
	//add_precompiled(fif_env, "forth.array-store", forth_array_store, { }, true);

	add_precompiled(fif_env, "select", f_select, { }, true);

	auto preinterpreted =
		": over >r dup r> swap ; "
		": nip >r drop r> ; "
		": tuck swap over ; "
		": 2dup >r dup r> dup >r swap r> ; "
		":s size array($0,$1) s:" // array -> array, int
		"	$1 "
		" ; "
		":s drop array($0,$1) s:" // array -> array, int
		"	array-map0 drop "
		" ; "
		":s dup array($0,$1) s:" // array -> array, int
		"	array-map2 dup "
		" ; "
		":s copy array($0,$1) s:" // array -> array, int
		"	array-map2 copy "
		" ; "
		":s init array($0,$1) s:" // array -> array, int
		"	array-map1 init "
		" ; "
		":s index@ i32 array($0,$1) s:" // array -> array, int
		"	forth.array-load "
		" ; "
		":s index! $0 i32 array($0,$1) s:" // array -> array, int
		"	forth.array-store "
		" ; "
		": buf-resize " // ptr old new -> ptr
		"	buf-alloc swap >r >r dup r> r> buf-copy swap buf-free "
		" ; "
		":struct dy-array-block ptr(nil) memory i32 size i32 capacity i32 refcount ; "
		":struct dy-array ptr(dy-array-block) ptr $ 1 ; "
		":s init dy-array($0) s: " // array -> array
		"	sizeof dy-array-block buf-alloc ptr-cast ptr(dy-array-block) swap .ptr! "
		" ; "
		":s dup dy-array($0) s: " // array -> array, array
		"	use-base dup .ptr@ " // original copy ptr-to-block
		"	.refcount dup @ 1 +  swap ! " // increase ref count
		" ; "
		":s drop dy-array($0) s: .ptr@ " // array ->
		"	.refcount dup @ -1 + " // decrement refcount
		"	dup 0 >= if "
		"		swap ! " // store refcount back into pointer
		"	else "
		"		drop drop " // drop pointer to refcount and -1
		"		.ptr@ dup .size let sz .memory let mem " // grab values
		"		while "
		"			sz @ 0 > "
		"		loop "
		"			sz @ -1 + sz ! " // reduce sz
		"			sz @ sizeof $0 * mem @ buf-add ptr-cast ptr($0) @@ " // copy last value, dupless
		"			drop " // run its destructor
		"		end-while "
		"		mem @ buf-free " // free managed buffer
		"		.ptr@ ptr-cast ptr(nil) buf-free " // destroy control block
		"	end-if "
		"	use-base drop "
		" ; "
		":s push dy-array($0) $0 s: " // array , value -> array
		"	let val " // store value to be saved in val
		"	.ptr@ "
		"	dup dup .size let sz .capacity let cap .memory let mem" // destructure
		"	sz @ sizeof $0 * cap @ >= if " // size >= capacity ?
		"		mem @ " // put old buffer on stack
		"		cap @ sizeof $0 * " // old size
		"		cap @ 1 + 2 * sizeof $0 * " // new size
		"		dup cap ! " // copy new size to capacity
		"		buf-resize "
		"		mem ! "
		"	end-if "
		"	sz @ sizeof $0 * mem @ buf-add ptr-cast ptr($0) val swap !! " // move value into last position
		"	sz @ 1 + sz ! " // +1 to stored size
		" ; "
		":s pop dy-array($0) s: " // array -> array , value
		"	.ptr@ " // ptr to control block on stack
		"	dup .size let sz .memory let mem" // destructure
		"	sz @ 0 > if "
		"		sz @ -1 + sz ! " // reduce sz
		"		sz @ sizeof $0 * mem @ buf-add ptr-cast ptr($0) @@ " // copy last value, dupless
		"	else "
		"		make $0 " // nothing left, make new value
		"	end-if "
		" ; "
		":s empty? dy-array($0) s: " // array -> array, value
		"	.ptr@ .size @ <= 0 "
		" ; "
		":s index-into dy-array($0) i32 s:" // array, int -> array, ptr(0)
		"	let index .ptr@ .memory @ sizeof $0 index * swap buf-add ptr-cast ptr($0) "
		" ; "
		":s size dy-array($0) s:" // array -> array, int
		"	.ptr@ .size @ "
		" ; "
		;

	fif::interpreter_stack values{ };
	fif::run_fif_interpreter(fif_env, preinterpreted, values);

}

}

