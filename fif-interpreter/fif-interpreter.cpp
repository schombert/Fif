
#include <iostream>
#include "fif.hpp"
#include <Windows.h>
#undef min
#undef max
#include <stdint.h>

#pragma comment(lib, "LLVM-C.lib")

fif::environment fif_env;


//using fif_call = int32_t * (*)(state_stack&, int32_t*, environment*);


void report_error(std::string_view err) {
	std::cout << "ERROR: " << err << std::endl;
}

int main() {
	fif::initialize_standard_vocab(fif_env);

	uint32_t position = 0;
	/*
	std::string fif_execute =
		": tuck swap over ; "
		":: foreach `->range `while `dup `empty? `not `loop `next ; "
		":: end-foreach `end-while `drop ; "
		 ": t3 2 >r .r r> . .nl ; "
		"t3 .s .r "
		;
	*/
	/*
	std::string fif_execute =
		": stub rec ; "
		": rec dup 10 < if 1 + stub then ; "
		"2 rec . .nl 13 rec . .nl .s .r "
		;
	*/
	std::string fif_execute =
		": add2 + + ; "
		"1 2 3 add2 "
		;
	fif_env.report_error = report_error;

	fif::run_fif_interpreter(fif_env, fif_execute);
	
	/*
	auto export_fn = fif::make_exportable_function("test_jit_fn", "zf", { }, { }, fif_env);

	fif::perform_jit(fif_env);
	FlushInstructionCache(GetCurrentProcess(), nullptr, 0);

	LLVMOrcExecutorAddress bare_address = 0;
	auto mang_name = LLVMOrcLLJITMangleAndIntern(fif_env.llvm_jit, "test_jit_fn");
	std::cout << "mangled as: " << LLVMOrcSymbolStringPoolEntryStr(mang_name) << std::endl;
	auto error = LLVMOrcLLJITLookup(fif_env.llvm_jit, &bare_address, LLVMOrcSymbolStringPoolEntryStr(mang_name));

	if(error) {
		auto msg = LLVMGetErrorMessage(error);
		std::cout << msg << std::endl;
		LLVMDisposeErrorMessage(msg);
		LLVMConsumeError(error);
	} else {
		using ftype = int32_t(*)();
		ftype fn = (ftype)bare_address;
		auto value = fn();
		std::cout << value << std::endl;
	}
	LLVMOrcReleaseSymbolStringPoolEntry(mang_name);
	/**/
}

