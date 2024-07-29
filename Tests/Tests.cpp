#define HEAP_CHECKS
#define CATCH_CONFIG_MAIN 
#pragma warning(disable : 4127)

#include "catch.hpp"
#include "..\fif-interpreter\fif.hpp"
#include <Windows.h>
#undef min
#undef max
#include <stdint.h>

#pragma comment(lib, "LLVM-C.lib")

TEST_CASE("trivial test cases", "fif jit tests") {
	SECTION("trivial") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t 42 ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

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
			auto value = fn();
			CHECK(value == 42);
		}
	}
}
TEST_CASE("fundamental calls", "fif interpreter tests") {
	SECTION("int add") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };
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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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

		fif::interpreter_stack values{ };

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



TEST_CASE("specialization test", "fif combined tests") {
	SECTION("specialization a bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":s t $0 s: drop $0 ; "
			":s t i32 s: 100 + ;"
			" true t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_type);
		CHECK(values.main_data(0) == fif::fif_bool);
	}
	SECTION("specialization a bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":s t $0 s: drop $0 ; "
			":s t i32 s: 100 + ;"
			" 0 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 100);
	}
	SECTION("specialization a llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":s t $0 s: drop $0 ; "
			":s t i32 s: 100 + ;",
		values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { fif::fif_bool }, { }, fif_env);

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
			using ftype = int32_t(*)(bool);
			ftype fn = (ftype)bare_address;
			CHECK(fn(true) == fif::fif_bool);
		}
	}
	SECTION("specialization b llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":s t $0 s: drop $0 ; "
			":s t i32 s: 100 + ;",
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
			CHECK(fn(1) == 101);
		}
	}
}


TEST_CASE("struct tests", "fif combined tests") {
	SECTION("struct definition bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low f32 high ; "
			": t 2.5 make pair .high! .high ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_f32);
		int32_t val = 0;
		float fval = 2.5;
		memcpy(&val, &fval, 4);
		CHECK(values.main_data(0) == val);
	}
	SECTION("struct definition llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low f32 high ; "
			": t 2.5 make pair .high! .high ; "
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
			using ftype = float(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 2.5);
		}
	}

	SECTION("struct template definition bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			": t 2.5 make pair(f32) .high! .high ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_f32);
		int32_t val = 0;
		float fval = 2.5;
		memcpy(&val, &fval, 4);
		CHECK(values.main_data(0) == val);
	}
	SECTION("struct template definition llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			": t 2.5 make pair(f32) .high! .high ; "
			,
			values);

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
			using ftype = float(*)();
			ftype fn = (ftype)bare_address;
			CHECK(fn() == 2.5);
		}
	}

	SECTION("struct overloading error bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup .low 1 + .low! ; "
			": t make pair(f32) dup swap drop .low ; "
			"t ",
			values);

		CHECK(error_count > 0);
		CHECK(error_list != "");
	}

	SECTION("struct overloading a bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup .low 1 + swap .low! ; "
			": t make pair(f32) dup swap drop .low ; "
			": t2 make pair(bool) dup swap drop .low ; "
			"t t2 ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.main_data(0) == 1);
		CHECK(values.main_data(1) == 0);
	}
	SECTION("struct overloading b bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			":s dup pair($0) s: use-base dup use-base dup .low 1 + swap .low! ; "
			": t make pair(f32) dup swap drop .low ; "
			": t2 make pair(bool) dup swap drop .low ; "
			"t t2 ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.main_data(0) == 1);
		CHECK(values.main_data(1) == 1);
	}
	SECTION("struct overloading a llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup .low 1 + swap .low! ; "
			": t make pair(f32) dup swap drop .low ; "
			": t2 make pair(bool) dup swap drop .low ; "
			,
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);
		fif::make_exportable_function("test_jit_fn2", "t2", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 1);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn2");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = int32_t(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 0);
			}
		}
	}
	SECTION("struct overloading b llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct pair i32 low $0 high ; "
			":s dup pair($0) s: use-base dup use-base dup .low 1 + swap .low! ; "
			": t make pair(f32) dup swap drop .low ; "
			": t2 make pair(bool) dup swap drop .low ; "
			"t t2 ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);
		fif::make_exportable_function("test_jit_fn2", "t2", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 1);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn2");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = int32_t(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 1);
			}
		}
	}
}

