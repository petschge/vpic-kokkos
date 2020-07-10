/* *****************************************************************
    
    This "header" file is meant to be included directly into the 
    move_p_kokkos(...) function in species_advance.h as 
    #include "accumulate_j_zigzag.hpp"
    
    This method was published by Umeda et al. in 2003 and can be 
    found at

        https://doi.org/10.1016/S0010-4655(03)00437-5

    THIS FILE IS THE ZIGZAG ONE!!!!!!!!!!!!!!!!!!!

   ****************************************************************/
    
    // In this approach, we keep the current accumulation the same,
    // and move the particles along the zigzag path. The position 
    // of the zig of the zigzag will be performed only if the 
    // particle leaves the cell. If the particle does not leave the
    // cell, then the zag is performed.

    int ii = pii;
    s_midx = p_dx;
    s_midy = p_dy;
    s_midz = p_dz;


    s_dispx = pm->dispx;
    s_dispy = pm->dispy;
    s_dispz = pm->dispz;

    //printf("\nParticle %d: pre axis %d x %e y %e z %e", pi, axis, p_dx, p_dy, p_dz);

    //printf("\nParticle %d: disp x %e y %e z %e", pi, s_dispx, s_dispy, s_dispz);

    // Compute the direction that the particle moves.
    // This value is the also the boundary of the cell 
    // the particle will intercept.
    s_dir[0] = (s_dispx>0) ? 1 : -1;
    s_dir[1] = (s_dispy>0) ? 1 : -1;
    s_dir[2] = (s_dispz>0) ? 1 : -1;

    // Compute the twice the fractional distance to each potential
    // streak/cell face intersection. This number is the amount of
    // for-loop steps that would need to be taken to reach the 
    // boundary of the cell along each axis. A maximum of 2 for-
    // loop steps are allowed within a single time-step though, so
    // if the value is greater than 2 in a particle direction, then
    // it does not leave the cell in that direction in this
    // function call.
    v0 = (s_dispx==0) ? 3.4e38f : (s_dir[0]-s_midx)/s_dispx;
    v1 = (s_dispy==0) ? 3.4e38f : (s_dir[1]-s_midy)/s_dispy;
    v2 = (s_dispz==0) ? 3.4e38f : (s_dir[2]-s_midz)/s_dispz;

    // Determine the fractional length and axis of current streak. The
    // streak ends on either the first face intersected by the
    // particle track or at the end of the particle track.
    //
    //   axis 0,1 or 2 ... streak ends on a x,y or z-face respectively
    //   axis 3        ... streak ends at end of the particle track
    /**/      v3=2,  axis=3;
    if(v0<v3) v3=v0, axis=0;
    if(v1<v3) v3=v1, axis=1;
    if(v2<v3) v3=v2, axis=2;
    // Multiply v3 by 1/2 because the particle first moves to the 
    // midpoint if axis != 3, or it stops if axis == 3.
    v3 *= 0.5;
    
    printf("\nParticle %d: axis, v0, v1, v2, v3 = %d, %e, %e, %e, %e",
            pi, axis, v0, v1, v2, 2.*v3);
    printf("\nParticle %d: s_midx, s_midy, s_midz = %e, %e, %e",
            pi, s_midx, s_midy, s_midz);
    printf("\nParticle %d: s_dispx, s_dispy, s_dispz = %e, %e, %e\n",
            pi, s_dispx, s_dispy, s_dispz);
    
    // Store the old values of s_mid and s_disp before I do crazy
    // things.
    float old_midx = s_midx, old_midy = s_midy, old_midz = s_midz;
    float old_dispx = s_dispx, old_dispy = s_dispy, old_dispz = s_dispz;

    // Umeda algorithm: assume axis == 3 and set xr, yr, and zr
    // to be the end of the midpoint of the zag (so the final
    // destination of the particle).
    // TODO: Is it necessary to half the displacement when
    // axis == 3?
    float xr = s_midx + 0.5 * s_dispx, yr = s_midy + 0.5 * s_dispy, zr = s_midz + 0.5 * s_dispz;
   
    // If the particle crosses the x-boundary change xr
    // to the boundary it hits.
    if ( v0 < 2. )
    {
        xr = s_dir[0];
        s_dispx *= v0;
    }
    // If the particle crosses the y-boundary change yr
    // to the boundary it hits.
    if ( v1 < 2. )
    {
        yr = s_dir[1];
        s_dispy *= v1;
    }
    // If the particle crosses the z-boundary change zr
    // to the boundary it hits.
    if ( v2 < 2. )
    {
        zr = s_dir[2];
        s_dispz *= v2;
    } 
    // With xr, yr, and zr known, we can treat them as the final 
    // location on either the zig or the zag. Now we just need 
    // the new midpoint along this new linear zig or zag.
    s_midx = 0.5 * ( s_midx + xr );
    s_midy = 0.5 * ( s_midy + yr );
    s_midz = 0.5 * ( s_midz + zr );

    // Change the displacement to the midpoint along the zig
    // or zag.
    s_dispx *= 0.5, s_dispy *= 0.5, s_dispz *= 0.5;

    // Compute the midpoint and the normalized displacement of the 
    // streak. By scaling the displacments by v3, if axis == 3, then
    // nothing is done (v3 == 1.0 at this point), but if axis != 3,
    // then s_disp is set to s_dir - s_mid along the appropriate
    // axis, while the other displacements are scaled to keep the
    // particle on the same linear trajectory.
    /*
    s_dispx *= v3;
    s_dispy *= v3;
    s_dispz *= v3;
    s_midx += s_dispx;
    s_midy += s_dispy;
    s_midz += s_dispz;
    */
    
    // Accumulate the streak.  Note: accumulator values are 4 times
    // the total physical charge that passed through the appropriate
    // current quadrant in a time-step
    v5 = q*s_dispx*s_dispy*s_dispz*(1.f/3.f);

    //a = (float *)(&d_accumulators[ci]);

