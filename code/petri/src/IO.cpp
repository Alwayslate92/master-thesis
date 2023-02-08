#include <petri/IO.hpp>

#include <ostream>

namespace petri {

void printLoLANet(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                  std::function<std::string(petri::Place)> pName,
                  std::function<std::string(petri::Transition)> tName) {
	assert(pName);
	assert(tName);
	{
		s << "PLACE\n  ";
		bool first = true;
		for(const auto p : net.places()) {
			if(first) first = false;
			else s << ", ";
			s << pName(p);
		}
		s << ";\n\n";
	}
	{
		s << "MARKING\n  ";
		bool first = true;
		for(const auto p : net.places()) {
			if(m[p] == 0) continue;
			if(first) first = false;
			else s << ", ";
			s << pName(p) << ":" << m[p];
		}
		s << ";\n\n";
	}
	for(const auto t : net.transitions()) {
		s << "TRANSITION " << tName(t) << "\n";
		{
			s << "  CONSUME ";
			bool first = true;
			for(const auto[p, c] : net.consumed(t)) {
				if(first) first = false;
				else s << ", ";
				s << pName(p) << ":" << c;
			}
			s << ";\n";
		}
		{
			s << "  PRODUCE ";
			bool first = true;
			for(const auto[p, c] : net.produced(t)) {
				if(first) first = false;
				else s << ", ";
				s << pName(p) << ":" << c;
			}
			s << ";\n";
		}
		s << '\n';
	}
	s << "\n{ END OF FILE }\n";
}


void printGraphvizNet(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                      std::function<std::string(petri::Place)> pName,
                      std::function<std::string(petri::Transition)> tName) {
	s << "// TODO: print graph" << std::endl;
}

void printLoLAReachabilityPredicate(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                                    std::function<std::string(petri::Place)> pName) {
	s << "REACHABLE ("; // the parenthesis is important!
	bool isFirst = true;
	for(const auto p : net.places()) {
		if(isFirst) isFirst = false;
		else s << " AND ";
		s << pName(p) << " = " << m[p];
	}
	s << ");";
}


} // namespace petri