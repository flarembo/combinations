#pragma once
#include <ctime>

enum class TypeOfOffset : char
{
    Day = 'd',
    Month = 'm',
    Quoter = 'q',
    Year = 'y'
};

struct Period
{
    TypeOfOffset type;
    std::size_t amount;
};

struct Expiration
{
    Expiration() = default;
    Expiration(const std::tm & tm);
    bool CheckOnPeriod(const Period & period, const Expiration & expiration);

    friend bool operator==(const Expiration & d1, const Expiration & d2);
    friend bool operator!=(const Expiration & d1, const Expiration & d2);
    friend bool operator<(const Expiration & d1, const Expiration & d2);
    friend bool operator>(const Expiration & d1, const Expiration & d2);
    friend bool operator<=(const Expiration & d1, const Expiration & d2);
    friend bool operator>=(const Expiration & d1, const Expiration & d2);

private:
    int day, month, year;
};