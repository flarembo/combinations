#pragma once

#include "Component.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Combinations
{
public:
    Combinations();
    ~Combinations();

    bool load(const std::filesystem::path & resource);

    std::string classify(const std::vector<Component> & components, std::vector<int> & order) const;

private:
    struct Implementation;
    const std::unique_ptr<Implementation> anImplementation;
};
