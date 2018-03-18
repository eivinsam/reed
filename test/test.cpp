#include <iostream>

#include <reed.h>

using namespace reed;

template <class T>
bool matches(const T& rule, string_view text) 
{ 
	const auto result = rule(text);

	return length(result) == text.size(); 
}



#define CHECK_MATCH(rule, text) std::cout << #rule << "(\"" << text << "\"): " << (matches(rule, text) ? "yes" : "no") << "\n";

int main()
{
	constexpr auto sp = ch<' ', '\t', '\n', '\r'>;
	constexpr auto lowercase = chr<'a', 'z'>;
	constexpr auto uppercase = chr<'A', 'Z'>;
	constexpr auto digit     = chr<'0', '9'>;
	constexpr auto letter = lowercase | uppercase;

	constexpr auto name_initial = ch<'_'> | letter;

	Rule name = (name_initial & *(name_initial | digit)) % (*sp & chs<':', ':'> & *sp);

	Rule constant = str("const");

	Rule type = maybe(constant & +sp) & name;

	Rule expr;

	Rule args = expr % (*sp & ch<','> & *sp);

	Rule suffixed = expr & maybe(*sp & (
		chs<'+', '+'> | chs<'-', '-'> | 
		((ch<'.'> | chs<'-', '>'>) & *sp & name ) |
		(ch<'('> & args & ch<')'>) | 
		(ch<'['> & expr & ch<']'>) |
		(ch<'['> & args & ch<']'>)));
	Rule prefixed = maybe((chs<'+', '+'>) & *sp) & suffixed;
	Rule term_op = ch<'*', '/', '%'>, term = prefixed % (*sp % term_op);
	Rule sum_op = ch<'+', '-'>, sum = term % (*sp % sum_op);
	expr = name | (ch<'('> & sum & ch<')'>);

	CHECK_MATCH(name, "_foo");
	CHECK_MATCH(name, "f00_b4r");
	CHECK_MATCH(name, "0f");
	CHECK_MATCH(name, "foo::bar");
	
	CHECK_MATCH(term, "a + b");
	CHECK_MATCH(term, "++a-> b * c");
	CHECK_MATCH(term, "a*b + c");
	CHECK_MATCH(sum, "a*b + c");
	CHECK_MATCH(term, "a*(b+c)");

	return 0;
}