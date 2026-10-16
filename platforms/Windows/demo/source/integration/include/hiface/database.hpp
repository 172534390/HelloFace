#pragma once
#include "hiface/frame.hpp"
#include "hiface/error.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <string>

namespace hiface {
struct Person {
    std::string id, group, tag, feature;
    int64_t updated = 0;
};
struct SearchMatch { Person entry; float score = 0; };
// Database operations do not initialize inference or load models. Use one worker
// per owner; strings returned here own their bytes beyond the C result lifetime.
class Database {
public:
    explicit Database(const std::filesystem::path& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    HiFaceSearchDatabaseHandle nativeHandle() const { return database_; }
    uint64_t count(const std::string& group = {}, const std::string& tag = {}) const;
    std::vector<Person> list(uint32_t offset = 0, uint32_t limit = 100,
        const std::string& group = {}, const std::string& tag = {}, bool features = false, bool recent = false) const;
    Person get(const std::string& id) const;
    void save(const std::vector<Person>& people, bool overwrite);
    uint64_t remove(const std::string& id);
    void updateMetadata(const std::string& id, const std::string& group, const std::string& tag);
    void releaseMemory();
    bool contains(const std::string& id) const;
    // Exact HF1 search, using the shared Q7 decoder and Core similarity.
    // Matches own their metadata; no model or camera session is needed.
    std::vector<SearchMatch> search(const std::string& feature, uint32_t top_k = 1,
        float threshold = HIFACE_DEFAULT_VERIFICATION_THRESHOLD,
        const std::string& group = {}, const std::string& tag = {}) const;
private:
    HiFaceSearchDatabaseHandle database_ = nullptr;
};
std::string encodeFeature(const Feature& feature);
Feature decodeFeature(const std::string& text);
float compareFeatureText(const std::string& left, const std::string& right);
}
