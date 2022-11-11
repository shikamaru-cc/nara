#include <iostream>
#include <bitset>
#include <chrono>

#include "bench.hpp"
// #include "board.hpp"
#include "eval_new.hpp"

int main()
{
    using namespace nara;

    /*
    std::string s1, s2;
    std::cin >> s1;
    std::cin >> s2;

    uint8_t px, py;
    px = static_cast<uint8_t>(std::bitset<8>(s1).to_ulong());
    py = static_cast<uint8_t>(std::bitset<8>(s2).to_ulong());
    */

    for (int px = 0; px <= 255; px++)
    {
        for (int py = 0; py <= 255; py++)
        {
            // std::bitset<8> s1((uint8_t)px), s2((uint8_t)py);

            // std::cout << px << ',' << py << std::endl;
            // std::cout << s1.to_string() << ',' << s2.to_string() << std::endl;

            auto start = std::chrono::system_clock::now();
            auto end = std::chrono::system_clock::now();

            int p = cal_category(px, py);

            std::cout 
                << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                << std::endl;

            // std::cout << p << std::endl;
        }
    }

    /*
    start = std::chrono::system_clock::now();
    end = std::chrono::system_clock::now();

    for (int px = 0; px <= 255; px++)
    {
        for (int py = 0; py <= 255; py++)
        {
            // std::bitset<8> s1((uint8_t)px), s2((uint8_t)py);

            // std::cout << px << ',' << py << std::endl;
            // std::cout << s1.to_string() << ',' << s2.to_string() << std::endl;

            int p = cal_category(px, py);
            // std::cout << p << std::endl;
        }
    }

    std::cout 
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
        << std::endl;
    */

    return 0;
}
