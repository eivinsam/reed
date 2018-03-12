#include <iostream>

#include <reed.h>

using namespace reed;


int main()
{
	auto space = ch(' ') | ch('\t') | ch('\n') | ch('\r');
	auto letter = ch('a', 'z') | ch('A', 'Z');
	auto digit = ch('0', '9');

	auto name_initial = ch('_') | letter;

	auto name = name_initial & (0 + (name_initial | digit));

	auto type = maybe(str("const") & (1 + space)) & name;

	std::cout << name("_foo") << "\n";
	std::cout << name("f00_b4r") << "\n";
	std::cout << name("0f") << "\n";
	std::cout << type("const baf") << "\n";
	std::cout << type("const 0") << "\n";
	std::cout << type("cnost foo") << "\n";

	return 0;
}