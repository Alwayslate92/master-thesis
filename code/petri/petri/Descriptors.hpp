#ifndef PETRI_DESCRIPTORS_HPP
#define PETRI_DESCRIPTORS_HPP

#include <functional>

namespace petri {
struct Marking;
struct Net;

//NOTE: Why does a place, not have a number of tokens?
//It seems that they have been delegated to Markings. 
struct Place {
	Place() : id(-1) {}
private:
	friend struct Marking;
	friend struct Net;
	explicit Place(int id) : id(id) {}
public:
	int getId() const {
		return id;
	}

	explicit operator bool() const {
		return *this != Place();
	}

	friend bool operator==(Place a, Place b) {
		return a.id == b.id;
	}

	friend bool operator!=(Place a, Place b) {
		return !(a == b);
	}

	//NOTE: When does this comparrison make sense?
	friend bool operator<(Place a, Place b) { 
		return a.id < b.id; 
	}

	friend std::size_t hash_value(Place p) {
		return p.id;
	}
private:
	int id;
};

struct Transition {

	//Default constructor for Transition
	Transition() : id(-1) {}
private:
	friend struct Marking;
	friend struct Net;
	explicit Transition(int id) : id(id) {} //A constructor, that turns of implicit conversions. 
public:
	int getId() const {
		return id;
	}

	explicit operator bool() const {
		return *this != Transition();
	}

	friend bool operator==(Transition a, Transition b) {
		return a.id == b.id;
	}

	friend bool operator!=(Transition a, Transition b) {
		return !(a == b);
	}

	//NOTE: When does this comparrison make sense?
	friend bool operator<(Transition a, Transition b) {
		return a.id < b.id;
	}

	//This is just a function.
	friend std::size_t hash_value(Transition t) {
		return t.id;
	}
private:
	int id;
};

} // namespace petri

//NOTE: Ask Jakob why he is overloading std, is this not ill advised?
namespace std {

template<>
struct hash<petri::Place> {
	auto operator()(petri::Place p) const {
		return hash_value(p);
	}
};

template<>
struct hash<petri::Transition> {
	auto operator()(petri::Transition t) const {
		return hash_value(t);
	}
};

} // namespace std

#endif // PETRI_DESCRIPTORS_HPP