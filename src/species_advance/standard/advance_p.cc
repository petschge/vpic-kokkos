// FIXME: PARTICLE MOVERS NEED TO BE OVERALLOCATED IN STRUCTORS TO
// ACCOUNT FOR SPLITTING THE MOVER ARRAY BETWEEN HOST AND PIPELINES

#define IN_spa
#define HAS_V4_PIPELINE
#include <stdio.h>
#include "spa_private.h"
#include "../../vpic/kokkos_helpers.h"

void
advance_p_kokkos(
        k_particles_t& k_particles,
        k_particles_i_t& k_particles_i,
        k_particle_copy_t& k_particle_copy,
        k_particle_i_copy_t& k_particle_i_copy,
        k_particle_movers_t& k_particle_movers,
        k_particle_i_movers_t& k_particle_movers_i,
        k_accumulators_sa_t k_accumulators_sa,
        k_interpolator_t& k_interp,
        //k_particle_movers_t k_local_particle_movers,
        k_iterator_t& k_nm,
        k_neighbor_t& k_neighbors,
        const grid_t *g,
        const float qdt_2mc,
        const float cdt_dx,
        const float cdt_dy,
        const float cdt_dz,
        const float qsp,
        const int na,
        const int np,
        const int max_nm,
        const int nx,
        const int ny,
        const int nz)
{

  constexpr float one            = 1.;
  constexpr float one_third      = 1./3.;
  constexpr float two_fifteenths = 2./15.;

  /*
  k_particle_movers_t *k_local_particle_movers_p = new k_particle_movers_t("k_local_pm", 1);
  k_particle_movers_t  k_local_particle_movers("k_local_pm", 1);

  k_iterator_t k_nm("k_nm");
  k_iterator_t::HostMirror h_nm = Kokkos::create_mirror_view(k_nm);
  h_nm(0) = 0;
  Kokkos::deep_copy(k_nm, h_nm);
  */
  // Determine which quads of particles quads this pipeline processes

  //DISTRIBUTE( args->np, 16, pipeline_rank, n_pipeline, itmp, n );
  //p = args->p0 + itmp;

  /*
  printf("original value %f\n\n", k_accumulators(0, 0, 0));
sp_[id]->
  Kokkos::parallel_for(Kokkos::RangePolicy < Kokkos::DefaultExecutionSpace > (0, 1), KOKKOS_LAMBDA (int i) {

      auto scatter_access = k_accumulators_sa.access();
      //auto scatter_access_atomic = scatter_view.template access<Kokkos::Experimental::ScatterAtomic>();
          printf("Writing to %d\n", i);
          scatter_access(i, 0, 0) += 4;
          //scatter_access_atomic(i, 1) += 2.0;
          //scatter_access(k, 2) += 1.0;
          //
  });

  // copy back
  Kokkos::Experimental::contribute(k_accumulators, k_accumulators_sa);
  printf("changed value %f\n", k_accumulators(0, 0, 0));
  */

  // Determine which movers are reserved for this pipeline
  // Movers (16 bytes) should be reserved for pipelines in at least
  // multiples of 8 such that the set of particle movers reserved for
  // a pipeline is 128-byte aligned and a multiple of 128-byte in
  // size.  The host is guaranteed to get enough movers to process its
  // particles with this allocation.
/*
  max_nm = args->max_nm - (args->np&15);
  if( max_nm<0 ) max_nm = 0;
  DISTRIBUTE( max_nm, 8, pipeline_rank, n_pipeline, itmp, max_nm );
  if( pipeline_rank==n_pipeline ) max_nm = args->max_nm - itmp;
  pm   = args->pm + itmp;
  nm   = 0;
  itmp = 0;

  // Determine which accumulator array to use
  // The host gets the first accumulator array

  if( pipeline_rank!=n_pipeline )
    a0 += (1+pipeline_rank)*
          POW2_CEIL((args->nx+2)*(args->ny+2)*(args->nz+2),2);
*/
  // Process particles for this pipeline

  #define p_dx    k_particles(p_index, particle_var::dx)
  #define p_dy    k_particles(p_index, particle_var::dy)
  #define p_dz    k_particles(p_index, particle_var::dz)
  #define p_ux    k_particles(p_index, particle_var::ux)
  #define p_uy    k_particles(p_index, particle_var::uy)
  #define p_uz    k_particles(p_index, particle_var::uz)
  #define p_w     k_particles(p_index, particle_var::w)
  #define pii     k_particles_i(p_index)

  #define f_cbx k_interp(ii, interpolator_var::cbx)
  #define f_cby k_interp(ii, interpolator_var::cby)
  #define f_cbz k_interp(ii, interpolator_var::cbz)
  #define f_ex  k_interp(ii, interpolator_var::ex)
  #define f_ey  k_interp(ii, interpolator_var::ey)
  #define f_ez  k_interp(ii, interpolator_var::ez)

  #define f_dexdy    k_interp(ii, interpolator_var::dexdy)
  #define f_dexdz    k_interp(ii, interpolator_var::dexdz)

  #define f_d2exdydz k_interp(ii, interpolator_var::d2exdydz)
  #define f_deydx    k_interp(ii, interpolator_var::deydx)
  #define f_deydz    k_interp(ii, interpolator_var::deydz)

  #define f_d2eydzdx k_interp(ii, interpolator_var::d2eydzdx)
  #define f_dezdx    k_interp(ii, interpolator_var::dezdx)
  #define f_dezdy    k_interp(ii, interpolator_var::dezdy)

  #define f_d2ezdxdy k_interp(ii, interpolator_var::d2ezdxdy)
  #define f_dcbxdx   k_interp(ii, interpolator_var::dcbxdx)
  #define f_dcbydy   k_interp(ii, interpolator_var::dcbydy)
  #define f_dcbzdz   k_interp(ii, interpolator_var::dcbzdz)

  // copy local memmbers from grid
  //auto nfaces_per_voxel = 6;
  //auto nvoxels = g->nv;
  //Kokkos::View<int64_t*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>
      //h_neighbors(g->neighbor, nfaces_per_voxel * nvoxels);
  //auto d_neighbors = Kokkos::create_mirror_view_and_copy(Kokkos::DefaultExecutionSpace(), h_neighbors);

  auto rangel = g->rangel;
  auto rangeh = g->rangeh;

  // TODO: is this the right place to do this?
  Kokkos::parallel_for("clear nm", Kokkos::RangePolicy < Kokkos::DefaultExecutionSpace > (0, 1), KOKKOS_LAMBDA (size_t i) {
    //printf("how many times does this run %d", i);
    k_nm(0) = 0;
    //local_pm_dispx = 0;
    //local_pm_dispy = 0;
    //local_pm_dispz = 0;
    //local_pm_i = 0;
  });


  Kokkos::parallel_for("advance_p", Kokkos::RangePolicy < Kokkos::DefaultExecutionSpace > (0, np),
    KOKKOS_LAMBDA (size_t p_index)
    {
//for(int p_index=0; p_index<np; p_index++) {
    float v0, v1, v2, v3, v4, v5;
    auto  k_accumulators_scatter_access = k_accumulators_sa.access();

    float dx   = p_dx;                             // Load position
    float dy   = p_dy;
    float dz   = p_dz;
    int   ii   = pii;
    float hax  = qdt_2mc*(    ( f_ex    + dy*f_dexdy    ) +
                           dz*( f_dexdz + dy*f_d2exdydz ) );
    float hay  = qdt_2mc*(    ( f_ey    + dz*f_deydz    ) +
                           dx*( f_deydx + dz*f_d2eydzdx ) );
    float haz  = qdt_2mc*(    ( f_ez    + dx*f_dezdx    ) +
                           dy*( f_dezdy + dx*f_d2ezdxdy ) );

    //hax = 0, hay = 0, haz = 0;
    //printf(" inter %d vs %ld \n", ii, k_interp.size());
    float cbx  = f_cbx + dx*f_dcbxdx;             // Interpolate B
    float cby  = f_cby + dy*f_dcbydy;
    float cbz  = f_cbz + dz*f_dcbzdz;
    // printf("\n\nParticle %ld: hax, hay, haz = %e, %e, %e", p_index, hax, hay, haz);
    // printf("\nParticle %ld: cbx, cby, cbz = %e, %e, %e", p_index, cbx, cby, cbz);
    //cbx = 0, cby = 0, cbz = 0;
    float ux   = p_ux;                             // Load momentum
    float uy   = p_uy;
    float uz   = p_uz;
    float q    = p_w;
    ux  += hax;                               // Half advance E
    uy  += hay;
    uz  += haz;
    v0   = qdt_2mc/sqrtf(one + (ux*ux + (uy*uy + uz*uz)));
    /**/                                      // Boris - scalars
    v1   = cbx*cbx + (cby*cby + cbz*cbz);
    v2   = (v0*v0)*v1;
    v3   = v0*(one+v2*(one_third+v2*two_fifteenths));
    v4   = v3/(one+v1*(v3*v3));
    v4  += v4;
    v0   = ux + v3*( uy*cbz - uz*cby );       // Boris - uprime
    v1   = uy + v3*( uz*cbx - ux*cbz );
    v2   = uz + v3*( ux*cby - uy*cbx );
    ux  += v4*( v1*cbz - v2*cby );            // Boris - rotation
    uy  += v4*( v2*cbx - v0*cbz );
    uz  += v4*( v0*cby - v1*cbx );
    ux  += hax;                               // Half advance E
    uy  += hay;
    uz  += haz;
    p_ux = ux;                               // Store momentum
    p_uy = uy;
    p_uz = uz;

    v0   = one/sqrtf(one + (ux*ux+ (uy*uy + uz*uz)));
    /**/                                      // Get norm displacement
    ux  *= cdt_dx;
    uy  *= cdt_dy;
    uz  *= cdt_dz;
    ux  *= v0;
    uy  *= v0;
    uz  *= v0;
    v0   = dx + ux;                           // Streak midpoint (inbnds)
    v1   = dy + uy;
    v2   = dz + uz;
    v3   = v0 + ux;                           // New position
    v4   = v1 + uy;
    v5   = v2 + uz;

    // printf("\n\nBEGINNING OF PARTICLE MOTION");
    // printf("\nParticle %d: xi, yi, zi = %e, %e, %e", p_index, dx, dy, dz);
    // printf("\nParticle %ld final-particle-velocity vxf, vyf, vzf = %e, %e %e", p_index, p_ux, p_uy, p_uz);
    // printf("\nParticle %d: xf, yf, zf = %e, %e, %e\n", p_index, dx + 2*ux, dy + 2*uy, dz + 2*uz);
    float s_dispx = ux;
    float s_dispy = uy;
    float s_dispz = uz;

    // bool zagger = true;

    // FIXME-KJB: COULD SHORT CIRCUIT ACCUMULATION IN THE CASE WHERE QSP==0!
    if(  !(v3<=one &&  v4<=one &&  v5<=one &&   // Check if inbnds
            -v3<=one && -v4<=one && -v5<=one) ) {

        // zagger = false;

        // If a particle leaves the cell, perform the zig of the 
        // zigzag algorithm. This moves the particle directly to 
        // the boundary of the cell. From here it will then zag
        // below, corresponding to motion within the same cell.

        /*
           local_pm_dispx = ux;
           local_pm_dispy = uy;
           local_pm_dispz = uz;

           local_pm_i     = p_index;
        */
      DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
      local_pm->dispx = ux;
      local_pm->dispy = uy;
      local_pm->dispz = uz;
      local_pm->i     = p_index;

      //printf("Calling move_p index %d dx %e y %e z %e ux %e uy %e yz %e \n", p_index, ux, uy, uz, p_ux, p_uy, p_uz);
      if( move_p_kokkos( k_particles, k_particles_i, local_pm,
                         k_accumulators_sa, g, k_neighbors, rangel, rangeh, qsp ) ) { // Unlikely
        if( k_nm(0)<max_nm ) {
          const unsigned int nm = Kokkos::atomic_fetch_add( &k_nm(0), 1 );
          if (nm >= max_nm) Kokkos::abort("overran max_nm");

          k_particle_movers(nm, particle_mover_var::dispx) = local_pm->dispx;
          k_particle_movers(nm, particle_mover_var::dispy) = local_pm->dispy;
          k_particle_movers(nm, particle_mover_var::dispz) = local_pm->dispz;
          k_particle_movers_i(nm)   = local_pm->i;

          // Keep existing mover structure, but also copy the particle data so we have a reduced set to move to host
          k_particle_copy(nm, particle_var::dx) = p_dx;
          k_particle_copy(nm, particle_var::dy) = p_dy;
          k_particle_copy(nm, particle_var::dz) = p_dz;
          k_particle_copy(nm, particle_var::ux) = p_ux;
          k_particle_copy(nm, particle_var::uy) = p_uy;
          k_particle_copy(nm, particle_var::uz) = p_uz;
          k_particle_copy(nm, particle_var::w) = p_w;
          k_particle_i_copy(nm) = pii;

          // Tag this one as having left
          //k_particles(p_index, particle_var::pi) = 999999;

          // Copy local local_pm back
          //local_pm_dispx = local_pm->dispx;
          //local_pm_dispy = local_pm->dispy;
          //local_pm_dispz = local_pm->dispz;
          //local_pm_i = local_pm->i;
          //printf("rank copying %d to nm %d \n", local_pm_i, nm);
          //copy_local_to_pm(nm);
        }
      }
      // Change the particle positions after the zig for the 
      // upcoming zag.
      dx = p_dx;                   // Store new position
      dy = p_dy;
      dz = p_dz;
      /*
      v0 = v3 + local_pm->dispx;   // Streak midpoint
      v1 = v4 + local_pm->dispy;
      v2 = v5 + local_pm->dispz;
      */

      s_dispx = local_pm->dispx;
      s_dispy = local_pm->dispy;
      s_dispz = local_pm->dispz;
      // printf("\nFINISHED MOVE_P\n");
    }
    // Common case (inbnds).  Note: accumulator values are 4 times
    // the total physical charge that passed through the appropriate
    // current quadrant in a time-step
    
    // printf("\n\nParticle %d: xi, yi, zi = %e, %e, %e", p_index, dx, dy, dz);
    // printf("\nParticle %d: xf, yf, zf = %e, %e, %e\n", p_index, dx + 2 * s_dispx, dy + 2 * s_dispy, dz + 2 * s_dispz);

    q *= qsp;
    /*
    p_dx = v3;                                              // Store new position
    p_dy = v4;
    p_dz = v5;
    dx = v0;                                                // Streak midpoint
    dy = v1;
    dz = v2;
    */
    v5 = q*s_dispx*s_dispy*s_dispz*one_third;               // Compute correction

    int axis = 3;
    
    float s_midx = dx + s_dispx;
    float s_midy = dy + s_dispy;
    float s_midz = dz + s_dispz;

    // printf("\nParticle %d: axis, v0, v1, v2, v3 = %d, %e, %e, %e, %e",
    //         p_index, axis, v0, v1, v2, 2.*v3);
    // printf("\nParticle %d: s_midx, s_midy, s_midz = %e, %e, %e",
    //         p_index, s_midx, s_midy, s_midz);
    // printf("\nParticle %d: s_dispx, s_dispy, s_dispz = %e, %e, %e",
    //         p_index, s_dispx, s_dispy, s_dispz);
    
// Create a single accumulate_j macro.
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


    accumulate_j( x,y,z );
    // if (zagger)
    //     printf("\nParticle %d zagging current (x,y,z) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    // else
    //     printf("\nParticle %d depositing (x,y,z) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jx, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jx, 3) += v3;

    accumulate_j( y,z,x );
    // if (zagger)
    //     printf("\nParticle %d zagging current (y,z,x) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    // else
    //     printf("\nParticle %d depositing (y,z,x) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jy, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jy, 3) += v3;

    accumulate_j( z,x,y );
    // if (zagger)
    //     printf("\nParticle %d zagging current (z,x,y) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    // else
    //     printf("\nParticle %d depositing (z,x,y) v0, v1, v2, v3 = %e, %e, %e, %e", p_index, v0, v1, v2, v3);
    k_accumulators_scatter_access(ii, accumulator_var::jz, 0) += v0;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 1) += v1;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 2) += v2;
    k_accumulators_scatter_access(ii, accumulator_var::jz, 3) += v3;

#     undef ACCUMULATE_J

    // Finally, set the final particle position
    p_dx = s_midx + s_dispx;
    p_dy = s_midy + s_dispy;
    p_dz = s_midz + s_dispz;

    // printf("\nParticle %ld final-particle-position xf, yf, zf = %e, %e %e", p_index, p_dx, p_dy, p_dz);
    // printf("\nParticle %ld final-particle-velocity vxf, vyf, vzf = %e, %e %e", p_index, p_ux, p_uy, p_uz);
    // printf("\nEND OF PARTICLE MOTION\n\n");

//}
  });


  // TODO: abstract this manual data copy
  //Kokkos::deep_copy(h_nm, k_nm);

  //args->seg[pipeline_rank].pm        = pm;
  //args->seg[pipeline_rank].max_nm    = max_nm;
  //args->seg[pipeline_rank].nm        = h_nm(0);
  //args->seg[pipeline_rank].n_ignored = 0; // TODO: update this
  //delete(k_local_particle_movers_p);
  //return h_nm(0);

}

