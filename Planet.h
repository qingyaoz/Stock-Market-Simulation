// Project identifier: AD48FB4835AF347EB0CA8009E24C3B13F8519882

#ifndef PLANET_H
#define PLANET_H

#include <queue>
#include "Deployment.h"


using namespace std;



struct Planet{
    uint32_t id;

    priority_queue<Deployment, vector<Deployment>, JediComparator> jedi;
    priority_queue<Deployment, vector<Deployment>, SithComparator> sith;
    priority_queue<uint32_t, vector<uint32_t>, greater<uint32_t>> max_half;
    priority_queue<uint32_t, vector<uint32_t>> min_half;

    struct Movie{
        enum class movieState {Init, One, Battled, Maybe} state = movieState::Init;
        uint32_t jedi_timestamp;
        uint32_t jedi_force = 0; // weakest
        uint32_t sith_timestamp;
        uint32_t sith_force = 0; // strongest
        uint32_t potl_time;
        uint32_t potl_force = 0; // jedi for attack, sith for ambush 
    };
    Movie attack;
    Movie ambush;
};

struct General{
    uint32_t sith;
    uint32_t jedi;
    uint32_t survived;

    General()
        : sith(0), jedi(0), survived(0) { }

    General(uint32_t sith_in, uint32_t jedi_in, uint32_t survived_in)
        : sith(sith_in), jedi(jedi_in), survived(survived_in){ }

};
#endif