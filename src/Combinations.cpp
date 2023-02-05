#include "Combinations.h"

#include "Time.h"
#include "pugixml.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <numeric>
#include <set>
#include <variant>

namespace {
struct Leg
{
    InstrumentType type;
    std::variant<double, bool> ratio;
    std::variant<char, int> strike;
    std::variant<char, int, Period> expiration;
};

struct Combination
{
    Combination(std::string && name)
        : name(std::move(name)){

          };
    virtual ~Combination() = default;

    bool check(const std::vector<Component> & components, std::vector<int> & order)
    {
        return fastCheck(components) && finalCheck(components, order);
    }

protected:
    virtual bool fastCheck(const std::vector<Component> & components) = 0;
    virtual bool finalCheck(const std::vector<Component> & components, std::vector<int> & order) = 0;

public:
    const std::string name;
};

struct More : Combination
{
public:
    More(Leg && leg, std::string && name, std::size_t minCount)
        : Combination(std::move(name))
        , leg(std::move(leg))
        , minCount(minCount)
    {
    }

protected:
    bool fastCheck(const std::vector<Component> & components) override
    {
        return components.size() >= minCount;
    }
    bool finalCheck(const std::vector<Component> & components, std::vector<int> & order) override
    {
        for (const auto & i : components) {
            if (!(leg.type == i.type || (leg.type == InstrumentType::O && (i.type == InstrumentType::P || i.type == InstrumentType::C)))) {
                return false;
            }
            if (std::holds_alternative<double>(leg.ratio)) {
                if (std::get<double>(leg.ratio) != i.ratio) {
                    return false;
                }
            }
            else {
                if (std::get<bool>(leg.ratio) != (i.ratio > 0)) {
                    return false;
                }
            }
        }
        std::iota(order.begin(), order.end(), 0);
        return true;
    }

private:
    Leg leg;
    const std::size_t minCount;
};

struct Multiply : Combination
{

    Multiply(std::vector<Leg> && legs, std::string && string)
        : Combination(std::move(string))
        , legs(std::move(legs))
    {
    }

protected:
    const std::vector<Leg> legs;

