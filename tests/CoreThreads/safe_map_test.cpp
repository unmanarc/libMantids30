#include <Mantids30/Core/Threads/safe_map2.h>
#include <Mantids30/Core/Threads/safe_map.h>

#include <atomic>
#include <functional>
#include <future>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using Mantids30::Core::Threads::Safe::Map2;
using Mantids30::Core::Threads::Safe::MapItem;

static void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("Shared map ownership check failed");
    }
}

struct Item : MapItem
{
    explicit Item(std::atomic<int> &destroyed) : destroyed(destroyed) {}
    ~Item() { ++destroyed; }
    std::atomic<int> &destroyed;
    std::atomic<int> stops{0};
    std::function<void()> onStop;

    void stopSignal() override
    {
        ++stops;
        if (onStop)
        {
            onStop();
        }
    }
};

int main()
{
    static_assert(std::is_same_v<decltype(std::declval<Mantids30::Core::Threads::Safe::Map<int> &>().openElement(1)), MapItem *>);
    static_assert(!std::is_invocable_v<decltype(&Map2<int>::addElement), Map2<int> &, int, MapItem *>);
    std::atomic<int> destroyed{0};
    Map2<int> map;
    require(!map.addElement(1, nullptr));
    require(!map.openElement(1));
    require(!map.destroyElement(1));
    auto item = std::make_shared<Item>(destroyed);
    std::weak_ptr<Item> lifetime = item;
    require(map.addElement(1, item));
    require(!map.addElement(1, item));
    require(map.getKeys() == std::set<int>{1});
    auto reader = map.openElement(1);
    require(reader == item);
    // stopSignal must be free to query the map without deadlocking.
    item->onStop = [&] { require(!map.isMember(1)); };
    item.reset();
    std::vector<std::future<void>> waiters;
    for (int i = 0; i < 3; ++i)
    {
        waiters.push_back(std::async(std::launch::async, [&] { map.waitForEmptyMap(); }));
    }
    require(map.destroyElement(1));
    for (auto &waiter : waiters)
    {
        waiter.get();
    }
    require(!map.openElement(1));
    require(!lifetime.expired() && destroyed == 0);
    require(std::static_pointer_cast<Item>(reader)->stops == 1);
    // Reusing a key must not let a reader of the old entry affect the new one.
    auto replacement = std::make_shared<Item>(destroyed);
    require(map.addElement(1, replacement));
    reader.reset();
    require(lifetime.expired() && destroyed == 1);
    require(map.openElement(1) == replacement);
    require(map.destroyElement(1));
    replacement.reset();
    require(destroyed == 2);

    // Removing a registry does not invalidate a retained item.
    {
        Map2<int> temporary;
        auto value = std::make_shared<Item>(destroyed);
        require(temporary.addElement(1, value));
        reader = temporary.openElement(1);
    }
    require(destroyed == 2);
    reader.reset();
    require(destroyed == 3);

    // Readers racing with removal must always receive owning references.
    auto concurrent = std::make_shared<Item>(destroyed);
    require(map.addElement(1, concurrent));
    std::promise<void> acquired;
    std::promise<void> removed;
    auto removal = removed.get_future();
    std::thread worker([&]
    {
        auto held = map.openElement(1);
        acquired.set_value();
        removal.wait();
        require(held == concurrent);
        require(std::static_pointer_cast<Item>(held)->stops == 1);
    });
    acquired.get_future().wait();
    require(map.destroyElement(1));
    removed.set_value();
    worker.join();
    concurrent.reset();
    require(destroyed == 4);
}
