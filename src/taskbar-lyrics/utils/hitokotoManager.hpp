#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct HitokotoEntry {
    std::string hitokoto;
    std::string from;
    std::string from_who;
};

class HitokotoManager {
public:
    static HitokotoManager& getInstance();

    void Load(const std::string& path);
    HitokotoEntry GetRandom();

private:
    HitokotoManager() = default;
    ~HitokotoManager() = default;

    std::vector<HitokotoEntry> m_entries;
    std::string m_lastPath;
};
