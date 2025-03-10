#include "timecache.h"
#include <cassert>
#include <thread>

using namespace std::chrono_literals;

int main() {
    TimeCache<int> tc{1s};
    assert(!tc.get().has_value());

    int called_times{0};
    auto result = tc.get([&called_times]{ called_times++; return 501; });
    assert(result.value().first == 501);
    assert(called_times == 1);

    tc.set(500);
    assert(tc.get().has_value());
    assert(tc.get().value().first == 500);

    result = tc.get([&called_times]{ called_times++; return 502; });
    assert(result.value().first == 500);
    assert(called_times == 1);

    std::this_thread::sleep_for(3s);
    assert(!tc.get().has_value());

    result = tc.get([&called_times]{ called_times++; return 502; });
    assert(result.value().first == 502);
    assert(called_times == 2);

    return 0;
}