#   define accumulate_j(X,Y,Z)                                        \
    v4  = q*s_disp##X;    /* v2 = q ux                            */  \
    v1  = v4*s_mid##Y;    /* v1 = q ux dy                         */  \
    v0  = v4-v1;          /* v0 = q ux (1-dy)                     */  \
    v1 += v4;             /* v1 = q ux (1+dy)                     */  \
    v4  = 1+s_mid##Z;     /* v4 = 1+dz                            */  \
    v2  = v0*v4;          /* v2 = q ux (1-dy)(1+dz)               */  \
    v3  = v1*v4;          /* v3 = q ux (1+dy)(1+dz)               */  \
    v4  = 1-s_mid##Z;     /* v4 = 1-dz                            */  \
    v0 *= v4;             /* v0 = q ux (1-dy)(1-dz)               */  \
    v1 *= v4;             /* v1 = q ux (1+dy)(1-dz)               */  \
    v0 += v5;             /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */  \
    v1 -= v5;             /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */  \
    v2 -= v5;             /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */  \
    v3 += v5;             /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */  \
    //Kokkos::atomic_add(&a[0], v0); \
    //Kokkos::atomic_add(&a[1], v1); \
    //Kokkos::atomic_add(&a[2], v2); \
    //Kokkos::atomic_add(&a[3], v3);
   
    accumulate_j(x,y,z);
    printf("\nParticle %d depositing (x,y,z) v0, v1, v2, v4 = %e, %e, %e, %e", pi, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jx, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 3) += v3;

    accumulate_j(y,z,x);
    printf("\nParticle %d depositing (y,z,x) v0, v1, v2, v4 = %e, %e, %e, %e", pi, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jy, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 3) += v3;

    accumulate_j(z,x,y);
    printf("\nParticle %d depositing (z,x,y) v0, v1, v2, v4 = %e, %e, %e, %e\n\n", pi, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jz, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 3) += v3;

#   undef accumulate_j


    // Compute the remaining particle displacment
    pm->dispx -= s_dispx;
    pm->dispy -= s_dispy;
    pm->dispz -= s_dispz;

    //printf("pre axis %d x %e y %e z %e disp x %e y %e z %e\n", axis, p_dx, p_dy, p_dz, s_dispx, s_dispy, s_dispz);
    // Compute the new particle offset
    // TODO: In the case that axis == 3, this seems to overcompensate for the change in position...
    p_dx += s_dispx+s_dispx;
    p_dy += s_dispy+s_dispy;
    p_dz += s_dispz+s_dispz;

    // If an end streak, return success (should be ~50% of the time)
    //printf("axis %d x %e y %e z %e disp x %e y %e z %e\n", axis, p_dx, p_dy, p_dz, s_dispx, s_dispy, s_dispz);

    if( axis==3 ) 
    {
        printf("\n*****************************\nParticle %d is done moving at p_dx, p_dy, p_dz = %e, %e, %e\nIt is supposed to stop at x2, y2, z2 = %e, %e, %e\n****************************\n",
                pi, p_dx, p_dy, p_dz, xr, yr, zr);
        break;
    }

    // Determine if the particle crossed into a local cell or if it
    // hit a boundary and convert the coordinate system accordingly.
    // Note: Crossing into a local cell should happen ~50% of the
    // time; hitting a boundary is usually a rare event.  Note: the
    // entry / exit coordinate for the particle is guaranteed to be
    // +/-1 _exactly_ for the particle.

    // TODO: Change this to allow for corner crossing.
    v0 = s_dir[axis];
    k_particles(pi, particle_var::dx + axis) = v0; // Avoid roundoff fiascos--put the particle
                           // _exactly_ on the boundary.
    face = axis; if( v0>0 ) face += 3;

    // TODO: clean this fixed index to an enum
    //neighbor = g->neighbor[ 6*ii + face ];
    neighbor = d_neighbor( 6*ii + face );

    // TODO: these two if statements used to be marked UNLIKELY,
    // but that intrinsic doesn't work on GPU.
    // for performance portability, maybe specialize UNLIKELY
    // for CUDA mode and put it back


    if( neighbor==reflect_particles ) {
      // Hit a reflecting boundary condition.  Reflect the particle
      // momentum and remaining displacement and keep moving the
      // particle.
      printf("\n*************************\nParticle %d was reflected.\n************************\n", pi);
      k_particles(pi, particle_var::ux + axis) = -k_particles(pi, particle_var::ux + axis);

      // TODO: make this safer
      //(&(pm->dispx))[axis] = -(&(pm->dispx))[axis];
      //k_local_particle_movers(0, particle_mover_var::dispx + axis) = -k_local_particle_movers(0, particle_mover_var::dispx + axis);
      // TODO: replace this, it's horrible
      (&(pm->dispx))[axis] = -(&(pm->dispx))[axis];


      continue;
    }

    if( neighbor<rangel || neighbor>rangeh ) {
      // Cannot handle the boundary condition here.  Save the updated
      // particle position, face it hit and update the remaining
      // displacement in the particle mover.
      pii = 8*pii + face;
      return 1; // Return "mover still in use"
      }

    // Crossed into a normal voxel.  Update the voxel index, convert the
    // particle coordinate system and keep moving the particle.

    pii = neighbor - rangel;
    /**/                         // Note: neighbor - rangel < 2^31 / 6
    k_particles(pi, particle_var::dx + axis) = -v0;      // Convert coordinate system

