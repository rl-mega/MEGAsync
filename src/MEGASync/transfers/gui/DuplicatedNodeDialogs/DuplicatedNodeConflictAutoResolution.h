#ifndef DUPLICATEDNODECONFLICTAUTORESOLUTION_H
#define DUPLICATEDNODECONFLICTAUTORESOLUTION_H

#include "megaapi.h"

#include <QHash>
#include <QList>
#include <QStringList>

#include <memory>

class DuplicatedNodeInfo;
struct ConflictTypes;

class DuplicatedNodeConflictAutoResolution
{
public:
    static void resolveFolderConflictsForCopy(std::shared_ptr<ConflictTypes> conflicts);

private:
    static void autoRenameFolderConflictsForCopy(
        std::shared_ptr<ConflictTypes> conflicts,
        QList<std::shared_ptr<DuplicatedNodeInfo>>& folderConflicts,
        QHash<mega::MegaHandle, QStringList>& usedNamesByParent);
};

#endif // DUPLICATEDNODECONFLICTAUTORESOLUTION_H
