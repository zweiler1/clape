#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static int64_t mod(int64_t x, int64_t y) {
    return ((x % y + y) % y);
}

int main() {
    std::ifstream f("input.txt");
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::vector<int64_t> nums;
    size_t pos = 0;
    while (pos < content.size()) {
        size_t next = content.find('\n', pos);
        if (next == std::string::npos)
            next = content.size();
        if (pos == next) {
            nums.push_back(0);
        } else {
            int64_t sign = (content[pos] == 'L') ? -1 : 1;
            int64_t val = std::stoll(content.substr(pos + 1, next - pos - 1));
            nums.push_back(val * sign);
        }
        pos = next + 1;
        if (next == content.size())
            break;
    }
    if (!content.empty() && content.back() == '\n')
        nums.push_back(0);

    int64_t dial = 50, count = 0;
    for (auto x : nums) {
        dial = mod(dial + x, 100);
        if (dial == 0)
            count++;
    }
    printf("part_1: %ld\n", (long)count);

    dial = 50;
    count = 0;
    for (auto x : nums) {
        if (dial > 0 && x < 0 && dial + x < 0)
            count++;
        int64_t adj = llabs(dial + x) - 1;
        count += adj / 100;
        dial = mod(dial + x, 100);
        if (dial == 0)
            count++;
    }
    printf("part_2: %ld\n", (long)count);
    return 0;
}
