/** \file stationaryQubit.cc
 *  \authors cldurand,takaakimatsuo
 *  \date 2018/03/14
 *
 *  \brief stationaryQubit
 */
#include "stationaryQubit.h"
#include <vector>
#include <omnetpp.h>
#include <classical_messages_m.h>
#include <unsupported/Eigen/MatrixFunctions>
#include <unsupported/Eigen/KroneckerProduct>
#include <PhotonicQubit_m.h>
#include<bitset>

namespace quisp {
namespace modules {


Define_Module(stationaryQubit);

/**
 * \brief Initialize stationaryQubit
 *
 * Omnet called method to initialize objects.
 *
 */
void stationaryQubit::initialize()
{

    double rand = dblrand();

    /*Photon emission time error rates*/
    double Z_error_ratio = par("emission_Z_error_ratio");//par("name") will be read from .ini or .ned file
    double X_error_ratio = par("emission_X_error_ratio");
    double Y_error_ratio = par("emission_Y_error_ratio");
    double ratio_sum = Z_error_ratio+X_error_ratio+Y_error_ratio;//Get the sum of x:y:z for normalization
    err.pauli_error_rate = par("emission_error_rate");//This is per photon emission.
    err.X_error_rate = err.pauli_error_rate * (X_error_ratio/ratio_sum);
    err.Y_error_rate = err.pauli_error_rate * (Y_error_ratio/ratio_sum);
    err.Z_error_rate = err.pauli_error_rate * (Z_error_ratio/ratio_sum);
    setErrorCeilings();

    /*Memory error rates*/

    double memory_Z_error_ratio = par("memory_Z_error_ratio");//par("name") will be read from .ini or .ned file
    double memory_X_error_ratio = par("memory_X_error_ratio");
    double memory_Y_error_ratio = par("memory_Y_error_ratio");
    double memory_excitation_error_ratio = par("memory_energy_excitation_ratio");
    double memory_relaxation_error_ratio = par("memory_energy_relaxation_ratio");
    double memory_complitely_mixed_ratio = par("memory_complitely_mixed_ratio");

    if(memory_Z_error_ratio == 0 && memory_X_error_ratio == 0 && memory_Y_error_ratio == 0 && memory_excitation_error_ratio == 0 && memory_relaxation_error_ratio == 0 && memory_complitely_mixed_ratio ==0){
        memory_Z_error_ratio = 1;
        memory_X_error_ratio = 1;
        memory_Y_error_ratio = 1;
        memory_excitation_error_ratio = 1;
        memory_relaxation_error_ratio = 1;
        memory_complitely_mixed_ratio = 1;
    }

    //EV<<"memory_excitation_error_ratio = "<<memory_excitation_error_ratio<<", memory_relaxation_error_ratio"<<memory_relaxation_error_ratio<<"\n";
    double memory_ratio_sum = memory_Z_error_ratio+memory_X_error_ratio+memory_Y_error_ratio + memory_excitation_error_ratio + memory_relaxation_error_ratio + memory_complitely_mixed_ratio;
    memory_err.pauli_error_rate = par("memory_error_rate");//This is per μs.
    memory_err.X_error_rate = memory_err.pauli_error_rate * (memory_X_error_ratio/memory_ratio_sum);
    memory_err.Y_error_rate = memory_err.pauli_error_rate * (memory_Y_error_ratio/memory_ratio_sum);
    memory_err.Z_error_rate = memory_err.pauli_error_rate * (memory_Z_error_ratio/memory_ratio_sum);
    memory_err.excitation_error_rate = memory_err.pauli_error_rate * (memory_excitation_error_ratio/memory_ratio_sum);
    memory_err.relaxation_error_rate = memory_err.pauli_error_rate * (memory_relaxation_error_ratio/memory_ratio_sum);
    memory_err.complitely_mixed_rate = memory_err.pauli_error_rate * (memory_complitely_mixed_ratio/memory_ratio_sum);

    /*EV<<"Err rate = "<<memory_err.pauli_error_rate<<"\n";
    EV<<"Ratio sum = "<<memory_ratio_sum<<"\n";
    EV<<"I error rate (mem) = "<<1-memory_err.pauli_error_rate<<"\n";
    EV<<"X error rate (mem) = "<<memory_err.X_error_rate<<"\n";
    EV<<"Y error rate (mem) = "<<memory_err.Y_error_rate<<"\n";
    EV<<"Z error rate (mem) = "<<memory_err.Z_error_rate<<"\n";
    EV<<"Excitation error rate (mem) = "<<memory_err.excitation_error_rate<<"\n";
    EV<<"Relaxation error rate (mem) = "<<memory_err.relaxation_error_rate<<"\n";*/
    Memory_Transition_matrix = MatrixXd::Zero(7,7);
    Memory_Transition_matrix << 1-memory_err.pauli_error_rate, memory_err.X_error_rate,memory_err.Z_error_rate, memory_err.Y_error_rate, memory_err.excitation_error_rate, memory_err.relaxation_error_rate,memory_err.complitely_mixed_rate,
               memory_err.X_error_rate, 1-memory_err.pauli_error_rate, memory_err.Y_error_rate,memory_err.Z_error_rate,memory_err.excitation_error_rate,memory_err.relaxation_error_rate,memory_err.complitely_mixed_rate,
               memory_err.Z_error_rate,memory_err.Y_error_rate, 1-memory_err.pauli_error_rate,memory_err.X_error_rate,memory_err.excitation_error_rate,memory_err.relaxation_error_rate,memory_err.complitely_mixed_rate,
               memory_err.Y_error_rate,memory_err.Z_error_rate, memory_err.X_error_rate, 1-memory_err.pauli_error_rate,memory_err.excitation_error_rate,memory_err.relaxation_error_rate,memory_err.complitely_mixed_rate,
               0,0,0,0, 1-memory_err.relaxation_error_rate-memory_err.complitely_mixed_rate, memory_err.relaxation_error_rate,memory_err.complitely_mixed_rate,
               0,0,0,0,memory_err.excitation_error_rate,1-memory_err.excitation_error_rate-memory_err.complitely_mixed_rate,memory_err.complitely_mixed_rate,
               0,0,0,0,memory_err.excitation_error_rate,memory_err.relaxation_error_rate,1-memory_err.excitation_error_rate-memory_err.relaxation_error_rate;
    std::cout<<"Memory_Transition_matrix = \n "<< Memory_Transition_matrix<<" done \n";


    //std::cout<<Memory_Transition_matrix<<"\n";
    //endSimulation();

    //Set error matrices. This is used in the process of simulating tomography.
    Pauli.X << 0,1,1,0;
    Pauli.Y << 0,Complex(0,-1),Complex(0,1),0;
    Pauli.Z << 1,0,0,-1;
    Pauli.I << 1,0,0,1;

    //Set measurement operators. This is used in the process of simulating tomography.

    meas_op.X_basis.plus << 0.5,0.5,0.5,0.5;
    meas_op.X_basis.minus << 0.5,-0.5,-0.5,0.5;
    meas_op.X_basis.plus_ket << 1/sqrt(2), 1/sqrt(2);
    meas_op.X_basis.minus_ket << 1/sqrt(2), -1/sqrt(2);
    meas_op.X_basis.basis = 'X';
    meas_op.Z_basis.plus << 1,0,0,0;
    meas_op.Z_basis.minus << 0,0,0,1;
    meas_op.Z_basis.plus_ket << 1, 0;
    meas_op.Z_basis.minus_ket << 0, 1;
    meas_op.Z_basis.basis = 'Z';
    meas_op.Y_basis.plus << 0.5,Complex(0,-0.5),Complex(0,0.5),0.5;
    meas_op.Y_basis.minus << 0.5,Complex(0,0.5),Complex(0,-0.5),0.5;
    meas_op.Y_basis.plus_ket << 1/sqrt(2), Complex(0,1/sqrt(2));
    meas_op.Y_basis.minus_ket << 1/sqrt(2), -Complex(0,1/sqrt(2));
    meas_op.Y_basis.basis = 'Y';
    meas_op.identity << 1,0,0,1;

    // Get parameters from omnet
    stationaryQubit_address = par("stationaryQubit_address");
    node_address = par("node_address");
    qnic_address = par("qnic_address");
    qnic_type = par("qnic_type");
    qnic_index = par("qnic_index");
    std = par("std");
    setFree(false);
    setFidelity(-1.);

    DEBUG_memory_X_count = 0;
    DEBUG_memory_Y_count = 0;
    DEBUG_memory_Z_count = 0;

    /* e^(t/T1) energy relaxation, e^(t/T2) phase relaxation. Want to use only 1/10 of T1 and T2 in general.*/
}


void stationaryQubit::setErrorCeilings(){
    No_error_ceil =1-err.pauli_error_rate;/* if 0 <= dblrand < fidelity = No error*/
    X_error_ceil = No_error_ceil + err.X_error_rate; /* if fidelity <= dblrand < fidelity+X error rate = X error*/
    Z_error_ceil = X_error_ceil + err.Z_error_rate;
    Y_error_ceil = 1;
}



/**
 * \brief cSimpleModule handleMessage function
 *
 * \param msg is the message
 */
void stationaryQubit::handleMessage(cMessage *msg){
    bubble("Got a photon!!");
    setBusy();
    setEmissionPauliError();
    send(msg, "tolens_quantum_port$o");
}

void stationaryQubit::setEmissionPauliError(){
    if(par("GOD_Xerror") || par("GOD_Zerror")){
        //std::cout<<"node["<<node_address<<"] qnic["<<qnic_address<<"] Emitting from "<<this<<"X="<<par("GOD_Xerror").str()<<"Z="<<par("GOD_Zerror").str()<<"\n";
        error("There shouldn't be an error existing before photon emission. This error may have not been reinitialized since last use. Better check!");
    }
    double rand = dblrand();//Gives a random double between 0.0 ~ 1.0
    if(rand < No_error_ceil){
               //Qubit will end up with no error
               //EV<<"No error :"<<rand<<" < "<<No_error_ceil<<"\n";
    }else if(No_error_ceil <= rand && rand < X_error_ceil && (No_error_ceil!=X_error_ceil)){
               //X error
                par("GOD_Xerror") = true;
                //EV<<"Xerror :"<<No_error_ceil<<"<="<<rand<<" < "<<X_error_ceil<<"\n";
    }else if(X_error_ceil <= rand && rand < Z_error_ceil && (X_error_ceil!=Z_error_ceil)){
               //Z error
                par("GOD_Zerror") = true;
               // EV<<"Zerror :"<<X_error_ceil<<"<="<<rand<<" < "<<Z_error_ceil<<"\n";
    }else if(Z_error_ceil <= rand && rand < Y_error_ceil && (Z_error_ceil!=Y_error_ceil)){
               //Y error
                par("GOD_Xerror") = true;
                par("GOD_Zerror") = true;
                //EV<<"Yerror :"<<Z_error_ceil<<"<="<<rand<<" < "<<Y_error_ceil<<"\n";
   }else{
       error("Either the error ceilings or the random double generator is wrong.");
   }
}

bool stationaryQubit::measure_X(){
    //Need to add noise here later

    return !par("GOD_Zerror");
}

/**
 *  Returns true if the measurement outcome was correct
 */
bool stationaryQubit::measure_Y(){
    //Need to add noise here later

    return !(par("GOD_Zerror") || par("GOD_Xerror"));
}

bool stationaryQubit::measure_Z(){
    //Need to add noise here later

    return !par("GOD_Xerror");
}

//Convert X to Z, and Z to X error. Therefore, Y error stays as Y.
void stationaryQubit::Hadamard_gate(){
    //Need to add noise here later

    bool z = par("GOD_Zerror");
    par("GOD_Zerror") = par("GOD_Xerror");
    par("GOD_Xerror") = z;
}

void stationaryQubit::Z_gate(){
    //Need to add noise here later
    par("GOD_Zerror") = !par("GOD_Zerror");
}

void stationaryQubit::X_gate(){
    //Need to add noise here later
    par("GOD_Xerror") = !par("GOD_Xerror");
}

void stationaryQubit::CNOT_gate(stationaryQubit *control_qubit){
    //Need to add noise here later

    if(control_qubit->par("GOD_Xerror")){
        par("GOD_Xerror") = !par("GOD_Xerror");//X error propagates from control to target. If an X error is already present, then it cancels out.
    }

    if(par("GOD_Zerror")){
        control_qubit->par("GOD_Zerror") = ! control_qubit->par("GOD_Zerror");//Z error propagates from target to control. If an Z error is already present, then it cancels out.
    }
}

//This is invoked whenever a photon is emitted out from this particular qubit.
void stationaryQubit::setBusy(){
    isBusy = true;
    emitted_time = simTime();
    updated_time = simTime();//Should be no error at this time.
    par("photon_emitted_at") = emitted_time.dbl();
    par("last_updated_at") = updated_time.dbl();
    par("isBusy") = true;
    // GUI part
    if(hasGUI()){
        getDisplayString().setTagArg("i", 1, "red");
    }
}

//Re-initialization of this stationary qubit
//This is called at the beginning of the simulation (in initialization() above), and whenever it is reinitialized via the RealTimeController.
void stationaryQubit::setFree(bool consumed){
    num_purified = 0;
    locked = false;
    locked_ruleset_id = -1;
    locked_rule_id = -1;
    action_index = -1;
    isBusy = false;
    allocated = false;
    emitted_time = -1;
    updated_time = -1;
    /**/
    partner_measured = false;
    completely_mixed = false;
    excited_or_relaxed = false;
    GOD_dm_Zerror = false;
    GOD_dm_Xerror = false;
    Density_Matrix_Collapsed << -111,-111,-111,-111;
    par("photon_emitted_at") = emitted_time.dbl();
    par("last_updated_at") = updated_time.dbl();
    par("GOD_Xerror") = false;
    par("GOD_Zerror") = false;
    par("GOD_CMerror") = false;
    par("GOD_EXerror") = false;
    par("GOD_REerror") = false;
    par("GOD_CMerror") = false;
    par("isBusy") = false;
    par("GOD_entangled_stationaryQubit_address") = -1;
    par("GOD_entangled_node_address") = -1;
    par("GOD_entangled_qnic_address") = -1;
    par("GOD_entangled_qnic_type") = -1;
    entangled_partner = nullptr;
    EV<<"!!!!!!!!!!!!!! Freeing this qubit!!!"<<this<<"\n";
    // GUI part
    if(hasGUI()){
        if(consumed){
            bubble("Consumed!");
            getDisplayString().setTagArg("i", 1, "yellow");
        }
        else{
            bubble("Failed to entangle!");
            getDisplayString().setTagArg("i", 1, "blue");
        }

    }
}


bool stationaryQubit::checkBusy(){
    Enter_Method("checkBusy()");
    return isBusy;
}

/*To avoid disturbing this qubit.*/
void stationaryQubit::Lock(int ruleset_id, int rule_id, int action_id){
    locked = true;
    locked_ruleset_id = ruleset_id;//Used to identify what this qubit is locked for.
    locked_rule_id = rule_id;
    action_index = action_id;

    if(hasGUI()){
        bubble("Locked!");
        getDisplayString().setTagArg("i", 1, "purple");
    }
}

void stationaryQubit::Unlock(){
    locked = false;
    locked_ruleset_id = -1;//Used to identify what this qubit is locked for.
    locked_rule_id = -1;
    action_index = -1;

    if(hasGUI()){
            bubble("Unlocked!");
            getDisplayString().setTagArg("i", 1, "pink");
    }
}

bool stationaryQubit::isLocked(){
    return locked;
}


void stationaryQubit::Allocate(){
    allocated = true;
    if(hasGUI()){
                bubble("Allocated!");
                getDisplayString().setTagArg("i", 1, "purple");
    }
}

void stationaryQubit::Deallocate(){
    allocated = false;
}

bool stationaryQubit::isAllocated(){
    return allocated;
}


/**
 * \brief Generate photon entangled with the memory
 * \warning Shouldn't we destroy a possibly existing photon object before? <- No, I dont think so...
 */
PhotonicQubit *stationaryQubit::generateEntangledPhoton(){
    Enter_Method("generateEntangledPhoton()");
    photon = new PhotonicQubit("Photon");

    //To simulate the actual physical entangled partner, not what the system thinks!!! we need this.
    photon->setNodeEntangledWith(node_address);//This photon is entangled with.... node_address = node's index
    photon->setQNICEntangledWith(qnic_address);//qnic_address != qnic_index. qnic_index is not unique because there are 3 types.
    photon->setStationaryQubitEntangledWith(stationaryQubit_address);//stationaryQubit_address = stationaryQubit's index
    photon->setQNICtypeEntangledWith(qnic_type);
    photon->setEntangled_with(this);
    return photon;
}

/**
 * \brief Emit photon
 *
 * \param pulse: 0 for nothing, 1 for first, 2 for last, 3 for first and last
 *
 * The stationary qubit shouldn't be already busy.
 */
void stationaryQubit::emitPhoton(int pulse)
{
    Enter_Method("emitPhoton()");
    if (checkBusy()) {
        error("Requested a photon emission to a busy qubit... this should not happen!");
        return;
    }
    PhotonicQubit *pk = generateEntangledPhoton();
    if (pulse & STATIONARYQUBIT_PULSE_BEGIN) pk->setFirst(true);
    if (pulse & STATIONARYQUBIT_PULSE_END) pk->setLast(true);
    if (pulse & STATIONARYQUBIT_PULSE_BOUND) pk->setKind(3);
    float jitter_timing = normal(0,std);
    float abso = fabs(jitter_timing);
    scheduleAt(simTime()+abso,pk); //cannot send back in time, so only positive lag
}

//This gets direcltly invoked when darkcount happened in BellStateAnalyzer.cc.
void stationaryQubit::setCompletelyMixedDensityMatrix(){
    Density_Matrix_Collapsed<<(double)1/(double)2,0,0,(double)1/(double)2;
    completely_mixed = true;
    excited_or_relaxed = false;
    entangled_partner = nullptr;
    par("GOD_CMerror")=true;
    par("GOD_EXerror")=false;
    par("GOD_REerror")=false;
    par("GOD_Xerror")=false;
    par("GOD_Zerror")=false;
    GOD_dm_Xerror = false;
    GOD_dm_Zerror = false;
    if(hasGUI()){
        bubble("Completely mixed. darkcount");
        getDisplayString().setTagArg("i", 1, "white");
    }
    /*if(this->entangled_partner!=nullptr){//If it used to be entangled...
        if(entangled_partner->entangled_partner == nullptr){
            error("Mistracking entanglement somewhere.");
        }
               entangled_partner->setCompletelyMixedDensityMatrix();
               entangled_partner = nullptr;
               //entangled_partner->entangled_partner = nullptr;
    }*/
    //error("Heh?");
}

//This gets invoked in memory error simulation or by BellStateAnalyzer if photonLoss + darkcount.
void stationaryQubit::setExcitedDensityMatrix(){
    Density_Matrix_Collapsed<<1,0,0,0;
    completely_mixed = false;
    excited_or_relaxed = true;

    par("GOD_EXerror")=true;
    par("GOD_REerror")=false;
    par("GOD_CMerror")=false;
    par("GOD_Xerror")=false;
    par("GOD_Zerror")=false;
    GOD_dm_Xerror = false;
    GOD_dm_Zerror = false;

    if(hasGUI()){
        bubble("Completely mixed. darkcount");
        getDisplayString().setTagArg("i", 1, "white");
    }

    if(this->entangled_partner!=nullptr){//If it used to be entangled...
            /*if(entangled_partner->entangled_partner == nullptr){//This actually could happen when doing purification
                error("setExcitedDensityMatrix(). Mistracking entanglement somewhere.");
            }*/
            entangled_partner->setCompletelyMixedDensityMatrix();
            entangled_partner = nullptr;
            //entangled_partner->entangled_partner = nullptr;
    }//else it is already not entangled. e.g. excited -> relaxed.
}

void stationaryQubit::setRelaxedDensityMatrix(){

    Density_Matrix_Collapsed<<0,0,0,1;
    completely_mixed = false;
    excited_or_relaxed = true;
    par("GOD_EXerror")=false;
    par("GOD_REerror")=true;
    par("GOD_CMerror")=false;
    par("GOD_Xerror")=false;
    par("GOD_Zerror")=false;
    GOD_dm_Xerror = false;
    GOD_dm_Zerror = false;
    if(hasGUI()){
        bubble("Completely mixed. darkcount");
        getDisplayString().setTagArg("i", 1, "white");
    }

    if(this->entangled_partner!=nullptr){//Still entangled
        /*if(entangled_partner->entangled_partner == nullptr){//This actually could happen when doing purification
            error("setRelaxedDensityMatrix(). Mistracking entanglement somewhere.");
        }*/
        entangled_partner->setCompletelyMixedDensityMatrix();
        entangled_partner = nullptr;
        //entangled_partner->entangled_partner = nullptr;
    }//else it is already not entangled. e.g. excited -> relaxed.
}

void stationaryQubit::setEntangledPartnerInfo(stationaryQubit *partner){
    //When BSA succeeds, this method gets invoked to store entangled partner information. This will also be sent classically to the partner node afterwards.
    entangled_partner = partner;

    par("GOD_entangled_stationaryQubit_address") = partner->par("stationaryQubit_address");
    par("GOD_entangled_node_address") = partner->par("node_address");
    par("GOD_entangled_qnic_address") = partner->par("qnic_address");
    par("GOD_entangled_qnic_type") = partner->par("qnic_type");
}


/*Add another X error. If an X error already exists, then they cancel out*/
void stationaryQubit::addXerror(){
    //error("Huh...?");
    bool Xerr = this->par("GOD_Xerror");
    this->par("GOD_Xerror") = !Xerr;/*Switches true to false or false to true*/
    //this->par("GOD_Xerror") = true;
}

/*Add another Z error. If an Z error already exists, then they cancel out*/
void stationaryQubit::addZerror(){
    bool Zerr = this->par("GOD_Zerror");
    this->par("GOD_Zerror") = !Zerr;/*Switches true to false or false to true*/
    //this->par("GOD_Zerror") = true;
}

// Only tracks error propagation. If two booleans (Alice and Bob) agree (truetrue or falsefalse), keep the purified ebit.
bool stationaryQubit::purify(stationaryQubit * resource_qubit/*Controlled*/) {
    apply_memory_error(this);
    apply_memory_error(resource_qubit);
    this->CNOT_gate(resource_qubit/*controlled qubit*/);
    bool meas = this->measure_Z();
    return meas;
}



//Single qubit memory error based on Markov-Chain
void stationaryQubit::apply_memory_error(stationaryQubit *qubit){
    //std::cout<<"memory_err = "<<memory_err.pauli_error_rate<<"\n";
    //Check when the error got updated last time. Errors will be performed depending on the difference between that time and the current time.
    if(qubit->memory_err.pauli_error_rate==0){//If no memory error occurs, or if the state is completely mixed, skip this memory error simulation.
        //error("memory error is set to 0. If on purpose, that is fine. Comment this out.");
        return;
    }
    if(qubit->completely_mixed){
        return;
    }


    double time_evolution = simTime().dbl() - qubit->updated_time.dbl();
    double time_evolution_microsec  = time_evolution * 1000000 /** 100*/;

    //std::cout<<"time_evolution"<<time_evolution<<", time_evolution_microsec"<<time_evolution_microsec<<"\n";
    //        endSimulation();

    if(time_evolution_microsec > 0){
        //EV<<"\n Memory error applied for time = "<<time_evolution_microsec<<" μs, on qubit "<<qubit<<"in node["<<qubit->par("node_address").str()<<"] \n";
        //Perform Monte-Carlo error simulation on this qubit.
        //std::cout<<"time_evolution_microsec "<<time_evolution_microsec<<"\n";
        bool Xerr = qubit->par("GOD_Xerror");
        bool Zerr = qubit->par("GOD_Zerror");
        bool EXerr = qubit->par("GOD_EXerror");
        bool REerr = qubit->par("GOD_REerror");
        bool CMerr = qubit->par("GOD_CMerror");
        //std::cout<<"\n\n\n\n First it was "<<Xerr<<","<<Zerr<<"\n";

        MatrixXd Initial_condition(1,7);//I, X, Z, Y, Ex, Re, Cm
        if(Zerr && Xerr){
            Initial_condition << 0,0,0,1,0,0,0;//Has a Y error
            //std::cout<<"node["<<this->node_address<<"], qubit["<<this->stationaryQubit_address<<"] time_evolution"<<time_evolution<<", time_evolution_microsec"<<time_evolution_microsec<<"\n";
            //error("err Y");
        }else if(Zerr && !Xerr){
            Initial_condition << 0,0,1,0,0,0,0;//Has a Z error
            //std::cout<<"node["<<this->node_address<<"], qubit["<<this->stationaryQubit_address<<"] time_evolution"<<time_evolution<<", time_evolution_microsec"<<time_evolution_microsec<<"\n";
            //error("err Z");
        }else if(!Zerr && Xerr){
            Initial_condition << 0,1,0,0,0,0,0;//Has an X error
            //std::cout<<"node["<<this->node_address<<"], qubit["<<this->stationaryQubit_address<<"] time_evolution"<<time_evolution<<", time_evolution_microsec"<<time_evolution_microsec<<"\n";
            //error("err X");
        }else if(EXerr){
            Initial_condition << 0,0,0,0,1,0,0;//Has an excitation error
            //error("err EX");
        }else if(REerr){
            Initial_condition << 0,0,0,0,0,1,0;//Has an relaxation error
            //error("err RE");
        }else if(CMerr){
            Initial_condition << 0,0,0,0,0,0,1;//Has an relaxation e
        }else{
            Initial_condition << 1,0,0,0,0,0,0;//No error
        }

        bool skip_exponentiation = false;
        for(int i = 0; i<Memory_Transition_matrix.cols(); i++){
            //std::cout<<"Memory_Transition_matrix(0,i) = "<<Memory_Transition_matrix(0,i)<<"\n";
            if(Memory_Transition_matrix(0,i) == 1){
                skip_exponentiation = true; //Do not to the exponentiation! Eigen will mess up the exponentiation anyway...
                break;
            }
        }

        MatrixXd Dynamic_transition_matrix(7,7);
        Dynamic_transition_matrix << -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1,
                                     -1,-1,-1,-1,-1,-1,-1;

        if(!skip_exponentiation){
        //std::cout<<"Init Condition = "<<Initial_condition<<"\n";
        //MatrixPower<MatrixXd> Apow(qubit->Memory_Transition_matrix);
            MatrixPower<MatrixXd> Apow(Memory_Transition_matrix);
            Dynamic_transition_matrix = Apow(time_evolution_microsec);
        }
        else{
            Dynamic_transition_matrix = Memory_Transition_matrix;
        }
        //Dynamic_transition_matrix = Memory_Transition_matrix.pow(time_evolution_microsec);

        //std::cout<<"Memory_Transition_matrix"<<qubit->Memory_Transition_matrix<<"\n";
        //std::cout<<"Memory_Transition_matrix^"<<time_evolution_microsec<<" = "<<Dynamic_transition_matrix ;

        for(int r = 0; r<Dynamic_transition_matrix.rows(); r++){
            double col_sum = 0;
            for(int i = 0; i<Dynamic_transition_matrix.cols(); i++){
               //std::cout<<"Dynamic_transition_matrix(0,i) = "<<Dynamic_transition_matrix(0,i)<<"\n";
               col_sum += Dynamic_transition_matrix(r,i);
            }
            if(col_sum > 1.01 || col_sum < 0.99){
                //std::cout<<"col_sum = "<<col_sum<<"\n";
                error("Eh....");
            }
        }

        if( std::isnan(Dynamic_transition_matrix(0,0))){
            //std::cout<<"!!!!!Check out this\n";
            //std::cout<<qubit->Memory_Transition_matrix.pow(time_evolution_microsec);
            error("Transition maatrix is NaN. This is Eigen's fault.");
        }



        //if(node_address == 3)
         //   std::cout<<"node["<<node_address<<"] TM^"<<time_evolution_microsec<<" = "<< Dynamic_transition_matrix<<"\n";
        //if(node_address == 3){
        //    error("eh...");
        //}
        //endSimulation();

        MatrixXd Output_condition(1,6);//I, X, Z, Y

        Output_condition = Initial_condition*Dynamic_transition_matrix;//I,X,Y,Z
        //std::cout<<"Output Condition = "<<Output_condition<<"\n";
        //std::cout<<"\n Input (I,X,Z,Y) was "<<Initial_condition<<"\n Output (I,X,Z,Y) is now"<<Output_condition<<"\n";
        double No_error_ceil = Output_condition(0,0);
        double X_error_ceil = No_error_ceil+Output_condition(0,1);
        double Z_error_ceil = X_error_ceil+Output_condition(0,2);
        double Y_error_ceil = Z_error_ceil+Output_condition(0,3);
        double EX_error_ceil = Y_error_ceil+Output_condition(0,4);
        double RE_error_ceil = EX_error_ceil+Output_condition(0,5);
        double rand = dblrand();//Gives a random double between 0.0 ~ 1.0

        //std::cout<<"dbl = "<<rand<<" No ceil = "<<No_error_ceil<<", "<<X_error_ceil<<", "<<Z_error_ceil<<","<<Y_error_ceil<<", "<<EX_error_ceil<<", "<<1<<"\n";
        if(rand < No_error_ceil){
            //std::cout<<"NO err\n";
            //Qubit will end up with no error
            qubit->par("GOD_Xerror") = false;
            qubit->par("GOD_Zerror") = false;

        }else if(No_error_ceil <= rand && rand < X_error_ceil && (No_error_ceil!=X_error_ceil)){
            //X error
            //std::cout<<"X err\n";
            qubit->par("GOD_Xerror") = true;
            qubit->par("GOD_Zerror") = false;
            DEBUG_memory_X_count++;


        }else if(X_error_ceil <= rand && rand < Z_error_ceil && (X_error_ceil!=Z_error_ceil)){
            //Z error
            //std::cout<<"Z err\n";
            qubit->par("GOD_Xerror") = false;
            qubit->par("GOD_Zerror") = true;
            DEBUG_memory_Z_count++;

        }else if (Z_error_ceil <= rand && rand < Y_error_ceil && (Z_error_ceil!=Y_error_ceil)){
            //Y error
            //std::cout<<"Y err\n";
            qubit->par("GOD_Xerror") = true;
            qubit->par("GOD_Zerror") = true;
            DEBUG_memory_Y_count++;

         }else if(Y_error_ceil <= rand && rand < EX_error_ceil && (Y_error_ceil!=EX_error_ceil)){
             //Excitation error
             //std::cout<<"Ex err\n";
             qubit->setExcitedDensityMatrix();//Also sets the partner completely mixed if it used to be entangled.
             //error("not implemented");

         }else if(EX_error_ceil <= rand && rand < RE_error_ceil && (EX_error_ceil!=RE_error_ceil)){
             //Excitation error
             //std::cout<<"Ex err\n";
             qubit->setRelaxedDensityMatrix();//Also sets the partner completely mixed if it used to be entangled.
             //error("not implemented");

         }else{
             //Memory completely mixed error
             //qubit->setRelaxedDensityMatrix();
             if(qubit->entangled_partner!=nullptr){
                 qubit->entangled_partner->setCompletelyMixedDensityMatrix();
             }
             qubit->setCompletelyMixedDensityMatrix();
         }

    }

    qubit->updated_time = simTime();//Update parameter, updated_time, to now.
    qubit->par("last_updated_at") = simTime().dbl();//For GUI
}


/*
void stationaryQubit::apply_memory_error(stationaryQubit *qubit){

    if(memory_err.pauli_error_rate==0){//If no memory error occurs, or if the state is completely mixed, skip this memory error simulation.
        error("はぁ？");
        return;
    }
    if(completely_mixed_OR_excited_OR_relaxed){
            error("Not fully implemented yet");
    }

    double time_evolution = simTime().dbl() - qubit->updated_time.dbl();
    double time_evolution_microsec  = time_evolution * 1000000;



    qubit->par("GOD_Xerror") = true;
    qubit->par("GOD_Zerror") = false;
    qubit->updated_time = simTime();//Update parameter, updated_time, to now.
    qubit->par("last_updated_at") = simTime().dbl();//For GUI
}*/



Matrix2cd stationaryQubit::getErrorMatrix(stationaryQubit *qubit){
    Matrix2cd error;
    if(qubit->par("GOD_Zerror") && qubit->par("GOD_Xerror")){//Y error on this qubit
            error = Pauli.Y;
            EV<<"Y error"<<"\n";
    }
    else if(qubit->par("GOD_Zerror") && !qubit->par("GOD_Xerror")){//Z error
            error = Pauli.Z;
            EV<<"Z error"<<"\n";
    }
    else if(!qubit->par("GOD_Zerror") && qubit->par("GOD_Xerror")){//X error
            error = Pauli.X;
            EV<<"X error"<<"\n";
    }
    else{
            error = Pauli.I;
            EV<<"I error"<<"\n";
    }
    return error;

    // Matrix4cd test = kroneckerProduct(Pauli.I,Pauli.Y).eval();
}

//returns the density matrix of the Bell pair with error. This assumes that this is entangled with another stationary qubit.
//Measurement output will be based on this matrix, as long as it is still entnagled.
quantum_state stationaryQubit::getQuantumState(){
    if(this->excited_or_relaxed || entangled_partner->excited_or_relaxed){
        error("Wrong");
    }


    //If Pauli errors
    Vector4cd ideal_Bell_state(1/sqrt(2),0,0,1/sqrt(2));//Assumes that the state is a 2 qubit state |00> + |11>

    Matrix4cd combined_errors = kroneckerProduct(getErrorMatrix(this),getErrorMatrix(entangled_partner)).eval();

    //EV<<"Combined errors  = "<<combined_errors<<"\n";
    Vector4cd actual_Bell_state = combined_errors*ideal_Bell_state;
    //EV<<"Current physical state is = "<<actual_Bell_state;
    Matrix4cd density_matrix = actual_Bell_state*actual_Bell_state.adjoint();
    //EV<<"dm = "<<density_matrix<<"\n";

    quantum_state q;
    q.state_in_density_matrix = density_matrix;
    q.state_in_ket = actual_Bell_state;
    return q;
}


measurement_outcome stationaryQubit::measure_density_independent(){
    if(this->entangled_partner == nullptr && this->Density_Matrix_Collapsed(0,0).real() ==-111){
            EV<<entangled_partner<<"\n";
            EV<<Density_Matrix_Collapsed<<"\n";
            std::cout<<"Measuring"<<this<<"\n";
            error("Measuring a qubit that is not entangled with another qubit. Probably not what you want! Check whether address for each node is unique!!!");
    }
    measurement_operator this_measurement = Random_Measurement_Basis_Selection();//Select basis randomly
    char Output;
    char Output_is_plus;


    apply_memory_error(this);//Add memory error depending on the idle time. If excited/relaxed, this will immediately break entanglement, leaving the other qubit as completely mixed.
    if(this->entangled_partner != nullptr){//This becomes nullptr if this qubit got excited/relaxed or measured.
        if(this->partner_measured)
            error("Entangled partner not nullptr but partner already measured....? Probably wrong.");
        if(this->completely_mixed || this->excited_or_relaxed)
            error("Entangled but completely mixed / Excited / Relaxed ? Probably wrong.");
        apply_memory_error(entangled_partner);//Also do the same on the partner if it is still entangled! This could break the entanglement due to relaxation/excitation error!
    }

    if(this->partner_measured || this->completely_mixed || this->excited_or_relaxed){//The case when the density matrix is completely local to this qubit.

        if(this->completely_mixed && !this->par("GOD_CMerror")){
            error("Mismatch between flags.");
        }
        if(this->Density_Matrix_Collapsed(0,0).real() == -111){//We always need some kind of density matrix inside this if statement.
            error("Single qubit density matrix not stored properly after partner's measurement, excitation/relaxation error.");
        }
        bool Xerr = this->par("GOD_Xerror");
        bool Zerr = this->par("GOD_Zerror");
        //This qubit's density matrix was created when the partner measured his own.
        //Because this qubit can be measured after that, we need to update the stored density matrix according to new errors occurred due to memory error.

        if(Xerr != GOD_dm_Xerror){//Another X error to the dm.
           //error("NO");
           Density_Matrix_Collapsed = Pauli.X*Density_Matrix_Collapsed*Pauli.X.adjoint();
        }
        if (Zerr != GOD_dm_Zerror){//Another Z error to the dm.
           //error("NO!");
           Density_Matrix_Collapsed = Pauli.Z*Density_Matrix_Collapsed*Pauli.Z.adjoint();
        }

        //std::cout<<"Not entangled anymore. Density matrix is "<<Density_Matrix_Collapsed<<"\n";

        Complex Prob_plus = (Density_Matrix_Collapsed*this_measurement.plus.adjoint()*this_measurement.plus).trace();
        Complex Prob_minus = (Density_Matrix_Collapsed*this_measurement.minus.adjoint()*this_measurement.minus).trace();
        double dbl = dblrand();
        if(dbl < Prob_plus.real()){
            Output = '+';
            Output_is_plus = true;
         }else{
            Output = '-';
            Output_is_plus = false;
         }
         //std::cout<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). \n";
    }else if(!this->partner_measured && !this->completely_mixed && !this->excited_or_relaxed && this->entangled_partner!=nullptr){
        //error("Entangled....Should not happen here.");
        quantum_state current_state = getQuantumState();//This is assuming that this is some other qubit is entangled. Only Pauli errors are assumed.
        EV<<"Current entangled state is "<<current_state.state_in_ket<<"\n";

        bool Xerr = this->par("GOD_Xerror");
        bool Zerr = this->par("GOD_Zerror");
                //std::cout<<"Entangled state is "<<current_state.state_in_ket<<"\n";

        Complex Prob_plus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval()*current_state.state_in_ket;
        Complex Prob_minus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval()*current_state.state_in_ket;

        //std::cout<<"Measurement basis = "<<this_measurement.basis<<"P(+) = "<<Prob_plus.real()<<", P(-) = "<<Prob_minus.real()<<"\n";
        double dbl = dblrand();

        Vector2cd ms;
        if(dbl < Prob_plus.real()){//Measurement output was plus
            Output = '+';
            ms = this_measurement.plus_ket;
            Output_is_plus = true;
        }else{//Otherwise, it was negative.
            Output = '-';
            ms = this_measurement.minus_ket;
            Output_is_plus = false;
        }
        //Now we have to calculate the density matrix of a single qubit that used to be entangled with this.
        Matrix2cd partners_dm, normalized_partners_dm;
        partners_dm = kroneckerProduct(ms.adjoint(),meas_op.identity).eval()*current_state.state_in_density_matrix*kroneckerProduct(ms.adjoint(),meas_op.identity).eval().adjoint() ;
        normalized_partners_dm = partners_dm/partners_dm.trace();
        EV<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). Partner's dm is now = "<<normalized_partners_dm<<"\n";
        entangled_partner->Density_Matrix_Collapsed = normalized_partners_dm;
        entangled_partner->partner_measured = true;//We actually do not need this as long as deleting entangled_partner completely is totally fine.
        entangled_partner->entangled_partner = nullptr;//Break entanglement.
        //Save what error it had, when this density matrix was calculated. Error may get updated in the future, so we need to track what error has been considered already in the dm.
        entangled_partner->GOD_dm_Xerror = entangled_partner->par("GOD_Xerror");
        entangled_partner->GOD_dm_Zerror = entangled_partner->par("GOD_Zerror");
    }else{
        error("Check condition in measure func.");
    }
    measurement_outcome o;
    o.basis =this_measurement.basis;
    o.outcome_is_plus = Output_is_plus;
    return o;

