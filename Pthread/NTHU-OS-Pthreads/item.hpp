#include <iostream>

#ifndef ITEM_HPP
#define ITEM_HPP

class Item {
public:
	Item();
	explicit Item(int key, unsigned long long val, char opcode);
	~Item();

	friend std::ostream& operator<<(std::ostream& os, const Item& item);
	friend std::istream& operator>>(std::istream& in, Item& item);

	int key;
	unsigned long long val;
	char opcode;
};

// Implementation start

Item::Item() {}

Item::Item(int key, unsigned long long val, char opcode) :
	key(key), val(val), opcode(opcode) {
}

Item::~Item() {}

std::istream& operator>>(std::istream& in, Item& item) {
	in >> item.key >> item.val >> item.opcode;
	return in;
}

std::ostream& operator<<(std::ostream& os, const Item& item) {
	os << item.key << ' ' << item.val << ' ' << item.opcode << '\n';
	return os;
}

#endif // ITEM_HPP
