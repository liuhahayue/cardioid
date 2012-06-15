#ifndef CA_SENSOR_HH
#define CA_SENSOR_HH

#include "VoronoiCoarsening.hh"
#include "Sensor.hh"

#include <string>
using namespace std;

class Reaction;
class Anatomy;

class CaAverageSensor : public Sensor
{
private:
   VoronoiCoarsening coarsening_;

   std::string filename_;

   const Reaction& reaction_;
   
   unsigned nx_;
   unsigned ny_;
   unsigned nz_;
   unsigned nlocal_;

   MPI_Comm comm_;

   LocalSums avg_valcolors_;

   void computeColorAverages(const vector<double>& val);
   void writeAverages(const string& filename,
                      const double current_time,
                      const int current_loop)const;
public:
   CaAverageSensor(const SensorParms& sp,
            string filename,
            const Anatomy& anatomy,
            const vector<Long64>& gid,
            const Reaction& reaction,
            MPI_Comm comm);
   ~CaAverageSensor(){};
   
   void print(double time, int loop);
   void eval(double time, int loop);
};


#endif