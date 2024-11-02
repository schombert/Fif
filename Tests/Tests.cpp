#define HEAP_CHECKS
#define CATCH_CONFIG_MAIN 
#pragma warning(disable : 4127)

#include "catch.hpp"
#include "..\fif-interpreter\fif.hpp"
#include <Windows.h>
#undef min
#undef max
#include <stdint.h>
#include "common_types.hpp"
#include "dcon_stuff.hpp"
#include "fif_dcon_stuff_copy.hpp"

#pragma comment(lib, "LLVM-C.lib")

using vo = fif::vsize_obj;

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
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
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
		values.push_back_main(vo(fif::fif_f32, 2.5f, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_f32, 3.25f, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);

		CHECK(values.main_type(0) == fif::fif_f32);
		CHECK(values.popr_main().as<float>() == 5.75f);
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
		values.push_back_main(vo(fif::fif_f32, 2.5f, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 0, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "+", values);

		CHECK(error_count > 0);
		CHECK(error_list != "");
	}

	SECTION("r at") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		values.push_back_return(vo(fif::fif_i16, int16_t(1), vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "r@", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		REQUIRE(values.return_size() == 1);
		
		CHECK(values.main_type(0) == fif::fif_i16);
		CHECK(values.popr_main().as<int16_t>() == 1);
		CHECK(values.return_type(0) == fif::fif_i16);
		CHECK(values.popr_return().as<int16_t>() == 1);
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

		values.push_back_return(vo(fif::fif_i16, int16_t(1), vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "r>", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		REQUIRE(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i16);
		CHECK(values.popr_main().as<int16_t>() == 1);
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

		values.push_back_main(vo(fif::fif_i16, int16_t(1), vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, ">r", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 0);
		REQUIRE(values.return_size() == 1);
		CHECK(values.return_type(0) == fif::fif_i16);
		CHECK(values.popr_return().as<int16_t>() == 1);
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
		values.push_back_main(vo(fif::fif_f32, 2.5f, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 14, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "swap", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);

		CHECK(values.main_type(1) == fif::fif_f32);
		CHECK(values.popr_main().as<float>() == 2.5f);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 14);
	}
	SECTION("drop") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "drop", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 2);
	}
	SECTION("dup") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "dup", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 3);
		CHECK(values.return_size() == 0);

		CHECK(values.main_type(2) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 3);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 3);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 2);
	}
	SECTION("compound") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "dup + +", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 8);
	}
	SECTION("compare") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, "<", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>());
	}
	SECTION("compareb") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) { ++error_count; error_list += std::string(s) + "\n"; };

		fif::interpreter_stack values{ };
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, ">", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>() == false);
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
		CHECK(values.popr_main().as<int32_t>() == 6);
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
		CHECK(values.popr_main().as<int32_t>() == 5);
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
		CHECK(values.popr_main().as<int32_t>() == 7);
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
		CHECK(values.popr_main().as<int32_t>() == 11);
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
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, ": t dup + + ; t", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 8);
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
		CHECK(values.popr_main().as<int32_t>() == 6);
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
		CHECK(values.popr_main().as<int32_t>() == 5);
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
		CHECK(values.popr_main().as<int32_t>() == 7);
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
		CHECK(values.popr_main().as<int32_t>() == 11);
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
		values.push_back_main(vo(fif::fif_i32, 2, vo::by_value{ }));
		values.push_back_main(vo(fif::fif_i32, 3, vo::by_value{ }));

		fif::run_fif_interpreter(fif_env, ": t dup + + ;", values);

		auto export_fn = fif::make_exportable_function("test_jit_fn", "t", { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

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
			using ftype = int32_t(*)(int32_t, int32_t);
			ftype fn = (ftype)bare_address;
			CHECK(fn(2, 3) == 8);
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
		CHECK(values.popr_main().as<int32_t>() == 5);
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
			": t 0 heap-alloc dup dup sizeof i16 swap ! @ swap heap-free ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 2);
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
			": t 0 heap-alloc dup dup sizeof i16 swap ! @ swap heap-free ; "
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
		CHECK(values.popr_main().as<int32_t>() == 256);
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
		CHECK(values.popr_main().as<int32_t>() == 2);
	}
	SECTION("custom swap bytecode wo lex") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": cswap ( a b ) b a ; "
			": t 1 cswap ; "
			"2.5 t drop ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
	}
	SECTION("custom swap bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": cswap lex-scope ( a b ) b a end-lex ; "
			": t 1 cswap ; "
			"2.5 t drop ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
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
	SECTION("custom swap llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": cswap lex-scope ( a b ) b a end-lex ; "
			": t 1 cswap drop ; "
			":export test_jit_fn f32 t ; ",
			values);

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
			using ftype = int32_t(*)(float);
			ftype fn = (ftype)bare_address;
			CHECK(fn(3.0f) == 1);
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
		CHECK(values.popr_main().as<int32_t>() == 7);
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
		CHECK(values.popr_main().as<int32_t>() == fif::fif_bool);
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
		CHECK(values.popr_main().as<int32_t>() == 100);
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
		CHECK(values.popr_main().as<float>() == 2.5f);
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
		CHECK(values.popr_main().as<float>() == 2.5f);
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
		CHECK(values.popr_main().as<int32_t>() == 0);
		CHECK(values.popr_main().as<int32_t>() == 1);
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
		CHECK(values.popr_main().as<int32_t>() == 1);
		CHECK(values.popr_main().as<int32_t>() == 1);
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
		CHECK(values.popr_main().as<int32_t>() == 42);
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
		CHECK(values.popr_main().as<int32_t>() == 0);
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
		CHECK(values.popr_main().as<int32_t>() == 4);
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
		CHECK(values.popr_main().as<int32_t>() == 7);
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
			//": t make dy-array(i32) 4 push pop swap drop ; ",
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
		CHECK(values.popr_main().as<int32_t>() == 8);
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
void set_global(int32_t low_stack_dummy, int32_t v) {
	our_global = v;
}
uint16_t* interpreted_set_global(fif::state_stack& s, uint16_t* p, fif::environment* e) {
	if(e->mode == fif::fif_mode::interpreting) {
		our_global = int32_t(s.popr_main().as<int32_t>());
		s.pop_main();
	}
	return p;
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
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 0 10 set-global ; "
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
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 0 12 set-global ; ",
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
		fif::add_import("set-global", set_global, interpreted_set_global, { fif::fif_i32, fif::fif_i32 }, { }, fif_env);

		fif::run_fif_interpreter(fif_env,
			": t 0 12 set-global ; "
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

TEST_CASE("dcon integration tests", "fif combined tests") {
	SECTION("read_value") {
		auto container = std::make_unique<dcon::data_container>();

		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		auto thandle = container->create_thingy();
		container->thingy_set_some_value(thandle, 42);

		fif::run_fif_interpreter(fif_env,
			fif::container_interface(),
			values);

		values.push_back_main(vo(fif::fif_opaque_ptr, container.get(), vo::by_value{ }));
		fif::run_fif_interpreter(fif_env,
			"set-container ",
			values);
		values.push_back_main(vo(fif::fif_opaque_ptr, dcon::shared_backing_storage.allocation, vo::by_value{ }));
		fif::run_fif_interpreter(fif_env,
			"set-vector-storage ",
			values);

		fif::run_fif_interpreter(fif_env,
			"0 >thingy_id some_value @ ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 42);

		auto thandleb = container->create_thingy();
		container->thingy_set_bf_value(thandleb, true);

		fif::run_fif_interpreter(fif_env,
			"1 >thingy_id bf_value @ ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>());


		fif::run_fif_interpreter(fif_env,
			"0 >thingy_id bf_value @ ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>() == false);

		fif::run_fif_interpreter(fif_env,
			"true 0 >thingy_id bf_value ! ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 0);
		CHECK(values.return_size() == 0);

		CHECK(container->thingy_get_bf_value(thandle) == true);

		container->thingy_resize_big_array(7);
		container->thingy_set_big_array(thandleb, 3, 2.5f);

		fif::run_fif_interpreter(fif_env,
			"1 >thingy_id 3 big_array @ ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		float fvalue = 2.5f;
		int64_t ivalue = 0;
		memcpy(&ivalue, &fvalue, 4);
		CHECK(values.main_type(0) == fif::fif_f32);
		CHECK(values.popr_main().as<int32_t>() == ivalue);

		container->thingy_resize_big_array_bf(13);
		container->thingy_set_big_array_bf(thandle, 6, true);

		fif::run_fif_interpreter(fif_env,
			"0 >thingy_id 6 big_array_bf @ ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>());

		auto vp = container->thingy_get_pooled_v(thandleb);
		vp.push_back(4);
		vp.push_back(18);
		vp.push_back(21);

		fif::run_fif_interpreter(fif_env,
			"1 >thingy_id pooled_v size ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 3);

		fif::run_fif_interpreter(fif_env,
			"0 >thingy_id pooled_v size ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 0);

		fif::run_fif_interpreter(fif_env,
			"1 >thingy_id pooled_v 1 index @ ",
			values);
		
		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i16);
		CHECK(values.popr_main().as<int32_t>() == 18);

		auto pa  = container->create_person(); 
		auto pb = container->create_person();

		auto ca = container->create_car();
		auto cb = container->create_car();
		auto cc = container->create_car();

		container->force_create_car_ownership(pa, cb);
		container->force_create_car_ownership(pb, ca);
		container->force_create_car_ownership(pb, cc);

		container->person_set_age(pb, 80);

		fif::run_fif_interpreter(fif_env,
			"0 >person_id live? ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_bool);
		CHECK(values.popr_main().as<bool>());


		fif::run_fif_interpreter(fif_env,
			"2 >car_id car_ownership-owned_car @ owner @ age @",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 80);

		fif::run_fif_interpreter(fif_env,
			"thingy-size ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 2);
	}
	SECTION("read_value llvm") {
		auto container = std::make_unique<dcon::data_container>();

		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		auto thandle = container->create_thingy();
		container->thingy_set_some_value(thandle, 42);
		auto thandleb = container->create_thingy();
		container->thingy_set_bf_value(thandleb, true);
		container->thingy_resize_big_array(7);
		container->thingy_set_big_array(thandleb, 3, 2.5f);
		container->thingy_resize_big_array_bf(13);
		container->thingy_set_big_array_bf(thandle, 6, true);
		auto vp = container->thingy_get_pooled_v(thandleb);
		vp.push_back(4);
		vp.push_back(18);
		vp.push_back(21);
		auto pa = container->create_person();
		auto pb = container->create_person();

		auto ca = container->create_car();
		auto cb = container->create_car();
		auto cc = container->create_car();

		container->force_create_car_ownership(pa, cb);
		container->force_create_car_ownership(pb, ca);
		container->force_create_car_ownership(pb, cc);

		container->person_set_age(pb, 80);

		fif::run_fif_interpreter(fif_env,
			fif::container_interface(),
			values);

		fif::run_fif_interpreter(fif_env,
			": t 0 >thingy_id some_value @ ; "
			": t2 1 >thingy_id bf_value @ ; "
			": t3 true 0 >thingy_id bf_value ! ; "
			": t4 1 >thingy_id 3 big_array @ ; "
			": t5 0 >thingy_id 6 big_array_bf @ ; "
			": t6 1 >thingy_id pooled_v size ; "
			": t7 1 >thingy_id pooled_v 1 index @ ; "
			": t8 2 >car_id car_ownership-owned_car @ owner @ age @ ; "
			":export test_jit_fn t ; "
			":export test_jit_fnB t2 ; "
			":export test_jit_fnC t3 ; "
			":export test_jit_fnD t4 ; "
			":export test_jit_fnE t5 ; "
			":export test_jit_fnF t6 ; "
			":export test_jit_fnG t7 ; "
			":export test_jit_fnH t8 ; ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		fif::perform_jit(fif_env);

		REQUIRE(bool(fif_env.llvm_jit));

		FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "set_container");
			REQUIRE(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = void(*)(void*);
				ftype fn = (ftype)bare_address;
				fn(container.get());
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "set_vector_storage");
			REQUIRE(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = void(*)(void*);
				ftype fn = (ftype)bare_address;
				fn(dcon::shared_backing_storage.allocation);
			}
		}
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

		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnB");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = bool(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == true);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnC");
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
				CHECK(container->thingy_get_bf_value(thandle) == true);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnD");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = float(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 2.5f);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnE");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = bool(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == true);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnF");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = int32_t(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 3);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnG");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = int16_t(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 18);
			}
		}
		{
			LLVMOrcExecutorAddress bare_address = 0;
			auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, "test_jit_fnH");
			CHECK(!(error));
			if(error) {
				auto msg = LLVMGetErrorMessage(error);
				std::cout << msg << std::endl;
				LLVMDisposeErrorMessage(msg);
			} else {
				REQUIRE(bare_address != 0);
				using ftype = int32_t(*)();
				ftype fn = (ftype)bare_address;
				CHECK(fn() == 80);
			}
		}
	}
}

TEST_CASE("relet tests", "fif combined tests") {
	SECTION("simple_relet") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 1 let x x 5 -> x x + ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 6);
	}
	SECTION("simple_relet llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 1 let x x 5 -> x x + ; "
			":export test_jit_fn t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				CHECK(fn() == 6);
			}
		}
	}
	SECTION("cond relet") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 3 let x 5 > if 1 -> x then x ; "
			"7 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
	}
	SECTION("cond relet llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 3 let x 5 > if 1 -> x then x ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(7) == 1);
				CHECK(fn(0) == 3);
			}
		}
	}
	SECTION("while relet") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 0 let x while dup 0 > loop 1 - dup 1 and 0 <> if x 1 + -> x then end-while drop x ; "
			"7 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 3);
	}
	SECTION("while relet llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 0 let x while dup 0 > loop 1 - dup 1 and 0 <> if x 1 + -> x then end-while drop x ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(7) == 3);
				CHECK(fn(4) == 2);
			}
		}
	}
}

