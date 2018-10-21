/** \file RuleSet.h
 *  \authors cldurand,takaakimatsuo
 *  \date 2018/06/25
 *
 *  \brief RuleSet
 */
#ifndef QUISP_RULES_RULESET_H_
#define QUISP_RULES_RULESET_H_

#include "Rule.h"
#include <omnetpp.h>

namespace quisp {
namespace rules {

/** \class RuleSet RuleSet.h
 *
 * \brief Set of rules for the RuleEngine.
 */

class RuleSet : public std::list<pRule> {
    public:
        int owner;
        int entangled_partner;
        RuleSet(int o, int e) : std::list<pRule> () { owner = o; entangled_partner = e;}
        void addRule(Rule * r) { push_back(pRule(r)); };
        void addRule(pRule& r) { push_back(pRule(std::move(r))); };
        void finalize();
        int getSize() {return this->size();};
        void destroyThis() {EV<<"Destroying this RuleSet. \n "; delete this; };

};

} // namespace rules
} // namespace quisp

#endif//QUISP_RULES_RULESET_H_
