#include "hitokotoManager.hpp"
#include <fstream>
#include <random>
#include "logger.hpp"

using json = nlohmann::json;

HitokotoManager& HitokotoManager::getInstance() {
    static HitokotoManager instance;
    return instance;
}

void HitokotoManager::Load(const std::string& path) {
    if (path == m_lastPath && !m_entries.empty()) return;

    m_entries.clear();
    m_lastPath = path;

    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            Logger::Error("Failed to open hitokoto file: %s", path.c_str());
            return;
        }

        json j;
        f >> j;

        for (const auto& item : j) {
            HitokotoEntry entry;
            entry.hitokoto = item.value("hitokoto", "");
            entry.from = item.value("from", "");
            if (item.contains("from_who") && !item["from_who"].is_null()) {
                entry.from_who = item["from_who"].get<std::string>();
            } else {
                entry.from_who = "";
            }
            m_entries.push_back(entry);
        }
        
        Logger::Info("Loaded %d hitokoto entries from %s", m_entries.size(), path.c_str());

    } catch (const std::exception& e) {
        Logger::Error("Error parsing hitokoto file: %s", e.what());
    }
}

HitokotoEntry HitokotoManager::GetRandom() {
    if (m_entries.empty()) return {"", "", ""};

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, m_entries.size() - 1);

    return m_entries[dis(gen)];
}
