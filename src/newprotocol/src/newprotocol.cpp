//
// Created by Vivek Sharma on 9/2/20.
//

#include <iostream>
#include "newprotocol.h"
#include <cassert>
#include "packedMod2.hpp"
#include "packedMod3.hpp"
#include "Toeplitz-by-x.hpp"
#include "OT.hpp"
#include "mains.hpp"
#include <typeinfo>//to determine the type of variables
#include "Timing.hpp"
#include <chrono>
#include "packed_PRF_central.h"
#include "PRF.hpp"

// in Toeplitz-by-x.hpp
// #define N_ROWS 256
// #define N_COLS 256

//using namespace std;


// Declaring the Keys and inputs of two parties (GLOBAL)


//Declaring shares of key and input masks of two parties
std::vector<uint64_t> rK1_global(toeplitzWords), rK2_global(toeplitzWords), rK_global(toeplitzWords);
PackedZ2<N_COLS> rx1_global, rx2_global, rx_global; //mask for input
PackedZ2<N_COLS> sw1_global, sw2_global, sw_global; //sw = rK * rx + rw
PackedZ2<N_COLS> rw1_global, rw2_global, rw_global; //random

//declare a variable in Z3 called r0z and r1z
PackedZ3<N_SIZE>r0z_global, r1z_global;
PackedZ3<N_SIZE> r0z1_global, r0z2_global, r1z1_global, r1z2_global;


/*
 * Preprocessing: Generates rx(and its shares rx1, rx2), rK(its shares rK1, rK2),
 * sw( and its shares sw1, sw2)
 */
void preProc_mod2_dm2020(unsigned int nTimes)
{
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/preProc_mod2_dm_2020: Preprocessing begins"<<std::endl;
#endif

    //1.generate mod2 random values for rK, rx and sw
    for(unsigned int i=0; i<nTimes; i++)
    {
        //2.generate random rx1, rx2 and rx = rx1 ^ rx2
        rx1_global.randomize(); // random rx[i]'s
        rx2_global.randomize();

        //4.generate rK1, rK2, rK = rK1 ^ rK2
        for (auto& w : rK1_global) w = randomWord(); //creating 8 random vector for rk1
        rK1_global[rK1_global.size() - 1] &= topelitzMask;

        for (auto& w : rK2_global) w = randomWord(); //creating 8 random vector for rk2
        rK2_global[rK2_global.size() - 1] &= topelitzMask;

        rw1_global.randomize(); // random sw1[i]
        rw2_global.randomize(); // random sw2[i]

#ifdef DEBUG
        rK1_global= {0,0,0,0,0,0,0,0};
        rK2_global = {1,0,0,0,0,0,0,0};

        rx1_global.reset();
        rx2_global.reset();
        rx1_global.set(0,1);
        rx1_global.set(63,0);
        rx2_global.set(1,1);
        rx2_global.set(63,0);

        rw1_global.reset();
        rw2_global.reset();
        rw1_global.set(0,1);
        rw2_global.set(1,1);

#endif
        rx_global = rx1_global; //rx = rx1
        rx_global ^= rx2_global;
        //rx_global.add(rx2_global); //rx = rx1 ^ rx2

        //3.generate random rw1, rw2 and rw = rw1 ^ rw2
        rw_global = rw1_global; //rw = rw1
        rw_global ^= rw2_global;
        //rw_global.add(rw2_global); //rw += rw2

        for (int i = 0; i < rK_global.size(); i++)
        {
            rK_global[i] = rK1_global[i] ^ rK2_global[i];   //rk = rk1 ^ rk2
        }

        //5.Calculate sw = rk_global * rx_global ^ rw_global
        sw1_global.toeplitzByVec(rK1_global,rx1_global); //sw1 = rk1 * rx1
        sw1_global ^= rw1_global;   //sw1 = sw1 ^ rw1
        sw2_global.toeplitzByVec(rK2_global,rx2_global); //sw2 = rk2 * rx2
        sw2_global ^= rw2_global;   //sw2 = sw2 ^ rw2
        sw_global = sw1_global; //sw = sw1 ^ sw2
        sw_global ^= sw2_global;

#ifdef DEBUG
        std::cout<<"sw_global "<<sw_global<<std::endl;
        std::cout<<"sw1_global "<<sw1_global<<std::endl;
        std::cout<<"sw2_global "<<sw2_global<<std::endl;
#endif

    }

#ifdef DEBUG
    std::cout<<"newprotocol.cpp/preProc_mod2_dm_2020: Preprocessing ends"<<std::endl;
#endif
}

