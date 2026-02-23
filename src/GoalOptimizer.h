#pragma once

#include <ll/api/mod/NativeMod.h>
#include <ll/api/Config.h>

namespace goal_optimizer {

struct Config {
    int version = 1;
    bool enabled = true;
    bool debug = false;

    // 分帧相位数（1 = 不分帧，4 = 每实体每 4 tick 评估一次）
    int phaseCount = 4;
};

Config& getConfig();
bool loadConfig();
bool saveConfig();

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
