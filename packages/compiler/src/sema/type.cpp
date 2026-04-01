#include <cassert>

#include "sema/symbol.hpp"
#include "sema/type.hpp"

namespace porpoise::sema {

auto Type::set_metadata(usize scope_table_idx, Optional<usize> parameter_table_idx) noexcept
    -> void {
    assert(scope_table_idx != SymbolTableRegistry::SENTINEL_IDX && "Illegal scope table index");
    metadata_.emplace(
        Metadata{scope_table_idx, parameter_table_idx.value_or(SymbolTableRegistry::SENTINEL_IDX)});
}

auto Type::has_parameter_table() const noexcept -> bool {
    return metadata_ && metadata_->parameter_table_idx != SymbolTableRegistry::SENTINEL_IDX;
}

} // namespace porpoise::sema