    virtual bool checkIf(const std::vector<Component> & components)
    {
        return components.empty() || components.size() % legs.size() != 0;
    }
    bool fastCheck(const std::vector<Component> & components) override
    {
        if (checkIf(components)) {
            return false;
        }
        std::set<InstrumentType> set;
        for (const auto & i : components) {
            set.insert(i.type);
        }
        for (const auto & i : legs) {
            if (set.find(i.type) == set.end()) {
                return false;
            }
        }
        return true;
    }
    bool finalCheck(const std::vector<Component> & components, std::vector<int> & order) override
    {
        std::iota(order.begin(), order.end(), 0);
        do {
            if (checkPermutation(components, order)) {
                return true;
            }
        } while (std::next_permutation(order.begin(), order.end()));
        return false;
    }

private:
    bool checkPermutation(const std::vector<Component> & components, const std::vector<int> & order)
    {
        for (std::size_t i = 0; i < components.size(); i += legs.size()) {
            std::map<char, double> strike;
            std::map<char, Expiration> expiration;
            std::map<int, double> strikeOffset;
            std::map<int, Expiration> expirationOffset;
            for (std::size_t j = 0; j < legs.size(); j++) {
                const auto & comb = components[order[j + i]];
                const auto & leg = legs[j];

                if (comb.type != leg.type) {
                    return false;
                }

                if (std::holds_alternative<double>(leg.ratio)) {
                    if (std::get<double>(leg.ratio) != comb.ratio) {
                        return false;
                    }
                }
                else {
                    if (std::get<bool>(leg.ratio) != (comb.ratio > 0)) {
                        return false;
                    }
                }

                if (!checkOnOffset(strike, strikeOffset, leg.strike, comb.strike)) {
                    return false;
                }

                if (std::holds_alternative<Period>(leg.expiration)) {
                    if (!expirationOffset[0].CheckOnPeriod(std::get<Period>(leg.expiration), comb.expiration)) {
                        return false;
                    }
                }
                else {
                    if (!checkOnOffset(expiration, expirationOffset, leg.expiration, Expiration(comb.expiration))) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    template <class T, class... Ts>
    static bool checkOnOffset(std::map<char, T> & map, std::map<int, T> & mapOfOffset, const std::variant<Ts...> & leg, const T & comb)
    {
        if (std::holds_alternative<char>(leg)) {
            if (std::get<char>(leg) != '\0') {
                if (!map.emplace(std::get<char>(leg), comb).second && map[std::get<char>(leg)] != comb) {
                    return false;
                }
            }
            mapOfOffset.clear();
            mapOfOffset[0] = comb;
        }
        else {
            if (mapOfOffset.emplace(std::get<int>(leg), comb).second) {
                auto index = std::get<int>(leg);
                for (const auto [q, amount] : mapOfOffset) {
                    if ((q < index && amount >= comb) || (q > index && amount <= comb)) {
                        return false;
                    }
                }
            }
            else if (mapOfOffset[std::get<int>(leg)] != comb) {
                return false;
            }
        }
        return true;
    }
};

struct Fixed : Multiply
{
    Fixed(std::vector<Leg> && legs, std::string && string)
        : Multiply(std::move(legs), std::move(string))
    {
    }

protected:
    bool checkIf(const std::vector<Component> & components) override
    {
        return Multiply::legs.size() != components.size();
    }
};
} // namespace

struct Combinations::Implementation
{
    std::vector<std::unique_ptr<Combination>> combinations;
    Implementation() = default;
    ~Implementation() = default;
    void add(Combination * combination)
    {
        combinations.emplace_back(combination);
    }
};

bool Combinations::load(const std::filesystem::path & resource)
{
    pugi::xml_document document;
    document.load_file(resource.c_str());
    if (!document) {
        return false;
    }
    const auto combinations = document.child("combinations");
    if (!combinations) {
        return false;
    }
    for (const auto & combination : combinations) {
        const auto & legsXml = combination.first_child();
        std::vector<Leg> legs;
        for (const auto & legXml : legsXml) {
            auto & leg = legs.emplace_back();

            leg.type = static_cast<InstrumentType>(legXml.attribute("type").value()[0]);

            const auto & ratio = legXml.attribute("ratio");
            if (ratio.value()[0] == '+') {
                leg.ratio = true;
            }
            else if (ratio.value()[0] == '-' && ratio.value()[1] == '\0') {
                leg.ratio = false;
            }
            else {
                leg.ratio = ratio.as_double();
            }

            if (const auto & strike = legXml.attribute("strike")) {
                leg.strike = strike.value()[0];
            }
            else if (const auto & strikeOffset = legXml.attribute("strike_offset")) {
                int tmp = std::strlen(strikeOffset.value());
                leg.strike = strikeOffset.value()[0] == '-' ? -tmp : tmp;
            }

            if (const auto & expiration = legXml.attribute("expiration")) {
                leg.expiration = expiration.value()[0];
            }
            else if (const auto & expirationOffset = legXml.attribute("expiration_offset")) {
                if (expirationOffset.value()[0] == '+' || expirationOffset.value()[0] == '-') {
                    int tmp = std::strlen(expirationOffset.value());
                    leg.expiration = expirationOffset.value()[0] == '-' ? -tmp : tmp;
                }
                else {
                    int tmp = std::atoi(expirationOffset.value());
                    if (!tmp)
                        ++tmp; // default value 1
                    Period period;
                    period.amount = tmp;
                    const char * type = expirationOffset.value();
                    while (std::isdigit(*type)) {
                        type++;
                    }
                    period.type = static_cast<TypeOfOffset>(*type);
                    leg.expiration = period;
                }
            }
        }
        std::string name = combination.attribute("name").value();
        std::string cardinality = legsXml.attribute("cardinality").value();
        if (cardinality == "more") {
            anImplementation->add(new More(std::move(legs[0]), std::move(name), legsXml.attribute("mincount").as_ullong()));
        }
        else if (cardinality == "fixed") {
            anImplementation->add(new Fixed(std::move(legs), std::move(name)));
        }
        else {
            anImplementation->add(new Multiply(std::move(legs), std::move(name)));
        }
    }
    return true;
}

std::string Combinations::classify(const std::vector<Component> & components, std::vector<int> & order) const
{
    std::vector<int> tmp_order(components.size());
    for (const auto & comb : anImplementation->combinations) {
        if (comb->check(components, tmp_order)) {
            order.resize(tmp_order.size());
            for (std::size_t i = 0; i < tmp_order.size(); i++) {
                order[tmp_order[i]] = i + 1;
            }
            return comb->name;
        }
    }
    return "Unclassified";
}

Combinations::Combinations()
    : anImplementation(new Implementation())
{
}

Combinations::~Combinations() = default;