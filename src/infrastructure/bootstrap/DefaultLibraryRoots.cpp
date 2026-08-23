#include "DefaultLibraryRoots.hpp"

#include <QDir>

namespace mnemis::infrastructure::bootstrap {

QStringList existingDefaultLibraryRoots(const QStringList& candidates) {
    QStringList roots;
    for (const QString& candidate : candidates) {
        if (!candidate.isEmpty() && QDir(candidate).exists() && !roots.contains(candidate)) {
            roots.append(candidate);
        }
    }
    return roots;
}

} // namespace mnemis::infrastructure::bootstrap
