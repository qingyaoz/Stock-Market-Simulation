// Project identifier: AD48FB4835AF347EB0CA8009E24C3B13F8519882

#include <getopt.h>
#include <iostream>
#include <algorithm>

#include "P2random.h"
#include "Planet.h"

using namespace std;

class Galaxy{
public:
    Galaxy():mode_v(false), mode_m(false), mode_g(false), mode_w(false),
             curr_time(0), num_battles(0), num_planets(0), num_generals(0){}
    void get_options(int argc, char** argv);
    void read_header(); // read first four lines
    void read_deploy();
    void end_day();

private:
    // ------ Helper Varible ------
    bool mode_v, mode_m, mode_g,mode_w;
    uint32_t curr_time;
    uint32_t num_battles;
    vector<Planet> planets;
    vector<General> generals; // only resize when general-eval mode
    vector<Deployment> data;

    // ------ Hearder Input ------
    string input_mode; // DL or PR
    uint32_t num_planets;
    uint32_t num_generals;

    // ------ Helper Function ------
    void battleSITH(Planet& planet, Deployment& new_arrive);
    void battleJEDI(Planet& planet, Deployment& new_arrive);
    void update_median(Planet& planet, uint32_t total_lost);
    void get_median();
    void movie_attack(Planet::Movie* attack, uint32_t force_in,
                                uint32_t time_in, string side);
    void movie_ambush(Planet::Movie* ambush, uint32_t force_in,
                                uint32_t time_in, string side);
    void general_eval();
    void print_movie();
};


// ----------------------------------------------------------------------------
//                                     Driver
// ----------------------------------------------------------------------------

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    // Instantiate a galaxy battle.
    Galaxy game; 
    game.get_options(argc, argv);
    game.read_header();
    game.read_deploy();
    game.end_day();
    return (0); 
}


// ----------------------------------------------------------------------------
//                             Public Function
// ----------------------------------------------------------------------------

// Read and process command line options.
void Galaxy::get_options(int argc, char** argv) {
    int option_index = 0, option = 0;
    
    // Don't display getopt error messages about options
    opterr = false;

    // use getopt to find command line options
    struct option longOpts[] = {{ "verbose", no_argument, nullptr, 'v'},
                                { "median", no_argument, nullptr, 'm'},
                                { "general-eval", no_argument, nullptr, 'g'},
                                { "watcher", no_argument, nullptr, 'w'}};
    
    while ((option=getopt_long(argc,argv,"vmgw",longOpts,&option_index))!=-1){
        switch (option) {
            case 'v':
                mode_v = true;
                break;
            
            case 'm':
                mode_m = true;
                break;
            
            case 'g':
                mode_g = true;
                break;

            case 'w':
                mode_w = true;
                break;
        } 
        // Unknown command line option
    }
}

void Galaxy::read_header(){
    // first line: comment
    string waste;
    getline(cin, waste);

    cin >> waste >> input_mode;
    cin >> waste >> num_generals;
    cin >> waste >>num_planets;

    if (mode_g) generals.resize(num_generals);
    planets.resize(num_planets);
    for (uint32_t i = 0; i < num_planets; ++i){
        planets[i].id = i;
    }
}

