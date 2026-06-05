#ifndef STALLEDISSUEHASHDISCARDTRACKER_H
#define STALLEDISSUEHASHDISCARDTRACKER_H

#include "StalledIssue.h"

#include <QMutexLocker>

#include <optional>
#include <unordered_map>

class StalledIssueHashDiscardTracker
{
public:
    StalledIssueHashDiscardTracker() = default;
    ~StalledIssueHashDiscardTracker() = default;

    void prepareForNewRequest(const StalledIssuesVariantList& issues);
    void keepTrackIssues(StalledIssuesVariantList& issues);
    void track(const StalledIssue* issue, StalledIssue::ResolutionState state);
    void markAsRecentlyReceived(const StalledIssue* issue);
    void purgeExpired();
    bool shouldDiscard(size_t hash) const;

private:
    struct TrackedHashEntry
    {
        StalledIssue::HashDiscardRule rule;
        std::optional<std::chrono::steady_clock::time_point> expireAt;
        bool recentlyReceived = true;
    };

    using DiscardedHashes = std::unordered_map<size_t, TrackedHashEntry>;

    mutable QMutex mMutex;
    DiscardedHashes mEntries;
};

#endif // STALLEDISSUEHASHDISCARDTRACKER_H
