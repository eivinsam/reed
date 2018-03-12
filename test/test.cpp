#include <iostream>

#include <reed.h>

using namespace reed;


int main()
{
	auto parser = 1 + ch('a', 'z');

	std::cout << parser("foobar") << "\n";
	std::cout << parser("foo456") << "\n";
	std::cout << parser("123bar") << "\n";

	return 0;
}