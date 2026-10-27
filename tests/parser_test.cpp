#include "../src/csv.hpp"
#include <iostream>
int main() {
    std::vector<double> values;
    int failures = 0;
    auto check = [&](bool ok) { if (!ok) ++failures; };
    check(parseCsv("1,2.5,-3\r", values) && values == std::vector<double>({1,2.5,-3}));
    check(parseCsv(" 1e2 , 0 ", values) && values == std::vector<double>({100,0}));
    check(parseCsv("42", values) && values.size() == 1);
    for (const auto *bad : {"", "1,", ",1", "1,,2", "abc", "1x,2", "nan", "inf", "1e999"})
        check(!parseCsv(bad, values) && values.empty());
    check(!parseCsv("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17", values));
    std::cout << "Parser: " << failures << " falhas\n";
    return failures ? 1 : 0;
}