void Galaxy::read_deploy(){
    // ---------------------------- PR Mode ----------------------------
    stringstream ss;
    if (input_mode == "PR") {  // inputMode is read from line 2 of the header
    // TODO: include number of generals and number of planets read from lines
    // 3-4 of the header and add code to read random seed, number of
    // deployments, and arrival rate from lines 5-7 of the input file
        uint32_t seed, num_deployments, arrival_rate;
        string waste;
        cin >> waste >> seed >> waste >> num_deployments >> waste
            >> arrival_rate;
        P2random::PR_init(ss, seed, num_generals, num_planets, num_deployments,
                          arrival_rate);
    }  // if ..inputMode

    // Create a reference variable that is ALWAYS used for reading input.
    // If PR mode is on, refer to the stringstream. Otherwise, refer to cin.
    // This is a place where the ternary operator must be used: an equivalent
    // if/else is impossible because reference variables must be initialized
    // when they are created.
    istream &inputStream = input_mode == "PR" ? ss : cin;


    // ----------------------- Deployment Prepare -----------------------
    uint32_t timestamp, general, planet, force, num_troops;
    string side;
    char waste;
    uint32_t deploy_id =0;

    // ------------------------ Deployment Begin ------------------------
    cout << "Deploying troops..." << "\n";
    // Make sure to read an entire deployment in the while statement
    while (inputStream >> timestamp >> side >> waste >> general >> waste
                       >> planet >> waste >> force >> waste >> num_troops){

        // ---------- Error Checking for DL Mode ----------
        if (input_mode == "DL") {
            if (timestamp < curr_time){
                cerr << "Invalid decreasing timestamp" << endl;
                exit(1);
            }
            if (general >= num_generals){
                cerr << "Invalid general ID" << endl;
                exit(1);
            }
            if (planet >= num_planets){
                cerr << "Invalid planet ID" << endl;
                exit(1);
            }
            if (force <= 0){
                cerr << "Invalid force sensntivity level" << endl;
                exit(1);
            }
            if (num_troops <= 0){
                cerr << "Invalid number of troops" << endl;
                exit(1);
            }
            if (side != "JEDI" && side != "SITH"){
                cerr << "Invalid side of force" << endl;
                exit(1);
            }
        } 

        // ---------- Timestamp Checking ----------
        if (timestamp != curr_time){
            if (mode_m) get_median();
            curr_time = timestamp;
        }

        // ---------- Deployment Processing ----------
        if (mode_w){
            movie_attack(&planets[planet].attack, force, timestamp, side);
            movie_ambush(&planets[planet].ambush, force, timestamp, side);
        }
        
        Deployment new_arrive (general, force, num_troops, deploy_id);
        if (side == "SITH"){
            if (mode_g){
                generals[general].sith += num_troops;
            }
            battleSITH(planets[planet], new_arrive);
        }
        else {
            if (mode_g){
                generals[general].jedi += num_troops;
            }
            battleJEDI(planets[planet], new_arrive);
        }
        deploy_id += 1;
    }   // while ..inputStream
    if (mode_m) get_median();
}

// Print end-of-day summary output.
void Galaxy::end_day(){
    cout << "---End of Day---" << "\n";
    cout << "Battles: " << num_battles << "\n";

    if (mode_g) general_eval();
    if (mode_w) print_movie();
}


// ----------------------------------------------------------------------------
//                         Private Function (Helper)
// ----------------------------------------------------------------------------

// A SITH is deploied
void Galaxy::battleSITH(Planet& planet, Deployment& new_arrive){

    while (!planet.jedi.empty() &&
           planet.jedi.top().force <= new_arrive.force){
        uint32_t lost_troops = new_arrive.num_troops;
        uint32_t potl_general = planet.jedi.top().general;

        if (planet.jedi.top().num_troops > new_arrive.num_troops){
            // new_arrive sith will be empty
            Deployment top = planet.jedi.top();
            planet.jedi.pop();
            top.num_troops -= lost_troops;
            planet.jedi.push(top);
            new_arrive.num_troops = 0;
        } else if (planet.jedi.top().num_troops == new_arrive.num_troops){
            // both will be empty
            planet.jedi.pop();
            new_arrive.num_troops = 0;
        } else {
            lost_troops = planet.jedi.top().num_troops;
            planet.jedi.pop();
            new_arrive.num_troops -= lost_troops;
            // may have more battle
        }

        if (mode_v)
        cout << "General " << new_arrive.general
             << "'s battalion attacked General " << potl_general
             << "'s battalion on planet " << planet.id << ". " << 2*lost_troops
             <<" troops were lost." << "\n";
        
        num_battles += 1;
        update_median(planet, 2*lost_troops);

        // no more battle & no need to push
        if (new_arrive.num_troops == 0) return;
    }
    // if new_arrive sith still has rest troops, push
    planet.sith.push(new_arrive);
}

