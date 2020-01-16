#ifndef _kokkos_helpers_h_
#define _kokkos_helpers_h_

#include <Kokkos_Core.hpp>
#include <Kokkos_ScatterView.hpp>
#include <iostream>

#include "../material/material.h" // Need material_t

//#include "cnl/all.h"
//using cnl::fixed_point;
#include "half.hpp"
using half_float::half;

// This module implements kokkos macros

#define FIELD_VAR_COUNT 16
#define FIELD_EDGE_COUNT 8
#define PARTICLE_VAR_COUNT 7
#define PARTICLE_MOVER_VAR_COUNT 3
#define ACCUMULATOR_VAR_COUNT 3
#define ACCUMULATOR_ARRAY_LENGTH 4
#define INTERPOLATOR_VAR_COUNT 18
#define MATERIAL_COEFFICIENT_VAR_COUNT 13

#ifdef KOKKOS_ENABLE_CUDA
  #define KOKKOS_SCATTER_DUPLICATED Kokkos::Experimental::ScatterNonDuplicated
  #define KOKKOS_SCATTER_ATOMIC Kokkos::Experimental::ScatterAtomic
  #define KOKKOS_LAYOUT Kokkos::LayoutLeft
#else
  #define KOKKOS_SCATTER_DUPLICATED Kokkos::Experimental::ScatterDuplicated
  #define KOKKOS_SCATTER_ATOMIC Kokkos::Experimental::ScatterNonAtomic
  #define KOKKOS_LAYOUT Kokkos::LayoutRight
#endif

typedef int16_t material_id;

class k_particles_struct {
    public:
        Kokkos::View<float *> dx;
        Kokkos::View<float *> dy;
        Kokkos::View<float *> dz;
        Kokkos::View<float *> ux;
        Kokkos::View<float *> uy;
        Kokkos::View<float *> uz;
        Kokkos::View<float *> w;
        Kokkos::View<int *> i;

        k_particles_struct() {}

        k_particles_struct(int num_particles) :
            dx("Particle dx position", num_particles),
            dy("Particle dy position", num_particles),
            dz("Particle dz position", num_particles),
            ux("Particle ux momentum", num_particles),
            uy("Particle uy momentum", num_particles),
            uz("Particle uz momentum", num_particles),
            w("Particle weight", num_particles),
            i("Particle index", num_particles){}
};

class k_particles_host_struct {
    public:
        Kokkos::View<float *>::HostMirror dx;
        Kokkos::View<float *>::HostMirror dy;
        Kokkos::View<float *>::HostMirror dz;
        Kokkos::View<float *>::HostMirror ux;
        Kokkos::View<float *>::HostMirror uy;
        Kokkos::View<float *>::HostMirror uz;
        Kokkos::View<float*>::HostMirror w;
        Kokkos::View<int*>::HostMirror i;

        k_particles_host_struct() {}

        k_particles_host_struct(k_particles_struct& particles) {
            dx = Kokkos::create_mirror_view(particles.dx);
            dy = Kokkos::create_mirror_view(particles.dy);
            dz = Kokkos::create_mirror_view(particles.dz);
            ux = Kokkos::create_mirror_view(particles.ux);
            uy = Kokkos::create_mirror_view(particles.uy);
            uz = Kokkos::create_mirror_view(particles.uz);
            w = Kokkos::create_mirror_view(particles.w);
            i = Kokkos::create_mirror_view(particles.i);
        }

//        k_particles_host_struct(int num_particles) {
//            pos("Particle positions", num_particles),
//            mom("Particle momentum", num_particles),
//            w("Particle weight", num_particles),
//            i("Particle index", num_particles){}
//        }
};

using k_particles_soa_t = k_particles_struct;
using k_particles_host_soa_t = k_particles_host_struct;

// TODO: we dont need the [1] here
using k_iterator_t = Kokkos::View<int[1]>;

using k_field_t = Kokkos::View<float *[FIELD_VAR_COUNT]>;
using k_field_edge_t = Kokkos::View<material_id* [FIELD_EDGE_COUNT]>;

// TODO Consolidate using Cabana?
//using k_particles_t = Kokkos::View<half *[PARTICLE_VAR_COUNT]>;
using k_particles_pos_t = Kokkos::View<half *[3]>;
using k_particles_mom_t = Kokkos::View<float *[3]>;
using k_particles_w_t = Kokkos::View<float *>;
using k_particles_t = Kokkos::View<float *[PARTICLE_VAR_COUNT]>;
using k_particles_i_t = Kokkos::View<int*>;

