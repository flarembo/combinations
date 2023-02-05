#include "Combinations.h"

#include <iomanip>
#include <iostream>
#include <string>

namespace {

template <typename... Args>
int fail(Args &&... args) noexcept
{
    ((std::cerr << args), ...);
    std::cerr << std::endl;
    return 1;
}

} // anonymous namespace

int main()
{
    Combinations combinations;
    const std::filesystem::path path{"etc/combinations.xml"};
    combinations.load(path);
    const std::vector<Component> components = {
            Component::from_string("F 1 2010-09-01"), // 3 or 7
            Component::from_string("F 1 2010-06-01"), // 2 or 6
            Component::from_string("F 1 2010-03-01"), // 1 or 5
            Component::from_string("F 1 2010-03-01"), // 1 or 5
            Component::from_string("F 1 2010-09-01"), // 3 or 7
            Component::from_string("F 1 2010-12-01"), // 4 or 8
            Component::from_string("F 1 2010-12-01"), // 4 or 8
            Component::from_string("F 1 2010-06-01"), // 2 or 6
    };
    std::vector<int> order;
    std::cout << combinations.classify(components, order);
}
