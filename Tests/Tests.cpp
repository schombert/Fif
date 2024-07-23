#define CATCH_CONFIG_MAIN 
#pragma warning(disable : 4127)

#include "catch.hpp"
#include "..\fif-interpreter\fif.hpp"
#include <Windows.h>
#undef min
#undef max
#include <stdint.h>

#pragma comment(lib, "LLVM-C.lib")

TEST_CASE("fundamental calls", "fif interpreter tests") {
	SECTION("int add") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 5);
	}
	SECTION("fp add") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };
		int64_t temp_dat = 0;
		float temp_val = 2.5f;
		memcpy(&temp_dat, &temp_val, 4);
		values.push_back_main(fif::fif_f32, temp_dat, nullptr);
		temp_val = 3.25f;
		memcpy(&temp_dat, &temp_val, 4);
		values.push_back_main(fif::fif_f32, temp_dat, nullptr);

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);

		temp_dat = values.main_data(0);
		memcpy(&temp_val, &temp_dat, 4);
		CHECK(temp_val == 5.75f);
		CHECK(values.main_type(0) == fif::fif_f32);
	}
	SECTION("invalid add") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };
		int64_t temp_dat = 0;
		float temp_val = 2.5f;
		memcpy(&temp_dat, &temp_val, 4);
		values.push_back_main(fif::fif_f32, temp_dat, nullptr);
		values.push_back_main(fif::fif_i32, 0, nullptr);

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count > 0);
		CHECK(error_list != "");
	}
	/*
		add_precompiled(fif_env, ":", colon_definition, { });
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
	*/
	SECTION("r at") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		values.push_back_return(fif::fif_i16, 1, nullptr);

		fif::run_fif_interpreter(fif_env, "r@", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		REQUIRE(values.return_size() == 1);
;
		CHECK(values.main_data(0) == 1);
		CHECK(values.main_type(0) == fif::fif_i16);
		CHECK(values.return_data(0) == 1);
		CHECK(values.return_type(0) == fif::fif_i16);
	}
	SECTION("from r") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		values.push_back_return(fif::fif_i16, 1, nullptr);

		fif::run_fif_interpreter(fif_env, "r>", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		REQUIRE(values.return_size() == 0);
		CHECK(values.main_data(0) == 1);
		CHECK(values.main_type(0) == fif::fif_i16);
	}
	SECTION("to r") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		values.push_back_main(fif::fif_i16, 1, nullptr);

		fif::run_fif_interpreter(fif_env, ">r", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 0);
		REQUIRE(values.return_size() == 1);
		CHECK(values.return_data(0) == 1);
		CHECK(values.return_type(0) == fif::fif_i16);
	}
	SECTION("swap") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };
		int64_t temp_dat = 0;
		float temp_val = 2.5f;
		memcpy(&temp_dat, &temp_val, 4);
		values.push_back_main(fif::fif_f32, temp_dat, nullptr);
		values.push_back_main(fif::fif_i32, 14, nullptr);

		fif::run_fif_interpreter(fif_env, "swap", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);

		temp_dat = values.main_data(1);
		memcpy(&temp_val, &temp_dat, 4);
		CHECK(temp_val == 2.5f);
		CHECK(values.main_type(1) == fif::fif_f32);
		CHECK(values.main_data(0) == 14);
		CHECK(values.main_type(0) == fif::fif_i32);
	}
	SECTION("drop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, "drop", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 2);
	}
	SECTION("dup") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, "dup", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 3);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 2);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.main_data(1) == 3);
		CHECK(values.main_type(2) == fif::fif_i32);
		CHECK(values.main_data(2) == 3);
	}
	SECTION("compound") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, "dup + +", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 8);
	}
	SECTION("compare") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, "<", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.main_data(0) != 0);
	}
	SECTION("compareb") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, ">", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.main_data(0) == 0);
	}
	SECTION("conditional a") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, "5 true if drop 6 then", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 6);
	}
	SECTION("conditional b") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n"; 
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, "5 false if drop 6 then", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 5);
	}
	SECTION("while loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, "1 while dup 5 < loop 3 + end-while", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 7);
	}
	SECTION("do loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, "1 do dup 5 < if  6 + else 2 + then until dup 10 > end-do", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 11);
	}
}

