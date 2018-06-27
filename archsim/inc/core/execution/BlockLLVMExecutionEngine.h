/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   BlockLLVMExecutionEngine.h
 * Author: harry
 *
 * Created on 11 May 2018, 16:32
 */

#ifndef BLOCKLLVMEXECUTIONENGINE_H
#define BLOCKLLVMEXECUTIONENGINE_H

#include "core/execution/ExecutionEngine.h"
#include "BlockJITExecutionEngine.h"
#include "translate/adapt/BlockJITToLLVM.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			class BlockJITLLVMMemoryManager : public llvm::RTDyldMemoryManager
			{
			public:
				BlockJITLLVMMemoryManager(wulib::MemAllocator &allocator);

				uint8_t* allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName) override;
				uint8_t* allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName, bool IsReadOnly) override;

				bool finalizeMemory(std::string* ErrMsg) override;

				uintptr_t GetSectionSize(uint8_t* ptr) const
				{
					return section_sizes_.at(ptr);
				}
			private:
				wulib::MemAllocator &allocator_;

				std::vector<std::pair<uint8_t*, uintptr_t>> outstanding_code_sections_;
				std::map<uint8_t*, uintptr_t> section_sizes_;

			};

			class BlockLLVMExecutionEngine : public BlockJITExecutionEngine
			{
			public:

				BlockLLVMExecutionEngine(gensim::blockjit::BaseBlockJITTranslate *translator);

				virtual ~BlockLLVMExecutionEngine()
				{

				}

				static ExecutionEngine *Factory(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix);

			private:
				bool translateBlock(thread::ThreadInstance* thread, archsim::Address block_pc, bool support_chaining, bool support_profiling) override;
				bool buildBlockJITIR(thread::ThreadInstance *thread, archsim::Address block_pc, captive::arch::jit::TranslationContext &ctx, archsim::blockjit::BlockTranslation &txln);

				llvm::LLVMContext llvm_ctx_;
				archsim::translate::adapt::BlockJITToLLVMAdaptor adaptor_;

				std::shared_ptr<BlockJITLLVMMemoryManager> memory_manager_;
				std::unique_ptr<llvm::TargetMachine> target_machine_;
				llvm::orc::RTDyldObjectLinkingLayer linker_;
				llvm::orc::IRCompileLayer<decltype(linker_), llvm::orc::SimpleCompiler> compiler_;
			};
		}
	}
}

#endif /* BLOCKLLVMEXECUTIONENGINE_H */
