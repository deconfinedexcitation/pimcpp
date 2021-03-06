/////////////////////////////////////////////////////////////
// Copyright (C) 2003-2013  B. Clark, K. Esler, E. Brown   //
//                                                         //
// This program is free software; you can redistribute it  //
// and/or modify it under the terms of the GNU General     //
// Public License as published by the Free Software        //
// Foundation; either version 2 of the License, or         //
// (at your option) any later version.  This program is    //
// distributed in the hope that it will be useful, but     //
// WITHOUT ANY WARRANTY; without even the implied warranty //
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. //
// See the GNU General Public License for more details.    //
// For more information, please see the PIMC++ Home Page:  //
// http://code.google.com/p/pimcplusplus/                  //
/////////////////////////////////////////////////////////////

#include "PermutationCount.h"


void PermutationCountClass::WriteBlock()
{
  //  Array<double,1> CorSum(Correlation.size());
  //  Path.Communicator.Sum(Correlation, CorSum);

  /// SectorCount
  if (PathData.Path.Communicator.MyProc() == 0) {
    // Map out the SectorCount vector
    map<int,double> SectorMap;
    for (int i = 0; i < NumSamples; i++) {
      int perm = SectorCount.back();
      if (SectorMap.find(perm) == SectorMap.end())
        SectorMap.insert(pair<int,double>(perm,1));
      else
        SectorMap[perm] += 1;
      SectorCount.pop_back();
    }

    // Put the map into an array and write
    map<int,double>::iterator it;
    for(it = SectorMap.begin(); it != SectorMap.end(); it++) {
      Array<double,1> tmpSectorCount(2);
      tmpSectorCount(0) = (*it).first;
      tmpSectorCount(1) = (*it).second;
      SectorCountVar.Write(tmpSectorCount);
      SectorCountVar.Flush();
    }

    /// CycleCount
    int NumCycles = 0;
    for (int i = 0; i < CycleCount.size(); i++)
      NumCycles += CycleCount(i);
    double norm = 1.0 / ((double) NumCycles);
    for (int i = 0; i < CycleCount.size(); i++)
      CycleCount(i) = Prefactor * CycleCount(i) * norm;
    CycleCountVar.Write(CycleCount);
    CycleCountVar.Flush();
    CycleCount = 0.0;
    NumSamples = 0;
  }
}


void PermutationCountClass::Read(IOSectionClass &in)
{
  ObservableClass::Read(in);

  // Maximum number of permutation sectors tracked
  int MaxNSectors;
  if(!in.ReadVar("MaxNSectors", MaxNSectors))
    MaxNSectors = 0; // 0 -> Track all sectors

  // Setup Permutation Sectors
  PathData.Path.SetupPermSectors(PathData.Path.NumParticles(),MaxNSectors);
  CycleCount.resize(PathData.Path.NumParticles());
  CycleCount = 0.0;
  NumSamples = 0;

  /// Now write the one-time output variables
  if (PathData.Path.Communicator.MyProc()==0) {
    WriteInfo();
  }

}


void PermutationCountClass::Accumulate()
{
  TimesCalled++;
  //Move the join to the end so we don't have to worry about permutations
  PathData.MoveJoin(PathData.NumTimeSlices() - 1);
  NumSamples++;

  int PermSector;
  vector<int> Cycles;
  PathData.Path.GetPermInfo(Cycles, PermSector);
  if (PathData.Path.Communicator.MyProc() == 0) {
    SectorCount.push_back(PermSector);
    for (vector<int>::size_type j=0; j != Cycles.size(); j++)
      CycleCount(Cycles[j]) += 1;
  }
}