TEST_CASE("adv control flow tests", "fif combined tests") {
	SECTION("simple_break") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t do dup 5 >= if break then 1 + until false end-do ; "
			"3 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("simple_break llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t do dup 5 >= if break then 1 + until false end-do ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(2) == 5);
			}
		}
	}

	SECTION("simple_break2") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t let i do i 5 >= if break then i 1 + -> i until false end-do i ; "
			"3 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("simple_break2 llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t let i do i 5 >= if break then i 1 + -> i until false end-do i ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(2) == 5);
			}
		}
	}


	SECTION("simple_return") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 5 >= if 1 return else 0 return then 7 ; "
			"8 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
	}
	SECTION("simple_return llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t 5 >= if 1 return else 0 return then 7 ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(2) == 0);
				CHECK(fn(8) == 1);
			}
		}
	}


	SECTION("3andif") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t dup 5 > if dup 10 < &if drop 1 else dup 10 >= &if drop 2 else drop 0 then ; "
			"3 t 12 t 7 t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 3);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(2) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 2);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 0);
		
	}
	SECTION("3andif llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			": t dup 5 > if dup 10 < &if drop 1 else dup 10 >= &if drop 2 else drop 0 then ; "
			":export test_jit_fn i32 t ;",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(2) == 0);
				CHECK(fn(7) == 1);
				CHECK(fn(13) == 2);
			}
		}
	}
}