void
advance_p( /**/  species_t            * RESTRICT sp,
           /**/  accumulator_array_t  * RESTRICT aa,
           interpolator_array_t * RESTRICT ia ) {
  //DECLARE_ALIGNED_ARRAY( advance_p_pipeline_args_t, 128, args, 1 );
  //DECLARE_ALIGNED_ARRAY( particle_mover_seg_t, 128, seg, MAX_PIPELINE+1 );
  //int rank;

  if( !sp )
  {
    ERROR(( "Bad args" ));
  }
  if( !aa )
  {
    ERROR(( "Bad args" ));
  }
  if( !ia  )
  {
    ERROR(( "Bad args" ));
  }
  if( sp->g!=aa->g )
  {
    ERROR(( "Bad args" ));
  }
  if( sp->g!=ia->g )
  {
    ERROR(( "Bad args" ));
  }


  float qdt_2mc  = (sp->q*sp->g->dt)/(2*sp->m*sp->g->cvac);
  float cdt_dx   = sp->g->cvac*sp->g->dt*sp->g->rdx;
  float cdt_dy   = sp->g->cvac*sp->g->dt*sp->g->rdy;
  float cdt_dz   = sp->g->cvac*sp->g->dt*sp->g->rdz;

  KOKKOS_TIC();
  advance_p_kokkos(
          sp->k_p_d,
          sp->k_p_i_d,
          sp->k_pc_d,
          sp->k_pc_i_d,
          sp->k_pm_d,
          sp->k_pm_i_d,
          aa->k_a_sa,
          ia->k_i_d,
          sp->k_nm_d,
          sp->g->k_neighbor_d,
          sp->g,
          qdt_2mc,
          cdt_dx,
          cdt_dy,
          cdt_dz,
          sp->q,
          aa->na,
          sp->np,
          sp->max_nm,
          sp->g->nx,
          sp->g->ny,
          sp->g->nz
  );
  KOKKOS_TOC( advance_p, 1);

  KOKKOS_TIC();
  // I need to know the number of movers that got populated so I can call the
  // compress. Let's copy it back
  Kokkos::deep_copy(sp->k_nm_h, sp->k_nm_d);
  // TODO: which way round should this copy be?

  //  int nm = sp->k_nm_h(0);

  //  printf("nm = %d \n", nm);

  // Copy particle mirror movers back so we have their data safe. Ready for
  // boundary_p_kokkos
  auto pc_d_subview = Kokkos::subview(sp->k_pc_d, std::make_pair(0, sp->k_nm_h(0)), Kokkos::ALL);
  auto pci_d_subview = Kokkos::subview(sp->k_pc_i_d, std::make_pair(0, sp->k_nm_h(0)));
  auto pc_h_subview = Kokkos::subview(sp->k_pc_h, std::make_pair(0, sp->k_nm_h(0)), Kokkos::ALL);
  auto pci_h_subview = Kokkos::subview(sp->k_pc_i_h, std::make_pair(0, sp->k_nm_h(0)));

  Kokkos::deep_copy(pc_h_subview, pc_d_subview);
  Kokkos::deep_copy(pci_h_subview, pci_d_subview);
  //  Kokkos::deep_copy(sp->k_pc_h, sp->k_pc_d);
  //  Kokkos::deep_copy(sp->k_pc_i_h, sp->k_pc_i_d);

  KOKKOS_TOC( PARTICLE_DATA_MOVEMENT, 1);

  //print_nm(sp->k_pm_d, nm);

/*
  args->p0       = sp->p;
  args->pm       = sp->pm;
  args->a0       = aa->a;
  args->f0       = ia->i;
  args->seg      = seg;
  args->g        = sp->g;

  args->qdt_2mc  = (sp->q*sp->g->dt)/(2*sp->m*sp->g->cvac);
  args->cdt_dx   = sp->g->cvac*sp->g->dt*sp->g->rdx;
  args->cdt_dy   = sp->g->cvac*sp->g->dt*sp->g->rdy;
  args->cdt_dz   = sp->g->cvac*sp->g->dt*sp->g->rdz;
  args->qsp      = sp->q;

  args->np       = sp->np;
  args->max_nm   = sp->max_nm;
  args->nx       = sp->g->nx;
  args->ny       = sp->g->ny;
  args->nz       = sp->g->nz;

  // Have the host processor do the last incomplete bundle if necessary.
  // Note: This is overlapped with the pipelined processing.  As such,
  // it uses an entire accumulator.  Reserving an entire accumulator
  // for the host processor to handle at most 15 particles is wasteful
  // of memory.  It is anticipated that it may be useful at some point
  // in the future have pipelines accumulating currents while the host
  // processor is doing other more substantive work (e.g. accumulating
  // currents from particles received from neighboring nodes).
  // However, it is worth reconsidering this at some point in the
  // future.

  EXEC_PIPELINES( advance_p, args, 0 );
  WAIT_PIPELINES();

  // FIXME: HIDEOUS HACK UNTIL BETTER PARTICLE MOVER SEMANTICS
  // INSTALLED FOR DEALING WITH PIPELINES.  COMPACT THE PARTICLE
  // MOVERS TO ELIMINATE HOLES FROM THE PIPELINING.


  sp->nm = 0;
  for( rank=0; rank<=N_PIPELINE; rank++ ) {
    if( args->seg[rank].n_ignored )
      WARNING(( "Pipeline %i ran out of storage for %i movers",
                rank, args->seg[rank].n_ignored ));
    if( sp->pm+sp->nm != args->seg[rank].pm )
      MOVE( sp->pm+sp->nm, args->seg[rank].pm, args->seg[rank].nm );
    sp->nm += args->seg[rank].nm;
  }
  */
  //sp->nm = nm;
}
