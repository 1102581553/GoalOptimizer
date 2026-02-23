#pragma once

#include <ll/api/Config.h>
#include <ll/api/mod/NativeMod.h>

namespace goal_optimizer {

struct Config {
    int  version    = 1;
    bool enabled    = true;
    bool debug      = false;
    int  phaseCount = 4;
};

Config& getConfig();
bool    loadConfig();
bool    saveConfig();

class GoalOptimizer {
public:
    static GoalOptimizer& getInstance();

    GoalOptimizer() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace goal_optimizer
