// Не используется, не удалять
/**
 * @file types.cpp
 * @brief Реализация вспомогательных методов базовых типов.
 */

#include "kernel/types.h"

namespace re36 {

std::string FilePermissions::toString() const {
    std::string result;
    result.reserve(9);
    result += ownerRead    ? 'r' : '-';
    result += ownerWrite   ? 'w' : '-';
    result += ownerExecute ? 'x' : '-';
    result += groupRead    ? 'r' : '-';
    result += groupWrite   ? 'w' : '-';
    result += groupExecute ? 'x' : '-';
    result += otherRead    ? 'r' : '-';
    result += otherWrite   ? 'w' : '-';
    result += otherExecute ? 'x' : '-';
    return result;
}

} // namespace re36