TEST_CASE("array_tests", "fif combined tests") {
	SECTION("buf resize") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t sizeof i32 2 * buf-alloc dup sizeof i32 swap buf-add ptr-cast ptr(i32) 42 swap ! "
			"	sizeof i32 2 * sizeof i32 4 * buf-resize sizeof i32 swap buf-add ptr-cast ptr(i32) @ "
			" ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 42);
	}
	SECTION("buf resize llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t sizeof i32 2 * buf-alloc dup sizeof i32 swap buf-add ptr-cast ptr(i32) 42 swap ! "
			"	sizeof i32 2 * sizeof i32 4 * buf-resize sizeof i32 swap buf-add ptr-cast ptr(i32) @ "
			" ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 42);
			}
		}
	}

	SECTION("dy-array defines") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct tdy-array-block ptr(nil) memory i32 size i32 capacity i32 refcount ; "
			": m sizeof tdy-array-block buf-alloc ptr-cast ptr(tdy-array-block) dup make tdy-array-block swap !! ; "
			": t m dup .capacity @ swap drop ; "
			" t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 0);
	}
	
	SECTION("dy-array defines llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":struct tdy-array-block ptr(nil) memory i32 size i32 capacity i32 refcount ; "
			": m sizeof tdy-array-block buf-alloc ptr-cast ptr(tdy-array-block) dup make tdy-array-block swap !! ; "
			": t m dup .capacity @ swap drop ; ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 0);
			}
		}
	}

	SECTION("dy-array defines b") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push pop swap drop ; "
			" t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 4);
	}

	SECTION("dy-array defines b llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push pop swap drop ; ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 4);
			}
		}
	}

	SECTION("dy-array multi-push") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push 5 push 6 push 7 push 8 push 9 push pop drop pop drop pop let result pop drop pop drop pop drop drop result ; "
			" t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 7);
	}

	SECTION("dy-array multi-push llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push 5 push 6 push 7 push 8 push 9 push pop drop pop drop pop let result pop drop pop drop pop drop drop result ; ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
	}

	SECTION("dy-array indexing") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push 5 push 6 push 7 push 8 push 9 push 4 index-into @ swap drop ; "
			" t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_data(0) == 8);
	}

	SECTION("dy-array indexing llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t make dy-array(i32) 4 push 5 push 6 push 7 push 8 push 9 push 4 index-into @ swap drop ; ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
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
				CHECK(fn() == 8);
			}
		}
	}
	/**/
}

static int32_t our_global = 0;
void set_global(int32_t v) {
	our_global = v;
}
int32_t* interpreted_set_global(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::interpreting)
		our_global = int32_t(s.main_data_back(0));
	s.pop_main();
	return p + 2;
}

TEST_CASE("import_export_tests", "fif combined tests") {
	SECTION("import") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		our_global = 0;
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 10 set-global ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		CHECK(values.main_size() == 0);
		CHECK(values.return_size() == 0);
		CHECK(our_global == 10);
	}
	SECTION("import llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		our_global = 0;
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 12 set-global ; ",
			values);

		fif::make_exportable_function("test_jit_fn", "t", { }, { }, fif_env);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = void(*)();
				ftype fn = (ftype)bare_address;
				fn();
				CHECK(our_global == 12);
			}
		}
	}
	SECTION("import export llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		our_global = 0;
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 12 set-global ; "
			":export test_jit_fn t ; ",
			values);

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fn");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = void(*)();
				ftype fn = (ftype)bare_address;
				fn();
				CHECK(our_global == 12);
			}
		}
	}
}