// A JEDI is deploied
void Galaxy::battleJEDI(Planet& planet, Deployment& new_arrive){

    while (!planet.sith.empty() &&
            planet.sith.top().force >= new_arrive.force){
        uint32_t lost_troops = new_arrive.num_troops;
        uint32_t potl_general = planet.sith.top().general;
       
        if (planet.sith.top().num_troops > new_arrive.num_troops){
            // new_arrive sith will be empty
            Deployment top = planet.sith.top();
            top.num_troops -= lost_troops;
            planet.sith.pop();
            planet.sith.push(top);
            new_arrive.num_troops = 0;
        } else if (planet.sith.top().num_troops == new_arrive.num_troops){
            // both will be empty
            planet.sith.pop();
            new_arrive.num_troops = 0;
        } else {
            lost_troops = planet.sith.top().num_troops;
            planet.sith.pop();
            new_arrive.num_troops -= lost_troops;
            // may have more battle
        }

        if (mode_v)
        cout << "General " << potl_general << "'s battalion attacked General "
             << new_arrive.general << "'s battalion on planet "
             << planet.id << ". " << 2*lost_troops <<" troops were lost."
             << "\n";
        
        num_battles += 1;
        update_median(planet, 2*lost_troops);

        // no more battle & no need to push
        if (new_arrive.num_troops == 0) return; 
    }
    // new_arrive sith still has rest troops, push
    planet.jedi.push(new_arrive);
}

// ------------------------ Median Output ------------------------
// The one more number always goes into min_half
void Galaxy::update_median(Planet& planet, uint32_t total_lost){
    // cout <<"update median"<<endl;
    if (planet.min_half.empty()){
        planet.min_half.push(total_lost);
        return;
    }
    if (planet.min_half.top() > total_lost){
        planet.min_half.push(total_lost);
        if (planet.min_half.size() > 1+planet.max_half.size()){
            planet.max_half.push(planet.min_half.top());
            planet.min_half.pop();
        }
    } else {
        planet.max_half.push(total_lost);
        if (planet.max_half.size() > planet.min_half.size()){
            planet.min_half.push(planet.max_half.top());
            planet.max_half.pop();
        }
    }
}

void Galaxy::get_median() {
    for (Planet &planet : planets){
        if (planet.min_half.size()== 0) continue;

        uint32_t median;
        if (planet.min_half.size() > planet.max_half.size()) 
            median = planet.min_half.top();
        else median = (planet.min_half.top()+planet.max_half.top())/2;

        cout << "Median troops lost on planet " << planet.id << " at time "
             << curr_time << " is " << median << "." << "\n";
    }
    
}

// ------------------------ General Output ------------------------
void Galaxy::general_eval(){
    for (Planet& planet : planets){
        // cout <<planet.id<<endl; //debug
        while (!planet.jedi.empty()){
            generals[planet.jedi.top().general].survived
                        += planet.jedi.top().num_troops;
            planet.jedi.pop();
        }   
        while (!planet.sith.empty()){
            generals[planet.sith.top().general].survived
                        += planet.sith.top().num_troops;
            planet.sith.pop();
        }
    }

    cout << "---General Evaluation---" << "\n";
    for (uint32_t id = 0; id < num_generals; ++id){
        cout << "General " << id << " deployed " << generals[id].jedi
             << " Jedi troops and "<< generals[id].sith << " Sith troops, and "
             << generals[id].survived << "/" 
             << generals[id].jedi+generals[id].sith << " troops survived."
             << "\n";
    }
}

// ------------------------ Movie Watcher Output ------------------------
void Galaxy::movie_attack(Planet::Movie* attack, uint32_t force_in,
                          uint32_t time_in, string side){
    if (side == "JEDI"){
        if (attack->state == Planet::Movie::movieState::Maybe){
            if (force_in < attack->potl_force){
                attack->potl_force = force_in;
                attack->potl_time = time_in;
            } 
        } else if (attack->state == Planet::Movie::movieState::Battled){
            if (force_in < attack->jedi_force){
                attack->state = Planet::Movie::movieState::Maybe;
                attack->potl_force = force_in;
                attack->potl_time = time_in;
            }
        } else if (attack->state == Planet::Movie::movieState::One){
            if (force_in < attack->jedi_force){
                attack->jedi_force = force_in;
                attack->jedi_timestamp = time_in;
            }
        } else{
            attack->state = Planet::Movie::movieState::One;
            attack->jedi_force = force_in;
            attack->jedi_timestamp = time_in;
        }
    }
    else { // side == SITH
        if (attack->state == Planet::Movie::movieState::Maybe){
            // include equal: since potl is always smaller 
            if (force_in > attack->potl_force && (force_in- attack->potl_force)
                         > (attack->sith_force - attack->jedi_force)){
                attack->sith_force = force_in;
                attack->sith_timestamp = time_in;
                attack->jedi_force = attack->potl_force;
                attack->jedi_timestamp = attack->potl_time;
                attack->state = Planet::Movie::movieState::Battled;
            }
        } else if (attack->state == Planet::Movie::movieState::Battled){
            if (force_in > attack->sith_force){
                attack->sith_force = force_in;
                attack->sith_timestamp = time_in;
            }
        } else if (attack->state == Planet::Movie::movieState::One){
            if (force_in >= attack->jedi_force){ // >=
                attack->sith_force = force_in;
                attack->sith_timestamp = time_in;
                attack->state = Planet::Movie::movieState::Battled;
            }
        }
    }
}

