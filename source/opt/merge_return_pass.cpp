// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "merge_return_pass.h"

#include "instruction.h"
#include "ir_context.h"

namespace spvtools {
namespace opt {

Pass::Status MergeReturnPass::Process(ir::IRContext* irContext) {
  InitializeProcessing(irContext);

  // TODO (alanbaker): Support structured control flow. Bail out in the
  // meantime.
  if (get_module()->HasCapability(SpvCapabilityShader))
    return Status::SuccessWithoutChange;

  bool modified = false;
  for (auto& function : *get_module()) {
    std::vector<ir::BasicBlock*> returnBlocks = CollectReturnBlocks(&function);
    modified |= MergeReturnBlocks(&function, returnBlocks);
  }

  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

std::vector<ir::BasicBlock*> MergeReturnPass::CollectReturnBlocks(
    ir::Function* function) {
  std::vector<ir::BasicBlock*> returnBlocks;
  for (auto& block : *function) {
    ir::Instruction& terminator = *block.tail();
    if (terminator.opcode() == SpvOpReturn ||
        terminator.opcode() == SpvOpReturnValue) {
      returnBlocks.push_back(&block);
    }
  }

  return returnBlocks;
}

bool MergeReturnPass::MergeReturnBlocks(
    ir::Function* function, const std::vector<ir::BasicBlock*>& returnBlocks) {
  if (returnBlocks.size() <= 1) {
    // No work to do.
    return false;
  }

  // Create a label for the new return block
  std::unique_ptr<ir::Instruction> returnLabel(
      new ir::Instruction(SpvOpLabel, 0u, TakeNextId(), {}));
  uint32_t returnId = returnLabel->result_id();

  // Create the new basic block
  std::unique_ptr<ir::BasicBlock> returnBlock(
      new ir::BasicBlock(std::move(returnLabel)));
  function->AddBasicBlock(std::move(returnBlock));
  ir::Function::iterator retBlockIter = --function->end();

  // Create the PHI for the merged block (if necessary)
  // Create new return
  std::vector<ir::Operand> phiOps;
  for (auto block : returnBlocks) {
    if (block->tail()->opcode() == SpvOpReturnValue) {
      phiOps.push_back(
          {SPV_OPERAND_TYPE_ID, {block->tail()->GetSingleWordInOperand(0u)}});
      phiOps.push_back({SPV_OPERAND_TYPE_ID, {block->id()}});
    }
  }

  if (!phiOps.empty()) {
    // Need a PHI node to select the correct return value.
    uint32_t phiResultId = TakeNextId();
    uint32_t phiTypeId = function->type_id();
    std::unique_ptr<ir::Instruction> phiInst(
        new ir::Instruction(SpvOpPhi, phiTypeId, phiResultId, phiOps));
    retBlockIter->AddInstruction(std::move(phiInst));
    ir::BasicBlock::iterator phiIter = retBlockIter->tail();

    std::unique_ptr<ir::Instruction> returnInst(new ir::Instruction(
        SpvOpReturnValue, 0u, 0u, {{SPV_OPERAND_TYPE_ID, {phiResultId}}}));
    retBlockIter->AddInstruction(std::move(returnInst));
    ir::BasicBlock::iterator ret = retBlockIter->tail();

    get_def_use_mgr()->AnalyzeInstDefUse(&*phiIter);
    get_def_use_mgr()->AnalyzeInstDef(&*ret);
  } else {
    std::unique_ptr<ir::Instruction> returnInst(
        new ir::Instruction(SpvOpReturn));
    retBlockIter->AddInstruction(std::move(returnInst));
  }

  // Replace returns with branches
  for (auto block : returnBlocks) {
    context()->KillInst(&*block->tail());
    block->tail()->SetOpcode(SpvOpBranch);
    block->tail()->ReplaceOperands({{SPV_OPERAND_TYPE_ID, {returnId}}});
    get_def_use_mgr()->AnalyzeInstUse(&*block->tail());
    get_def_use_mgr()->AnalyzeInstUse(block->GetLabelInst());
  }

  get_def_use_mgr()->AnalyzeInstDefUse(retBlockIter->GetLabelInst());

  return true;
}

}  // namespace opt
}  // namespace spvtools