
// This file is part of HemeLB and is Copyright (C)
// the HemeLB team and/or their institutions, as detailed in the
// file AUTHORS. This software is provided under the terms of the
// license in the file LICENSE.
#include "lb/iolets/InOutLetWK.h"
#include "lb/iolets/BoundaryComms.h"
#include "configuration/SimConfig.h"

namespace hemelb
{
  namespace lb
  {
    namespace iolets
    {
      InOutLetWK::InOutLetWK() :
          InOutLet(), radius(1.0), rwk(1.0), density(1.0)
      {
      }

      InOutLet* InOutLetWK::Clone() const
      {
        InOutLetWK* copy = new InOutLetWK(*this);
        return copy;
      }

      InOutLetWK::~InOutLetWK()
      {
      }

      void InOutLetWK::DoComms(const BoundaryCommunicator& boundaryComm, const LatticeTimeStep timeStep)
      {
        const int localRank = boundaryComm.Rank();
        const int nProcs = comms->GetNumProcs();
        const std::vector<int> procsList = comms->GetListOfProcs();
        const int centreRank = comms->GetCentreRank();
        for (int proc = 0; proc < nProcs; proc++)
        {
          //printf("localRank: %d, proc: %d, procsList[proc]: %d\n", localRank, proc, procsList[proc]);
        }

        if (nProcs == 1) return;

        LatticeDensity density_new = density;
        MPI_Request *sendRequest, receiveRequest;

        if (localRank != centreRank && localRank != 0)
        {
          HEMELB_MPI_CALL(
            MPI_Irecv, (
              &density,
              1,
              net::MpiDataType(density),
              centreRank,
              100,
              boundaryComm,
              &receiveRequest
            )
          );
        }
        else if (localRank == centreRank)
        {
          sendRequest = new MPI_Request[nProcs];
          for (int proc = 0; proc < nProcs; proc++)
          {
            HEMELB_MPI_CALL(
              MPI_Isend, (
                &density_new,
                1,
                net::MpiDataType(density_new),
                procsList[proc],
                901,
                boundaryComm,
                &sendRequest[proc]
              )
            );
          }

          HEMELB_MPI_CALL(
            MPI_Waitall, (nProcs, sendRequest, MPI_STATUS_IGNORE)
          );

          delete[] sendRequest;

          if (timeStep % 1 == 0){
            for (int proc = 0; proc < nProcs; proc++){
              printf("Time: %d, proc %d sent density of %.15lf to proc %d\n", timeStep, localRank, density_new, procsList[proc]);
            }
          }
        }

        if (localRank != centreRank && localRank != 0)
        {
          printf("Before Wait in DoComms by rank %d\n", localRank);

          HEMELB_MPI_CALL(
            MPI_Wait, (&receiveRequest, MPI_STATUS_IGNORE)
          );
          
          if (timeStep % 1 == 0)
            printf("Time: %d, proc %d received density of %.15lf\n", timeStep, localRank, density);
        }
      }

      distribn_t InOutLetWK::GetDistance(const LatticePosition& x) const
      {
        LatticePosition displ = x - position;
        LatticeDistance z = displ.Dot(normal);

	return std::sqrt(displ.GetMagnitudeSquared() - z * z);
      }
	      
      distribn_t InOutLetWK::GetQtScaleFactor(const LatticePosition& x) const
      {
        // Q = vLocal (0.5pi a**2)(a**2/(a**2 - r**2)
        // where r is the distance from the centreline
	// a is the radius of the circular iolet
        LatticePosition displ = x - position;
        LatticeDistance z = displ.Dot(normal);
        Dimensionless rFactor = (radius * radius)/(radius * radius - (displ.GetMagnitudeSquared() - z * z) );

        return 0.5 * PI * radius * radius * rFactor;
      }
    }
  }
}
