// CRTP
#include <iostream>
#include <vector>
#include <unordered_map>


using Place = int;
using Trans = int;


template<typename Derived>
struct Marking {
	int operator[](Place p) const {
		std::cout << __PRETTY_FUNCTION__ << "(" << p << ")" << std::endl;
		return getDerived().getTokens(p);
	}

	void fire(Trans t) {}
	void addTokens(Place p, int c) {
		getDerived().addTokensImpl(p, c);
	}
private:
	Derived &getDerived() {
		return *static_cast<Derived*>(this);
	}
	const Derived &getDerived() const {
		return *static_cast<const Derived*>(this);
	}
private:
	std::vector<Trans> enabledTransitions;
};


struct VectorMarking : Marking<VectorMarking> {
	VectorMarking() : data(10, 42) {}

	int getTokens(Place p) const {
		std::cout << __PRETTY_FUNCTION__ << "(" << p << ")" << std::endl;
		return data[p];
	}

	void addTokensImpl(Place p, int c) {
		data[p] += c;
	}
private:
	std::vector<int> data;
};


struct SparseMarking : Marking<SparseMarking> {
	SparseMarking() {
		data[0] = 60;
	}
private:
	friend class Marking<SparseMarking>;
	int getTokens(Place p) const {
		std::cout << __PRETTY_FUNCTION__ << "(" << p << ")" << std::endl;
		const auto iter = data.find(p);
		if(iter == data.end())
			return 0;
		else
			return iter->second;
	}
private:
	std::unordered_map<int, int> data;
};


int main() {
	VectorMarking v;
	auto c = v[0];
	std::cout << "[0]: " << c << std::endl;	
	SparseMarking v2;
	auto c2 = v2[0];
	std::cout << "[0]: " << c2 << std::endl;	
}
