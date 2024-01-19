// Project identifier: AD48FB4835AF347EB0CA8009E24C3B13F8519882

#ifndef DEPLOYMENT_H
#define DEPLOYMENT_H

#include <vector>
#include <string>

using namespace std;

struct Deployment {
    uint32_t general;
    uint32_t force;
    uint32_t num_troops;
    uint32_t id; // to specify the order

    Deployment(uint32_t general_in, uint32_t force_in, uint32_t num_in, uint32_t id_in)
        : general(general_in), force(force_in), num_troops(num_in), id(id_in) { }

};

struct JediComparator {    
    bool operator()(const Deployment &d1, const Deployment &d2) const {
        // Jedi with smaller force has higher priority
        if (d1.force != d2.force) return d1.force > d2.force; 
        // Who comes eailer has higher priority
        return d1.id > d2.id;
    }
};

struct SithComparator {    
    bool operator()(const Deployment &d1, const Deployment &d2) const {
        // Sith with higher force has higher priority
        if (d1.force != d2.force) return d1.force < d2.force; 
        // who comes eailer has higher priority
        return d1.id > d2.id;
    }
};

#endif