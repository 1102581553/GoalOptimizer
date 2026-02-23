#pragma once

#include <ll/api/mod/NativeMod.h>

namespace goal_optimizer {

struct Config {
    int  version    = 1;
    bool enabled    = true;
    bool debug      = false;
    int  phaseCount = 2;
};

Config& getConfig();
bool    loadConfig();
bool    saveConfig();

class GoalOptimizer {
public:
    static GoalOptimizer& getInstance();

    GoalOptimizer()                                = default;
    GoalOptimizer(const GoalOptimizer&)            = delete;
    GoalOptimizer& operator=(const GoalOptimizer&) = delete;

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *mSelf; }

    bool load();
    bool enable();
    bool disable();

    ll::mod::NativeMod* mSelf = nullptr;
};

} // namespace goal_optimizer