void preProc_mod3_dm2020(unsigned int nTimes)
{
    //Cast rw and 1-rw  to mod3; single line assignment of r0z_global and r1z_global
    std::vector<unsigned int> sw_int; //array to act as a bridge between packedZ2 to packedZ3 conversion
    sw_global.toArray(sw_int); //convert packedZ2 to an array
    r0z_global.fromArray(sw_int); //convert an array to packedZ3

    std::vector<unsigned int> not_sw_int;//to store ~sw_val
    PackedZ2<N_COLS> not_sw_global;//to store ~sw_val
    not_sw_global = sw_global; //copy the value of sw_global
    not_sw_global.negate(); //negate the values
    not_sw_global.toArray(not_sw_int); //convert the values in an array
    r1z_global.fromArray(not_sw_int);   //convert array into packedZ3 for further processing

    //generate random values for r0z1 and r1z1; share of r0z and r1z for party 1
    r0z1_global.randomize();
    r1z1_global.randomize();

    //perform xor operation and calculate r0z2 and r1z2; share of r0z and r1z for party 2
    r0z2_global = r0z_global;    //r0z2 = r0z ^ r0z1
    r0z2_global.subtract(r0z1_global);

    r1z2_global = r1z_global;    //r1z2 = r1z ^ r1z1
    r1z2_global.subtract(r1z1_global);

    /*
#ifdef DEBUG//print the values of r0z, r0z1  and r0z2
    std::cout<<"newprotocol.cpp/preProc_mod3_dm2020():"<<std::endl;

    std::cout<<"sw_global "<<sw_global<<std::endl;
    std::cout<<"r0z "<<r0z_global<<std::endl;
    std::cout<<"r0z1 "<<r0z1_global<<std::endl;
    std::cout<<"r0z2 "<<r0z2_global<<std::endl;
    std::cout<<"not_sw_global "<<not_sw_global<<std::endl;
    std::cout<<"r1z "<<r1z_global<<std::endl;
    std::cout<<"r1z1 "<<r1z1_global<<std::endl;
    std::cout<<"r1z2 "<<r1z2_global<<std::endl;

#endif*/

}


void fetchPreproc_party1(PackedZ2<N_COLS>& rx1, PackedZ2<N_COLS>& rw1, PackedZ2<N_COLS>& sw1,
                         std::vector<uint64_t>& rK1, PackedZ3<N_SIZE>& r0z1, PackedZ3<N_SIZE>& r1z1)
{
    rx1 = rx1_global;
    rK1 = rK1_global;
    rw1 = rw1_global;
    sw1 = sw1_global;
    r0z1 = r0z1_global;
    r1z1 = r1z1_global;
}

void fetchPreproc_party2(PackedZ2<N_COLS>& rx2, PackedZ2<N_COLS>& rw2, PackedZ2<N_COLS>& sw2,
                         std::vector<uint64_t>& rK2, PackedZ3<N_SIZE>& r0z2, PackedZ3<N_SIZE>& r1z2)
{
    rx2 = rx2_global;
    rK2 = rK2_global;
    rw2 = rw2_global;
    sw2 = sw2_global;
    r0z2 = r0z2_global;
    r1z2 = r1z2_global;
}