    /*
    if(this->entangled_partner != nullptr && !this->completely_mixed && !this->excited_or_relaxed && !entangled_partner->completely_mixed && !entangled_partner->excited_or_relaxed){
        //error("NO");
        //todo: This is probably not right when entangled_partner is excited/relaxed
        quantum_state current_state = getQuantumState();//This is assuming that only Pauli errors will occur....
        EV<<"Current entangled state is "<<current_state.state_in_ket<<"\n";

        bool Xerr = this->par("GOD_Xerror");
        bool Zerr = this->par("GOD_Zerror");

        //std::cout<<"Entangled state is "<<current_state.state_in_ket<<"\n";

        Complex Prob_plus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval()*current_state.state_in_ket;
        Complex Prob_minus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval()*current_state.state_in_ket;

        //std::cout<<"Measurement basis = "<<this_measurement.basis<<"P(+) = "<<Prob_plus.real()<<", P(-) = "<<Prob_minus.real()<<"\n";
        double dbl = dblrand();

        Vector2cd ms;
        if(dbl < Prob_plus.real()){
            //Measurement output was plus
            Output = '+';
            ms = this_measurement.plus_ket;
            Output_is_plus = true;
        }else{//Otherwise, it was negative.
            Output = '-';
            ms = this_measurement.minus_ket;
            Output_is_plus = false;
        }

        //Now we have to calculate the density matrix of a single qubit that used to be entangled with this.
        Matrix2cd partners_dm, normalized_partners_dm;
        partners_dm = kroneckerProduct(ms.adjoint(),meas_op.identity).eval()*current_state.state_in_density_matrix*kroneckerProduct(ms.adjoint(),meas_op.identity).eval().adjoint() ;
        normalized_partners_dm = partners_dm/partners_dm.trace();
        EV<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). Partner's dm is now = "<<normalized_partners_dm<<"\n";
        entangled_partner->Density_Matrix_Collapsed = normalized_partners_dm;
        entangled_partner->partner_measured = true;
        //Save what error it had, when this density matrix was calculated. Error may get updated in the future, so we need to track what error has been considered already in the dm.
        entangled_partner->GOD_dm_Xerror = entangled_partner->par("GOD_Xerror");
        entangled_partner->GOD_dm_Zerror = entangled_partner->par("GOD_Zerror");
    }else{
                bool Xerr = this->par("GOD_Xerror");
                bool Zerr = this->par("GOD_Zerror");

                //This qubit's density matrix was created when the partner measured his own.
                //Because this qubit can be measured after that, we need to update the stored density matrix according to new errors occurred due to memory error.

                if(Xerr != GOD_dm_Xerror){
                    //Another X error to the dm.
                    //error("NO error for now!");
                    Density_Matrix_Collapsed = Pauli.X*Density_Matrix_Collapsed*Pauli.X.adjoint();
                }
                if (Zerr != GOD_dm_Zerror){
                    //Another Z error to the dm.
                    //error("NO error for now!");
                    Density_Matrix_Collapsed = Pauli.Z*Density_Matrix_Collapsed*Pauli.Z.adjoint();
                }

                //std::cout<<"Not entangled anymore. Density matrix is "<<Density_Matrix_Collapsed<<"\n";

                Complex Prob_plus = (Density_Matrix_Collapsed*this_measurement.plus.adjoint()*this_measurement.plus).trace();
                Complex Prob_minus = (Density_Matrix_Collapsed*this_measurement.minus.adjoint()*this_measurement.minus).trace();
                double dbl = dblrand();

                if(dbl < Prob_plus.real()){
                    Output = '+';
                    Output_is_plus = true;
                }else{
                    Output = '-';
                    Output_is_plus = false;
                }
                //std::cout<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). \n";

    }
    measurement_outcome o;
    o.basis =this_measurement.basis;
    o.outcome_is_plus = Output_is_plus;
    return o;*/
}


/*
measurement_outcome stationaryQubit::measure_density_independent(){
    if(entangled_partner == nullptr && Density_Matrix_Collapsed(0,0).real() ==-1){
            EV<<entangled_partner<<"\n";
            EV<<Density_Matrix_Collapsed<<"\n";
            error("Measuring a qubit that is not entangled with another qubit. Probably not what you want! Check whether address for each node is unique!!!");
    }
    measurement_operator this_measurement = Random_Measurement_Basis_Selection();//Select basis randomly
    char Output;
    char Output_is_plus;

    //apply_memory_error(this);//Before starting the calculation, update the error due to memory.
    //if(!(partner_measured || completely_mixed || excited_or_relaxed)){
    //    apply_memory_error(entangled_partner);//Also do the same on the partner!
    //}

    if(partner_measured || completely_mixed || excited_or_relaxed){
        //This qubit is not entangled anymore.
        //Its single qubit state will be stored in Density_Matrix_Collapsed.

        apply_memory_error(this);//Before starting the calculation, update the error due to memory.

        bool Xerr = this->par("GOD_Xerror");
        bool Zerr = this->par("GOD_Zerror");

        //This qubit's density matrix was created when the partner measured his own.
        //Because this qubit can be measured after that, we need to update the stored density matrix according to new errors occurred due to memory error.

        if(Xerr != GOD_dm_Xerror){
            //Another X error to the dm.
            //error("NO error for now!");
            Density_Matrix_Collapsed = Pauli.X*Density_Matrix_Collapsed*Pauli.X.adjoint();
        }
        if (Zerr != GOD_dm_Zerror){
            //Another Z error to the dm.
            //error("NO error for now!");
            Density_Matrix_Collapsed = Pauli.Z*Density_Matrix_Collapsed*Pauli.Z.adjoint();
        }

        //std::cout<<"Not entangled anymore. Density matrix is "<<Density_Matrix_Collapsed<<"\n";

        Complex Prob_plus = (Density_Matrix_Collapsed*this_measurement.plus.adjoint()*this_measurement.plus).trace();
        Complex Prob_minus = (Density_Matrix_Collapsed*this_measurement.minus.adjoint()*this_measurement.minus).trace();
        double dbl = dblrand();

        if(dbl < Prob_plus.real()){
            Output = '+';
            Output_is_plus = true;
        }else{
            Output = '-';
            Output_is_plus = false;
        }
        //std::cout<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). \n";

    }else{
        //Still entangled. After measurement, we need to update parameters, Density_Matrix_Collapsed and partner_measured, of the partner qubit.

        apply_memory_error(this);//Add memory error depending on the idle time.
        apply_memory_error(entangled_partner);//Also do the same on the partner!
        //todo: This is probably not right when entangled_partner is excited/relaxed
        quantum_state current_state = getQuantumState();//This is assuming that only Pauli errors will occur....
        EV<<"Current entangled state is "<<current_state.state_in_ket<<"\n";

        bool Xerr = this->par("GOD_Xerror");
        bool Zerr = this->par("GOD_Zerror");

        //std::cout<<"Entangled state is "<<current_state.state_in_ket<<"\n";

        Complex Prob_plus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.plus,meas_op.identity).eval()*current_state.state_in_ket;
        Complex Prob_minus = current_state.state_in_ket.adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval().adjoint()*kroneckerProduct(this_measurement.minus,meas_op.identity).eval()*current_state.state_in_ket;

        //std::cout<<"Measurement basis = "<<this_measurement.basis<<"P(+) = "<<Prob_plus.real()<<", P(-) = "<<Prob_minus.real()<<"\n";
        double dbl = dblrand();

        Vector2cd ms;
        if(dbl < Prob_plus.real()){
            //Measurement output was plus
            Output = '+';
            ms = this_measurement.plus_ket;
            Output_is_plus = true;
        }else{//Otherwise, it was negative.
            Output = '-';
            ms = this_measurement.minus_ket;
            Output_is_plus = false;
        }

        //Now we have to calculate the density matrix of a single qubit that used to be entangled with this.
        Matrix2cd partners_dm, normalized_partners_dm;
        partners_dm = kroneckerProduct(ms.adjoint(),meas_op.identity).eval()*current_state.state_in_density_matrix*kroneckerProduct(ms.adjoint(),meas_op.identity).eval().adjoint() ;
        normalized_partners_dm = partners_dm/partners_dm.trace();
        EV<<"\n This qubit was "<<this_measurement.basis<<"("<<Output<<"). Partner's dm is now = "<<normalized_partners_dm<<"\n";
        entangled_partner->Density_Matrix_Collapsed = normalized_partners_dm;
        entangled_partner->partner_measured = true;
        //Save what error it had, when this density matrix was calculated. Error may get updated in the future, so we need to track what error has been considered already in the dm.
        entangled_partner->GOD_dm_Xerror = entangled_partner->par("GOD_Xerror");
        entangled_partner->GOD_dm_Zerror = entangled_partner->par("GOD_Zerror");
    }
    measurement_outcome o;
    o.basis =this_measurement.basis;
    o.outcome_is_plus = Output_is_plus;
    return o;
}
*/

void stationaryQubit::finish(){
    /*std::cout<<"---Node["<<node_address<<"] qubit["<<stationaryQubit_address<<"]---\n";
    std::cout<<"DEBUG_memory_X_count == "<<DEBUG_memory_X_count++<<"\n";
    std::cout<<"DEBUG_memory_Y_count == "<<DEBUG_memory_Y_count++<<"\n";
    std::cout<<"DEBUG_memory_Z_count == "<<DEBUG_memory_Z_count++<<"\n";
    std::cout<<"-------------------\n";*/
}

measurement_operator stationaryQubit::Random_Measurement_Basis_Selection(){

    measurement_operator this_measurement;
    double dbl = dblrand();//Random double value for random basis selection.
    EV<<"Random dbl = "<<dbl<<"! \n ";

    if(dbl <( (double)1/(double)3)){//X measurement!
        EV<<"X measurement\n";
        this_measurement.plus = meas_op.X_basis.plus;
        this_measurement.minus = meas_op.X_basis.minus;
        this_measurement.basis = meas_op.X_basis.basis;
        this_measurement.plus_ket = meas_op.X_basis.plus_ket;
        this_measurement.minus_ket = meas_op.X_basis.minus_ket;
    }else if(dbl >= ( (double)1/(double)3) && dbl < ( (double)2/(double)3)){
        EV<<"Z measurement\n";
        this_measurement.plus = meas_op.Z_basis.plus;
        this_measurement.minus = meas_op.Z_basis.minus;
        this_measurement.basis = meas_op.Z_basis.basis;
        this_measurement.plus_ket = meas_op.Z_basis.plus_ket;
        this_measurement.minus_ket = meas_op.Z_basis.minus_ket;
    }else{
        EV<<"Y measurement\n";
        this_measurement.plus = meas_op.Y_basis.plus;
        this_measurement.minus = meas_op.Y_basis.minus;
        this_measurement.basis = meas_op.Y_basis.basis;
        this_measurement.plus_ket = meas_op.Y_basis.plus_ket;
        this_measurement.minus_ket = meas_op.Y_basis.minus_ket;
    }
    return this_measurement;
}


































//Assumes that this qubit is entangled with another one, as Bell pair 00+11. 1st qubit is self, 2nd is partner.
//When do we perform measurements? I think we can just ignore the success/fail of entanglement attempt, and measure it beforehand anyway. Waiting cause error.
//How do we know when to measure though?
//Return value: 1 if output is +, 0 if output is -.
/*std::bitset<1> stationaryQubit::measure_density(char basis_this_qubit){
    if(entangled_partner == nullptr){
        error("Measuring a qubit that is not entangled with another qubit. Not allowed!");
    }
    //if(entangled_partner->emitted_time=-1 && single_qubit_dm == null){
    //    error("Something is wrong.");
    }

    //todo Check if entangled state has collapsed already (partner qubit measured already).

    //if not, then measure it and get the output (density matrix required)!
    apply_memory_error(this);//Noise due to idle time in memory. This updates the par("GOD_Zerror") and par("GOD_Xerror") of this particular qubit.
    quantum_state current_state = getQuantumState();//Get the density matrix of the Bell pair that involves this particular qubit.
    measurement_output_probabilities p = getOutputProbabilities(current_state, basis_this_qubit);
    EV <<" P(++) = "<<p.probability_plus_plus<<", P(+-) = "<<p.probability_plus_minus<<", P(-+) = "<<p.probability_minus_plus<<", P(--) = "<<p.probability_minus_minus<<"\n";
    double rand = dblrand();//Gives a random double between 0.0 ~ 1.0

    std::bitset<2> output(0);//by default, output is -- (in binary 00)
    if(rand < p.probability_plus_plus){
        //Output is ++
        output.set(0);//00->01
        output.set(1);//01->11
        EV<<"Output is ++"<<output<<"\n";
    }else if(p.probability_plus_plus <= rand && rand < (p.probability_plus_plus+p.probability_plus_minus)){
        //Output is +-
        output.set(1);//00->10
        EV<<"Output is +-"<<output<<"\n";
    }else if((p.probability_plus_plus+p.probability_plus_minus) <= rand && rand < (p.probability_plus_plus+p.probability_plus_minus+p.probability_minus_plus)){
        //Output is -+
        output.set(0);//00->01
        EV<<"Output is -+"<<output<<"\n";
    }else{
        EV<<"Output is --"<<output<<"\n";
    }
    //return output.test(1);
}*/





} // namespace modules
} // namespace quisp
