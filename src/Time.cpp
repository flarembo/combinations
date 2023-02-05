#include "Time.h"

namespace {
void addQuoter(int amount, int & month, int & year)
{
    month += amount * 3;
    year += month / 12;
    month %= 12;
}
} // namespace
Expiration::Expiration(const std::tm & tm)
    : day(tm.tm_mday)
    , month(tm.tm_mon)
    , year(tm.tm_year)
{
}

bool operator==(const Expiration & d1, const Expiration & d2)
{
    return d1.day == d2.day && d1.month == d2.month && d1.year == d2.year;
}
bool operator!=(const Expiration & d1, const Expiration & d2)
{
    return !(d1 == d2);
}
bool operator<(const Expiration & d1, const Expiration & d2)
{
    if (d1.year == d2.year) {
        if (d1.month == d2.month) {
            return d1.day < d2.day;
        }
        return d1.month < d2.month;
    }
    return d1.year < d2.year;
}

bool operator>(const Expiration & d1, const Expiration & d2)
{
    return d2 < d1;
}
bool operator<=(const Expiration & d1, const Expiration & d2)
{
    return !(d1 > d2);
}
bool operator>=(const Expiration & d1, const Expiration & d2)
{
    return !(d2 > d1);
}

bool Expiration::CheckOnPeriod(const Period & period, const Expiration & expiration)
{
    auto tmp = *this;

    switch (period.type) {
    case TypeOfOffset::Quoter:
        addQuoter(period.amount, tmp.month, tmp.year);
        if (expiration < tmp) {
            return false;
        }
        addQuoter(1, tmp.month, tmp.year);
        return expiration <= tmp;
    case TypeOfOffset::Day:
        tmp.day += period.amount;
        break;
    case TypeOfOffset::Month:
        tmp.month += period.amount;
        break;
    case TypeOfOffset::Year:
        tmp.year += period.amount;
    }
    std::tm tm = {};
    tm.tm_mday = tmp.day;
    tm.tm_mon = tmp.month;
    tm.tm_year = tmp.year;
    std::mktime(&tm);
    return tm == expiration;
}