void party1_round_1(PackedZ2<N_COLS>& x1_mask, std::vector<uint64_t>& K1_mask,
                    PackedZ2<N_COLS>& x1, PackedZ2<N_COLS>& rx1, std::vector<uint64_t>& K1, std::vector<uint64_t>& rK1)
{
    x1_mask = x1;
    x1_mask.add(rx1);
    for(int word_count=0; word_count < K1.size(); word_count++) {
        K1_mask[word_count] = K1[word_count] ^ rK1[word_count];
    }

    #ifdef DEBUG
        std::cout<<"newprotocol.cpp/party1_round_1():"<<std::endl;
        std::cout<<"x1 "<<x1<<std::endl;
        std::cout<<"K1 "<<K1<<std::endl;
        std::cout<<"rK1 "<<rK1<<std::endl;
        std::cout<<"x1_mask "<<x1_mask<<std::endl;
        std::cout<<"K1_mask "<<K1_mask<<std::endl;
    #endif
}

void party2_round_1(PackedZ2<N_COLS>& x2_mask, std::vector<uint64_t>& K2_mask,
                    PackedZ2<N_COLS>& x2, PackedZ2<N_COLS>& rx2, std::vector<uint64_t>& K2, std::vector<uint64_t>& rK2)
{
    x2_mask = x2;
    x2_mask ^= rx2;

    for(int word_count=0; word_count < K2.size(); word_count++) {
        K2_mask[word_count] = K2[word_count] ^ rK2[word_count];
    }

 //-commented to reduce clutter
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/party2_round_1():"<<std::endl;
    std::cout<<"x2 "<<x2<<std::endl;
    std::cout<<"K2 "<<K2<<std::endl;
    std::cout<<"rK2 "<<rK2<<std::endl;
    std::cout<<"x2_mask "<<x2_mask<<std::endl;
    std::cout<<"K2_mask "<<K2_mask<<std::endl;
#endif
}

void compute_input_mask(PackedZ2<N_COLS>& x_mask, std::vector<uint64_t>& K_mask,PackedZ2<N_COLS>& x1_mask,
                  PackedZ2<N_COLS>& x2_mask,std::vector<uint64_t>& K1_mask,std::vector<uint64_t>& K2_mask)
{
    x_mask = x1_mask;
    x_mask ^= x2_mask;
    //x_mask.add(x2_mask);
    //x_mask = x1_mask ^ x2_mask;

    for(int word_count=0; word_count < K_mask.size(); word_count++)
    {
        K_mask[word_count] = K1_mask[word_count] ^ K2_mask[word_count];
    }
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/compute_input_mask(): "<<std::endl;
    std::cout<<"x_mask "<<x_mask<<std::endl;
    std::cout<<"K_mask "<<K_mask<<std::endl;
#endif
}

void party1_round2(PackedZ2<N_COLS>& w1_mask, std::vector<uint64_t>& K_mask, PackedZ2<N_COLS>& x_mask, PackedZ2<N_COLS>& rx1,
        std::vector<uint64_t>& rK1, PackedZ2<N_COLS>& sw1)
{
    PackedZ2<N_COLS> x_rx1 = x_mask;    //x_rx1 = x_mask - rx1
    x_rx1 ^= rx1;

    PackedZ2<N_COLS> Kx1;   //kx1 = K_mask * x_rx1
    Kx1.toeplitzByVec(K_mask,x_rx1);

    PackedZ2<N_COLS> x_rK1;//x_rk1 = rK1 * x_mask
    x_rK1.toeplitzByVec(rK1,x_mask);

    w1_mask = Kx1;//w1 = kx1 - rk_x1
    w1_mask ^= x_rK1;
    w1_mask ^= sw1;
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/party1_round2(): "<<std::endl;
    std::cout<<"w1_mask "<<w1_mask<<std::endl;
#endif
}
void party2_round2(PackedZ2<N_COLS>& w2_mask, std::vector<uint64_t>& K_mask,PackedZ2<N_COLS>& x_mask, PackedZ2<N_COLS>& rx2,
                   std::vector<uint64_t>& rK2, PackedZ2<N_COLS>& sw2)
{
    PackedZ2<N_COLS> x_rx2 = x_mask;    //x_rx2 = x_mask - rx2
    x_rx2 ^= rx2;

    PackedZ2<N_COLS> Kx2;   //kx2 = K_mask * x_rx2
    Kx2.toeplitzByVec(K_mask,x_rx2);

    PackedZ2<N_COLS> x_rK2; //x_rk2 = rK2 * x_mask
    x_rK2.toeplitzByVec(rK2,x_mask);

    w2_mask = Kx2; //w2 = kx2 - rk_x2
    w2_mask ^= x_rK2;
    w2_mask ^= sw2;
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/party2_round2(): "<<std::endl;
    std::cout<<"w2_mask "<<w2_mask<<std::endl;
#endif
}

