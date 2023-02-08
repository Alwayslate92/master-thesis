#ifndef PETRI_IO_HPP
#define PETRI_IO_HPP

#include <petri/Marking.hpp>

#include <functional>
#include <iosfwd>

namespace petri {

void printLoLANet(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                  std::function<std::string(petri::Place)> pName,
                  std::function<std::string(petri::Transition)> tName);
void printGraphvizNet(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                      std::function<std::string(petri::Place)> pName,
                      std::function<std::string(petri::Transition)> tName);

void printLoLAReachabilityPredicate(std::ostream &s, const petri::Net &net, const petri::Marking &m,
                                    std::function<std::string(petri::Place)> pName);

} // namespace petri

#endif // PETRI_IO_HPP