TEST_CASE("parameter permutation detection", "fif compiler tests") {
	SECTION("trivial") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t >r dup r> + ; 5 4 t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 9);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);

		auto& wi = std::get<fif::interpreted_word_instance>(fif_env.dict.all_instances.back());
		REQUIRE(wi.llvm_parameter_permutation.size() >= 1);
		CHECK(wi.llvm_parameter_permutation.size() == 1);
		CHECK(wi.llvm_parameter_permutation[0] == 0);
	}

	SECTION("many returns") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": y dup 1 + dup 1 + dup 1 + dup 1 + ; "
			": z dup 1 + dup 1 + dup 1 + ; "
			": t y + + + + ; "
			": u z + + + ; "
			":export test_jit_fn i32 t ; "
			":export test_jit_fn i32 u ; ", values);

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(1) == 15);
			}
		}
	}
}


TEST_CASE("advanced structures", "fif combined tests") {
	SECTION("anon struct and destruct") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t 5 < if { 1 0.5 }struct else { 7 2.5 }struct then de-struct drop ; 1 t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
	}

	SECTION("anon struct and destruct llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t 5 < if { 1 0.5 }struct else { 7 2.5 }struct then de-struct drop ; "
			":export test_jit_fn i32 t ; ", values);

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(1) == 1);
				CHECK(fn(5) == 7);
			}
		}
	}

	SECTION("empty struct A") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, 
			":struct A ; "
			":struct B ; "
			":struct C A high B low ; "
			": t make C swap 5 < if drop make C then .high ; "
			"1 t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) != 0);
		CHECK(values.popr_main().size == 0);
	}

	SECTION("partially empty struct B") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			":struct A ; "
			":struct C f32 f A mid i32 i ; "
			": t make C swap 5 < if 3 swap .i! then .i ; "
			"1 t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 3);
	}

	SECTION("empty struct A llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			":struct A ; "
			":struct B ; "
			":struct C A high B low ; "
			": t make C swap 5 < if drop make C then .high drop 1 ; "
			":export test_jit_fn i32 t ; ", values);

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(1) == 1);
				CHECK(fn(5) == 1);
			}
		}
	}

	SECTION("partially empty struct B llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			":struct A ; "
			":struct C f32 f A mid i32 i ; "
			": t make C swap 5 < if 3 swap .i! then .i ; "
			":export test_jit_fn i32 t ; ", values);

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
				using ftype = int32_t(*)(int32_t);
				ftype fn = (ftype)bare_address;
				CHECK(fn(1) == 3);
				CHECK(fn(5) == 0);
			}
		}
	}

	SECTION("array size") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t make array(i32,4) size ; t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 4);
	}
	SECTION("array size llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": t make array(i32,4) size ; "
			":export test_jit_fn t ; ", values);

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
	SECTION("array store value") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t make array(i32,4) let x 5 1 x index ! 2 x index @ 1 x index @ + ; t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("array store value llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": t make array(i32,4) let x 5 1 x index ! 2 x index @ 1 x index @ + ; "
			":export test_jit_fn t ; ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

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
				auto fn_result = fn();
				CHECK(fn_result == 5);
			}
		}
	}
	SECTION("array copy") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t make array(i32,4) let x 5 1 x index ! x copy let y drop 1 y index @ ; t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("array copy llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": t make array(i32,4) let x 5 1 x index ! x copy let y drop 1 y index @ ; "
			":export test_jit_fn t ; ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

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
				auto fn_result = fn();
				CHECK(fn_result == 5);
			}
		}
	}
	SECTION("array of array copy") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t make array(array(i32,2),4) let x "
			"10 1 3 x index index ! "
			"1 3 x index index @ ; t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 10);
	}
	SECTION("array of array copy llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": t make array(array(i32,2),4) let x "
			"10 1 3 x index index ! "
			"1 3 x index index @ ; "
			":export test_jit_fn t ; ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

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
				auto fn_result = fn();
				CHECK(fn_result == 10);
			}
		}
	}
	SECTION("inline array") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, ": t 2 { 10 20 30 }array index @ ; t ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 30);
	}
	SECTION("inline array llvm") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			": t 2 { 10 20 30 }array index @ ; "
			":export test_jit_fn t ; ", values);

		CHECK(error_count == 0);
		CHECK(error_list == "");

		//std::cout << LLVMPrintModuleToString(fif_env.llvm_module) << std::endl;

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
				auto fn_result = fn();
				CHECK(fn_result == 30);
			}
		}
	}
	SECTION("m struct definition bytecode") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };

		fif::run_fif_interpreter(fif_env,
			":m-struct pair i32 low f32 high ; "
			": t 2.5 make pair let X X .high ! X .high @ ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_f32);
		CHECK(values.popr_main().as<float>() == 2.5f);
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
			":m-struct pair i32 low f32 high ; "
			": t 2.5 make pair let X X .high ! X .high @ ; "
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
			":m-struct pair i32 low $0 high ; "
			": t 2.5 make pair(f32) let X X .high !  X .high @ ; "
			"t",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_f32);
		CHECK(values.popr_main().as<float>() == 2.5f);
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
			":m-struct pair i32 low $0 high ; "
			": t 2.5 make pair(f32) let X X .high !  X .high @ ; "
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
			":m-struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup .low @ 1 + .low ! ; "
			": t make pair(f32) dup swap drop .low @ ; "
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
			":m-struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup use-base dup .low @ 1 + swap .low ! ; "
			": t make pair(f32) dup swap drop .low @ ; "
			": t2 make pair(bool) dup swap drop .low @ ; "
			"t t2 ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 0);
		CHECK(values.popr_main().as<int32_t>() == 1);
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
			":m-struct pair i32 low $0 high ; "
			":s dup pair($0) s: use-base dup use-base dup use-base dup .low @ 1 + swap .low ! ; "
			": t make pair(f32) dup swap drop .low @ ; "
			": t2 make pair(bool) dup swap drop .low @ ; "
			"t t2 ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 2);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.main_type(1) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 1);
		CHECK(values.popr_main().as<int32_t>() == 1);
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
			":m-struct pair i32 low $0 high ; "
			":s dup pair(f32) s: use-base dup use-base dup use-base dup .low @ 1 + swap .low ! ; "
			": t make pair(f32) dup swap drop .low @ ; "
			": t2 make pair(bool) dup swap drop .low @ ; "
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
			":m-struct pair i32 low $0 high ; "
			":s dup pair($0) s: use-base dup use-base dup use-base dup .low @ 1 + swap .low ! ; "
			": t make pair(f32) dup swap drop .low @ ; "
			": t2 make pair(bool) dup swap drop .low @ ; "
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