void compute_wmask(PackedZ2<N_COLS>& w_mask, PackedZ2<N_COLS>& w1_mask, PackedZ2<N_COLS>& w2_mask)
{
    w_mask = w1_mask;
    w_mask ^= w2_mask;
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/compute_wmask(): "<<std::endl;
    std::cout<<"w_mask "<<w_mask<<std::endl;
#endif
}

void party1_round3(PackedZ3<81>& y1_z3,PackedZ3<N_SIZE>& r0z1,
                   PackedZ3<N_SIZE>& r1z1, std::vector<PackedZ3<81> >& Rmat,PackedZ2<N_COLS>& w_mask)
{
    //PARTY 1: calculates the value of mux(w', r0z1, r1z1)
    PackedZ3<N_SIZE> res1;
    res1 = r0z1; //copy the contents of r0z to res1.

    //perform the mux functionality, pass the Packedz3 and converted vector of w_mask
    res1.mux(r1z1, w_mask.bits);

    y1_z3.matByVec(Rmat, res1); //party 1 computes y1 =  M * res1
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/party1_round3(): "<<std::endl;
    std::cout<<"res1 "<<res1<<std::endl;
    std::cout<<"y1_z3 "<<y1_z3<<std::endl;
#endif
}

void party2_round3(PackedZ3<81>& y2_z3,PackedZ3<N_SIZE>& r0z2,
                   PackedZ3<N_SIZE>& r1z2, std::vector<PackedZ3<81> >& Rmat,PackedZ2<N_COLS>& w_mask)
{
    //party 2 calculates the value of mux(w', r0z, r1z)
    PackedZ3<N_SIZE> res2;
    res2 = r0z2;

    //perform the mux functionality, pass the Packedz3 and converted vector of w_mask
    res2.mux(r1z2, w_mask.bits);

    //party computes y2 = M* res2
    y2_z3.matByVec(Rmat, res2);
#ifdef DEBUG
    std::cout<<"newprotocol.cpp/party2_round3(): "<<std::endl;
    std::cout<<"y2_z3 "<<y2_z3<<std::endl;
#endif
}

/*
void compute_y_out(PackedZ3<81>&  y_out_z3, PackedZ3<81>& y1_z3, PackedZ3<81>& y2_z3)
{
    //==============EXPERIMENTAL MOD 3 CALUCLATION(without the loop)==============

    y_out_z3.first = (y1_z3.second.negate() & y2_z3.second.negate()) &
            (y1_z3.first.negate() && y2_z3.first) | (y1_z3.first & y2_z3.first.negate()) |
            (y1_z3.second & y1_z3.first.negate()) & (y2_z3.second & y2_z3.first.negate());

    y_out_z3.second = (y1_z3.first.negate() & y2_z3.first.negate()) &
                     (y1_z3.second.negate() & y2_z3.second) | (y1_z3.second & y2_z3.second.negate()) |
                     (y1_z3.second.negate() & y1_z3.first) & (y2_z3.second.negate() & y2_z3.first);
    //====================================================================
    //calculating y = y1 + y2
    y_out_z3 = y1_z3;
    y_out_z3 ^= (y2_z3);
}*/

/*
 * This function generates the variables and perform computation to simulate centralized
 * version of the new protocol
 */