// TODO: think about the layout here
//using k_particle_copy_t = Kokkos::View<half *[PARTICLE_VAR_COUNT], Kokkos::LayoutRight>;
using k_particles_pos_copy_t = Kokkos::View<half *[3]>;
using k_particles_mom_copy_t = Kokkos::View<float *[3]>;
using k_particles_w_copy_t = Kokkos::View<float *>;
using k_particle_copy_t = Kokkos::View<float *[PARTICLE_VAR_COUNT], Kokkos::LayoutRight>;
using k_particle_i_copy_t = Kokkos::View<int*, Kokkos::LayoutRight>;

using k_particle_movers_t = Kokkos::View<float *[PARTICLE_MOVER_VAR_COUNT]>;
using k_particle_i_movers_t = Kokkos::View<int*>;

using k_neighbor_t = Kokkos::View<int64_t*>;

using k_interpolator_t = Kokkos::View<float *[INTERPOLATOR_VAR_COUNT]>;

using k_accumulators_t = Kokkos::View<float *[ACCUMULATOR_VAR_COUNT][ACCUMULATOR_ARRAY_LENGTH]>;

using k_accumulators_sa_t = Kokkos::Experimental::ScatterView<float *[ACCUMULATOR_VAR_COUNT][ACCUMULATOR_ARRAY_LENGTH]>;

using k_accumulators_sah_t = Kokkos::Experimental::ScatterView<float *[ACCUMULATOR_VAR_COUNT][ACCUMULATOR_ARRAY_LENGTH], Kokkos::LayoutRight, Kokkos::HostSpace, Kokkos::Experimental::ScatterSum, Kokkos::Experimental::ScatterDuplicated, Kokkos::Experimental::ScatterNonAtomic>;

using static_sched = Kokkos::Schedule<Kokkos::Static>;
using host_execution_policy = Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace, static_sched, int>;

using k_material_coefficient_t = Kokkos::View<float* [MATERIAL_COEFFICIENT_VAR_COUNT]>;

using k_field_sa_t = Kokkos::Experimental::ScatterView<float *[FIELD_VAR_COUNT]>;

#define KOKKOS_TEAM_POLICY_DEVICE  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>
#define KOKKOS_TEAM_POLICY_HOST  Kokkos::TeamPolicy<Kokkos::DefaultHostExecutionSpace>

namespace field_var {
  enum f_v {
    ex        = 0,
    ey        = 1,
    ez        = 2,
    div_e_err = 3,
    cbx       = 4,
    cby       = 5,
    cbz       = 6,
    div_b_err = 7,
    tcax      = 8,
    tcay      = 9,
    tcaz      = 10,
    rhob      = 11,
    jfx       = 12,
    jfy       = 13,
    jfz       = 14,
    rhof      = 15
  };
};
namespace field_edge_var { \
  enum f_e_v {
    ematx = 0,
    ematy = 1,
    ematz = 2,
    nmat  = 3,
    fmatx = 4,
    fmaty = 5,
    fmatz = 6,
    cmat  = 7
  };
};

namespace interpolator_var {
  enum i_r {
    ex       = 0,
    dexdy    = 1,
    dexdz    = 2,
    d2exdydz = 3,
    ey       = 4,
    deydz    = 5,
    deydx    = 6,
    d2eydzdx = 7,
    ez       = 8,
    dezdx    = 9,
    dezdy    = 10,
    d2ezdxdy = 11,
    cbx      = 12,
    dcbxdx   = 13,
    cby      = 14,
    dcbydy   = 15,
    cbz      = 16,
    dcbzdz   = 17
  };
};

namespace particle_pos_var {
  enum pos_v {
    dx = 0,
    dy,
    dz,
  };
};

namespace particle_mom_var {
  enum mom_v {
    ux = 0,
    uy,
    uz,
  };
};

namespace particle_var {
  enum p_v {
    dx = 0,
    dy,
    dz,
    //pi = 3,
    ux,
    uy,
    uz,
    w,
  };
};

namespace particle_mover_var {
  enum p_m_v {
     dispx = 0,
     dispy = 1,
     dispz = 2,
     //pmi   = 3,
  };
};

namespace accumulator_var {
  enum a_v { 
    jx = 0, 
    jy = 1, 
    jz = 2, 
  };
};

namespace material_coeff_var {
    enum mc_v {
        decayx        = 0,
        drivex        = 1,
        decayy        = 2,
        drivey        = 3,
        decayz        = 4,
        drivez        = 5,
        rmux          = 6,
        rmuy          = 7,
        rmuz          = 8,
        nonconductive = 9,
        epsx          = 10,
        epsy          = 11,
        epsz          = 12,
    };
};

void print_particles_d(
        k_particles_t particles,
        int np
        );
void print_accumulator(k_accumulators_t fields, int n);

// The templating here is to defer the type until later in the head include chain
template <class P>
bool compareParticleMovers(P& a, P& b) {
    return a.i < b.i;
}


#endif // _kokkos_helpers_h_
