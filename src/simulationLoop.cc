#include "simulationLoop.hh"

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "Simulate.hh"
#include "Diffusion.hh"
#include "Reaction.hh"
#include "Stimulus.hh"
#include "Sensor.hh"
#include "HaloExchange.hh"
#include "GridRouter.hh"
#include "ioUtils.h"
#include "writeCells.hh"

using namespace std;

void simulationLoop(Simulate& sim)
{
  int myRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
   
  vector<double> dVmDiffusion(sim.anatomy_.nLocal());
  vector<double> dVmReaction(sim.anatomy_.nLocal());
  vector<double> dVmExternal(sim.anatomy_.nLocal(), 0.0);
  vector<double> iStim(sim.anatomy_.nLocal(), 0.0);
   
  // for now, hardcode initialization of voltage.
  // use TT04 value from BlueBeats
  sim.VmArray_.resize(sim.anatomy_.size(), -86.2); // in Volts
   
  sim.voltageExchange_ = new HaloExchange<double>(sim.router_->sendMap(), sim.router_->commTable());


  if ( myRank == 0)
  {
    cout << "    Loop     Time           Vm        dVm_r        dVm_d        dVm_e" <<endl;
    cout << setw(8) << sim.loop_ <<" "
         << setw(8) << sim.time_ <<" "
         << setw(12) << sim.VmArray_[0] << " "
         << setw(12) << dVmReaction[0] << " "
         << setw(12) << dVmDiffusion[0] << " "
         << setw(12) << dVmExternal[0]  << endl;
  }
      


  while ( sim.loop_<sim.maxLoop_ )
  {
    int nLocal = sim.anatomy_.nLocal();
    sim.voltageExchange_->execute(sim.VmArray_, nLocal);
      
    // DIFFUSION
    sim.diffusion_->calc(sim.VmArray_, dVmDiffusion);
      
    // code to limit or set iStimArray goes here.
    for (unsigned ii=0; ii<sim.stimulus_.size(); ++ii)
      sim.stimulus_[ii]->stim(sim.time_, dVmDiffusion, dVmExternal);

    for (unsigned ii=0; ii<nLocal; ++ii)
      iStim[ii] = -(dVmDiffusion[ii] + dVmExternal[ii]);
      
    // REACTION
    sim.reaction_->calc(sim.dt_, sim.VmArray_, iStim, dVmReaction);
    for (unsigned ii=0; ii<nLocal; ++ii)
    {
      double dVm = dVmReaction[ii] + dVmDiffusion[ii] + dVmExternal[ii] ;
      sim.VmArray_[ii] += sim.dt_*dVm;
    }
    sim.time_ += sim.dt_;
    ++sim.loop_;

    // print output to file
    for (unsigned ii=0; ii<sim.sensor_.size(); ++ii)
    {
      if (sim.loop_ % sim.sensor_[ii]->printRate() == 0)
      {
        if (sim.sensor_[ii]->printDerivs())
          sim.sensor_[ii]->print(sim.time_,sim.VmArray_,dVmReaction,dVmDiffusion,dVmExternal);
        else
          sim.sensor_[ii]->print(sim.time_,sim.VmArray_);
      }
    }
    
    if ( (sim.loop_ % sim.printRate_ == 0) && myRank == 0)
    {
      cout << setw(8) << sim.loop_ <<" "
           << setw(8) << sim.time_ <<" "
           << setw(12) << sim.VmArray_[0] << " "
           << setw(12) << dVmReaction[0] << " "
           << setw(12) << dVmDiffusion[0] << " "
           << setw(12) << dVmExternal[0]  << " "
           << setw(12) << sim.anatomy_.gid(0)  << endl;
    }
    
    if (sim.loop_ % sim.snapshotRate_ == 0)
    {
      stringstream name;
      name << "snapshot."<<setfill('0')<<setw(8)<<sim.loop_;
      string fullname = name.str();
      DirTestCreate(fullname.c_str());
      fullname += "/anatomy";
      writeCells(sim, fullname.c_str());
    }
      
  }
}
