#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

using llvm::Module;
using llvm::LLVMContext;
using llvm::Type;
using llvm::ModulePass;
using llvm::Function;
using llvm::checkSanitizerInterfaceFunction;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::IntegerType;
using llvm::PointerType;
using llvm::StringRef;
using llvm::Value;

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

using llvm::PassManagerBuilder;
using llvm::RegisterStandardPasses;
using llvm::legacy::PassManagerBase;

static char NondeterminismPassID;
namespace {
	struct NondeterminismPass : public ModulePass {
	private:
		StringRef name;
		Function *callback;
		bool doInitialization(Module &module);
		bool runOnFunction(Function &function);
	public:
		NondeterminismPass() : ModulePass(NondeterminismPassID)
		{}
		bool runOnModule(Module &module);
	};
}

#define CALLBACK_NAME "__nondeterminism_record_node"

bool NondeterminismPass::doInitialization(Module &module)
{
	LLVMContext &context = module.getContext();
	Type *void_type = Type::getVoidTy(context);
	IntegerType *char_type = IntegerType::getInt8Ty(context);
	PointerType *string_type = PointerType::getUnqual(char_type);

	name = module.getName();
	callback = checkSanitizerInterfaceFunction(
			module.getOrInsertFunction(CALLBACK_NAME, void_type,
							string_type, nullptr));
	return true;
}

bool NondeterminismPass::runOnFunction(Function &function)
{
	for (BasicBlock &basic_block : function) {
		BasicBlock::iterator start = basic_block.getFirstInsertionPt();
		IRBuilder<> block_instrumenter(&(*start));
		Value *
		name_str = block_instrumenter.CreateGlobalStringPtr(name);
		block_instrumenter.CreateCall(callback, {name_str});
	}

	return true;
}

bool NondeterminismPass::runOnModule(Module &module)
{
	doInitialization(module);

	for (Function &function : module) {
		runOnFunction(function);
	}

	return true;
}

static void registerNondeterminismPass(const PassManagerBuilder &builder,
				PassManagerBase &pass_manager)
{
	pass_manager.add(new NondeterminismPass());
}
static RegisterStandardPasses
RegisterReversePass(PassManagerBuilder::EP_OptimizerLast,
			registerNondeterminismPass);
static RegisterStandardPasses
RegisterReversePass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
		     registerNondeterminismPass);