void Galaxy::movie_ambush(Planet::Movie* ambush, uint32_t force_in,
                          uint32_t time_in, string side){
    if (side == "SITH"){
        if (ambush->state == Planet::Movie::movieState::Maybe){
            if (force_in > ambush->potl_force){
                ambush->potl_force = force_in;
                ambush->potl_time = time_in;
            } 
        } else if (ambush->state == Planet::Movie::movieState::Battled){
            if (force_in > ambush->sith_force){
                ambush->state = Planet::Movie::movieState::Maybe;
                ambush->potl_force = force_in;
                ambush->potl_time = time_in;
            }
        } else if (ambush->state == Planet::Movie::movieState::One){
            if (force_in > ambush->sith_force){
                ambush->sith_force = force_in;
                ambush->sith_timestamp = time_in;
            }
        } else{
            ambush->state = Planet::Movie::movieState::One;
            ambush->sith_force = force_in;
            ambush->sith_timestamp = time_in;
        }
    }
    else { // side == JEDI
        if (ambush->state == Planet::Movie::movieState::Maybe){
            // include equal: since potl is always larger 
            if (force_in < ambush->potl_force && (ambush->potl_force -force_in)
                         > (ambush->sith_force - ambush->jedi_force)){ // old
                ambush->jedi_force = force_in;
                ambush->jedi_timestamp = time_in;
                ambush->sith_force = ambush->potl_force;
                ambush->sith_timestamp = ambush->potl_time;
                ambush->state = Planet::Movie::movieState::Battled;
            }
        } else if (ambush->state == Planet::Movie::movieState::Battled){
            if (force_in < ambush->jedi_force){
                ambush->jedi_force = force_in;
                ambush->jedi_timestamp = time_in;
            }
        } else if (ambush->state == Planet::Movie::movieState::One){
            if (force_in <= ambush->sith_force){
                ambush->jedi_force = force_in;
                ambush->jedi_timestamp = time_in;
                ambush->state = Planet::Movie::movieState::Battled;
            }
        }
    }
}

void Galaxy::print_movie(){
    cout << "---Movie Watcher---" << "\n";
    for (Planet &planet : planets){
        if (planet.ambush.state == Planet::Movie::movieState::Battled 
            || planet.ambush.state == Planet::Movie::movieState::Maybe){
            cout << "A movie watcher would enjoy an ambush on planet "
                 << planet.id << " with Sith at time "
                 << planet.ambush.sith_timestamp << " and Jedi at time "
                 << planet.ambush.jedi_timestamp
                 << " with a force difference of "
                 << planet.ambush.sith_force - planet.ambush.jedi_force
                 << "." << "\n";
        } else {
            cout << "A movie watcher would not see an interesting ambush "
                 << "on planet " << planet.id << "." << "\n";
        }

        if (planet.attack.state == Planet::Movie::movieState::Battled 
            || planet.attack.state == Planet::Movie::movieState::Maybe){
            cout << "A movie watcher would enjoy an attack on planet "
                 << planet.id << " with Jedi at time "
                 << planet.attack.jedi_timestamp << " and Sith at time "
                 << planet.attack.sith_timestamp
                 << " with a force difference of "
                 << planet.attack.sith_force - planet.attack.jedi_force
                 << "." << "\n";
        } else {
            cout << "A movie watcher would not see an interesting attack "
                 << "on planet " << planet.id << "." << "\n";
        }
    }
}