#include "PetriNet.hpp"

#include <mod/BuildConfig.hpp>
#include <mod/Config.hpp>
#include <mod/Error.hpp>
#include <mod/Post.hpp>
#include <mod/lib/IO/IO.hpp>
#include <mod/lib/causality/LoLA.hpp>

#include <jla_boost/Functional.hpp>
#include <jla_boost/graph/PairToRangeAdaptor.hpp>

#include <boost/graph/graphviz.hpp>

#include <optional>
#include <sstream>

#ifdef MOD_HAVE_PNAPI

// yes, this is ugly, but we really need to go to C++17 ...
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define throw(...)

#include <pnapi/pnapi.h>

#undef throw
#pragma clang diagnostic pop
// you can open your eyes again

#else
#define MOD_NO_PNAPI_ERROR                                                      \
	throw mod::FatalError("Call to '" + std::string(__FUNCTION__) + "' failed.\n" \
		+ "Petri net features are not available. Rebuild with Petri Net API enabled.\n")
#endif // ifdef MOD_HAVE_PNAPI
#ifndef MOD_HAVE_LOLA
#define MOD_NO_LOLA_ERROR                                                       \
	throw mod::FatalError("Call to '" + std::string(__FUNCTION__) + "' failed.\n" \
		+ "Lola features are not available. Rebuild with Lola enabled.\n")
#endif

#define MOD_PETRINET_TRY try {
#define MOD_PETRINET_CATCH                                                   \
   } catch(const pnapi::exception::Error &error) {                           \
      using pnapi::io::operator<<;                                           \
      std::cout << "PetriNet API error: " << error << std::endl;             \
      MOD_ABORT;                                                             \
   }

