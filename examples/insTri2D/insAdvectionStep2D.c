#include "ins2D.h"

// complete a time step using LSERK4
void insAdvectionStep2D(solver_t *ins,
	                    iint tstep, 
	                    iint haloBytes,
				        dfloat * sendBuffer, 
				        dfloat * recvBuffer, 
				        char   * options){

	mesh2D *mesh = ins->mesh; 

	dfloat t = tstep*ins->dt;


   //Exctract Halo On Device

	if(mesh->totalHaloPairs>0){

		printf("%d \n", mesh->totalHaloPairs);
      // EXCTRACT HALO  on DEVICE
      iint Nentries = mesh->Np*ins->Nfields;
      
  // ins->haloExtractKernel(mesh->totalHaloPairs,
		//                  mesh->Np,
		//                  ins->Nfields,
		//                  mesh->o_haloElementList,
		//                  ins->o_U,
		//                  mesh->o_haloBuffer);

	 mesh->haloExtractKernel(mesh->totalHaloPairs,
			                 Nentries,
			                 mesh->o_haloElementList,
			                 ins->o_U,
			                 mesh->o_haloBuffer);	
      
      // copy extracted halo to HOST 
      mesh->o_haloBuffer.copyTo(sendBuffer);      
      
      // start halo exchange
      meshHaloExchangeStart(mesh,
			                mesh->Np*ins->Nfields*sizeof(dfloat), 
			                sendBuffer,
			                recvBuffer);
  	}

  	// Compute Volume Contribution

   ins->advectionVolumeKernel(
   	         mesh->Nelements,
   	         mesh->o_vgeo,
			 mesh->o_DrT,
			 mesh->o_DsT,
			 ins->o_U,
			 ins->o_rhsU
			 );


    // COMPLETE HALO EXCHANGE
    if(mesh->totalHaloPairs>0){
      // wait for halo data to arrive
      meshHaloExchangeFinish(mesh);
      //
      size_t offset = mesh->Np*mesh->Nelements*sizeof(dfloat);

      ins->o_U.copyFrom(recvBuffer+UXID*haloBytes/ins->Nfields, 
      	                haloBytes/ins->Nfields,
      	                offset + UXID*ins->NtotalDofs);
      ins->o_U.copyFrom(recvBuffer+UYID*haloBytes/ins->Nfields, 
      	                haloBytes/ins->Nfields,
      	                offset+UYID*ins->NtotalDofs);
    }

 // Compute Surface Conribution
  ins->advectionSurfaceKernel(
   	           mesh->Nelements,
			   mesh->o_sgeo,
			   mesh->o_LIFTT,
			   mesh->o_vmapM,
			   mesh->o_vmapP,
			   mesh->o_EToB,
			   t,
			   mesh->o_x,
			   mesh->o_y,
			   ins->o_U,
			   ins->o_rhsU
			 );
 // Update fields
  ins->advectionUpdateKernel(
   	           mesh->Nelements,
   	           ins->dt,	
               ins->a0,
               ins->a1,
               ins->b0,
               ins->b1,
               ins->g0,
               ins->o_U,
               ins->o_UO,
               ins->o_rhsU,
               ins->o_NU,
               ins->o_UI
               );

   
}