TEST_CASE("basic colon defs", "fif bytecode tests") {
	SECTION("compound") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, ": t dup + + ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 8);
	}
	SECTION("conditional a") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 5 true if drop 6 then ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 6);
	}
	SECTION("conditional b") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 5 false if drop 6 then ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 5);
	}
	SECTION("while loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 1 while dup 5 < loop 3 + end-while ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 7);
	}
	SECTION("do loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 1 do dup 5 < if  6 + else 2 + then until dup 10 > end-do ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 11);
	}
}


TEST_CASE("basic colon defs JIT", "fif jit tests") {
	SECTION("compound") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };
		values.push_back_main(fif::fif_i32, 2, nullptr);
		values.push_back_main(fif::fif_i32, 3, nullptr);

		fif::run_fif_interpreter(fif_env, ": t dup + + ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));
		
		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)(int32_t, int32_t);
			ftype fn = (ftype)bare_address;
			CHECK(fn(3, 2) == 8);
		}
	}
	SECTION("conditional a") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 5 true if drop 6 then ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 6);
		}
	}
	SECTION("conditional b") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 5 false if drop 6 then ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 5);
		}
	}
	SECTION("while loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 1 while dup 5 < loop 3 + end-while ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 7);
		}
	}
	SECTION("do loop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, ": t 1 do dup 5 < if  6 + else 2 + then until dup 10 > end-do ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 11);
		}
	}
}

TEST_CASE("bracket test", "fif combined tests") {
	SECTION("bracket bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env, 
			": if2 ] if [ ; immediate "
			": t 5 false if2 drop 6 then ; "
			"t", 
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 5);
	}
	SECTION("bracket llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": if2 ] if [ ; immediate "
			": t 5 false if2 drop 6 then ; "
			"t", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 5);
		}
	}
}

TEST_CASE("pointers tests", "fif combined tests") {
	SECTION("single pointer bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t 0 heap-alloc dup dup sizeof i16 swap ! @ swap heap-free drop ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 8);
	}
	SECTION("single pointer llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t 0 heap-alloc dup dup sizeof i16 swap ! @ swap heap-free drop ; "
			"t",
			values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 2);
		}
	}

	SECTION("buffer pointer bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t 10 buf-alloc dup 1 swap buf-add ptr-cast ptr(i32) 1 swap ! dup ptr-cast ptr(i32) @ swap buf-free ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 256);
	}

	SECTION("buffer pointer llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t 10 buf-alloc dup 1 swap buf-add ptr-cast ptr(i32) 1 swap ! dup ptr-cast ptr(i32) @ swap buf-free ; "
			"t",
			values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		// std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			auto v = fn();
			CHECK(256 <= v); // first byte is uninitialized in the llvm version -- it could be anything
			CHECK(v < 512);
		}
	}
}


TEST_CASE("variables test", "fif combined tests") {
	SECTION("let bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t let a let b a b ; "
			"1 2 t drop",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 2);
	}
	SECTION("let llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			": t let a let b a b ; "
			": r 1 2 t drop ; "
			, values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "r", { }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 2);
		}
	}

	SECTION("locals bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			"i32 global start "
			": t " // I
			"	0 var sum "
			"	dup start !! " // I 
			"	while "
			"		dup 4 < " // I bool
			"	loop " // I
			"		dup sum @ + sum ! " // I 
			"		1 + " // I
			"	end-while "
			"	drop " // -
			"	start @ sum @ + ; "
			"2 t", // 2 + (2 + 3) = 7
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 7);
	}
	SECTION("locals llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ fif_env };

		fif::run_fif_interpreter(fif_env,
			"i32 global start "
			": t "
			"	0 var sum "
			"	dup start !! "
			"	while "
			"		dup 4 < "
			"	loop "
			"		dup sum @ + sum ! "
			"		1 + "
			"	end-while "
			"	drop "
			"	start @ sum @ + ; "
			"", // 2 + (2 + 3) = 7
			values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { fif::fif_i32 }, { }, fif_env);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		LLVMOrcExecutorAddress bare_address = 0;
		auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");

		CHECK(!(error));
		if(error) {
			auto msg = LLVMGetErrorMessage(error);
			std::cout << msg << std::endl;
			LLVMDisposeErrorMessage(msg);
		} else {
			REQUIRE(bare_address != 0);
			using ftype = int32_t(*)(int32_t);
			ftype fn = (ftype)bare_address;
			CHECK(fn(2) == 7);
			CHECK(fn(1) == 7);
			CHECK(fn(0) == 6);
		}
	}
}