TEST_CASE("modules tests", "fif combined tests") {
	SECTION("find in module") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env, 
			"new-module test-mod "
			": t 5 ; "
			"end-module test-mod "
			"test-mod.t ", 
		values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("nested module") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			"new-module testA "
			"new-module testB "
			": t 5 ; "
			"end-module testB "
			"end-module testA "
			"testA.testB.t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("nested module 2") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			"new-module testA "
			": c 5 ; "
			"new-module testB "
			": t c ; "
			"end-module testB "
			"end-module testA "
			"testA.testB.t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("name ambiguity") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			"new-module testA "
			": t 5 ; "
			"end-module testA "
			"new-module testB "
			": t 7 ; "
			"end-module testB "
			"testA.t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("using directive") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			"new-module testA "
			"new-module testB "
			": c 5 ; "
			"end-module testB "
			"using-module testA.testB "
			": t c ; "
			"end-module testA "
			"testA.t ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
	SECTION("ADL") {
		fif::environment fif_env;
		fif::initialize_standard_vocab(fif_env);

		int32_t error_count = 0;
		std::string error_list;
		fif_env.report_error = [&](std::string_view s) {
			++error_count; error_list += std::string(s) + "\n";
		};

		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(fif_env,
			"new-module testA "
			":struct vals i32 v ; "
			"end-module testA "
			"5 make testA.vals .v! .v ",
			values);

		CHECK(error_count == 0);
		CHECK(error_list == "");
		REQUIRE(values.main_size() == 1);
		CHECK(values.return_size() == 0);
		CHECK(values.main_type(0) == fif::fif_i32);
		CHECK(values.popr_main().as<int32_t>() == 5);
	}
}

