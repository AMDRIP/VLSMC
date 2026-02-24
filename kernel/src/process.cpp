/**
 * @file process.cpp
 * @brief Реализация методов структуры Process.
 */

#include "kernel/process.h"

namespace re36 {

const Instruction* Process::currentInstruction() const {
    if (programCounter < static_cast<uint32_t>(instructions.size())) {
        return &instructions[programCounter];
    }
    return nullptr;
}

void Process::advanceProgramCounter() {
    if (programCounter < static_cast<uint32_t>(instructions.size())) {
        ++programCounter;
    }
}

bool Process::hasMoreInstructions() const {
    return programCounter < static_cast<uint32_t>(instructions.size());
}

} // namespace re36
