#pragma once

#include "database/DatabaseConnection.hpp"
#include "core/errors/Result.hpp"
#include "library/models/LibraryRoot.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mnemis::library {

class LibraryRootRepository final {
public:
    explicit LibraryRootRepository(
        database::DatabaseConnection& database
    );

    core::Result<LibraryRootId> add(
        const filesystem::Path& path,
        const std::string& name
    );

    core::Result<void> remove(
        LibraryRootId id
    );

    core::Result<void> setEnabled(
        LibraryRootId id,
        bool enabled
    );

    core::Result<LibraryRoot> findById(
        LibraryRootId id
    );

    core::Result<LibraryRoot> findByPath(
        const filesystem::Path& path
    );

    core::Result<std::vector<LibraryRoot>> list(
        bool includeDisabled = true
    );

    core::Result<std::size_t> count();

private:
    database::DatabaseConnection& m_database;
};

} // namespace mnemis::library
