#include "../src/serialcsvbuffer.hpp"
#include <iostream>
int main() {
    SerialCsvBuffer buffer;
    std::vector<std::vector<double>> samples;
    int errors=0, failures=0;
    auto sample=[&](const std::vector<double> &values) { samples.push_back(values); };
    auto error=[&](const QString &) { ++errors; };
    auto check=[&](bool ok) { if(!ok) ++failures; };
    buffer.feed("1,2",sample,error);
    check(samples.empty());
    buffer.feed("\r\n3,4\ninvalid\n",sample,error);
    check(samples == std::vector<std::vector<double>>({{1,2},{3,4}}) && errors==1);
    buffer.feed(QByteArray(4097,'1'),sample,error);
    buffer.feed("ignored\n5,6\n",sample,error);
    check(samples.size()==3 && samples.back()==std::vector<double>({5,6}) && errors==2);
    buffer.feed("123",sample,error); buffer.reset();
    buffer.feed("7,8\n",sample,error);
    check(samples.back()==std::vector<double>({7,8}));
    std::cout << "SerialCsvBuffer: " << failures << " falhas\n";
    return failures ? 1 : 0;
}
