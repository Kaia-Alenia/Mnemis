#pragma once

#include <QStringList>

namespace mnemis::infrastructure::bootstrap {

// Returns only existing, unique locations. It never creates directories.
QStringList existingDefaultLibraryRoots(const QStringList& candidates);

} // namespace mnemis::infrastructure::bootstrap
