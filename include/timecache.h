#ifndef _TIME_CACHE
#define _TIME_CACHE

#include <chrono>
#include <functional>
#include <optional>
#include <utility>

template <typename CachedType>
class TimeCache {
    public:
        using Got_t = std::optional<std::pair<CachedType, std::chrono::time_point<std::chrono::system_clock>>>;
        using Getter_t = std::function<CachedType()>;

        TimeCache(std::chrono::milliseconds stale): m_stale{stale} {}

        Got_t get() const {
            if (m_cached.has_value()) {
                auto now = std::chrono::system_clock::now();
                if (now - m_last < m_stale) {
                    return Got_t{std::pair(m_cached.value(), m_last)};
                }
            }
            return {};
        }

        Got_t get(Getter_t getter) {
            auto current = get();

            if (current.has_value()) {
                return current;
            }

            auto new_value = getter();
            set(new_value);
            return Got_t{std::pair(m_cached.value(), m_last)};
        }

        void set(CachedType value) {
            m_cached = std::optional<CachedType>{value};
            m_last = std::chrono::system_clock::now();
        }

    private:
        std::optional<CachedType> m_cached;   
        std::chrono::time_point<std::chrono::system_clock> m_last;
        std::chrono::milliseconds m_stale;
};

#endif