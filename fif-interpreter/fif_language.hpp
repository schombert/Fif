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
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildAdd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b + a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* f32_add(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFAdd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
#endif
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
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64_add(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFAdd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f64, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &a, 8);
		memcpy(&fb, &b, 8);
		fa = fa + fb;
		memcpy(&a, &fa, 8);
		s.push_back_main(fif::fif_f64, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* isub(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildSub(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b - a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* f32_sub(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFSub(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &a, 4);
		memcpy(&fb, &b, 4);
		fa = fa - fb;
		memcpy(&a, &fa, 4);
		s.push_back_main(fif::fif_f32, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64_sub(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFSub(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f64, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &a, 8);
		memcpy(&fb, &b, 8);
		fa = fa - fb;
		memcpy(&a, &fa, 8);
		s.push_back_main(fif::fif_f64, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* imul(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildMul(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b * a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* f32_mul(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFMul(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &a, 4);
		memcpy(&fb, &b, 4);
		fa = fb * fa;
		memcpy(&a, &fa, 4);
		s.push_back_main(fif::fif_f32, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64_mul(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFMul(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f64, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &a, 8);
		memcpy(&fb, &b, 8);
		fa = fb * fa;
		memcpy(&a, &fa, 8);
		s.push_back_main(fif::fif_f64, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sidiv(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildSDiv(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b / a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* uidiv(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildUDiv(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.set_main_data_back(0, b / a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* f32_div(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFDiv(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &a, 4);
		memcpy(&fb, &b, 4);
		fa = fb / fa;
		memcpy(&a, &fa, 4);
		s.push_back_main(fif::fif_f32, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64_div(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFDiv(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f64, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &a, 8);
		memcpy(&fb, &b, 8);
		fa = fb / fa;
		memcpy(&a, &fa, 8);
		s.push_back_main(fif::fif_f64, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* simod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildSRem(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b % a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* uimod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto add_result = LLVMBuildURem(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.set_main_data_back(0, b % a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* f32_mod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFRem(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f32, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &a, 4);
		memcpy(&fb, &b, 4);
		fa = fmodf(fb, fa);
		memcpy(&a, &fa, 4);
		s.push_back_main(fif::fif_f32, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64_mod(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto add_result = LLVMBuildFDiv(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_f64, 0, add_result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &a, 8);
		memcpy(&fb, &b, 8);
		fa = fmod(fb, fa);
		memcpy(&a, &fa, 8);
		s.push_back_main(fif::fif_f64, a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* dup(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto expr = s.main_ex_back(0);
		s.push_back_main(type, 0, expr);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
	}
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
inline int32_t* copy(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto expr = s.main_ex_back(0);
		s.push_back_main(type, 0, expr);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		auto type = s.main_type_back(0);
		auto dat = s.main_data_back(0);
		s.push_back_main(type, dat, nullptr);
	}
	return p + 2;
}
inline int32_t* drop(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type = s.main_type_back(0);
		s.pop_main();
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto type = s.main_type_back(0);
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
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
		s.pop_main();
		s.pop_main();
		s.push_back_main(type_a, dat_a, nullptr);
		s.push_back_main(type_b, dat_b, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto type_a = s.main_type_back(0);
		auto type_b = s.main_type_back(1);
		auto dat_a = s.main_data_back(0);
		auto dat_b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(type_a, dat_a, nullptr);
		s.push_back_main(type_b, dat_b, nullptr);
	}
	return p + 2;
}


inline int32_t* lex_scope(fif::state_stack& s, int32_t* p, fif::environment* e) {
	e->compiler_stack.emplace_back(std::make_unique<fif::lexical_scope>(e->compiler_stack.back().get(), *e));
	return p + 2;
}
inline int32_t* lex_scope_end(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->compiler_stack.empty() || e->compiler_stack.back()->get_type() != fif::control_structure::lexical_scope) {
		e->report_error("lexical scope ended in incorrect context");
		e->mode = fif::fif_mode::error;
		return nullptr;
	} else {
		e->compiler_stack.back()->finish(*e);
		e->compiler_stack.pop_back();
	}
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
			} else if(s_top->get_type() == fif::control_structure::rt_function) {
				auto saved_fn = s_top;
				s_top = e->compiler_stack.back().get();
				while(s_top != saved_fn) {
					s_top->delete_locals();
					if(s_top->get_type() == control_structure::mode_switch)
						s_top = static_cast<mode_switch_scope*>(s_top)->interpreted_link;
					else
						s_top = s_top->parent;
				}
				saved_fn->delete_locals();
				return nullptr;
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
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto type_a = s.return_type_back(0);
		auto dat_a = s.return_data_back(0);
		s.push_back_main(type_a, dat_a, nullptr);
		s.pop_return();
	}
	return p + 2;
}
inline int32_t* r_at(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
		auto type_a = s.return_type_back(0);
		auto expr_a = s.return_ex_back(0);
		s.push_back_main(type_a, 0, expr_a);
		s.pop_return();

		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto type_b = s.main_type_back(0);
		auto expr_b = s.main_ex_back(0);
		s.push_back_return(s.main_type_back(1), 0, s.main_ex_back(1));

		s.pop_main();
		s.pop_main();
		s.push_back_main(type_b, 0, expr_b);

	} else if(e->mode == fif::fif_mode::interpreting || (fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode))) {
		auto type_a = s.return_type_back(0);
		auto dat_a = s.return_data_back(0);
		s.push_back_main(type_a, dat_a, nullptr);
		s.pop_return();

		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		auto type_b = s.main_type_back(0);
		auto dat_b = s.main_data_back(0);
		s.push_back_return(s.main_type_back(1), s.main_data_back(1), nullptr);

		s.pop_main();
		s.pop_main();
		s.push_back_main(type_b, dat_b, nullptr);
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
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto type_a = s.main_type_back(0);
		auto dat_a = s.main_data_back(0);
		s.push_back_return(type_a, dat_a, nullptr);
		s.pop_main();
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

inline int32_t* ilt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b < a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32lt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb < fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64lt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb < fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uilt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntULT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b < a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* igt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b > a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uigt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntUGT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b > a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32gt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb > fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64gt(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGT, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb > fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* ile(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSLE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b <= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uile(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntULE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b <= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32le(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb <= fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64le(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOLE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb <= fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* ige(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntSGE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b >= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uige(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntUGE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = uint64_t(s.main_data_back(0));
		auto b = uint64_t(s.main_data_back(1));
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b >= a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ge(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb >= fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ge(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOGE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb >= fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* ieq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntEQ, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b == a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32eq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOEQ, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb == fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64eq(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealOEQ, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb == fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* ine(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildICmp(e->llvm_builder, LLVMIntPredicate::LLVMIntNE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, b != a, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ne(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealONE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		float fa = 0;
		float fb = 0;
		memcpy(&fa, &ia, 4);
		memcpy(&fb, &ib, 4);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb != fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ne(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFCmp(e->llvm_builder, LLVMRealPredicate::LLVMRealONE, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ia = s.main_data_back(0);
		auto ib = s.main_data_back(1);
		double fa = 0;
		double fb = 0;
		memcpy(&fa, &ia, 8);
		memcpy(&fb, &ib, 8);
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, fb != fa, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}

inline int32_t* f_select(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(3);
		auto result = LLVMBuildSelect(e->llvm_builder, s.main_ex_back(0), s.main_ex_back(1), s.main_ex_back(2), "");

		if(e->dict.type_array[s.main_type_back(1)].flags != 0) {
			auto drop_result = LLVMBuildSelect(e->llvm_builder, s.main_ex_back(0), s.main_ex_back(2), s.main_ex_back(1), "");
			s.push_back_main(s.main_type_back(1), 0, drop_result);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}

		s.pop_main();
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(3);
		auto ib = s.main_data_back(2);
		auto ia = s.main_data_back(1);
		auto ex = s.main_data_back(0);

		if(e->dict.type_array[s.main_type_back(1)].flags != 0) {
			s.push_back_main(s.main_type_back(1), ex != 0 ? ib : ia, nullptr);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}

		s.pop_main();
		s.pop_main();
		s.set_main_data_back(0, ex != 0 ? ia : ib);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(3);
		s.pop_main();
		s.pop_main();
		s.set_main_data_back(0, e->dict.type_array[s.main_type_back(0)].stateless() ? 0 : e->new_ident());
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
		auto val = s.main_ex_back(0);
		auto new_mem = LLVMBuildMalloc(e->llvm_builder, e->dict.type_array[s.main_type_back(0)].llvm_type, "");
		LLVMBuildStore(e->llvm_builder, val, new_mem);
		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), s.main_type_back(0), -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
		s.pop_main();
		s.push_back_main(mem_type.type, 0, new_mem);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto ptr = malloc(8);
		int64_t dat = s.main_data_back(0);
		memcpy(ptr, &dat, 8);

		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), s.main_type_back(0), -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
		s.pop_main();
		s.push_back_main(mem_type.type, (int64_t)ptr, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		int32_t ptr_type[] = { fif_ptr, std::numeric_limits<int32_t>::max(), s.main_type_back(0), -1 };
		std::vector<int32_t> subs;
		auto mem_type = resolve_span_type(std::span<int32_t const>(ptr_type, ptr_type + 4), subs, *e);
		s.pop_main();
		s.push_back_main(mem_type.type, e->new_ident(), nullptr);
	}
	return p + 2;
}

inline int32_t* impl_heap_free(fif::state_stack& s, int32_t* p, fif::environment* e) { // must drop contents ?
	if(fif::skip_compilation(e->mode))
		return p + 2;

	auto ptr_type = s.main_type_back(0);
	auto decomp = e->dict.type_array[ptr_type].decomposed_types_start;
	auto pointer_contents = e->dict.all_stack_types[decomp + 1];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto iresult = LLVMBuildLoad2(e->llvm_builder, e->dict.type_array[pointer_contents].llvm_type, s.main_ex_back(0), "");
		LLVMBuildFree(e->llvm_builder, s.main_ex_back(0));
		s.pop_main();
		s.push_back_main(pointer_contents, 0, iresult);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {

		auto a = s.main_data_back(0);
		int64_t* ptr = (int64_t*)a;
		auto val = *ptr;
		free(ptr);
		s.pop_main();
		s.push_back_main(pointer_contents, val, 0);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(pointer_contents, e->new_ident(), 0);
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
		auto result = LLVMBuildLoad2(e->llvm_builder, e->dict.type_array[pointer_contents].llvm_type, s.main_ex_back(0), "");
		s.pop_main();
		s.push_back_main(pointer_contents, 0, result);

		if(e->dict.type_array[pointer_contents].flags != 0) {
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);

			// remove the extra value and save the duplicated expression on top of the stack
			auto expr_b = s.main_ex_back(0);
			s.set_main_ex_back(1, expr_b);
			s.pop_main();
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		s.pop_main();
		int64_t* ptr_val = (int64_t*)v;
		auto effective_type = pointer_contents;

		if(pointer_contents != -1 && e->dict.type_array[pointer_contents].refcounted_type()) {
			if(e->dict.type_array[pointer_contents].decomposed_types_count >= 2) {
				auto child_index = e->dict.type_array[pointer_contents].decomposed_types_start + 1;
				auto child_type = e->dict.all_stack_types[child_index];

				if(e->dict.type_array[pointer_contents].single_member_struct()) {
					effective_type = child_type;
				}
			}
		}

		if(effective_type == fif_i32 || effective_type == fif_f32)
			s.push_back_main(pointer_contents, *(int32_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u32)
			s.push_back_main(pointer_contents, *(uint32_t*)(ptr_val), nullptr);
		else if(effective_type == fif_i16)
			s.push_back_main(pointer_contents, *(int16_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u16)
			s.push_back_main(pointer_contents, *(uint16_t*)(ptr_val), nullptr);
		else if(effective_type == fif_i8)
			s.push_back_main(pointer_contents, *(int8_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u8 || effective_type == fif_bool)
			s.push_back_main(pointer_contents, *(uint8_t*)(ptr_val), nullptr);
		else
			s.push_back_main(pointer_contents, *ptr_val, nullptr);

		if(e->dict.type_array[pointer_contents].flags != 0) {
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			// remove the extra value and save the duplicated expression on top of the stack
			auto data_b = s.main_data_back(0);
			s.set_main_data_back(1, data_b);
			s.pop_main();
		}
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		s.push_back_main(pointer_contents, e->dict.type_array[pointer_contents].stateless() ? 0 : e->new_ident(), nullptr);
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
		auto result = LLVMBuildLoad2(e->llvm_builder, e->dict.type_array[pointer_contents].llvm_type, s.main_ex_back(0), "");
		s.pop_main();
		s.push_back_main(pointer_contents, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		s.pop_main();
		int64_t* ptr_val = (int64_t*)v;
		auto effective_type = pointer_contents;

		if(pointer_contents != -1 && e->dict.type_array[pointer_contents].refcounted_type()) {
			if(e->dict.type_array[pointer_contents].decomposed_types_count >= 2) {
				auto child_index = e->dict.type_array[pointer_contents].decomposed_types_start + 1;
				auto child_type = e->dict.all_stack_types[child_index];

				if(e->dict.type_array[pointer_contents].single_member_struct()) {
					effective_type = child_type;
				}
			}
		}

		if(effective_type == fif_i32 || effective_type == fif_f32)
			s.push_back_main(pointer_contents, *(int32_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u32)
			s.push_back_main(pointer_contents, *(uint32_t*)(ptr_val), nullptr);
		else if(effective_type == fif_i16)
			s.push_back_main(pointer_contents, *(int16_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u16)
			s.push_back_main(pointer_contents, *(uint16_t*)(ptr_val), nullptr);
		else if(effective_type == fif_i8)
			s.push_back_main(pointer_contents, *(int8_t*)(ptr_val), nullptr);
		else if(effective_type == fif_u8 || effective_type == fif_bool)
			s.push_back_main(pointer_contents, *(uint8_t*)(ptr_val), nullptr);
		else
			s.push_back_main(pointer_contents, *ptr_val, nullptr);

	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		s.push_back_main(pointer_contents, e->dict.type_array[pointer_contents].stateless() ? 0 : e->new_ident(), nullptr);
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
		if(e->dict.type_array[pointer_contents].flags != 0) {
			auto iresult = LLVMBuildLoad2(e->llvm_builder, e->dict.type_array[pointer_contents].llvm_type, s.main_ex_back(0), "");
			s.push_back_main(pointer_contents, 0, iresult);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}

		auto result = LLVMBuildStore(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0));
		s.pop_main();
		s.pop_main();
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {

		auto v = s.main_data_back(0);
		int64_t* ptr_val = (int64_t*)v;

		if(e->dict.type_array[pointer_contents].flags != 0) {
			s.push_back_main(pointer_contents, *ptr_val, nullptr);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		}

		auto to_write = s.main_data_back(1);

		auto effective_type = pointer_contents;
		if(pointer_contents != -1 && e->dict.type_array[pointer_contents].refcounted_type()) {
			if(e->dict.type_array[pointer_contents].decomposed_types_count >= 2) {
				auto child_index = e->dict.type_array[pointer_contents].decomposed_types_start + 1;
				auto child_type = e->dict.all_stack_types[child_index];

				if(e->dict.type_array[pointer_contents].single_member_struct()) {
					effective_type = child_type;
				}
			}
		}

		if(effective_type == fif_i32 || effective_type == fif_f32 || effective_type == fif_u32)
			memcpy(ptr_val, &to_write, 4);
		else if(effective_type == fif_u16 || effective_type == fif_i16)
			memcpy(ptr_val, &to_write, 2);
		else if(effective_type == fif_u8 || effective_type == fif_bool || effective_type == fif_i8)
			memcpy(ptr_val, &to_write, 1);
		else
			memcpy(ptr_val, &to_write, 8);

		s.pop_main();
		s.pop_main();
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
		auto result = LLVMBuildStore(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0));
		s.pop_main();
		s.pop_main();
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {

		auto v = s.main_data_back(0);
		int64_t* ptr_val = (int64_t*)v;

		auto to_write = s.main_data_back(1);

		auto effective_type = pointer_contents;
		if(pointer_contents != -1 && e->dict.type_array[pointer_contents].refcounted_type()) {
			if(e->dict.type_array[pointer_contents].decomposed_types_count >= 2) {
				auto child_index = e->dict.type_array[pointer_contents].decomposed_types_start + 1;
				auto child_type = e->dict.all_stack_types[child_index];

				if(e->dict.type_array[pointer_contents].single_member_struct()) {
					effective_type = child_type;
				}
			}
		}

		if(effective_type == fif_i32 || effective_type == fif_f32 || effective_type == fif_u32)
			memcpy(ptr_val, &to_write, 4);
		else if(effective_type == fif_u16 || effective_type == fif_i16)
			memcpy(ptr_val, &to_write, 2);
		else if(effective_type == fif_u8 || effective_type == fif_bool || effective_type == fif_i8)
			memcpy(ptr_val, &to_write, 1);
		else
			memcpy(ptr_val, &to_write, 8);

		s.pop_main();
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode)) {
		s.pop_main();
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_pointer_cast(fif::state_stack& s, int32_t* p, fif::environment* e) {
	s.set_main_type_back(0, *(p + 2));
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
		} else if(e->dict.all_stack_types[e->dict.type_array[resolved_type].decomposed_types_start] != fif_ptr) {
			bad_type = true;
		}
	}

	if(bad_type) {
		e->report_error("pointer attempted to be cast to a non pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	if(!skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		s.set_main_type_back(0, resolved_type);
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
		s.push_back_main(fif_i32, 0, LLVMConstTrunc(LLVMSizeOf(e->dict.type_array[resolved_type].llvm_type), LLVMInt32TypeInContext(e->llvm_context)));
#endif
	} else if(e->mode == fif::fif_mode::compiling_bytecode) {
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = immediate_i32;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(interepreter_size(resolved_type, *e));
		}
		s.push_back_main(fif_i32, 8, 0);
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.push_back_main(fif_i32, 8, 0);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.push_back_main(fif_i32, e->new_ident(), 0);
	}
	return p + 2;
}

inline int32_t* impl_index(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto num_bytes = s.main_ex_back(1);
		auto iresult = LLVMBuildInBoundsGEP2(e->llvm_builder, LLVMInt8TypeInContext(e->llvm_context), s.main_ex_back(0), &num_bytes, 1, "");
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, 0, iresult);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto bytes = s.main_data_back(1);
		auto pval = s.main_data_back(0);
		uint8_t* ptr = (uint8_t*)pval;
		pval += bytes;
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, (int64_t)pval, 0);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, e->new_ident(), 0);
	}

	return p + 2;
}

inline int32_t* allocate_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto iresult = LLVMBuildArrayMalloc(e->llvm_builder, LLVMInt8TypeInContext(e->llvm_context), s.main_ex_back(0), "");
		LLVMBuildMemSet(e->llvm_builder, iresult, LLVMConstInt(LLVMInt8TypeInContext(e->llvm_context), 0, false), s.main_ex_back(0), 1);
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, 0, iresult);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {

		auto bytes = s.main_data_back(0);
		auto val = malloc(size_t(bytes));
		memset(val, 0, size_t(bytes));
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, (int64_t)val, 0);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, e->new_ident(), 0);
	}
	return p + 2;
}

inline int32_t* copy_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto bytes = s.main_ex_back(0);
		auto dest_ptr = s.main_ex_back(1);
		auto source_ptr = s.main_ex_back(2);
		s.pop_main();
		s.pop_main();
		s.pop_main();
		auto res = LLVMBuildMemCpy(e->llvm_builder, dest_ptr, 1, source_ptr, 1, bytes);
		s.push_back_main(fif_opaque_ptr, 0, dest_ptr);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto bytes = s.main_data_back(0);
		auto dest_ptr = s.main_data_back(1);
		auto source_ptr = s.main_data_back(2);
		s.pop_main();
		s.pop_main();
		s.pop_main();
		memcpy((void*)dest_ptr, (void*)source_ptr, size_t(bytes));
		s.push_back_main(fif_opaque_ptr, dest_ptr, 0);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		auto dest_ptr = s.main_data_back(1);
		s.pop_main();
		s.pop_main();
		s.pop_main();
		s.push_back_main(fif_opaque_ptr, dest_ptr, 0);
	}
	return p + 2;
}

inline int32_t* free_buffer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		LLVMBuildFree(e->llvm_builder, s.main_ex_back(0));
		s.pop_main();
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto a = s.main_data_back(0);
		void* ptr = (void*)a;
		free(ptr);
		s.pop_main();
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_relet_creation(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index = *(p + 2);

	e->compiler_stack.back()->re_let(index, s.main_type_back(0), s.main_data_back(0), nullptr);
	s.pop_main();

	return p + 3;
}

inline int32_t* create_relet(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("let was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(typechecking_mode(e->mode) && !skip_compilation(e->mode)) {
		auto l = e->compiler_stack.back()->get_var(std::string{ name.content });
		if(l == -1 || e->compiler_stack.back()->get_lvar_storage(l)->is_stack_variable == true) {
			e->report_error("could not find a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		} else {
			auto cv = e->compiler_stack.back()->get_lvar_storage(l); // prevent typechecking from changing stored values
			if(!e->compiler_stack.back()->re_let(l, cv->type, s.main_data_back(0), nullptr))
				e->mode = fail_typechecking(e->mode);
		}
	} else if(e->mode == fif_mode::interpreting) {
		auto l = e->compiler_stack.back()->get_var(std::string{ name.content });
		if(l == -1 || e->compiler_stack.back()->get_lvar_storage(l)->is_stack_variable == true) {
			e->report_error("could not find a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		} else {
			e->compiler_stack.back()->re_let(l, s.main_type_back(0), s.main_data_back(0), nullptr);
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto l = e->compiler_stack.back()->get_var(std::string{ name.content });
		if(l == -1 || e->compiler_stack.back()->get_lvar_storage(l)->is_stack_variable == true) {
			e->report_error("could not find a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		} else {
			e->compiler_stack.back()->re_let(l, s.main_type_back(0), 0, s.main_ex_back(0));
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto l = e->compiler_stack.back()->get_var(std::string{ name.content });
		if(l == -1 || e->compiler_stack.back()->get_lvar_storage(l)->is_stack_variable == true) {
			e->report_error("could not find a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
		e->compiler_stack.back()->re_let(l, s.main_type_back(0), 0, 0);
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_relet_creation;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(l);
		}
	}

	if(!skip_compilation(e->mode)) {
		s.pop_main();
	}
	return p + 2;
}

inline int32_t* do_let_creation(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index = *(p + 2);

	auto l = e->compiler_stack.back()->get_lvar_storage(index);
	if(!l) {
		e->report_error("could not create a var with that name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	l->data = s.main_data_back(0);
	l->type = s.main_type_back(0);
	l->is_stack_variable = false;
	s.pop_main();
	return p + 3;
}

inline int32_t* create_let(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("let was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(e->mode == fif_mode::interpreting || (typechecking_mode(e->mode) && !skip_compilation(e->mode))) {
		auto l = e->compiler_stack.back()->create_let(std::string{ name.content }, s.main_type_back(0), s.main_data_back(0), nullptr);
		if(l == -1) {
			e->report_error("could not create a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		auto l = e->compiler_stack.back()->create_let(std::string{ name.content }, s.main_type_back(0), 0, s.main_ex_back(0));
		if(l == -1) {
			e->report_error("could not create a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto l = e->compiler_stack.back()->create_let(std::string{ name.content }, s.main_type_back(0), 0, 0);
		if(l == -1) {
			e->report_error("could not create a let with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_let_creation;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(l);
		}
	}

	if(!skip_compilation(e->mode)) {
		s.pop_main();
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
		if(e->mode == fif_mode::interpreting || (typechecking_mode(e->mode) && !skip_compilation(e->mode))) {
			auto l = e->compiler_stack.back()->create_let(std::string{ n.content }, s.main_type_back(0), s.main_data_back(0), nullptr);
			if(l == -1) {
				e->report_error("could not create a let with that name");
				e->mode = fif_mode::error;
				return nullptr;
			}
		} else if(e->mode == fif_mode::compiling_llvm) {
			auto l = e->compiler_stack.back()->create_let(std::string{ n.content }, s.main_type_back(0), 0, s.main_ex_back(0));
			if(l == -1) {
				e->report_error("could not create a let with that name");
				e->mode = fif_mode::error;
				return nullptr;
			}
		} else if(e->mode == fif_mode::compiling_bytecode) {
			auto l = e->compiler_stack.back()->create_let(std::string{ n.content }, s.main_type_back(0), 0, 0);
			if(l == -1) {
				e->report_error("could not create a let with that name");
				e->mode = fif_mode::error;
				return nullptr;
			}
			auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
			if(compile_bytes) {
				fif_call imm = do_let_creation;
				uint64_t imm_bytes = 0;
				memcpy(&imm_bytes, &imm, 8);
				compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
				compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
				compile_bytes->push_back(l);
			}
		}

		if(!skip_compilation(e->mode)) {
			s.pop_main();
		}
	}
	return p + 2;
}

inline int32_t* do_var_creation(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index = *(p + 2);

	auto l = e->compiler_stack.back()->get_lvar_storage(index);
	if(!l) {
		e->report_error("could not create a var with that name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	l->data = s.main_data_back(0);
	l->type = s.main_type_back(0);
	l->is_stack_variable = true;
	s.pop_main();
	return p + 3;
}

inline int32_t* create_var(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->source_stack.empty()) {
		e->report_error("war was unable to read the declaration name");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto name = read_token(e->source_stack.back(), *e);

	if(e->mode == fif_mode::interpreting || (typechecking_mode(e->mode) && !skip_compilation(e->mode))) {
		auto l = e->compiler_stack.back()->create_var(std::string{ name.content }, s.main_type_back(0));
		if(l == -1) {
			e->report_error("could not create a var with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
		e->compiler_stack.back()->get_lvar_storage(l)->data = s.main_data_back(0);
	} else if(e->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto l = e->compiler_stack.back()->create_var(std::string{ name.content }, s.main_type_back(0));
		if(l == -1) {
			e->report_error("could not create a var with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
		LLVMBuildStore(e->llvm_builder, s.main_ex_back(0), e->compiler_stack.back()->get_lvar_storage(l)->expression);
#endif
	} else if(e->mode == fif_mode::compiling_bytecode) {
		auto l = e->compiler_stack.back()->create_var(std::string{ name.content }, s.main_type_back(0));
		if(l == -1) {
			e->report_error("could not create a var with that name");
			e->mode = fif_mode::error;
			return nullptr;
		}
		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = do_var_creation;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
			compile_bytes->push_back(l);
		}
	}

	if(!skip_compilation(e->mode)) {
		s.pop_main();
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
	auto type = s.main_data_back(0);

	if(skip_compilation(e->mode))
		return p + 2;

	if(typechecking_mode(e->mode)) {
		s.pop_main();
		return p + 2;
	}

	s.pop_main();
	for(auto& ptr : e->compiler_stack) {
		if(ptr->get_type() == control_structure::globals) {
			auto existing = static_cast<compiler_globals_layer*>(ptr.get())->get_global_var(std::string{ name.content });

			if(existing) {
				e->report_error("duplicate global definition");
				e->mode = fif_mode::error;
				return nullptr;
			}

			static_cast<compiler_globals_layer*>(ptr.get())->create_global_var(std::string{ name.content }, int32_t(type));
			return p + 2;
		}
	}

	e->report_error("could not find the globals layer");
	e->mode = fif_mode::error;
	return nullptr;
}

inline int32_t* do_fextract(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->dict.type_array[stype].single_member_struct()) {
		s.mark_used_from_main(1);
		s.set_main_type_back(0, child_type);
	} else {
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);
		auto child_data = children ? children[index_value] : 0;

		
		s.push_back_main(child_type, child_data, nullptr);
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		child_data = s.main_data_back(0);
		s.pop_main();
		if(children)
			children[index_value] = s.main_data_back(0);
		s.pop_main();
		
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		
		s.push_back_main(child_type, child_data, nullptr);
	}

	return p + 3;
}

inline int32_t actual_index_value(int32_t struct_type, int32_t raw_index, fif::environment& env) {
	int32_t result = 0;
	for(int32_t i = 0; i < raw_index && i + 1 < env.dict.type_array[struct_type].decomposed_types_count; ++i) {
		auto child_index = env.dict.type_array[struct_type].decomposed_types_start + 1 + i;
		auto child_type = env.dict.all_stack_types[child_index];
		if(env.dict.type_array[child_type].stateless() == false)
			++result;
	}
	return result;
}

inline int32_t* forth_extract(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].refcounted_type() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[child_type].stateless()) {
			s.pop_main();
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto result = LLVMBuildExtractValue(e->llvm_builder, struct_expr, actual_index_value(stype, index_value, *e), "");
			auto original_result = result;

			s.push_back_main(child_type, 0, result);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			result = s.main_ex_back(0);
			s.pop_main();
			if(s.main_ex_back(0) != original_result) {
				auto new_s_expr = LLVMBuildInsertValue(e->llvm_builder, struct_expr, s.main_ex_back(0), actual_index_value(stype, index_value, *e), "");
				s.set_main_ex_back(1, new_s_expr);
			}
			s.pop_main();
			
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			
			s.push_back_main(child_type, 0, result);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[child_type].stateless()) {
			s.pop_main();
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);
			auto child_data = children ? children[index_value] : 0;

			s.push_back_main(child_type, child_data, nullptr);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			child_data = s.main_data_back(0);
			s.pop_main();
			if(children)
				children[index_value] = s.main_data_back(0);
			s.pop_main();
			
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			
			s.push_back_main(child_type, child_data, nullptr);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		if(e->dict.type_array[child_type].stateless()) {
			s.pop_main();
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
		} else {
			auto original_result = e->new_ident();
			s.push_back_main(child_type, original_result, nullptr);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			auto result = s.main_data_back(0);
			s.pop_main();
			if(s.main_data_back(0) != original_result) {
				s.set_main_data_back(1, e->new_ident());
			}
			s.pop_main();
		
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			s.push_back_main(child_type, result, nullptr);
		}
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
		s.push_back_main(child_type, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* do_fextractc(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->dict.type_array[stype].single_member_struct()) {
		s.mark_used_from_main(1);
		s.set_main_type_back(0, child_type);
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		s.set_main_type_back(1, stype);
	} else {
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);
		auto child_data = children ? children[index_value] : 0;

		s.push_back_main(child_type, child_data, nullptr);
		execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
		child_data = s.main_data_back(0);
		s.pop_main();
		if(children)
			children[index_value] = s.main_data_back(0);
		s.pop_main();
		

		s.mark_used_from_main(1);
		s.push_back_main(child_type, child_data, nullptr);
	}
	return p + 3;
}

inline int32_t* forth_extract_copy(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].refcounted_type() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[child_type].stateless()) {
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			s.set_main_type_back(1, stype);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto result = LLVMBuildExtractValue(e->llvm_builder, struct_expr, actual_index_value(stype, index_value, *e), "");
			auto original_result = result;
			s.mark_used_from_main(1);

			s.push_back_main(child_type, 0, result);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			result = s.main_ex_back(0);
			s.pop_main();
			if(s.main_ex_back(0) != original_result) {
				auto new_s_expr = LLVMBuildInsertValue(e->llvm_builder, struct_expr, s.main_ex_back(0), actual_index_value(stype, index_value, *e), "");
				s.set_main_ex_back(1, new_s_expr);
			}
			s.pop_main();
			
			s.push_back_main(child_type, 0, result);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[child_type].stateless()) {
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			s.set_main_type_back(1, stype);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);
			auto child_data = children ? children[index_value] : 0;
			s.mark_used_from_main(1);

			s.push_back_main(child_type, child_data, nullptr);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			child_data = s.main_data_back(0);
			s.pop_main();
			if(children)
				children[index_value] = s.main_data_back(0);
			s.pop_main();
			
			s.push_back_main(child_type, child_data, nullptr);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		if(e->dict.type_array[child_type].stateless()) {
			s.mark_used_from_main(1);
			s.push_back_main(child_type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			s.set_main_type_back(1, stype);
		} else {
			auto original_result = e->new_ident();
			s.push_back_main(child_type, original_result, nullptr);
			execute_fif_word(fif::parse_result{ "dup", false }, *e, false);
			auto result = s.main_data_back(0);
			s.pop_main();
			if(s.main_data_back(0) != original_result) {
				s.set_main_data_back(1, e->new_ident());
			}
			s.pop_main();
			s.push_back_main(child_type, result, nullptr);
		}
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
		s.push_back_main(child_type, 0, nullptr);
	}
	return p + 2;
}
inline int32_t* do_finsert(fif::state_stack& s, int32_t* p, fif::environment* e) {
	int32_t index_value = *(p + 2);
	auto stype = s.main_type_back(0);
	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->dict.type_array[stype].single_member_struct()) {
		s.mark_used_from_main(2);
		s.set_main_type_back(0, child_type);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		s.set_main_type_back(0, stype);
	} else {
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);

		s.push_back_main(child_type, children ? children[index_value] : 0, 0);
		execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
		
		s.pop_main();
		if(children)
			children[index_value] = s.main_data_back(0);
		s.pop_main();

		s.push_back_main(stype, ptr, 0);
	}
	return p + 3;
}

inline int32_t* forth_insert(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto index_str = read_token(e->source_stack.back(), *e);
	auto index_value = parse_int(index_str.content);
	auto stype = s.main_type_back(0);

	if(stype == -1 || e->dict.type_array[stype].refcounted_type() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[child_type].stateless()) {
			auto struct_expr = s.main_ex_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(stype, 0, struct_expr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(2);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			s.set_main_type_back(0, stype);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto actual_index = actual_index_value(stype, index_value, *e);

			auto oldv = LLVMBuildExtractValue(e->llvm_builder, struct_expr, actual_index, "");
			s.push_back_main(child_type, 0, oldv);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			
			s.pop_main();
			struct_expr = LLVMBuildInsertValue(e->llvm_builder, struct_expr, s.main_ex_back(0), actual_index, "");
			s.pop_main();
			s.push_back_main(stype, 0, struct_expr);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[child_type].stateless()) {
			auto ptr = s.main_data_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(stype, ptr, 0);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(2);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			s.set_main_type_back(0, stype);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);

			s.push_back_main(child_type, children ? children[index_value] : 0, 0);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			
			s.pop_main();
			if(children)
				children[index_value] = s.main_data_back(0);
			s.pop_main();

			s.push_back_main(stype, ptr, 0);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		if(e->dict.type_array[child_type].stateless()) {
			auto ptr = s.main_data_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(stype, ptr, 0);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(2);
			s.set_main_type_back(0, child_type);
			execute_fif_word(fif::parse_result{ "drop", false }, *e, false);
			s.set_main_type_back(0, stype);
		} else {
			s.pop_main();
			s.pop_main();
			s.push_back_main(stype, e->new_ident(), nullptr);
		}
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
		s.push_back_main(stype, 0, nullptr);
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
	if(e->dict.type_array[ptr_type].decomposed_types_count == 0 || e->dict.all_stack_types[e->dict.type_array[ptr_type].decomposed_types_start] != fif_ptr) {
		e->report_error("attempted to use a struct-pointer operation on a non-struct-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}
	auto stype = e->dict.all_stack_types[decomp + 1];

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	if(e->dict.type_array[stype].single_member_struct()) {
		int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
		std::vector<int32_t> subs;
		auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

		s.mark_used_from_main(1);
		s.set_main_type_back(0, child_ptr_type.type);
	} else {
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);
		auto child_ptr = children ? children + index_value : nullptr;

		int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
		std::vector<int32_t> subs;
		auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

		s.pop_main();
		s.push_back_main(child_ptr_type.type, (int64_t)child_ptr, 0);
	}
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

	if(e->dict.type_array[ptr_type].decomposed_types_count == 0 || e->dict.all_stack_types[e->dict.type_array[ptr_type].decomposed_types_start] != fif_ptr) {
		e->report_error("attempted to use a struct-pointer operation on a non-struct-pointer type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	auto stype = e->dict.all_stack_types[decomp + 1];

	if(stype == -1 || e->dict.type_array[stype].refcounted_type() == false) {
		e->report_error("attempted to use a structure operation on a non-structure type");
		e->mode = fif_mode::error;
		return nullptr;
	}

	assert(1 + index_value < e->dict.type_array[stype].decomposed_types_count);
	auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + index_value;
	auto child_type = e->dict.all_stack_types[child_index];

	int32_t type_storage[] = { fif_ptr, std::numeric_limits<int32_t>::max(), child_type, -1 };
	std::vector<int32_t> subs;
	auto child_ptr_type = resolve_span_type(std::span<int32_t const>(type_storage, type_storage + 4), subs, *e);

	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[child_type].stateless()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_ptr_type.type);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_ptr_type.type);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto ptr_expr = LLVMBuildStructGEP2(e->llvm_builder, e->dict.type_array[stype].llvm_type, struct_expr, uint32_t(actual_index_value(stype, index_value, *e)), "");
			s.pop_main();
			s.push_back_main(child_ptr_type.type, 0, ptr_expr);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[child_type].stateless()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_ptr_type.type);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_ptr_type.type);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);
			auto child_ptr = children ? children + index_value : nullptr;

			s.pop_main();
			s.push_back_main(child_ptr_type.type, (int64_t)child_ptr, 0);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		if(e->dict.type_array[child_type].stateless()) {
			s.pop_main();
			s.push_back_main(child_ptr_type.type, 0, nullptr);
		} else if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_ptr_type.type);
		} else {
			s.pop_main();
			s.push_back_main(child_ptr_type.type, e->new_ident(), nullptr);
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
		s.push_back_main(child_ptr_type.type, 0, nullptr);
	}
	return p + 2;
}

inline int32_t* do_fsmz(fif::state_stack& s, int32_t* p, fif::environment* e) {
	char* command = nullptr;
	memcpy(&command, p + 2, 8);

	auto stype = s.main_type_back(0);

	if(e->dict.type_array[stype].single_member_struct()) {
		auto child_index = e->dict.type_array[stype].decomposed_types_start + 1;
		auto child_type = e->dict.all_stack_types[child_index];

		s.mark_used_from_main(1);
		s.set_main_type_back(0, child_type);
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
	} else {
		auto children_count = struct_child_count(stype, *e);
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);

		for(int32_t i = 0; i < children_count; ++i) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
			auto child_type = e->dict.all_stack_types[child_index];
			s.push_back_main(child_type, children ? children[i] : 0, nullptr);
			execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
		}

		free(children);
		s.pop_main();
	}
	return p + 4;
}

inline int32_t* forth_struct_map_zero(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);
	auto children_count = struct_child_count(stype, *e);


	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1;
			auto child_type = e->dict.all_stack_types[child_index];

			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
		} else {
			auto struct_expr = s.main_ex_back(0);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				if(e->dict.type_array[child_type].stateless() == false) {
					s.push_back_main(child_type, 0, LLVMBuildExtractValue(e->llvm_builder, struct_expr, uint32_t(actual_index_value(stype, i, *e)), ""));
				} else {
					s.push_back_main(child_type, 0, nullptr);
				}
				execute_fif_word(mapped_function, *e, false);
			}

			s.pop_main();
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1;
			auto child_type = e->dict.all_stack_types[child_index];

			s.mark_used_from_main(1);
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, children[i], nullptr);
				execute_fif_word(mapped_function, *e, false);
			}
			free(children);
			s.pop_main();
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

	if(e->dict.type_array[stype].single_member_struct()) {
		auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
		auto child_type = e->dict.all_stack_types[child_index];
		s.set_main_type_back(0, child_type);
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
		s.set_main_type_back(0, stype);
	} else {
		auto children_count = struct_child_count(stype, *e);
		auto ptr = s.main_data_back(0);
		auto children = (int64_t*)(ptr);

		for(int32_t i = 0; i < children_count; ++i) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
			auto child_type = e->dict.all_stack_types[child_index];
			s.push_back_main(child_type, children ? children[i] : 0, nullptr);
			execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
			if(children)
				children[i] = s.main_data_back(0);
			s.pop_main();
		}
	}

	s.mark_used_from_main(1);
	return p + 4;
}
inline int32_t* forth_struct_map_one(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);
	auto children_count = struct_child_count(stype, *e);


	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[stype].single_member_struct()) {
			assert(children_count == 1);
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
			s.set_main_type_back(0, stype);
		} else {
			auto struct_expr = s.main_ex_back(0);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				if(e->dict.type_array[child_type].stateless() == false)
					s.push_back_main(child_type, 0, LLVMBuildExtractValue(e->llvm_builder, struct_expr, uint32_t(actual_index_value(stype, i, *e)), ""));
				else
					s.push_back_main(child_type, 0, nullptr);
				execute_fif_word(mapped_function, *e, false);
				if(e->dict.type_array[child_type].stateless() == false)
					struct_expr = LLVMBuildInsertValue(e->llvm_builder, struct_expr, s.main_ex_back(0), uint32_t(actual_index_value(stype, i, *e)), "");
				s.pop_main();
			}
			s.set_main_ex_back(0, struct_expr);
		}
		s.mark_used_from_main(1);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
			s.set_main_type_back(0, stype);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, children[i], nullptr);
				execute_fif_word(mapped_function, *e, false);

				children[i] = s.main_data_back(0);

				s.pop_main();
			}
		}
		s.mark_used_from_main(1);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		if(!e->dict.type_array[stype].stateless())
			s.set_main_data_back(0, e->new_ident());
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

	if(e->dict.type_array[stype].single_member_struct()) {
		s.mark_used_from_main(1);
		auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
		auto child_type = e->dict.all_stack_types[child_index];
		s.set_main_type_back(0, child_type);
		execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);
		s.set_main_type_back(0, stype);
		s.set_main_type_back(1, stype);
	} else if(e->dict.type_array[stype].stateless()) {
		s.mark_used_from_main(1);
		s.push_back_main(stype, 0, nullptr);
	} else {
		auto children_count = struct_child_count(stype, *e);
		auto ptr = s.main_data_back(0);
		auto new_ptr = e->dict.type_array[stype].interpreter_zero(stype, e);

		auto children = (int64_t*)(ptr);
		auto new_children = (int64_t*)(new_ptr);

		for(int32_t i = 0; i < children_count; ++i) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
			auto child_type = e->dict.all_stack_types[child_index];
			s.push_back_main(child_type, children ? children[i] : 0, nullptr);
			execute_fif_word(parse_result{ std::string_view{ command }, false }, *e, false);

			if(new_children)
				new_children[i] = s.main_data_back(0);

			s.pop_main();
			s.pop_main();
		}

		s.mark_used_from_main(1);
		s.push_back_main(stype, new_ptr, nullptr);
	}
	return p + 4;
}
inline int32_t* forth_struct_map_two(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto mapped_function = read_token(e->source_stack.back(), *e);
	auto stype = s.main_type_back(0);
	auto children_count = struct_child_count(stype, *e);


	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
			s.set_main_type_back(0, stype);
			s.set_main_type_back(1, stype);
		} else if(e->dict.type_array[stype].stateless()) {
			s.push_back_main(stype, 0, nullptr);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto new_struct_expr = e->dict.type_array[stype].zero_constant(e->llvm_context, stype, e);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				if(e->dict.type_array[child_type].stateless() == false)
					s.push_back_main(child_type, 0, LLVMBuildExtractValue(e->llvm_builder, struct_expr, uint32_t(actual_index_value(stype, i, *e)), ""));
				else 
					s.push_back_main(child_type, 0, nullptr);

				execute_fif_word(mapped_function, *e, false);
		
				if(e->dict.type_array[child_type].stateless() == false)
					new_struct_expr = LLVMBuildInsertValue(e->llvm_builder, new_struct_expr, s.main_ex_back(0), uint32_t(actual_index_value(stype, i, *e)), "");
				s.pop_main();
				s.pop_main();
			}
			s.mark_used_from_main(1);
			s.push_back_main(stype, 0, new_struct_expr);
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[stype].single_member_struct()) {
			s.mark_used_from_main(1);
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			execute_fif_word(mapped_function, *e, false);
			s.set_main_type_back(0, stype);
			s.set_main_type_back(1, stype);
		} else if(e->dict.type_array[stype].stateless()) {
			s.push_back_main(stype, 0, nullptr);
		} else {
			auto ptr = s.main_data_back(0);
			auto new_ptr = e->dict.type_array[stype].interpreter_zero(stype, e);

			auto children = (int64_t*)(ptr);
			auto new_children = (int64_t*)(new_ptr);

			for(int32_t i = 0; i < children_count; ++i) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, children[i], nullptr);
				execute_fif_word(mapped_function, *e, false);


				new_children[i] = s.main_data_back(0);


				s.pop_main();
				s.pop_main();
			}

			s.mark_used_from_main(1);
			s.push_back_main(stype, new_ptr, nullptr);
		}
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		s.push_back_main(stype, e->dict.type_array[stype].stateless() ? 0 : e->new_ident(), nullptr);
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
		s.push_back_main(stype, 0, nullptr);
	}
	return p + 2;
}

inline int32_t* type_construction(state_stack& s, int32_t* p, environment* e) {
	auto type = *(p + 2);
	auto is_refcounted = e->dict.type_array[type].refcounted_type();
	auto is_stateless = e->dict.type_array[type].stateless();
	s.push_back_main(type, (!is_stateless && is_refcounted && e->dict.type_array[type].interpreter_zero) ? e->dict.type_array[type].interpreter_zero(type, e) : 0, nullptr);
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

	if(typechecking_mode(env->mode)) {
		s.push_back_main(resolved_type, env->new_ident(), nullptr);
		return p + 2;
	}

	auto compile_bytes = env->compiler_stack.back()->bytecode_compilation_progress();
	if(compile_bytes && env->mode == fif_mode::compiling_bytecode) {
		fif_call imm = type_construction;
		uint64_t imm_bytes = 0;
		memcpy(&imm_bytes, &imm, 8);
		compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
		compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		compile_bytes->push_back(resolved_type);
	}
	LLVMValueRef val = nullptr;
	if(env->mode == fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		if(env->dict.type_array[resolved_type].stateless()) {
			//nothing
		} else if(env->dict.type_array[resolved_type].zero_constant) {
			val = env->dict.type_array[resolved_type].zero_constant(env->llvm_context, resolved_type, env);
		} else {
			env->report_error("attempted to compile a type without an llvm representation");
			env->mode = fif_mode::error;
			return nullptr;
		}
#endif
	}
	int64_t data = 0;
	if(env->mode == fif_mode::interpreting) {
		if(!env->dict.type_array[resolved_type].stateless() && env->dict.type_array[resolved_type].refcounted_type() && env->dict.type_array[resolved_type].interpreter_zero)
			data = env->dict.type_array[resolved_type].interpreter_zero(resolved_type, env);
	} else if(typechecking_mode(env->mode)) {
		if(!env->dict.type_array[resolved_type].stateless())
			data = env->new_ident();
	}
	s.push_back_main(resolved_type, data, val);
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
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_type_back(0, fif_i64);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFFFFFF);
		s.set_main_type_back(0, fif_i32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFF);
		s.set_main_type_back(0, fif_i16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FF);
		s.set_main_type_back(0, fif_i8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_type_back(0, fif_u64);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFFFFFF);
		s.set_main_type_back(0, fif_u32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFF);
		s.set_main_type_back(0, fif_u16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* zext_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildZExt(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FF);
		s.set_main_type_back(0, fif_u8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_i64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_i64);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_i32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_i16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_i8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_u64);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_u32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_u16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sext_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSExt(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		if(s.main_type_back(0) == fif_i8 || s.main_type_back(0) == fif_u8) {
			auto v = int8_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i16 || s.main_type_back(0) == fif_u16) {
			auto v = int16_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} else if(s.main_type_back(0) == fif_i32 || s.main_type_back(0) == fif_u32) {
			auto v = int32_t(s.main_data_back(0));
			s.set_main_data_back(0, v);
		} if(s.main_type_back(0) == fif_bool) {
			auto v = s.main_data_back(0);
			s.set_main_data_back(0, v != 0 ? -1 : 0);
		}
		s.set_main_type_back(0, fif_u8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FF);
		s.set_main_type_back(0, fif_u8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int8_t(s.main_data_back(0) & 0x0FF));
		s.set_main_type_back(0, fif_i8);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_i1(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt1TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_bool, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x01);
		s.set_main_type_back(0, fif_bool);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_bool, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFF);
		s.set_main_type_back(0, fif_u16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int16_t(s.main_data_back(0) & 0x0FFFF));
		s.set_main_type_back(0, fif_i16);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFFFFFF);
		s.set_main_type_back(0, fif_u32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* trunc_i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildTrunc(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int32_t(s.main_data_back(0) & 0x0FFFFFFFF));
		s.set_main_type_back(0, fif_i32);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i32, e->new_ident(), nullptr);
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
		auto result = LLVMBuildFPTrunc(e->llvm_builder, s.main_ex_back(0), LLVMFloatTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		float fout = float(fin);
		int64_t iout = 0;
		memcpy(&iout, &fout, 4);
		s.pop_main();
		s.push_back_main(fif_f32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* fext(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPExt(e->llvm_builder, s.main_ex_back(0), LLVMFloatTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		double fout = double(fin);
		int64_t iout = 0;
		memcpy(&iout, &fout, 8);
		s.pop_main();
		s.push_back_main(fif_f64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int8_t(fin);
		s.pop_main();
		s.push_back_main(fif_i8, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sif32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSIToFP(e->llvm_builder, s.main_ex_back(0), LLVMFloatTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fout = float(v);
		int64_t iout = 0;
		memcpy(&iout, &fout, 4);
		s.pop_main();
		s.push_back_main(fif_f32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uif32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildUIToFP(e->llvm_builder, s.main_ex_back(0), LLVMFloatTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = uint64_t(s.main_data_back(0));
		float fout = float(v);
		int64_t iout = 0;
		memcpy(&iout, &fout, 4);
		s.pop_main();
		s.push_back_main(fif_f32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* sif64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildSIToFP(e->llvm_builder, s.main_ex_back(0), LLVMDoubleTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fout = double(v);
		int64_t iout = 0;
		memcpy(&iout, &fout, 8);
		s.pop_main();
		s.push_back_main(fif_f64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* uif64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildUIToFP(e->llvm_builder, s.main_ex_back(0), LLVMDoubleTypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_f64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = uint64_t(s.main_data_back(0));
		double fout = double(v);
		int64_t iout = 0;
		memcpy(&iout, &fout, 8);
		s.pop_main();
		s.push_back_main(fif_f64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_f64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int16_t(fin);
		s.pop_main();
		s.push_back_main(fif_i16, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int32_t(fin);
		s.pop_main();
		s.push_back_main(fif_i32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32i64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int64_t(fin);
		s.pop_main();
		s.push_back_main(fif_i64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int64_t(uint8_t(fin));
		s.pop_main();
		s.push_back_main(fif_u8, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int64_t(uint16_t(fin));
		s.pop_main();
		s.push_back_main(fif_u16, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int64_t(uint32_t(fin));
		s.pop_main();
		s.push_back_main(fif_u32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f32ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		float fin = 0;
		memcpy(&fin, &v, 4);
		int64_t iout = int64_t(uint64_t(fin));
		s.pop_main();
		s.push_back_main(fif_u64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u64, e->new_ident(), nullptr);
	}
	return p + 2;
}

inline int32_t* f64i8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int8_t(fin);
		s.pop_main();
		s.push_back_main(fif_i8, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64i16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int16_t(fin);
		s.pop_main();
		s.push_back_main(fif_i16, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64i32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int32_t(fin);
		s.pop_main();
		s.push_back_main(fif_i32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64i64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToSI(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_i64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int64_t(fin);
		s.pop_main();
		s.push_back_main(fif_i64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_i64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ui8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt8TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u8, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int64_t(uint8_t(fin));
		s.pop_main();
		s.push_back_main(fif_u8, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u8, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ui16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt16TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u16, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int64_t(uint16_t(fin));
		s.pop_main();
		s.push_back_main(fif_u16, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u16, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ui32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt32TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u32, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int64_t(uint32_t(fin));
		s.pop_main();
		s.push_back_main(fif_u32, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u32, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* f64ui64(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		auto result = LLVMBuildFPToUI(e->llvm_builder, s.main_ex_back(0), LLVMInt64TypeInContext(e->llvm_context), "");
		s.pop_main();
		s.push_back_main(fif_u64, 0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		auto v = s.main_data_back(0);
		double fin = 0;
		memcpy(&fin, &v, 8);
		int64_t iout = int64_t(uint64_t(fin));
		s.pop_main();
		s.push_back_main(fif_u64, iout, nullptr);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.pop_main();
		s.push_back_main(fif::fif_u64, e->new_ident(), nullptr);
	}
	return p + 2;
}
inline int32_t* bit_and(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildAnd(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, a & b);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_or(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildOr(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, a | b);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_xor(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildXor(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, a ^ b);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_not(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(1);
		auto result = LLVMBuildNot(e->llvm_builder, s.main_ex_back(0), "");
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		auto a = s.main_data_back(0);
		if(s.main_type_back(0) == fif_bool)
			s.set_main_data_back(0, a == 0);
		else
			s.set_main_data_back(0, ~a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_shl(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildShl(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b << a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_ashr(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildAShr(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, b >> a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}
inline int32_t* bit_lshr(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
#ifdef USE_LLVM
		s.mark_used_from_main(2);
		auto result = LLVMBuildLShr(e->llvm_builder, s.main_ex_back(1), s.main_ex_back(0), "");
		s.pop_main();
		s.set_main_ex_back(0, result);
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(2);
		auto a = s.main_data_back(0);
		auto b = s.main_data_back(1);
		s.pop_main();
		s.set_main_data_back(0, uint64_t(b) >> a);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(2);
		s.pop_main();
		s.set_main_data_back(0, e->new_ident());
	}
	return p + 2;
}

inline int32_t* ident32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFFFFFF);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* ident16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FFFF);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* ident8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x0FF);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* idents32(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int32_t(s.main_data_back(0) & 0x0FFFFFFFF));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* idents16(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int16_t(s.main_data_back(0) & 0x0FFFF));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* idents8(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, int8_t(s.main_data_back(0) & 0x0FF));
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* ident1(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::compiling_llvm) {
	} else if(e->mode == fif::fif_mode::interpreting) {
		s.mark_used_from_main(1);
		s.set_main_data_back(0, s.main_data_back(0) & 0x01);
	} else if(fif::typechecking_mode(e->mode) && !fif::skip_compilation(e->mode)) {
		s.mark_used_from_main(1);
	}
	return p + 2;
}
inline int32_t* insert_stack_token(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(!fif::skip_compilation(e->mode))
		s.push_back_main(fif_stack_token, 0, nullptr);
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
inline int32_t* structify(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::skip_compilation(e->mode))
		return p + 2;

	std::vector<int32_t> types;
	types.push_back(fif::fif_anon_struct);
	types.push_back(std::numeric_limits<int32_t>::max());

	size_t i = 0;
	for(; i < s.main_size(); ++i) {
		s.mark_used_from_main(i + 1);
		if(s.main_type_back(i) == fif::fif_stack_token) {
			break;
		}
		types.push_back(s.main_type_back(i));
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
		if(e->dict.type_array[resolved_struct_type.type].single_member_struct()) {
			auto expr = s.main_data_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(resolved_struct_type.type, expr, nullptr);
		} else {
			while(s.main_size() > 0) {
				if(s.main_type_back(0) == fif::fif_stack_token) {
					s.pop_main();
					break;
				}
				s.pop_main();
			}
			s.push_back_main(resolved_struct_type.type, e->dict.type_array[resolved_struct_type.type].stateless() ? 0 : e->new_ident(), nullptr);
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		while(s.main_size() > 0) {
			if(s.main_type_back(0) == fif::fif_stack_token) {
				s.pop_main();
				break;
			}
			s.pop_main();
		}
		s.push_back_main(resolved_struct_type.type, e->dict.type_array[resolved_struct_type.type].stateless() ? 0 : e->new_ident(), nullptr);

		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = structify;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
		}
	} else if(e->mode == fif_mode::interpreting) {
		if(e->dict.type_array[resolved_struct_type.type].stateless()) {
			s.pop_main();
			s.push_back_main(resolved_struct_type.type, 0, nullptr);
		} else if(e->dict.type_array[resolved_struct_type.type].single_member_struct()) {
			auto expr = s.main_data_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(resolved_struct_type.type, expr, nullptr);
		} else {
			auto new_ptr = e->dict.type_array[resolved_struct_type.type].interpreter_zero(resolved_struct_type.type, e);
			auto new_children = (int64_t*)(new_ptr);
			uint32_t i = 0;
			while(s.main_size() > 0) {
				if(s.main_type_back(0) == fif::fif_stack_token) {
					s.pop_main();
					break;
				}
				new_children[i] = s.main_data_back(0);
				++i;
				s.pop_main();
			}
			s.push_back_main(resolved_struct_type.type, new_ptr, nullptr);
		}
	} else if(e->mode == fif_mode::compiling_llvm) {
		if(e->dict.type_array[resolved_struct_type.type].stateless()) {
			s.pop_main();
			s.push_back_main(resolved_struct_type.type, 0, nullptr);
		} else if(e->dict.type_array[resolved_struct_type.type].single_member_struct()) {
			auto expr = s.main_ex_back(0);
			s.pop_main();
			s.pop_main();
			s.push_back_main(resolved_struct_type.type, 0, expr);
		} else {
			auto struct_expr = s.main_ex_back(0);
			auto new_struct_expr = e->dict.type_array[resolved_struct_type.type].zero_constant(e->llvm_context, resolved_struct_type.type, e);

			uint32_t i = 0;
			while(s.main_size() > 0) {
				if(s.main_type_back(0) == fif::fif_stack_token) {
					s.pop_main();
					break;
				}
				if(e->dict.type_array[s.main_type_back(0)].stateless() == false)
					new_struct_expr = LLVMBuildInsertValue(e->llvm_builder, new_struct_expr, s.main_ex_back(0), actual_index_value(resolved_struct_type.type, i, *e), "");
				++i;
				s.pop_main();
			}

			s.push_back_main(resolved_struct_type.type, 0, new_struct_expr);
		}
	}

	return p + 2;
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
		if(e->dict.type_array[stype].single_member_struct()) {
			assert(children_count == 1);
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			s.mark_used_from_main(1);
		} else {
			auto struct_expr = s.main_ex_back(0);
			s.pop_main();

			for(int32_t i = children_count; i --> 0; ) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				if(e->dict.type_array[child_type].stateless() == false)
					s.push_back_main(child_type, 0, LLVMBuildExtractValue(e->llvm_builder, struct_expr, uint32_t(actual_index_value(stype, i, *e)), ""));
				else
					s.push_back_main(child_type, 0, nullptr);
			}
		}
#endif
	} else if(e->mode == fif::fif_mode::interpreting) {
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			s.mark_used_from_main(1);
		} else {
			auto ptr = s.main_data_back(0);
			auto children = (int64_t*)(ptr);
			s.pop_main();

			for(int32_t i = children_count; i-- > 0;) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, children[i], nullptr);
			}

			free(children);
		}
	} else if(fif::typechecking_mode(e->mode)) {
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			s.mark_used_from_main(1);
		} else {
			s.pop_main();
			for(int32_t i = children_count; i-- > 0;) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, e->dict.type_array[child_type].stateless() ? 0 : e->new_ident(), nullptr);
			}
		}
	} else if(e->mode == fif_mode::compiling_bytecode) {
		if(e->dict.type_array[stype].single_member_struct()) {
			auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + 0;
			auto child_type = e->dict.all_stack_types[child_index];
			s.set_main_type_back(0, child_type);
			s.mark_used_from_main(1);
		} else {
			s.pop_main();
			for(int32_t i = children_count; i-- > 0;) {
				auto child_index = e->dict.type_array[stype].decomposed_types_start + 1 + i;
				auto child_type = e->dict.all_stack_types[child_index];
				s.push_back_main(child_type, 0, nullptr);
			}
		}

		auto compile_bytes = e->compiler_stack.back()->bytecode_compilation_progress();
		if(compile_bytes) {
			fif_call imm = de_struct;
			uint64_t imm_bytes = 0;
			memcpy(&imm_bytes, &imm, 8);
			compile_bytes->push_back(int32_t(imm_bytes & 0xFFFFFFFF));
			compile_bytes->push_back(int32_t((imm_bytes >> 32) & 0xFFFFFFFF));
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
	add_precompiled(fif_env, "+", f32_add, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "+", f64_add, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "-", isub, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "-", isub, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "-", isub, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "-", f32_sub, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "-", f64_sub, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "/", sidiv, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "/", sidiv, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "/", uidiv, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "/", f32_div, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "/", f64_div, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "mod", simod, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "mod", simod, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "mod", uimod, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "mod", f32_mod, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "mod", f64_mod, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

	add_precompiled(fif_env, "*", imul, { fif::fif_i32, fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u32, fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u64, fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i64, fif::fif_i64, -1, fif::fif_i64 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i16, fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u16, fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, "*", imul, { fif::fif_i8, fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, "*", imul, { fif::fif_u8, fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, "*", f32_mul, { fif::fif_f32, fif::fif_f32, -1, fif::fif_f32 });
	add_precompiled(fif_env, "*", f64_mul, { fif::fif_f64, fif::fif_f64, -1, fif::fif_f64 });

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

	add_precompiled(fif_env, "<", f32lt, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", f32le, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", f32gt, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", f32ge, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", f32eq, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", f32ne, { fif::fif_f32, fif::fif_f32, -1, fif::fif_bool });
	add_precompiled(fif_env, "<", f64lt, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<=", f64le, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">", f64gt, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, ">=", f64ge, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "=", f64eq, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });
	add_precompiled(fif_env, "<>", f64ne, { fif::fif_f64, fif::fif_f64, -1, fif::fif_bool });

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
	add_precompiled(fif_env, ">i64", f32i64, { fif::fif_f32, -1, fif::fif_i64 });
	add_precompiled(fif_env, ">i64", f64i64, { fif::fif_f64, -1, fif::fif_i64 });

	add_precompiled(fif_env, ">u64", nop1, { fif::fif_u64, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u16, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_u8, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i64, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i16, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", sext_ui64, { fif::fif_i8, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", zext_ui64, { fif::fif_bool, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", f32ui64, { fif::fif_f32, -1, fif::fif_u64 });
	add_precompiled(fif_env, ">u64", f64ui64, { fif::fif_f64, -1, fif::fif_u64 });

	add_precompiled(fif_env, ">i32", trunc_i32, { fif::fif_u64, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u16, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_u8, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", trunc_i32, { fif::fif_i64, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", idents32, { fif::fif_i32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", sext_i32, { fif::fif_i16, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", sext_i32, { fif::fif_i8, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", zext_i32, { fif::fif_bool, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", f32i32, { fif::fif_f32, -1, fif::fif_i32 });
	add_precompiled(fif_env, ">i32", f64i32, { fif::fif_f64, -1, fif::fif_i32 });

	add_precompiled(fif_env, ">u32", trunc_ui32, { fif::fif_u64, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", ident32, { fif::fif_u32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_u16, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_u8, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", trunc_ui32, { fif::fif_i64, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i16, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", sext_ui32, { fif::fif_i8, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", zext_ui32, { fif::fif_bool, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", f32ui32, { fif::fif_f32, -1, fif::fif_u32 });
	add_precompiled(fif_env, ">u32", f64ui32, { fif::fif_f64, -1, fif::fif_u32 });

	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_u64, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_u32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_u16, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_u8, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_i64, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", trunc_i16, { fif::fif_i32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", idents16, { fif::fif_i16, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", sext_i16, { fif::fif_i8, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", zext_i16, { fif::fif_bool, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", f32i16, { fif::fif_f32, -1, fif::fif_i16 });
	add_precompiled(fif_env, ">i16", f64i16, { fif::fif_f64, -1, fif::fif_i16 });

	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_u64, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_u32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", ident16, { fif::fif_u16, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", zext_ui16, { fif::fif_u8, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_i64, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", trunc_ui16, { fif::fif_i32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", sext_ui16, { fif::fif_i16, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", sext_ui16, { fif::fif_i8, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", zext_ui16, { fif::fif_bool, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", f32ui16, { fif::fif_f32, -1, fif::fif_u16 });
	add_precompiled(fif_env, ">u16", f64ui16, { fif::fif_f64, -1, fif::fif_u16 });

	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u64, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_u16, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", zext_i8, { fif::fif_u8, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i64, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", trunc_i8, { fif::fif_i16, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", idents8, { fif::fif_i8, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", zext_i8, { fif::fif_bool, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", f32i8, { fif::fif_f32, -1, fif::fif_i8 });
	add_precompiled(fif_env, ">i8", f64i8, { fif::fif_f64, -1, fif::fif_i8 });

	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u64, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_u16, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", ident8, { fif::fif_u8, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i64, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", trunc_ui8, { fif::fif_i16, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", sext_ui8, { fif::fif_i8, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", zext_ui8, { fif::fif_bool, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", f32ui8, { fif::fif_f32, -1, fif::fif_u8 });
	add_precompiled(fif_env, ">u8", f64ui8, { fif::fif_f64, -1, fif::fif_u8 });

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
	add_precompiled(fif_env, ">bool", ident1, { fif::fif_bool, -1, fif::fif_bool });

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

	add_precompiled(fif_env, "init", init, { -2, -1, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0 };
	add_precompiled(fif_env, "dup", dup, { -2, -1, -2, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0, 0 };
	add_precompiled(fif_env, "copy", copy, { -2, -1, -2, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0, 0 };
	add_precompiled(fif_env, "drop", drop, { -2 });
	add_precompiled(fif_env, "swap", fif_swap, { -2, -3, -1, -2, -3 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0, 1 };
	
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

	add_precompiled(fif_env, ">r", to_r, { -2, -1, -1, -1, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0 };
	add_precompiled(fif_env, "r>", from_r, { -1, -2, -1, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0 };
	add_precompiled(fif_env, "r@", r_at, { -1, -2, -1, -2, -1, -2 });
	std::get< compiled_word_instance>(fif_env.dict.all_instances.back()).llvm_parameter_permutation = { 0, 0 };
	add_precompiled(fif_env, "immediate", make_immediate, { });
	add_precompiled(fif_env, "[", open_bracket, { }, true);
	add_precompiled(fif_env, "]", close_bracket, { }, true);

	add_precompiled(fif_env, "ptr-cast", pointer_cast, { fif_opaque_ptr }, true);
	add_precompiled(fif_env, "ptr-cast", pointer_cast, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1 }, true);
	add_precompiled(fif_env, "heap-alloc", impl_heap_allot, { -2, -1, fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1 });
	add_precompiled(fif_env, "heap-free", impl_heap_free, { fif_ptr, std::numeric_limits<int32_t>::max(), -2, -1, -1, -2 });
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

	add_precompiled(fif_env, "select", f_select, { fif_bool, -2, -2, -1, -2 });

	auto preinterpreted =
		": over >r dup r> swap ; "
		": nip >r drop r> ; "
		": tuck swap over ; "
		": 2dup >r dup r> dup >r swap r> ; "
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

