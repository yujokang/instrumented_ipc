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
using llvm::ConstantInt;
using llvm::Constant;
using llvm::ConstantPointerNull;
using llvm::GlobalValue;
using llvm::GlobalVariable;
using llvm::LoadInst;

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

using llvm::PassManagerBuilder;
using llvm::RegisterStandardPasses;
using llvm::legacy::PassManagerBase;

static char NondeterminismPassID;
namespace {
	struct NondeterminismPass : public ModulePass {
	private:
		Function *checker, *callback;
		IntegerType *status_type;
		PointerType *need_check_type, *status_ptr_type;
		ConstantPointerNull *known_name_init;
		Constant *ignore_init;
		bool doInitialization(Module &module);
		bool runPerFunction(Function &function, Module &module);
	public:
		NondeterminismPass() : ModulePass(NondeterminismPassID)
		{}
		bool runOnModule(Module &module);
	};
}

#define CHECKER_NAME "__nondeterminism_check_node"
#define CALLBACK_NAME "__nondeterminism_record_node"

bool NondeterminismPass::doInitialization(Module &module)
{
	LLVMContext &context = module.getContext();
	Type *void_type = Type::getVoidTy(context);
	IntegerType *char_type = IntegerType::getInt8Ty(context);
	PointerType *string_type = PointerType::getUnqual(char_type);

	need_check_type = PointerType::getUnqual(string_type);
	known_name_init = ConstantPointerNull::get(string_type);

	status_type = IntegerType::getInt32Ty(context);
	status_ptr_type = PointerType::getUnqual(status_type);
	ignore_init = ConstantInt::getFalse(status_type);
	checker = checkSanitizerInterfaceFunction(
			module.getOrInsertFunction(CHECKER_NAME, void_type,
							string_type,
							need_check_type,
							status_ptr_type,
							nullptr));
	callback = checkSanitizerInterfaceFunction(
			module.getOrInsertFunction(CALLBACK_NAME, void_type,
							status_type,
							string_type,
							nullptr));
	return true;
}

bool NondeterminismPass::runPerFunction(Function &function, Module &module)
{
	bool need_check_code = true;
	StringRef name = function.getName();
	GlobalVariable *
	need_check_var = new GlobalVariable(module, need_check_type, false,
						GlobalValue::InternalLinkage,
						0, "__nondeterminism_checked_" +
							name),
	*result_var = new GlobalVariable(module, status_type, false,
					GlobalValue::InternalLinkage,
					0, "__nondeterminism_ignore_" + name);

	need_check_var->setInitializer(known_name_init);
	result_var->setInitializer(ignore_init);
	for (BasicBlock &basic_block : function) {
		BasicBlock::iterator start = basic_block.getFirstInsertionPt();
		IRBuilder<> block_instrumenter(&(*start));
		Value *
		name_str = block_instrumenter.CreateGlobalStringPtr(name);
		LoadInst *result;

		if (need_check_code) {
			block_instrumenter.CreateCall(checker,
							{name_str,
								need_check_var,
								result_var});
			need_check_code = false;
		}
		result = block_instrumenter.CreateLoad(result_var);
		block_instrumenter.CreateCall(callback, {result, name_str});
	}

	return true;
}

bool NondeterminismPass::runOnModule(Module &module)
{
	doInitialization(module);

	for (Function &function : module) {
		runPerFunction(function, module);
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