namespace mod::lib::PetriNet {
namespace {

struct TransitionStore {
#ifdef MOD_HAVE_PNAPI
	pnapi::Transition *t;
	std::vector<Place> tails, heads;
#endif
};
} // namespace

struct Net::Pimpl {
#ifdef MOD_HAVE_PNAPI
#ifdef MOD_HAVE_LOLA
	// returns stdout from LoLA if reachable, otherwise boost::none
	std::optional<std::string> lolaIsReachable(const Marking &input,
	                                           const Marking &output,
	                                           const Net &owner,
	                                           bool getDAG) const;
#endif
public:
	pnapi::PetriNet net;
	std::vector<pnapi::Place *> places;
	std::vector<TransitionStore> transitions;
	std::unordered_map<pnapi::Place *, std::size_t> placesMap;
	std::unordered_map<pnapi::Transition *, std::size_t> transitionMap;
#endif // ifdef MOD_HAVE_PNAPI
};

#ifdef MOD_HAVE_PNAPI
namespace {

struct LolaVProp {
	std::string id;
	std::string shape; // circle -> place, box -> transition
	bool isPlace;
	std::string label;
	Place p;
	Transition t;
	bool isOriginal = true;
};

using LolaGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, LolaVProp>;
using LolaVertex = boost::graph_traits<LolaGraph>::vertex_descriptor;
using LolaEdge = boost::graph_traits<LolaGraph>::edge_descriptor;

template<typename FVertexAttrib, typename FExtra, typename Label>
void printLolaGraph(const Net::Pimpl &pimpl,
                    const LolaGraph &g,
                    const std::string &id,
                    FVertexAttrib fVertexAttrib,
                    FExtra fExtra,
                    Label label) {
	post::FileHandle s(IO::getUniqueFilePrefix() + "lolaGraph_" + id + ".dot");
	IO::post() << "dot -Tpdf -o \"" << std::string(s) << ".pdf\" \"" << std::string(s) << "\"\n";
	s << "digraph g {\nrankdir=LR;\n";
	for(LolaVertex v : asRange(vertices(g))) {
		s << g[v].id << " [ shape=";
		if(g[v].isPlace) s << "ellipse";
		else s << "box";
		s << " label=\"";
		if(g[v].isPlace) s << pimpl.places[g[v].p.id]->getName();
		else s << pimpl.transitions[g[v].t.id].t->getName();
		label(g, v, s);
		s << "\"";
		if(!g[v].isOriginal) s << " fontcolor=purple";
		fVertexAttrib(g, v, s);
		s << " ];\n";
	}
	for(LolaEdge e : asRange(edges(g))) {
		LolaVertex vSrc = source(e, g), vTar = target(e, g);
		s << g[vSrc].id << " -> " << g[vTar].id << ";\n";
	}
	fExtra(g, s);
	s << "}\n";
}

bool printLolaAttribIOColour(const Marking &input, const Marking &output, Place p, std::ostream &s) {
	if(input.getCount(p) > 0) {
		s << " color=blue";
		return true;
	} else if(output.getCount(p) > 0) {
		s << " color=green";
		return true;
	}
	return false;
}

} // namespace
#endif // ifdef MOD_HAVE_PNAPI

Net::Net() : p(new Pimpl()) {
#ifndef MOD_HAVE_PNAPI
	MOD_NO_PNAPI_ERROR;
#endif // ifdnef MOD_HAVE_PNAPI
}

Net::~Net() {}

Place Net::addPlace(const std::string &name) {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		if(p->net.findPlace(name)) MOD_ABORT;
		pnapi::Place *place = &p->net.createPlace(name);
		p->places.push_back(place);
		p->placesMap[place] = p->places.size() - 1;
		return Place(p->places.size() - 1);
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

Transition Net::addTransition(const std::string &name) {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		if(p->net.findTransition(name)) MOD_ABORT;
		pnapi::Transition *trans = &p->net.createTransition(name);
		p->transitions.push_back(TransitionStore{trans,
		                                         {},
		                                         {}});
		p->transitionMap[trans] = p->transitions.size() - 1;
		return Transition(p->transitions.size() - 1);
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

void Net::addArc(Place p, Transition t) {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		assert(p.id < this->p->places.size());
		assert(t.id < this->p->transitions.size());
		pnapi::Place *place = this->p->places[p.id];
		pnapi::Transition *trans = this->p->transitions[t.id].t;
		this->p->transitions[t.id].tails.push_back(p);
		if(auto *arc = this->p->net.findArc(*place, *trans)) {
			arc->setWeight(arc->getWeight() + 1);
		} else {
			this->p->net.createArc(*place, *trans);
		}
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

void Net::addArc(Transition t, Place p) {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		assert(p.id < this->p->places.size());
		assert(t.id < this->p->transitions.size());
		pnapi::Place *place = this->p->places[p.id];
		pnapi::Transition *trans = this->p->transitions[t.id].t;
		this->p->transitions[t.id].heads.push_back(p);
		if(auto *arc = this->p->net.findArc(*trans, *place)) {
			arc->setWeight(arc->getWeight() + 1);
		} else {
			this->p->net.createArc(*trans, *place);
		}
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

void Net::printDot(std::ostream &s, const Marking &marking) const {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		using pnapi::io::operator<<;
		setMarking(marking);
		s << pnapi::io::dot << p->net;
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

void Net::printLola(std::ostream &s, const Marking &marking) const {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		using pnapi::io::operator<<;
		setMarking(marking);
		s << pnapi::io::lola << p->net;
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

#if defined(MOD_HAVE_PNAPI) && defined(MOD_HAVE_LOLA)

std::optional<std::string> Net::Pimpl::lolaIsReachable(const Marking &input,
                                                       const Marking &output,
                                                       const Net &owner,
                                                       bool getDAG) const {
	MOD_PETRINET_TRY ;
		std::stringstream s;
		owner.printLola(s, input);
		std::stringstream f;
		f << "REACHABLE ("; // the parenthesis is important!
		// it is a predicate, so we must fully specify the marking, including the zeros
		bool isFirst = true;
		for(std::size_t i = 0; i < places.size(); i++) {
			if(!isFirst) f << " AND ";
			isFirst = false;
			f << places[i]->getName() << " = " << output.getCount(Place(i));
		}
		f << ");";
		return causality::LoLA::call(f.str(), s.str(), [&owner, &input](std::ostream &s) {
			owner.printDot(s, input);
		});
	MOD_PETRINET_CATCH
}

#endif

std::optional<GraphType> Net::isReachable(const Marking &input, const Marking &output) const {
#if defined(MOD_HAVE_PNAPI) && defined(MOD_HAVE_LOLA)
	MOD_PETRINET_TRY ;
		const std::optional<std::string> stdout = p->lolaIsReachable(input, output, *this, true);
		if(!stdout) return {};
		if(stdout->find("digraph") != 0) MOD_ABORT;

		LolaGraph gLola;
		auto pId = get(&LolaVProp::id, gLola);
		auto pShape = get(&LolaVProp::shape, gLola);
		auto pLabel = get(&LolaVProp::label, gLola);
		boost::dynamic_properties dp(boost::ignore_other_properties);
		dp.property("id", pId);
		dp.property("shape", pShape);
		dp.property("label", pLabel);
		try {
			boost::read_graphviz(*stdout, gLola, dp, "id");
		} catch(const boost::bad_graphviz_syntax &e) {
			post::FileHandle fOut(lib::IO::getUniqueFilePrefix() + "lolaOuput.dot");
			fOut << *stdout;
			throw mod::FatalError("Error when parsing the Graphviz output from LoLA. Error is:\n"
			                      + e.errmsg + "\nOutput printed to " + std::string(fOut) + "\n");
		}
		{ // translate label to Place/Transition
			for(LolaVertex v : asRange(vertices(gLola))) {
				if(gLola[v].shape == "circle") {
					gLola[v].isPlace = true;
					pnapi::Place *pnapiPlace = p->net.findPlace(gLola[v].label);
					if(!pnapiPlace) MOD_ABORT;
					auto iter = p->placesMap.find(pnapiPlace);
					assert(iter != end(p->placesMap));
					gLola[v].p = Place(iter->second);
				} else if(gLola[v].shape == "box") {
					gLola[v].isPlace = false;
					pnapi::Transition *pnapiTransition = p->net.findTransition(gLola[v].label);
					if(!pnapiTransition) MOD_ABORT;
					auto iter = p->transitionMap.find(pnapiTransition);
					assert(iter != end(p->transitionMap));
					gLola[v].t = Transition(iter->second);
				} else
					MOD_ABORT;
			}
		}
		{ // first try to fix the clusterfuck Lola gives us
			const bool verbose = true;

			struct Missing {
				std::vector<LolaVertex> asHeadTransitions, asTailTransitions;
				std::vector<LolaVertex> missingHeadTransitionPlaces, missingTailTransitionPlaces;
			};
			std::unordered_map<Place, Missing, Place::Hash> placeMissing;
			Marking fixInput(input), fixOutput(output); // these should reach 0 in the end of the fixing

			const auto doPrintout = [&](std::string name, auto label) {
				printLolaGraph(*p, gLola, name, [&input, &output](const LolaGraph &g, LolaVertex v, std::ostream &s) {
					if(g[v].isPlace) {
						printLolaAttribIOColour(input, output, g[v].p, s);
					}
				}, [&](const LolaGraph &g, std::ostream &s) {
					std::size_t tailCount = 0, headCount = 0;
					for(const auto &pPair : placeMissing) {
						Place place = pPair.first;
						for(LolaVertex v : pPair.second.asTailTransitions) {
							s << "dTail_" << g[v].id << "_" << tailCount
							  << " [ shape=ellipse fillcolor=red style=filled label=\"";
							s << p->places[place.id]->getName() << "\" ];\n";
							s << "dTail_" << g[v].id << "_" << tailCount << " -> " << g[v].id << ";\n";
							tailCount++;
						}
						for(LolaVertex v : pPair.second.asHeadTransitions) {
							s << "dHead_" << g[v].id << "_" << headCount
							  << " [ shape=ellipse fillcolor=red style=filled label=\"";
							s << p->places[place.id]->getName() << "\" ];\n";
							s << g[v].id << " -> " << "dHead_" << g[v].id << "_" << headCount << ";\n";
							headCount++;
						}
					}
				}, label);
			};


			// First make lists of missing tail and head memberships for each place.
			// Do it by see what is missing in the transitions.
			for(LolaVertex v : asRange(vertices(gLola))) {
				if(gLola[v].isPlace) continue;
				const auto &trans = p->transitions[gLola[v].t.id];
				std::vector<Place> tailPlaces = trans.tails, headPlaces = trans.heads; // the real ones
				std::vector<Place> tailPlacesCand, headPlacesCand; // what Lola gives us
				for(LolaVertex vTail : asRange(inv_adjacent_vertices(v, gLola))) {
					assert(gLola[vTail].isPlace);
					tailPlacesCand.push_back(gLola[vTail].p);
				}
				for(LolaVertex vHead : asRange(adjacent_vertices(v, gLola))) {
					assert(gLola[vHead].isPlace);
					headPlacesCand.push_back(gLola[vHead].p);
				}
				auto handleHT = [&placeMissing, &gLola, v, this](bool isTail,
				                                                 std::vector<Place> &places,
				                                                 std::vector<Place> &placesCand) {
					std::sort(begin(places), end(places));
					std::sort(begin(placesCand), end(placesCand));
					if(placesCand.size() < places.size()) {
						// placesCand \subset places, so do merge to add the missing
						std::size_t iWhole = 0, iSub = 0;
						while(iWhole < places.size()) {
							if(iSub < placesCand.size() && placesCand[iSub] == places[iWhole]) {
								iSub++;
								iWhole++;
							} else {
								assert(iSub == placesCand.size() || places[iWhole] < placesCand[iSub]);
								Place place = places[iWhole];
								if(verbose) {
									std::cout << "Vertex " << get(boost::vertex_index_t(), gLola, v) << ", transition "
									          << p->transitions[gLola[v].t.id].t->getName() << " missing ";
									if(isTail) std::cout << "tail ";
									else std::cout << "head ";
									std::cout << p->places[place.id]->getName();
									std::cout << std::endl;
								}
								if(isTail) {
									placeMissing[place].asTailTransitions.push_back(v);
								} else {
									placeMissing[place].asHeadTransitions.push_back(v);
								}
								iWhole++;
							}
						}
						assert(iWhole == places.size());
						assert(iSub == placesCand.size());
					} else { // just for assertion
						assert(placesCand.size() == places.size());
						assert(std::equal(begin(places), end(places), begin(placesCand)));
					}
				};
				handleHT(true, tailPlaces, tailPlacesCand);
				handleHT(false, headPlaces, headPlacesCand);
			}

			// Now check which places in the DAG have out-/in-degree 0, and decrease the input/ouput
			Marking inputUsed, outputUsed;
			for(LolaVertex v : asRange(vertices(gLola))) {
				if(!gLola[v].isPlace) continue;
				Place place = gLola[v].p;
				if(in_degree(v, gLola) == 0) {
					if(input.getCount(place) > 0) {
						inputUsed.add(place, 1);
					} else {
						placeMissing[place].missingHeadTransitionPlaces.push_back(v);
						if(verbose) {
							std::cout << "Vertex " << get(boost::vertex_index_t(), gLola, v) << ", place "
							          << p->places[gLola[v].p.id]->getName() << " missing head transition\n";
						}
					}
				}
				if(out_degree(v, gLola) == 0) {
					if(output.getCount(place) > 0) {
						outputUsed.add(place, 1);
					} else {
						placeMissing[place].missingTailTransitionPlaces.push_back(v);
						if(verbose) {
							std::cout << "Vertex " << get(boost::vertex_index_t(), gLola, v) << ", place "
							          << p->places[gLola[v].p.id]->getName() << " missing tail transition\n";
						}
					}
				}
			}
			// if we used more input/output than we should, then it's ambiguous to connect it
			for(std::size_t i = 0; i < p->places.size(); i++) {
				Place place(i);
				if(inputUsed.getCount(place) > fixInput.getCount(place)) {
					std::cout << "Place is " << p->places[i]->getName() << "\n";
					std::cout << "Input:     " << fixInput.getCount(place) << "\n";
					std::cout << "InputUsed: " << inputUsed.getCount(place) << "\n";
					doPrintout("no-example-input-too-many", jla_boost::Nop<>());
					MOD_ABORT;
				}
				if(outputUsed.getCount(place) > fixOutput.getCount(place)) {
					std::cout << "Place is " << p->places[i]->getName() << "\n";
					std::cout << "Output:     " << fixOutput.getCount(place) << "\n";
					std::cout << "OutputUsed: " << outputUsed.getCount(place) << "\n";
					doPrintout("no-example-output-too-many", jla_boost::Nop<>());
					MOD_ABORT;
				}
				fixInput.add(place, inputUsed.getCount(place));
				fixOutput.add(place, output.getCount(place));
			}

			// Let's try to repair the unambiguous
			for(auto &pp : placeMissing) {
				auto inputCount = fixInput.getCount(pp.first);
				auto outputCount = fixOutput.getCount(pp.first);
				auto &data = pp.second;
				// check if we can create input vertices
				if(inputCount > 0 && data.asTailTransitions.size() > 0 && data.missingTailTransitionPlaces.size() == 0) {
					if(inputCount < data.asTailTransitions.size()) {
						std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
						std::cout << "InputCount: " << inputCount << "\n";
						std::cout << "asTailTransitions.size(): " << data.asTailTransitions.size() << "\n";
						doPrintout("no-example", jla_boost::Nop<>());
						MOD_ABORT;
					}
					for(LolaVertex vTrans : data.asTailTransitions) {
						LolaVertex vTail = add_vertex(gLola);
						gLola[vTail].id = "input_" + std::to_string(get(boost::vertex_index_t(), gLola, vTail));
						gLola[vTail].isPlace = true;
						gLola[vTail].p = pp.first;
						gLola[vTail].isOriginal = false;
						add_edge(vTail, vTrans, gLola);
					}
					fixInput.add(pp.first, -data.asTailTransitions.size());
					inputCount = fixInput.getCount(pp.first);
					data.asTailTransitions.clear();
				} else if(data.asTailTransitions.size() == 1 && data.missingTailTransitionPlaces.size() == 1) {
					if(inputCount > 0) {
						// so now we have too few transitions to connect to?
						std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
						doPrintout("no-example", jla_boost::Nop<>());
						MOD_ABORT;
					}
					LolaVertex vPlace = data.missingTailTransitionPlaces.front();
					LolaVertex vTrans = data.asTailTransitions.front();
					add_edge(vPlace, vTrans, gLola);
					data.asTailTransitions.clear();
					data.missingTailTransitionPlaces.clear();
				}
				// check if we can create output vertices
				if(outputCount > 0 && data.asHeadTransitions.size() > 0 && data.missingHeadTransitionPlaces.size() == 0) {
					if(outputCount != data.asHeadTransitions.size()) {
						std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
						std::cout << "OutputCount: " << outputCount << "\n";
						std::cout << "asHeadTransitions.size(): " << data.asHeadTransitions.size() << "\n";
						doPrintout("no-example", jla_boost::Nop<>());
						MOD_ABORT;
					}
					std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
					doPrintout("no-example", jla_boost::Nop<>());
					MOD_ABORT;
					for(LolaVertex vTrans : data.asHeadTransitions) {
						LolaVertex vHead = add_vertex(gLola);
						gLola[vHead].id = "output_" + std::to_string(get(boost::vertex_index_t(), gLola, vHead));
						gLola[vHead].isPlace = true;
						gLola[vHead].p = pp.first;
						gLola[vHead].isOriginal = false;
						add_edge(vTrans, vHead, gLola);
					}
					fixOutput.setCount(pp.first, 0);
					outputCount = fixOutput.getCount(pp.first);
				} else if(data.asHeadTransitions.size() == 1 && data.missingHeadTransitionPlaces.size() == 1) {
					if(outputCount > 0) {
						// so now we have too few transitions to connect to?
						std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
						doPrintout("no-example", jla_boost::Nop<>());
						MOD_ABORT;
					}
					std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
					doPrintout("no-example", jla_boost::Nop<>());
					MOD_ABORT;
				}
				// check if we can create an intermediary vertex
				if(inputCount == 0 && outputCount == 0
				   && data.asTailTransitions.size() == 1 && data.asHeadTransitions.size() == 1) {
					LolaVertex vPlace = add_vertex(gLola);
					gLola[vPlace].id = p->places[pp.first.id]->getName();
					gLola[vPlace].isPlace = true;
					gLola[vPlace].p = pp.first;
					gLola[vPlace].isOriginal = false;
					add_edge(data.asHeadTransitions.front(), vPlace, gLola);
					add_edge(vPlace, data.asTailTransitions.front(), gLola);
					data.asHeadTransitions.clear();
					data.asTailTransitions.clear();
				}
			}

			// now we are lazy and just hope the first perfect matching of the missing places is ok
			for(auto &pp : placeMissing) {
				// perhaps we are just missing edges
				if(pp.second.missingTailTransitionPlaces.size() == pp.second.asTailTransitions.size()) {
					for(std::size_t i = 0; i < pp.second.missingTailTransitionPlaces.size(); i++) {
						LolaVertex vPlace = pp.second.missingTailTransitionPlaces[i],
								vTrans = pp.second.asTailTransitions[i];
						add_edge(vPlace, vTrans, gLola);
					}
					pp.second.missingTailTransitionPlaces.clear();
					pp.second.asTailTransitions.clear();
					continue;
				}
				// or perhaps we are missing muliple intermediary places
				if(pp.second.missingTailTransitionPlaces.empty() && pp.second.missingHeadTransitionPlaces.empty()) {
					if(pp.second.asTailTransitions.size() == pp.second.asHeadTransitions.size()) {
						for(int i = 0; i != pp.second.asTailTransitions.size(); ++i) {
							LolaVertex vPlace = add_vertex(gLola);
							gLola[vPlace].id = p->places[pp.first.id]->getName();
							gLola[vPlace].isPlace = true;
							gLola[vPlace].p = pp.first;
							gLola[vPlace].isOriginal = false;
							LolaVertex tIn = pp.second.asHeadTransitions[i];
							LolaVertex tOut = pp.second.asTailTransitions[i];
							add_edge(tIn, vPlace, gLola);
							add_edge(vPlace, tOut, gLola);
						}
						pp.second.asHeadTransitions.clear();
						pp.second.asTailTransitions.clear();
						continue;
					}
				}
				std::cout << "Place is " << p->places[pp.first.id]->getName() << "\n";
				std::cout << "missingTailTransitionPlaces.size(): " << pp.second.missingTailTransitionPlaces.size()
				          << "\n";
				std::cout << "asTailTransitions.size(): " << pp.second.asTailTransitions.size() << "\n";
				std::cout << "missingHeadTransitionPlaces.size(): " << pp.second.missingHeadTransitionPlaces.size()
				          << "\n";
				std::cout << "asHeadTransitions.size(): " << pp.second.asHeadTransitions.size() << "\n";
				doPrintout("no-example", jla_boost::Nop<>());
				MOD_ABORT; // well, this becomes difficult
			}

			{ // do play-through
				Marking tempInput(input);
				std::vector<LolaVertex> beach, newBeach;
				for(LolaVertex v : asRange(vertices(gLola))) {
					if(!gLola[v].isPlace) continue;
					if(in_degree(v, gLola) > 0) continue;
					auto count = tempInput.getCount(gLola[v].p);
					if(count > 0) {
						tempInput.setCount(gLola[v].p, count - 1);
						beach.push_back(v);
					} else {
						if(verbose) {
							std::cout << "Vertex " << get(boost::vertex_index_t(), gLola, v) << ", place "
							          << p->places[gLola[v].p.id]->getName() << " dangling." << std::endl;
						}
						MOD_ABORT;
					}
				}
				// now tempInput == 0
				for(std::size_t i = 0; i < p->places.size(); i++) {
					if(tempInput.getCount(Place(i)) != 0) {
						std::cout << "Play-through error: input marking not satisfied, place "
						          << p->places[i]->getName() << " are missing " << tempInput.getCount(Place(i)) << " tokens."
						          << std::endl;
						doPrintout("playthrough-error", jla_boost::Nop<>());
						MOD_ABORT;
					}
				}

				Marking tempOutput(output);
				std::vector<bool> marked(num_vertices(gLola), false);
				auto idx = get(boost::vertex_index_t(), gLola);
				while(!beach.empty()) {
					for(LolaVertex v : beach) {
						if(marked[idx[v]]) continue;
						if(gLola[v].isPlace) {
							marked[idx[v]] = true;
							if(out_degree(v, gLola) == 0) {
								auto count = tempOutput.getCount(gLola[v].p);
								if(count == 0) {
									std::cout << "Play-through error: output marking not satisfied, place "
									          << p->places[gLola[v].p.id]->getName() << " is already zero." << std::endl;
									doPrintout("playthrough-error", jla_boost::Nop<>());
									MOD_ABORT;
								}
								tempOutput.setCount(gLola[v].p, count - 1);
							} else {
								for(LolaVertex vOut : asRange(adjacent_vertices(v, gLola)))
									newBeach.push_back(vOut);
							}
						} else {
							auto vIns = inv_adjacent_vertices(v, gLola);
							if(std::all_of(vIns.first, vIns.second, [&marked, idx](LolaVertex vIn) {
								return marked[idx[vIn]];
							})) {
								marked[idx[v]] = true;
								for(LolaVertex vOut : asRange(adjacent_vertices(v, gLola)))
									newBeach.push_back(vOut);
							}
						}
					}
					beach = newBeach;
					newBeach.clear();
				}
				for(std::size_t i = 0; i < p->places.size(); i++) {
					if(tempOutput.getCount(Place(i)) != 0) {
						std::cout << "Play-through error: output marking not satisfied, place "
						          << p->places[i]->getName() << " are missing " << tempOutput.getCount(Place(i)) << " tokens."
						          << std::endl;
						doPrintout("playthrough-error", [&marked, &idx](const LolaGraph &g, LolaVertex v, std::ostream &s) {
							if(marked[idx[v]]) {
								s << ", mark";
							}
						});
						MOD_ABORT;
					}
				}
			} // end of play-through

			doPrintout("repaired", jla_boost::Nop<>());
		} // end fix Lolas output

		// convert the Lola graph to our graph
		GraphType g;
		auto idx = get(boost::vertex_index_t(), gLola);
		std::vector<bool> marked(num_vertices(gLola), false);
		std::vector<Vertex> lolaToVertex(num_vertices(gLola), g.null_vertex());
		std::deque<LolaVertex> work;
		for(LolaVertex v : asRange(vertices(gLola))) {
			if(in_degree(v, gLola) == 0) work.push_back(v);
		}
		while(!work.empty()) {
			LolaVertex vLola = work.front();
			work.pop_front();
			if(marked[idx[vLola]]) continue;
			if(gLola[vLola].isPlace) {
				marked[idx[vLola]] = true;
				assert(out_degree(vLola, gLola) <= 1);
				for(LolaVertex vOut : asRange(adjacent_vertices(vLola, gLola)))
					work.push_back(vOut);
			} else {
				auto vIns = inv_adjacent_vertices(vLola, gLola);
				bool allMarked = std::all_of(vIns.first, vIns.second, [&marked, idx](LolaVertex vIn) {
					return marked[idx[vIn]];
				});
				if(!allMarked) continue;
				marked[idx[vLola]] = true;
				Vertex v = add_vertex(g);
				lolaToVertex[idx[vLola]] = v;
				g[v].t = gLola[vLola].t;
				for(LolaVertex vInLola : asRange(inv_adjacent_vertices(vLola, gLola))) {
					if(in_degree(vInLola, gLola) == 0) continue;
					assert(in_degree(vInLola, gLola) == 1);
					Vertex vPrevLola = *inv_adjacent_vertices(vInLola, gLola).first;
					assert(!gLola[vPrevLola].isPlace);
					assert(lolaToVertex[idx[vPrevLola]] != g.null_vertex());
					add_edge(lolaToVertex[idx[vPrevLola]], v, g);
				}
				for(LolaVertex vOut : asRange(adjacent_vertices(vLola, gLola)))
					work.push_back(vOut);
			}
		} // end while(!work.empty())

		{
			post::FileHandle s(lib::IO::getUniqueFilePrefix() + "convertedLola.dot");
			s << "digraph g {\n";
			s << "rankdir=LR;\n";
			for(Vertex v : asRange(vertices(g))) {
				s << get(boost::vertex_index_t(), g, v) << " [";
				s << " label=\"" << p->transitions[g[v].t.id].t->getName() << "\"";
				s << " ];\n";
			}
			for(Edge e : asRange(edges(g))) {
				s << get(boost::vertex_index_t(), g, source(e, g)) << " -> "
				  << get(boost::vertex_index_t(), g, target(e, g)) << ";\n";
			}
			s << "}\n";
			lib::IO::post() << "dot -Tpdf -o \"" << std::string(s) << ".pdf\" \"" << std::string(s) << "\"\n";
		}
		return g;
	MOD_PETRINET_CATCH
#else
#if !defined(MOD_HAVE_PNAPI)
	MOD_NO_PNAPI_ERROR;
#else
	MOD_NO_LOLA_ERROR;
#endif // !defined(MOD_HAVE_PNAPI)
#endif // defined(MOD_HAVE_PNAPI) && defined(MOD_HAVE_LOLA)
}

void Net::setMarking(const Marking &marking) const {
#ifdef MOD_HAVE_PNAPI
	MOD_PETRINET_TRY
		for(unsigned int i = 0; i < p->places.size(); i++)
			p->places[i]->setTokenCount(marking.getCount(Place(i)));
	MOD_PETRINET_CATCH
#else
	MOD_NO_PNAPI_ERROR;
#endif // ifdef MOD_HAVE_PNAPI
}

} // namespace mod::lib::PetriNet