void PRF_new_protocol(std::vector<uint64_t>& K1, PackedZ2<N_COLS>& x1,
                              std::vector<uint64_t>& K2, PackedZ2<N_COLS>& x2,
                              std::vector<PackedZ3<81> >& Rmat, PackedZ3<81>& y1_z3, PackedZ3<81>& y2_z3, int nTimes)
{
    //local declarations=======9/15
    std::vector<uint64_t> rK1(toeplitzWords), rK2(toeplitzWords), rK(toeplitzWords);
    PackedZ2<N_COLS> rx1, rx2, rx; //mask for input
    PackedZ2<N_COLS> sw1, sw2, sw; //sw = rK * rx + rw
    PackedZ2<N_COLS> rw1, rw2, rw;

    //declare a variable in Z3 called r0z and r1z
    PackedZ3<N_SIZE>r0z, r1z;
    PackedZ3<N_SIZE> r0z1, r0z2, r1z1, r1z2;

    PackedZ3<81> y_out_z3;//output of each parties

    //============================

    //1. Perform preprocessing
    preProc_mod2_dm2020(nTimes);//it was nRuns * 2 in previous protocol, needs to be changed later
    preProc_mod3_dm2020(nTimes);

    fetchPreproc_party1(rx1,rw1,sw1,rK1,r0z1,r1z1);
    fetchPreproc_party2(rx2,rw2,sw2,rK2, r0z2,r1z2);

    #ifdef DEBUG
        std::cout<<"in PRF_new_protocol, rx1= "<<rx1<<std::endl;
        std::cout<<"PRF_new_protocol, rx2 "<<rx2<<std::endl;
    #endif

    //3. Parties locally compute [x'] = [x] + [rx] and [K'] = [K] + [rk]
    PackedZ2<N_COLS> x1_mask,x_mask, x2_mask;
    std::vector<uint64_t> K1_mask(toeplitzWords), K2_mask(toeplitzWords), K_mask(toeplitzWords);

    party1_round_1(x1_mask,K1_mask,x1,rx1,K1,rK1);
    party2_round_1(x2_mask,K2_mask,x2,rx2,K2,rK2);

    //std::cout<<""<<std::endl;
    //both the parties are supposed to exchange their mask values
    compute_input_mask(x_mask, K_mask, x1_mask, x2_mask, K1_mask, K2_mask);


#ifdef DEBUG

    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): Round 1 ends"<<std::endl;
#endif



#ifdef DEBUG
    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): Round 2 begins"<<std::endl;
#endif

    //Round 2: Both parties compute w' = [k'x'] - k'[rx] - rk.x' + [rk * rk + rw]

    PackedZ2<N_COLS> w1_mask, w2_mask, w_mask; //w' = K'(x' - rx) - rK'*x' + sw
    party1_round2(w1_mask, K_mask,x_mask, rx1, rK1, sw1);
    party2_round2(w2_mask, K_mask,x_mask, rx2, rK2, sw2);

    compute_wmask(w_mask, w1_mask, w2_mask);

#ifdef DEBUG
    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): Round 2 ends"<<std::endl;
#endif


#ifdef DEBUG
    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): Round 3 begins"<<std::endl;
#endif

    //Round 3: Both parties compute w' = [k'x'] - k'[rx] - rk.x' + [rk * rk + rw]

    PackedZ3<N_SIZE> z1, z2;
    PackedZ3<N_SIZE> res1, res2, res; //stores the mux result of both the parties
    //PackedZ3<81> y_out_z3; //y_out = y1 + y2


    party1_round3(y1_z3,r0z1,r1z1,Rmat,w_mask);
    party2_round3(y2_z3,r0z2,r1z2,Rmat,w_mask);

    std::cout<<""<<std::endl;
   /*
    //compute res =  res1 + res2; res is equivalent to z
    res = res1;
    res ^= res2;*/

#ifdef DEBUG
    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): Round 3 ends"<<std::endl;
#endif
    
#ifdef NP_TEST
    std::cout<<"newprotocol.cpp/PRF_new_protocol_central(): NP_TEST is enabled; calling the test function";
#endif
}

