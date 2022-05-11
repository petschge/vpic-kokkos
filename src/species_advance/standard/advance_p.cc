// FIXME: PARTICLE MOVERS NEED TO BE OVERALLOCATED IN STRUCTORS TO
// ACCOUNT FOR SPLITTING THE MOVER ARRAY BETWEEN HOST AND PIPELINES

#define IN_spa
#define HAS_V4_PIPELINE
#include <stdio.h>
#include "spa_private.h"
#include "../../vpic/kokkos_helpers.h"
#include "../../vpic/kokkos_tuning.hpp"

template<class CurrentScatterAccess>
void KOKKOS_INLINE_FUNCTION
accumulate_current(CurrentScatterAccess& current_sa, int ii,
                   const int nx, const int ny, const int nz, 
                   const float cx, const float cy, const float cz, 
                   const float v0, const float v1, const float v2, const float v3,
                   const float v4, const float v5, const float v6, const float v7,
                   const float v8, const float v9, const float v10, const float v11) {
#ifdef VPIC_ENABLE_ACCUMULATORS
  current_sa(ii, 0)  += cx*v0;
  current_sa(ii, 1)  += cx*v1;
  current_sa(ii, 2)  += cx*v2;
  current_sa(ii, 3)  += cx*v3;
  
  current_sa(ii, 4)  += cy*v4;
  current_sa(ii, 5)  += cy*v5;
  current_sa(ii, 6)  += cy*v6;
  current_sa(ii, 7)  += cy*v7;
  
  current_sa(ii, 8)  += cz*v8;
  current_sa(ii, 9)  += cz*v9;
  current_sa(ii, 10) += cz*v10;
  current_sa(ii, 11) += cz*v11;
#else
  int iii = ii;
  int zi = iii/((nx+2)*(ny+2));
  iii -= zi*(nx+2)*(ny+2);
  int yi = iii/(nx+2);
  int xi = iii - yi*(nx+2);
  
  current_sa(ii, field_var::jfx)                           += cx*v0;
  current_sa(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx)   += cx*v1;
  current_sa(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx)   += cx*v2;
  current_sa(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx) += cx*v3;
  
  current_sa(ii, field_var::jfy)                           += cy*v4;
  current_sa(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy)   += cy*v5;
  current_sa(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy)   += cy*v6;
  current_sa(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v7;
  
  current_sa(ii, field_var::jfz)                           += cz*v8;
  current_sa(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz)   += cz*v9;
  current_sa(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz)   += cz*v10;
  current_sa(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v11;
#endif
}

template<class TeamMember, class CurrentScatterAccess>
void
KOKKOS_INLINE_FUNCTION
reduce_and_accumulate_current(TeamMember& team_member, CurrentScatterAccess& access, 
                              const int num_iters, const int ii, 
                              const int nx, const int ny, const int nz, 
                              const float cx, const float cy, const float cz, 
                              float *v0, float *v1,  float *v2,  float *v3,
                              float *v4, float *v5,  float *v6,  float *v7,
                              float *v8, float *v9,  float *v10, float *v11) {

#ifdef VPIC_ENABLE_VECTORIZATION
  float val0 = 0.0, val1 = 0.0, val2 = 0.0, val3 = 0.0;
  float val4 = 0.0, val5 = 0.0, val6 = 0.0, val7 = 0.0;
  float val8 = 0.0, val9 = 0.0, val10 = 0.0, val11 = 0.0;
  #pragma omp simd reduction(+:val0,val1,val2,val3,val4,val5,val6,val8,val9,val10,val11)
  for(int lane=0; lane<num_iters; lane++) {
    val0  += v0[lane];
    val1  += v1[lane];
    val2  += v2[lane];
    val3  += v3[lane];
    val4  += v4[lane];
    val5  += v5[lane];
    val6  += v6[lane];
    val7  += v7[lane];
    val8  += v8[lane];
    val9  += v9[lane];
    val10 += v10[lane];
    val11 += v11[lane];
  }
  accumulate_current(access, ii, nx, ny, nz, cx, cy, cz, 
                     val0, val1, val2, val3, 
                     val4, val5, val6, val7, 
                     val8, val9, val10, val11);
#elif defined( __CUDA_ARCH__ )
  int mask = 0xffffffff;
  for(int i=16; i>0; i=i/2) {
    v0[0]  += __shfl_down_sync(mask, v0[0],  i);
    v1[0]  += __shfl_down_sync(mask, v1[0],  i);
    v2[0]  += __shfl_down_sync(mask, v2[0],  i);
    v3[0]  += __shfl_down_sync(mask, v3[0],  i);
    v4[0]  += __shfl_down_sync(mask, v4[0],  i);
    v5[0]  += __shfl_down_sync(mask, v5[0],  i);
    v6[0]  += __shfl_down_sync(mask, v6[0],  i);
    v7[0]  += __shfl_down_sync(mask, v7[0],  i);
    v8[0]  += __shfl_down_sync(mask, v8[0],  i);
    v9[0]  += __shfl_down_sync(mask, v9[0],  i);
    v10[0] += __shfl_down_sync(mask, v10[0], i);
    v11[0] += __shfl_down_sync(mask, v11[0], i);
  }
  if(team_member.team_rank()%32 == 0) {
    accumulate_current(access, ii, nx, ny, nz, cx, cy, cz, 
                       v0[0], v1[0], v2[0], v3[0], 
                       v4[0], v5[0], v6[0], v7[0], 
                       v8[0], v9[0], v10[0], v11[0]);
  }
#else
  team_member.team_reduce(Kokkos::Sum<float>(v0[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v1[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v2[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v3[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v4[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v5[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v6[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v7[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v8[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v9[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v10[0]));
  team_member.team_reduce(Kokkos::Sum<float>(v11[0]));
  if(team_member.team_rank() == 0) {
    accumulate_current(access, ii, nx, ny, nz, cx, cy, cz, 
                       v0[0], v1[0], v2[0], v3[0], 
                       v4[0], v5[0], v6[0], v7[0], 
                       v8[0], v9[0], v10[0], v11[0]);
  }
#endif
}

template<class TeamMember, class field_sa_t, class field_var>
void
KOKKOS_INLINE_FUNCTION
contribute_current(TeamMember& team_member, field_sa_t& access, int i0, int i1, int i2, int i3, field_var j, float v0, float v1,  float v2, float v3) {
#ifdef __CUDA_ARCH__
  int mask = 0xffffffff;
  int team_rank = team_member.team_rank();
  for(int i=16; i>0; i=i/2) {
    v0 += __shfl_down_sync(mask, v0, i);
    v1 += __shfl_down_sync(mask, v1, i);
    v2 += __shfl_down_sync(mask, v2, i);
    v3 += __shfl_down_sync(mask, v3, i);
  }
  if(team_rank%32 == 0) {
    access(i0, j) += v0;
    access(i1, j) += v1;
    access(i2, j) += v2;
    access(i3, j) += v3;
  }
#else
  team_member.team_reduce(Kokkos::Sum<float>(v0));
  team_member.team_reduce(Kokkos::Sum<float>(v1));
  team_member.team_reduce(Kokkos::Sum<float>(v2));
  team_member.team_reduce(Kokkos::Sum<float>(v3));
  if(team_member.team_rank() == 0) {
    access(i0, j) += v0;
    access(i1, j) += v1;
    access(i2, j) += v2;
    access(i3, j) += v3;
  }
#endif
}

template<class TeamMember, class IndexView, class BoundsView>
int KOKKOS_INLINE_FUNCTION particles_in_same_cell(TeamMember& team_member, IndexView& ii, BoundsView& inbnds, const int num_lanes) {
#ifdef USE_GPU
  int min_inbnds = inbnds[0];
  int max_inbnds = inbnds[0];
  team_member.team_reduce(Kokkos::Max<int>(min_inbnds));
  team_member.team_reduce(Kokkos::Min<int>(max_inbnds));
  int min_index = ii[0];
  int max_index = ii[0];
  team_member.team_reduce(Kokkos::Max<int>(max_index));
  team_member.team_reduce(Kokkos::Min<int>(min_index));
  return min_inbnds == max_inbnds && min_index == max_index;
#else
  for(int lane=0; lane<num_lanes; lane++) {
    if(ii[0] != ii[lane] || inbnds[0] != inbnds[lane])
      return 0;
  }
  return 1;
#endif
}

KOKKOS_INLINE_FUNCTION
void load_particles(float* dx, float* dy, float* dz, float* ux, float* uy, float* uz, float* q, int* ii,
                    const k_particles_t& k_particles, 
                    const k_particles_i_t& k_particles_i, 
                    int num_lanes, int chunk) {
  #pragma omp simd 
  for(int lane=0; lane<num_lanes; lane++) {
    size_t p_index = chunk*num_lanes + lane;
    // Load position
    dx[lane] = k_particles(p_index, particle_var::dx);
    dy[lane] = k_particles(p_index, particle_var::dy);
    dz[lane] = k_particles(p_index, particle_var::dz);
    // Load momentum
    ux[lane] = k_particles(p_index, particle_var::ux);
    uy[lane] = k_particles(p_index, particle_var::uy);
    uz[lane] = k_particles(p_index, particle_var::uz);
    // Load weight
    q[lane] = k_particles(p_index, particle_var::w);
    ii[lane] = k_particles_i(p_index);
  }
}

KOKKOS_INLINE_FUNCTION
void simd_load_interpolator_var(float* v0, const int ii, const k_interpolator_t& k_interp, int len) {
  #pragma omp simd
  for(int i=0; i<len; i++) {
    v0[i] = k_interp(ii, i);
  }
}

template<typename TeamMember>
KOKKOS_INLINE_FUNCTION
void simd_load_interpolator_var(TeamMember& team_member, float* v0, const int ii, const k_interpolator_t& k_interp, int len) {
  Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, len), [&] (const int i) {
    v0[i] = k_interp(ii,i);
  });
}


// Template for unrolling a loop in reverse order
// Necessary to avoid the looping/pointer overhead when loading interpolator data
template<int N>
KOKKOS_INLINE_FUNCTION
void unrolled_simd_load(float* vals, const int* ii, const k_interpolator_t& k_interp, int len) {
  simd_load_interpolator_var(vals+(N-1)*18, ii[N-1], k_interp, len);
  unrolled_simd_load<N-1>(vals, ii, k_interp, len);
}
template<>
KOKKOS_INLINE_FUNCTION
void unrolled_simd_load<0>(float* vals, const int* ii, const k_interpolator_t& k_interp, int len) {}

template<typename TeamMember>
KOKKOS_INLINE_FUNCTION
void unrolled_simd_load(TeamMember& team_member, float* vals, const int* ii, const k_interpolator_t& k_interp, int len, int N) {
  for(int i=0; i<N; i++) { 
    simd_load_interpolator_var(team_member, vals+i*18, ii[i], k_interp, len);
  }
}

template<int NumLanes>
KOKKOS_INLINE_FUNCTION
void load_interpolators(
                        float* fex,
                        float* fdexdy,
                        float* fdexdz,
                        float* fd2exdydz,
                        float* fey,
                        float* fdeydz,
                        float* fdeydx,
                        float* fd2eydzdx,
                        float* fez,
                        float* fdezdx,
                        float* fdezdy,
                        float* fd2ezdxdy,
                        float* fcbx,
                        float* fdcbxdx,
                        float* fcby,
                        float* fdcbydy,
                        float* fcbz,
                        float* fdcbzdz,
                        const int* ii,
                        const k_interpolator_t& k_interp
                        ) {
  #define f_cbx k_interp(ii[LANE], interpolator_var::cbx)
  #define f_cby k_interp(ii[LANE], interpolator_var::cby)
  #define f_cbz k_interp(ii[LANE], interpolator_var::cbz)
  #define f_ex  k_interp(ii[LANE], interpolator_var::ex)
  #define f_ey  k_interp(ii[LANE], interpolator_var::ey)
  #define f_ez  k_interp(ii[LANE], interpolator_var::ez)

  #define f_dexdy    k_interp(ii[LANE], interpolator_var::dexdy)
  #define f_dexdz    k_interp(ii[LANE], interpolator_var::dexdz)

  #define f_d2exdydz k_interp(ii[LANE], interpolator_var::d2exdydz)
  #define f_deydx    k_interp(ii[LANE], interpolator_var::deydx)
  #define f_deydz    k_interp(ii[LANE], interpolator_var::deydz)

  #define f_d2eydzdx k_interp(ii[LANE], interpolator_var::d2eydzdx)
  #define f_dezdx    k_interp(ii[LANE], interpolator_var::dezdx)
  #define f_dezdy    k_interp(ii[LANE], interpolator_var::dezdy)

  #define f_d2ezdxdy k_interp(ii[LANE], interpolator_var::d2ezdxdy)
  #define f_dcbxdx   k_interp(ii[LANE], interpolator_var::dcbxdx)
  #define f_dcbydy   k_interp(ii[LANE], interpolator_var::dcbydy)
  #define f_dcbzdz   k_interp(ii[LANE], interpolator_var::dcbzdz)

#if !defined( VPIC_ENABLE_VECTORIZATION ) || defined( USE_GPU )
  for(int lane=0; lane<NumLanes; lane++) {
    // Load interpolators
    fex[LANE]       = f_ex;     
    fdexdy[LANE]    = f_dexdy;  
    fdexdz[LANE]    = f_dexdz;  
    fd2exdydz[LANE] = f_d2exdydz;
    fey[LANE]       = f_ey;     
    fdeydz[LANE]    = f_deydz;  
    fdeydx[LANE]    = f_deydx;  
    fd2eydzdx[LANE] = f_d2eydzdx;
    fez[LANE]       = f_ez;     
    fdezdx[LANE]    = f_dezdx;  
    fdezdy[LANE]    = f_dezdy;  
    fd2ezdxdy[LANE] = f_d2ezdxdy;
    fcbx[LANE]      = f_cbx;    
    fdcbxdx[LANE]   = f_dcbxdx; 
    fcby[LANE]      = f_cby;    
    fdcbydy[LANE]   = f_dcbydy; 
    fcbz[LANE]      = f_cbz;    
    fdcbzdz[LANE]   = f_dcbzdz; 
  }
#else
  int first = ii[0];
  int index = first;
  #pragma omp simd reduction(&:index)
  for(int lane=0; lane<NumLanes; lane++) {
    index &= ii[lane];
  }

  if(ii[0] == index) {
    float vals[18];

    simd_load_interpolator_var(vals, ii[0], k_interp, 18);
    #pragma omp simd
    for(int i=0; i<NumLanes; i++) {
      fex[i]       = vals[0];
      fdexdy[i]    = vals[1];
      fdexdz[i]    = vals[2];
      fd2exdydz[i] = vals[3];
      fey[i]       = vals[4];
      fdeydz[i]    = vals[5];
      fdeydx[i]    = vals[6];
      fd2eydzdx[i] = vals[7];
      fez[i]       = vals[8];
      fdezdx[i]    = vals[9];
      fdezdy[i]    = vals[10];
      fd2ezdxdy[i] = vals[11];
      fcbx[i]      = vals[12];
      fdcbxdx[i]   = vals[13];
      fcby[i]      = vals[14];
      fdcbydy[i]   = vals[15];
      fcbz[i]      = vals[16];
      fdcbzdz[i]   = vals[17];
    }
  } else {

    float vals[18*NumLanes];
    unrolled_simd_load<NumLanes>(vals, ii, k_interp, 18);

    #pragma omp simd
    for(int i=0; i<NumLanes; i++) {
      fex[i]       = vals[18*i];
      fdexdy[i]    = vals[1+18*i];
      fdexdz[i]    = vals[2+18*i];
      fd2exdydz[i] = vals[3+18*i];
      fey[i]       = vals[4+18*i];
      fdeydz[i]    = vals[5+18*i];
      fdeydx[i]    = vals[6+18*i];
      fd2eydzdx[i] = vals[7+18*i];
      fez[i]       = vals[8+18*i];
      fdezdx[i]    = vals[9+18*i];
      fdezdy[i]    = vals[10+18*i];
      fd2ezdxdy[i] = vals[11+18*i];
      fcbx[i]      = vals[12+18*i];
      fdcbxdx[i]   = vals[13+18*i];
      fcby[i]      = vals[14+18*i];
      fdcbydy[i]   = vals[15+18*i];
      fcbz[i]      = vals[16+18*i];
      fdcbzdz[i]   = vals[17+18*i];
    }
  }
#endif

  #undef f_cbx 
  #undef f_cby 
  #undef f_cbz 
  #undef f_ex  
  #undef f_ey  
  #undef f_ez  

  #undef f_dexdy    
  #undef f_dexdz    

  #undef f_d2exdydz 
  #undef f_deydx    
  #undef f_deydz    

  #undef f_d2eydzdx 
  #undef f_dezdx    
  #undef f_dezdy    

  #undef f_d2ezdxdy 
  #undef f_dcbxdx   
  #undef f_dcbydy   
  #undef f_dcbzdz   
}

template<typename ScatterAccess>
KOKKOS_INLINE_FUNCTION
void simd_update_accumulators( ScatterAccess& access, int num_lanes, int ii, float* v0) {
  float* a0  = &((access(ii, 0)).value);
  #pragma omp simd
  for(int lane=0; lane<num_lanes; lane++) {
    *(a0+lane) += v0[lane];
  }
}

void
advance_p_kokkos_cpu(
        k_particles_t& k_particles,
        k_particles_i_t& k_particles_i,
        k_particle_copy_t& k_particle_copy,
        k_particle_i_copy_t& k_particle_i_copy,
        k_particle_movers_t& k_particle_movers,
        k_particle_i_movers_t& k_particle_movers_i,
//        k_accumulators_sa_t k_accumulators_sa,
        k_field_sa_t k_f_sa,
        k_interpolator_t& k_interp,
        //k_particle_movers_t k_local_particle_movers,
        k_counter_t& k_nm,
        k_neighbor_t& k_neighbors,
        field_array_t* RESTRICT fa,
        const grid_t *g,
        const float qdt_2mc,
        const float cdt_dx,
        const float cdt_dy,
        const float cdt_dz,
        const float qsp,
//        const int na,
        const int np,
        const int max_nm,
        const int nx,
        const int ny,
        const int nz)
{

  constexpr float one            = 1.;
  constexpr float one_third      = 1./3.;
  constexpr float two_fifteenths = 2./15.;

  k_field_t k_field = fa->k_f_d;
  k_field_sa_t k_f_sv = Kokkos::Experimental::create_scatter_view<>(k_field);
  float cx = 0.25 * g->rdy * g->rdz / g->dt;
  float cy = 0.25 * g->rdz * g->rdx / g->dt;
  float cz = 0.25 * g->rdx * g->rdy / g->dt;

  #define p_dx    k_particles(p_index, particle_var::dx)
  #define p_dy    k_particles(p_index, particle_var::dy)
  #define p_dz    k_particles(p_index, particle_var::dz)
  #define p_ux    k_particles(p_index, particle_var::ux)
  #define p_uy    k_particles(p_index, particle_var::uy)
  #define p_uz    k_particles(p_index, particle_var::uz)
  #define p_w     k_particles(p_index, particle_var::w)
  #define pii     k_particles_i(p_index)

  #define f_cbx k_interp(ii[lane], interpolator_var::cbx)
  #define f_cby k_interp(ii[lane], interpolator_var::cby)
  #define f_cbz k_interp(ii[lane], interpolator_var::cbz)
  #define f_ex  k_interp(ii[lane], interpolator_var::ex)
  #define f_ey  k_interp(ii[lane], interpolator_var::ey)
  #define f_ez  k_interp(ii[lane], interpolator_var::ez)

  #define f_dexdy    k_interp(ii[lane], interpolator_var::dexdy)
  #define f_dexdz    k_interp(ii[lane], interpolator_var::dexdz)

  #define f_d2exdydz k_interp(ii[lane], interpolator_var::d2exdydz)
  #define f_deydx    k_interp(ii[lane], interpolator_var::deydx)
  #define f_deydz    k_interp(ii[lane], interpolator_var::deydz)

  #define f_d2eydzdx k_interp(ii[lane], interpolator_var::d2eydzdx)
  #define f_dezdx    k_interp(ii[lane], interpolator_var::dezdx)
  #define f_dezdy    k_interp(ii[lane], interpolator_var::dezdy)

  #define f_d2ezdxdy k_interp(ii[lane], interpolator_var::d2ezdxdy)
  #define f_dcbxdx   k_interp(ii[lane], interpolator_var::dcbxdx)
  #define f_dcbydy   k_interp(ii[lane], interpolator_var::dcbydy)
  #define f_dcbzdz   k_interp(ii[lane], interpolator_var::dcbzdz)

  int maxi = k_field.extent(0);
  Kokkos::View<float*[12], Kokkos::LayoutRight> accumulator("Accumulator", maxi);
  Kokkos::deep_copy(accumulator, 0);
  auto accum_sv = Kokkos::Experimental::create_scatter_view(accumulator);

  auto rangel = g->rangel;
  auto rangeh = g->rangeh;

  // TODO: is this the right place to do this?
  Kokkos::deep_copy(k_nm, 0);

//  int num_leagues = g->nv/4;
  int num_leagues = 1;
  constexpr int num_lanes = 32;
  constexpr int chunk_size = 1;

  int num_chunks = np/num_lanes;
  int chunks_per_league = num_chunks/num_leagues;

//  Kokkos::TeamPolicy<> policy = Kokkos::TeamPolicy<>(num_chunks, 1, num_lanes).set_scratch_size(1, Kokkos::PerTeam(39*4*num_lanes));
  Kokkos::TeamPolicy<> policy = Kokkos::TeamPolicy<>(num_chunks, 1, num_lanes);
  typedef Kokkos::DefaultExecutionSpace::scratch_memory_space ScratchSpace;
  typedef Kokkos::View<float[num_lanes],ScratchSpace,Kokkos::MemoryTraits<Kokkos::Unmanaged>> shared_float;
  typedef Kokkos::View<int[num_lanes],ScratchSpace,Kokkos::MemoryTraits<Kokkos::Unmanaged>> shared_int;

  Kokkos::parallel_for("advance_p", policy, 
  KOKKOS_LAMBDA(const KOKKOS_TEAM_POLICY_DEVICE::member_type team_member) {
//if(team_member.league_rank() == 0 && team_member.team_rank() == 0)
//printf("(League size, team size, vector len): (%d,%d,%d)\n", team_member.league_size(), team_member.team_size(), policy.impl_vector_length());
    size_t chunk_offset = team_member.league_rank();

//printf("Num leagues: %d, team size: %d, chunks per league: %d\n", team_member.league_size(), team_member.team_size(), chunks_per_league);
 
//    auto k_field_scatter_access = k_f_sv.access();
//    Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, chunks_per_league), [=] (size_t chunk_offset) {
//    Kokkos::parallel_for("push particles", Kokkos::RangePolicy<>(0, chunks_per_league), KOKKOS_LAMBDA(const size_t chunk_offset) {

//for(size_t chunk_offset = 0; chunk_offset<chunks_per_league; chunk_offset++) {
      float v0[num_lanes];
      float v1[num_lanes];
      float v2[num_lanes];
      float v3[num_lanes];
      float v4[num_lanes];
      float v5[num_lanes];
      float dx[num_lanes];
      float dy[num_lanes];
      float dz[num_lanes];
      float ux[num_lanes];
      float uy[num_lanes];
      float uz[num_lanes];
      float hax[num_lanes];
      float hay[num_lanes];
      float haz[num_lanes];
      float cbx[num_lanes];
      float cby[num_lanes];
      float cbz[num_lanes];
      float q[num_lanes];
      int   ii[num_lanes];
      int   inbnds[num_lanes];

      float fcbx[num_lanes];
      float fcby[num_lanes];
      float fcbz[num_lanes];
      float fex[num_lanes];
      float fey[num_lanes];
      float fez[num_lanes];
      float fdexdy[num_lanes];
      float fdexdz[num_lanes];
      float fd2exdydz[num_lanes];
      float fdeydx[num_lanes];
      float fdeydz[num_lanes];
      float fd2eydzdx[num_lanes];
      float fdezdx[num_lanes];
      float fdezdy[num_lanes];
      float fd2ezdxdy[num_lanes];
      float fdcbxdx[num_lanes];
      float fdcbydy[num_lanes];
      float fdcbzdz[num_lanes];

//      shared_float v0(team_member.thread_scratch(1));
//      shared_float v1(team_member.thread_scratch(1));
//      shared_float v2(team_member.thread_scratch(1));
//      shared_float v3(team_member.thread_scratch(1));
//      shared_float v4(team_member.thread_scratch(1));
//      shared_float v5(team_member.thread_scratch(1));
//      shared_float dx(team_member.thread_scratch(1));
//      shared_float dy(team_member.thread_scratch(1));
//      shared_float dz(team_member.thread_scratch(1));
//      shared_float ux(team_member.thread_scratch(1));
//      shared_float uy(team_member.thread_scratch(1));
//      shared_float uz(team_member.thread_scratch(1));
//      shared_float hax(team_member.thread_scratch(1));
//      shared_float hay(team_member.thread_scratch(1));
//      shared_float haz(team_member.thread_scratch(1));
//      shared_float cbx(team_member.thread_scratch(1));
//      shared_float cby(team_member.thread_scratch(1));
//      shared_float cbz(team_member.thread_scratch(1));
//      shared_float q(team_member.thread_scratch(1));
//      shared_int   ii(team_member.thread_scratch(1));
//      shared_int   inbnds(team_member.thread_scratch(1));
//
//      shared_float fcbx(team_member.thread_scratch(1));
//      shared_float fcby(team_member.thread_scratch(1));
//      shared_float fcbz(team_member.thread_scratch(1));
//      shared_float fex(team_member.thread_scratch(1));
//      shared_float fey(team_member.thread_scratch(1));
//      shared_float fez(team_member.thread_scratch(1));
//      shared_float fdexdy(team_member.thread_scratch(1));
//      shared_float fdexdz(team_member.thread_scratch(1));
//      shared_float fd2exdydz(team_member.thread_scratch(1));
//      shared_float fdeydx(team_member.thread_scratch(1));
//      shared_float fdeydz(team_member.thread_scratch(1));
//      shared_float fd2eydzdx(team_member.thread_scratch(1));
//      shared_float fdezdx(team_member.thread_scratch(1));
//      shared_float fdezdy(team_member.thread_scratch(1));
//      shared_float fd2ezdxdy(team_member.thread_scratch(1));
//      shared_float fdcbxdx(team_member.thread_scratch(1));
//      shared_float fdcbydy(team_member.thread_scratch(1));
//      shared_float fdcbzdz(team_member.thread_scratch(1));

      auto accum_sa = accum_sv.access();
      auto k_field_scatter_access = k_f_sv.access();
      size_t chunk = chunk_offset;

//#pragma omp simd 
//      for(int lane=0; lane<num_lanes; lane++) {
//        size_t p_index = chunk*num_lanes + lane;
//        // Load position
//        dx[lane] = p_dx;
//        dy[lane] = p_dy;
//        dz[lane] = p_dz;
//  
//        // Load momentum
//        ux[lane] = p_ux;
//        uy[lane] = p_uy;
//        uz[lane] = p_uz;
//        q[lane] = p_w;
//
//        // Load index
//        ii[lane] = pii;
//      }
//      load_particles(dx, dy, dz, ux, uy, uz, q, ii,
//                     k_particles, k_particles_i, 
//                     num_lanes, chunk);
      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
        size_t p_index = chunk*num_lanes + lane;
        // Load position
        dx[lane] = p_dx;
        dy[lane] = p_dy;
        dz[lane] = p_dz;
        // Load momentum
        ux[lane] = p_ux;
        uy[lane] = p_uy;
        uz[lane] = p_uz;
        // Load weight
        q[lane]  = p_w;
        // Load index
        ii[lane] = pii;
      });

//#pragma omp simd 
//      for(int lane=0; lane<num_lanes; lane++) {
//        // Load interpolators
//        fex[lane]       = f_ex;     
//        fdexdy[lane]    = f_dexdy;  
//        fdexdz[lane]    = f_dexdz;  
//        fd2exdydz[lane] = f_d2exdydz;
//        fey[lane]       = f_ey;     
//        fdeydz[lane]    = f_deydz;  
//        fdeydx[lane]    = f_deydx;  
//        fd2eydzdx[lane] = f_d2eydzdx;
//        fez[lane]       = f_ez;     
//        fdezdx[lane]    = f_dezdx;  
//        fdezdy[lane]    = f_dezdy;  
//        fd2ezdxdy[lane] = f_d2ezdxdy;
//        fcbx[lane]      = f_cbx;    
//        fdcbxdx[lane]   = f_dcbxdx; 
//        fcby[lane]      = f_cby;    
//        fdcbydy[lane]   = f_dcbydy; 
//        fcbz[lane]      = f_cbz;    
//        fdcbzdz[lane]   = f_dcbzdz; 
//      }
      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
        size_t p_index = chunk*num_lanes + lane;
//if(p_index < np) {
        // Load interpolators
        fex[lane]       = f_ex;     
        fdexdy[lane]    = f_dexdy;  
        fdexdz[lane]    = f_dexdz;  
        fd2exdydz[lane] = f_d2exdydz;
        fey[lane]       = f_ey;     
        fdeydz[lane]    = f_deydz;  
        fdeydx[lane]    = f_deydx;  
        fd2eydzdx[lane] = f_d2eydzdx;
        fez[lane]       = f_ez;     
        fdezdx[lane]    = f_dezdx;  
        fdezdy[lane]    = f_dezdy;  
        fd2ezdxdy[lane] = f_d2ezdxdy;
        fcbx[lane]      = f_cbx;    
        fdcbxdx[lane]   = f_dcbxdx; 
        fcby[lane]      = f_cby;    
        fdcbydy[lane]   = f_dcbydy; 
        fcbz[lane]      = f_cbz;    
        fdcbzdz[lane]   = f_dcbzdz; 
//}
      });

//      load_interpolators<num_lanes>(
//                         fex,
//                         fdexdy,
//                         fdexdz,
//                         fd2exdydz,
//                         fey,
//                         fdeydz,
//                         fdeydx,
//                         fd2eydzdx,
//                         fez,
//                         fdezdx,
//                         fdezdy,
//                         fd2ezdxdy,
//                         fcbx,
//                         fdcbxdx,
//                         fcby,
//                         fdcbydy,
//                         fcbz,
//                         fdcbzdz,
//                         ii,
//                         k_interp,
//                         num_lanes,
//                         team_member
//                         );

//      advance_pos(dx, dy, dz, ux, uy, uz, ii, 
//                  v0, v1, v2, v3, v4, v5, inbnds,
//                  hax, hay, haz, 
//                  cbx, cby, cbz, 
//                  fex, fey, fez, 
//                  fcbx, fcby, fcbz, 
//                  fdcbxdx, fdcbydy, fdcbzdz, 
//                  fdexdy, fdexdz, fd2exdydz, 
//                  fdeydx, fdeydz, fd2eydzdx, 
//                  fdezdx, fdezdy, fd2ezdxdy,
//                  k_particles, 
//                  cdt_dx, cdt_dy, cdt_dz, 
//                  qdt_2mc, num_lanes, chunk);

//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//
//        // Interpolate E
//        hax[lane] = qdt_2mc*( (fex[lane] + dy[lane]*fdexdy[lane] ) + dz[lane]*(fdexdz[lane] + dy[lane]*fd2exdydz[lane]) );
//        hay[lane] = qdt_2mc*( (fey[lane] + dz[lane]*fdeydz[lane] ) + dx[lane]*(fdeydx[lane] + dz[lane]*fd2eydzdx[lane]) );
//        haz[lane] = qdt_2mc*( (fez[lane] + dx[lane]*fdezdx[lane] ) + dy[lane]*(fdezdy[lane] + dx[lane]*fd2ezdxdy[lane]) );
//  
//        // Interpolate B
//        cbx[lane] = fcbx[lane] + dx[lane]*fdcbxdx[lane];
//        cby[lane] = fcby[lane] + dy[lane]*fdcbydy[lane];
//        cbz[lane] = fcbz[lane] + dz[lane]*fdcbzdz[lane];
//  
//        // Half advance e
//        ux[lane] += hax[lane];
//        uy[lane] += hay[lane];
//        uz[lane] += haz[lane];
//      });
//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//        v0[lane] = qdt_2mc/sqrtf(one + (ux[lane]*ux[lane] + (uy[lane]*uy[lane] + uz[lane]*uz[lane])));
//      });
//
//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//        // Boris - scalars
//        v1[lane] = cbx[lane]*cbx[lane] + (cby[lane]*cby[lane] + cbz[lane]*cbz[lane]);
//        v2[lane] = (v0[lane]*v0[lane])*v1[lane];
//        v3[lane] = v0[lane]*(one+v2[lane]*(one_third+v2[lane]*two_fifteenths));
//        v4[lane] = v3[lane]/(one+v1[lane]*(v3[lane]*v3[lane]));
//        v4[lane] += v4[lane];
//        // Boris - uprime
//        v0[lane] = ux[lane] + v3[lane]*(uy[lane]*cbz[lane] - uz[lane]*cby[lane]);
//        v1[lane] = uy[lane] + v3[lane]*(uz[lane]*cbx[lane] - ux[lane]*cbz[lane]);
//        v2[lane] = uz[lane] + v3[lane]*(ux[lane]*cby[lane] - uy[lane]*cbx[lane]);
//        // Boris - rotation
//        ux[lane] += v4[lane]*(v1[lane]*cbz[lane] - v2[lane]*cby[lane]);
//        uy[lane] += v4[lane]*(v2[lane]*cbx[lane] - v0[lane]*cbz[lane]);
//        uz[lane] += v4[lane]*(v0[lane]*cby[lane] - v1[lane]*cbx[lane]);
//        // Half advance e
//        ux[lane] += hax[lane];
//        uy[lane] += hay[lane];
//        uz[lane] += haz[lane];
//        // Store momentum
//        p_ux = ux[lane];
//        p_uy = uy[lane];
//        p_uz = uz[lane];
//      });
//
//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//        v0[lane]   = one/sqrtf(one + (ux[lane]*ux[lane]+ (uy[lane]*uy[lane] + uz[lane]*uz[lane])));
//      });
//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//
//        /**/                                      // Get norm displacement
//        ux[lane]  *= cdt_dx;
//        uy[lane]  *= cdt_dy;
//        uz[lane]  *= cdt_dz;
//        ux[lane]  *= v0[lane];
//        uy[lane]  *= v0[lane];
//        uz[lane]  *= v0[lane];
//        v0[lane]   = dx[lane] + ux[lane];                           // Streak midpoint (inbnds)
//        v1[lane]   = dy[lane] + uy[lane];
//        v2[lane]   = dz[lane] + uz[lane];
//        v3[lane]   = v0[lane] + ux[lane];                           // New position
//        v4[lane]   = v1[lane] + uy[lane];
//        v5[lane]   = v2[lane] + uz[lane];
//  
//        inbnds[lane] = v3[lane]<=one &&  v4[lane]<=one &&  v5[lane]<=one &&
//                      -v3[lane]<=one && -v4[lane]<=one && -v5[lane]<=one;
//      });

#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        size_t p_index = chunk*num_lanes + lane;
  
        // Interpolate E
        hax[lane] = qdt_2mc*( (fex[lane] + dy[lane]*fdexdy[lane] ) + dz[lane]*(fdexdz[lane] + dy[lane]*fd2exdydz[lane]) );
        hay[lane] = qdt_2mc*( (fey[lane] + dz[lane]*fdeydz[lane] ) + dx[lane]*(fdeydx[lane] + dz[lane]*fd2eydzdx[lane]) );
        haz[lane] = qdt_2mc*( (fez[lane] + dx[lane]*fdezdx[lane] ) + dy[lane]*(fdezdy[lane] + dx[lane]*fd2ezdxdy[lane]) );
  
        // Interpolate B
        cbx[lane] = fcbx[lane] + dx[lane]*fdcbxdx[lane];
        cby[lane] = fcby[lane] + dy[lane]*fdcbydy[lane];
        cbz[lane] = fcbz[lane] + dz[lane]*fdcbzdz[lane];
  
        // Half advance e
        ux[lane] += hax[lane];
        uy[lane] += hay[lane];
        uz[lane] += haz[lane];
      }

#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        v0[lane] = qdt_2mc/sqrtf(one + (ux[lane]*ux[lane] + (uy[lane]*uy[lane] + uz[lane]*uz[lane])));
      }

#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        size_t p_index = chunk*num_lanes + lane;
        // Boris - scalars
        v1[lane] = cbx[lane]*cbx[lane] + (cby[lane]*cby[lane] + cbz[lane]*cbz[lane]);
        v2[lane] = (v0[lane]*v0[lane])*v1[lane];
        v3[lane] = v0[lane]*(one+v2[lane]*(one_third+v2[lane]*two_fifteenths));
        v4[lane] = v3[lane]/(one+v1[lane]*(v3[lane]*v3[lane]));
        v4[lane] += v4[lane];
        // Boris - uprime
        v0[lane] = ux[lane] + v3[lane]*(uy[lane]*cbz[lane] - uz[lane]*cby[lane]);
        v1[lane] = uy[lane] + v3[lane]*(uz[lane]*cbx[lane] - ux[lane]*cbz[lane]);
        v2[lane] = uz[lane] + v3[lane]*(ux[lane]*cby[lane] - uy[lane]*cbx[lane]);
        // Boris - rotation
        ux[lane] += v4[lane]*(v1[lane]*cbz[lane] - v2[lane]*cby[lane]);
        uy[lane] += v4[lane]*(v2[lane]*cbx[lane] - v0[lane]*cbz[lane]);
        uz[lane] += v4[lane]*(v0[lane]*cby[lane] - v1[lane]*cbx[lane]);
        // Half advance e
        ux[lane] += hax[lane];
        uy[lane] += hay[lane];
        uz[lane] += haz[lane];
        // Store momentum
        p_ux = ux[lane];
        p_uy = uy[lane];
        p_uz = uz[lane];
      }
  
#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        v0[lane]   = one/sqrtf(one + (ux[lane]*ux[lane]+ (uy[lane]*uy[lane] + uz[lane]*uz[lane])));
      }

#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        /**/                                      // Get norm displacement
        ux[lane]  *= cdt_dx;
        uy[lane]  *= cdt_dy;
        uz[lane]  *= cdt_dz;
        ux[lane]  *= v0[lane];
        uy[lane]  *= v0[lane];
        uz[lane]  *= v0[lane];
        v0[lane]   = dx[lane] + ux[lane];                           // Streak midpoint (inbnds)
        v1[lane]   = dy[lane] + uy[lane];
        v2[lane]   = dz[lane] + uz[lane];
        v3[lane]   = v0[lane] + ux[lane];                           // New position
        v4[lane]   = v1[lane] + uy[lane];
        v5[lane]   = v2[lane] + uz[lane];
  
        inbnds[lane] = v3[lane]<=one &&  v4[lane]<=one &&  v5[lane]<=one &&
                      -v3[lane]<=one && -v4[lane]<=one && -v5[lane]<=one;
      }

      float *v6, *v7, *v8, *v9, *v10, *v11, *v12, *v13;
      v6 = fex;
      v7 = fdexdy;
      v8 = fdexdz;
      v9 = fd2exdydz;
      v10 = fey;
      v11 = fdeydz;
      v12 = fdeydx;
      v13 = fd2eydzdx;
//      shared_float& v6 = fex;
//      shared_float& v7 = fdexdy;
//      shared_float& v8 = fdexdz;
//      shared_float& v9 = fd2exdydz;
//      shared_float& v10 = fey;
//      shared_float& v11 = fdeydz;
//      shared_float& v12 = fdeydx;
//      shared_float& v13 = fd2eydzdx;

//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane) {
//        size_t p_index = chunk*num_lanes + lane;
//
//        v3[lane] = static_cast<float>(inbnds[lane])*v3[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dx;
//        v4[lane] = static_cast<float>(inbnds[lane])*v4[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dy;
//        v5[lane] = static_cast<float>(inbnds[lane])*v5[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dz;
//        q[lane]  = static_cast<float>(inbnds[lane])*q[lane]*qsp;
//
//        p_dx = v3[lane];
//        p_dy = v4[lane];
//        p_dz = v5[lane];
//        dx[lane] = v0[lane];
//        dy[lane] = v1[lane];
//        dz[lane] = v2[lane];
//        v5[lane] = q[lane]*ux[lane]*uy[lane]*uz[lane]*one_third;
//
//#       define ACCUMULATE_J(X,Y,Z,v0,v1,v2,v3)                                              \
//        v4[lane]  = q[lane]*u##X[lane];   /* v2 = q ux                            */        \
//        v1[lane]  = v4[lane]*d##Y[lane];  /* v1 = q ux dy                         */        \
//        v0[lane]  = v4[lane]-v1[lane];    /* v0 = q ux (1-dy)                     */        \
//        v1[lane] += v4[lane];             /* v1 = q ux (1+dy)                     */        \
//        v4[lane]  = one+d##Z[lane];       /* v4 = 1+dz                            */        \
//        v2[lane]  = v0[lane]*v4[lane];    /* v2 = q ux (1-dy)(1+dz)               */        \
//        v3[lane]  = v1[lane]*v4[lane];    /* v3 = q ux (1+dy)(1+dz)               */        \
//        v4[lane]  = one-d##Z[lane];       /* v4 = 1-dz                            */        \
//        v0[lane] *= v4[lane];             /* v0 = q ux (1-dy)(1-dz)               */        \
//        v1[lane] *= v4[lane];             /* v1 = q ux (1+dy)(1-dz)               */        \
//        v0[lane] += v5[lane];             /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */        \
//        v1[lane] -= v5[lane];             /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */        \
//        v2[lane] -= v5[lane];             /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */        \
//        v3[lane] += v5[lane];             /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */
//
//        ACCUMULATE_J( x,y,z, v6,v7,v8,v9 );
//
//        ACCUMULATE_J( y,z,x, v10,v11,v12,v13 );
//
//        ACCUMULATE_J( z,x,y, v0,v1,v2,v3 );
//      });

#pragma omp simd 
      for(int lane=0; lane<num_lanes; lane++) {
        size_t p_index = chunk*num_lanes + lane;

        v3[lane] = static_cast<float>(inbnds[lane])*v3[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dx;
        v4[lane] = static_cast<float>(inbnds[lane])*v4[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dy;
        v5[lane] = static_cast<float>(inbnds[lane])*v5[lane] + (1.0-static_cast<float>(inbnds[lane]))*p_dz;
        q[lane]  = static_cast<float>(inbnds[lane])*q[lane]*qsp;

        p_dx = v3[lane];
        p_dy = v4[lane];
        p_dz = v5[lane];
        dx[lane] = v0[lane];
        dy[lane] = v1[lane];
        dz[lane] = v2[lane];
        v5[lane] = q[lane]*ux[lane]*uy[lane]*uz[lane]*one_third;

#       define ACCUMULATE_J(X,Y,Z,v0,v1,v2,v3)                                              \
        v4[lane]  = q[lane]*u##X[lane];   /* v2 = q ux                            */        \
        v1[lane]  = v4[lane]*d##Y[lane];  /* v1 = q ux dy                         */        \
        v0[lane]  = v4[lane]-v1[lane];    /* v0 = q ux (1-dy)                     */        \
        v1[lane] += v4[lane];             /* v1 = q ux (1+dy)                     */        \
        v4[lane]  = one+d##Z[lane];       /* v4 = 1+dz                            */        \
        v2[lane]  = v0[lane]*v4[lane];    /* v2 = q ux (1-dy)(1+dz)               */        \
        v3[lane]  = v1[lane]*v4[lane];    /* v3 = q ux (1+dy)(1+dz)               */        \
        v4[lane]  = one-d##Z[lane];       /* v4 = 1-dz                            */        \
        v0[lane] *= v4[lane];             /* v0 = q ux (1-dy)(1-dz)               */        \
        v1[lane] *= v4[lane];             /* v1 = q ux (1+dy)(1-dz)               */        \
        v0[lane] += v5[lane];             /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */        \
        v1[lane] -= v5[lane];             /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */        \
        v2[lane] -= v5[lane];             /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */        \
        v3[lane] += v5[lane];             /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */

        ACCUMULATE_J( x,y,z, v6,v7,v8,v9 );

        ACCUMULATE_J( y,z,x, v10,v11,v12,v13 );

        ACCUMULATE_J( z,x,y, v0,v1,v2,v3 );
      }

//accumulate_current(dx, dy, dz, ux, uy, uz, ii, 
//             v0, v1, v2, v3, v4, v5, v6,
//             v7, v8, v9, v10, v11, v12, v13,
//             inbnds, q,
//             k_particles,
//             qsp, num_lanes, chunk);

#ifdef VPIC_ENABLE_TEAM_REDUCTION
    int first = ii[0];
    int index = first;
    int in_bounds = 1;
    #pragma omp simd reduction(&:index,in_bounds)
    for(int lane=0; lane<num_lanes; lane++) {
      index &= ii[lane];
      in_bounds &= inbnds[lane];
    }
//    Kokkos::parallel_reduce(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int& lane, int& index) {
//      index = index & ii[lane];
//    }, Kokkos::BAnd<int>(index));
//    Kokkos::parallel_reduce(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int& lane, int& inbounds) {
//      inbounds = inbounds & inbnds[lane];
//    }, Kokkos::BAnd<int>(in_bounds));

    if(first == index && in_bounds == 1) {
      int iii = first;
      int zi = iii/((nx+2)*(ny+2));
      iii -= zi*(nx+2)*(ny+2);
      int yi = iii/(nx+2);
      int xi = iii - yi*(nx+2);

      float val0 = 0.0, val1 = 0.0, val2 = 0.0, val3 = 0.0;
      float val4 = 0.0, val5 = 0.0, val6 = 0.0, val7 = 0.0;
      float val8 = 0.0, val9 = 0.0, val10 = 0.0, val11 = 0.0;
      #pragma omp simd reduction(+:val0,val1,val2,val3,val4,val5,val6,val8,val9,val10,val11)
      for(int lane=0; lane<num_lanes; lane++) {
        val0  += v6[lane];
        val1  += v7[lane];
        val2  += v8[lane];
        val3  += v9[lane];
        val4  += v10[lane];
        val5  += v11[lane];
        val6  += v12[lane];
        val7  += v13[lane];
        val8  += v0[lane];
        val9  += v1[lane];
        val10 += v2[lane];
        val11 += v3[lane];
      }

//      CurrentType tr;
//      Kokkos::parallel_reduce(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (int lane, CurrentType& update) {
//        update.v0  += v6[lane];
//        update.v1  += v7[lane];
//        update.v2  += v8[lane];
//        update.v3  += v9[lane];
//        update.v4  += v10[lane];
//        update.v5  += v11[lane];
//        update.v6  += v12[lane];
//        update.v7  += v13[lane];
//        update.v8  += v0[lane];
//        update.v9  += v1[lane];
//        update.v10 += v2[lane];
//        update.v11 += v3[lane];
//      }, CurrentAccumulation<Kokkos::DefaultExecutionSpace::memory_space>(tr));
//
//      Kokkos::single(Kokkos::PerThread(team_member), [&] () {
//        accum_sa(first, 0)  += cx*tr.v0;
//        accum_sa(first, 1)  += cx*tr.v1;
//        accum_sa(first, 2)  += cx*tr.v2;
//        accum_sa(first, 3)  += cx*tr.v3;
//        accum_sa(first, 4)  += cy*tr.v4;
//        accum_sa(first, 5)  += cy*tr.v5;
//        accum_sa(first, 6)  += cy*tr.v6;
//        accum_sa(first, 7)  += cy*tr.v7;
//        accum_sa(first, 8)  += cz*tr.v8;
//        accum_sa(first, 9)  += cz*tr.v9;
//        accum_sa(first, 10) += cz*tr.v10;
//        accum_sa(first, 11) += cz*tr.v11;
//      });

//      k_field_scatter_access(first, field_var::jfx) += cx*val0;
//      k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx) += cx*val1;
//      k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx) += cx*val2;
//      k_field_scatter_access(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx) += cx*val3;
//      k_field_scatter_access(first, field_var::jfy) += cy*val4;
//      k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*val5;
//      k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy) += cy*val6;
//      k_field_scatter_access(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*val7;
//      k_field_scatter_access(first, field_var::jfz) += cz*val8;
//      k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz) += cz*val9;
//      k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*val10;
//      k_field_scatter_access(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*val11;

      accum_sa(first, 0)  += cx*val0;
      accum_sa(first, 1)  += cx*val1;
      accum_sa(first, 2)  += cx*val2;
      accum_sa(first, 3)  += cx*val3;

      accum_sa(first, 4)  += cy*val4;
      accum_sa(first, 5)  += cy*val5;
      accum_sa(first, 6)  += cy*val6;
      accum_sa(first, 7)  += cy*val7;

      accum_sa(first, 8)  += cz*val8;
      accum_sa(first, 9)  += cz*val9;
      accum_sa(first, 10) += cz*val10;
      accum_sa(first, 11) += cz*val11;
    } else {
#endif
#pragma omp simd
      for(int lane=0; lane<num_lanes; lane++) {
        size_t p_index = chunk*num_lanes + lane;
        int iii = ii[lane];
        int zi = iii/((nx+2)*(ny+2));
        iii -= zi*(nx+2)*(ny+2);
        int yi = iii/(nx+2);
        int xi = iii - yi*(nx+2);

//        k_field_scatter_access(ii[lane], field_var::jfx) += cx*v6[lane];
//        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx) += cx*v7[lane];
//        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx) += cx*v8[lane];
//        k_field_scatter_access(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx) += cx*v9[lane];
//
//        k_field_scatter_access(ii[lane], field_var::jfy) += cy*v10[lane];
//        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v11[lane];
//        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy) += cy*v12[lane];
//        k_field_scatter_access(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v13[lane];
//
//        k_field_scatter_access(ii[lane], field_var::jfz) += cz*v0[lane];
//        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz) += cz*v1[lane];
//        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v2[lane];
//        k_field_scatter_access(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v3[lane];

        accum_sa(ii[lane], 0)  += cx*v6[lane];
        accum_sa(ii[lane], 1)  += cx*v7[lane];
        accum_sa(ii[lane], 2)  += cx*v8[lane];
        accum_sa(ii[lane], 3)  += cx*v9[lane];

        accum_sa(ii[lane], 4)  += cy*v10[lane];
        accum_sa(ii[lane], 5)  += cy*v11[lane];
        accum_sa(ii[lane], 6)  += cy*v12[lane];
        accum_sa(ii[lane], 7)  += cy*v13[lane];

        accum_sa(ii[lane], 8)  += cz*v0[lane];
        accum_sa(ii[lane], 9)  += cz*v1[lane];
        accum_sa(ii[lane], 10) += cz*v2[lane];
        accum_sa(ii[lane], 11) += cz*v3[lane];
      }
//      Kokkos::parallel_for(Kokkos::ThreadVectorRange(team_member, num_lanes), [&] (const int lane) {
//        size_t p_index = chunk*num_lanes + lane;
////if(p_index < np) {
//        int iii = ii[lane];
//        int zi = iii/((nx+2)*(ny+2));
//        iii -= zi*(nx+2)*(ny+2);
//        int yi = iii/(nx+2);
//        int xi = iii - yi*(nx+2);
//
//        accum_sa(ii[lane], 0)  += cx*v6[lane];
//        accum_sa(ii[lane], 1)  += cx*v7[lane];
//        accum_sa(ii[lane], 2)  += cx*v8[lane];
//        accum_sa(ii[lane], 3)  += cx*v9[lane];
//
//        accum_sa(ii[lane], 4)  += cy*v10[lane];
//        accum_sa(ii[lane], 5)  += cy*v11[lane];
//        accum_sa(ii[lane], 6)  += cy*v12[lane];
//        accum_sa(ii[lane], 7)  += cy*v13[lane];
//
//        accum_sa(ii[lane], 8)  += cz*v0[lane];
//        accum_sa(ii[lane], 9)  += cz*v1[lane];
//        accum_sa(ii[lane], 10) += cz*v2[lane];
//        accum_sa(ii[lane], 11) += cz*v3[lane];
////}
//      });

//        #pragma omp simd
//        for(int lane=0; lane<num_lanes; lane++) {
//          v4[lane] = 0.0f;
//          v5[lane] = 0.0f;
//          v14[lane] = 0.0f;
//          v15[lane] = 0.0f;
//        }
//        #pragma omp simd
//        for(int lane=0; lane<num_lanes; lane++) {
//          v6[lane]  *= cx;
//          v7[lane]  *= cx;
//          v8[lane]  *= cx;
//          v9[lane]  *= cx;
//          v10[lane] *= cy;
//          v11[lane] *= cy;
//          v12[lane] *= cy;
//          v13[lane] *= cy;
//          v0[lane]  *= cz;
//          v1[lane]  *= cz;
//          v2[lane]  *= cz;
//          v3[lane]  *= cz;
//        }
//        transpose(v6, v7, v8, v9, v10, v11, v12, v13, v0, v1, v2, v3, v4, v5, v14, v15);
//        simd_update_accumulators( accum_sa, num_lanes, ii[0],  v6);
//        simd_update_accumulators( accum_sa, num_lanes, ii[1],  v7);
//        simd_update_accumulators( accum_sa, num_lanes, ii[2],  v8);
//        simd_update_accumulators( accum_sa, num_lanes, ii[3],  v9);
//        simd_update_accumulators( accum_sa, num_lanes, ii[4],  v10);
//        simd_update_accumulators( accum_sa, num_lanes, ii[5],  v11);
//        simd_update_accumulators( accum_sa, num_lanes, ii[6],  v12);
//        simd_update_accumulators( accum_sa, num_lanes, ii[7],  v13);
//        simd_update_accumulators( accum_sa, num_lanes, ii[8],  v0);
//        simd_update_accumulators( accum_sa, num_lanes, ii[9],  v1);
//        simd_update_accumulators( accum_sa, num_lanes, ii[10], v2);
//        simd_update_accumulators( accum_sa, num_lanes, ii[11], v3);
//        simd_update_accumulators( accum_sa, num_lanes, ii[12], v4);
//        simd_update_accumulators( accum_sa, num_lanes, ii[13], v5);
//        simd_update_accumulators( accum_sa, num_lanes, ii[14], v14);
//        simd_update_accumulators( accum_sa, num_lanes, ii[15], v15);
#ifdef VPIC_ENABLE_TEAM_REDUCTION
    }
#endif
#       undef ACCUMULATE_J

      for(int lane=0; lane<num_lanes; lane++) {
        if(!inbnds[lane]) {
          size_t p_index = chunk*num_lanes + lane;
          DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
          local_pm->dispx = ux[lane];
          local_pm->dispy = uy[lane];
          local_pm->dispz = uz[lane];
          local_pm->i     = p_index;

          if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
                             k_f_sv, accum_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
//          if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
//                             k_f_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
          {
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
            }
          }
        }
      }
//      Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, num_lanes), [&] (const int lane) {
////        size_t p_index = chunk*num_lanes + lane;
////if(p_index < np) {
//        if(!inbnds[lane]) {
//          size_t p_index = chunk*num_lanes + lane;
//          DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
//          local_pm->dispx = ux[lane];
//          local_pm->dispy = uy[lane];
//          local_pm->dispz = uz[lane];
//          local_pm->i     = p_index;
//
//          if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
//                             k_f_sv, accum_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
////          if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
////                             k_f_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
//          {
//            if( k_nm(0)<max_nm ) {
//              const unsigned int nm = Kokkos::atomic_fetch_add( &k_nm(0), 1 );
//              if (nm >= max_nm) Kokkos::abort("overran max_nm");
//
//              k_particle_movers(nm, particle_mover_var::dispx) = local_pm->dispx;
//              k_particle_movers(nm, particle_mover_var::dispy) = local_pm->dispy;
//              k_particle_movers(nm, particle_mover_var::dispz) = local_pm->dispz;
//              k_particle_movers_i(nm)   = local_pm->i;
//
//              // Keep existing mover structure, but also copy the particle data so we have a reduced set to move to host
//              k_particle_copy(nm, particle_var::dx) = p_dx;
//              k_particle_copy(nm, particle_var::dy) = p_dy;
//              k_particle_copy(nm, particle_var::dz) = p_dz;
//              k_particle_copy(nm, particle_var::ux) = p_ux;
//              k_particle_copy(nm, particle_var::uy) = p_uy;
//              k_particle_copy(nm, particle_var::uz) = p_uz;
//              k_particle_copy(nm, particle_var::w) = p_w;
//              k_particle_i_copy(nm) = pii;
//            }
//          }
//        }
////}
//      });
//}
//    });
  });

#undef p_dx
#undef p_dy
#undef p_dz
#undef p_ux
#undef p_uy
#undef p_uz
#undef p_w 
#undef pii 

#undef f_cbx
#undef f_cby
#undef f_cbz
#undef f_ex 
#undef f_ey 
#undef f_ez 

#undef f_dexdy
#undef f_dexdz

#undef f_d2exdydz
#undef f_deydx   
#undef f_deydz   

#undef f_d2eydzdx
#undef f_dezdx   
#undef f_dezdy   

#undef f_d2ezdxdy
#undef f_dcbxdx  
#undef f_dcbydy  
#undef f_dcbzdz  

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
  if(num_chunks*num_lanes < np) {
    Kokkos::parallel_for("push particles", Kokkos::RangePolicy<>(num_chunks*num_lanes, np), KOKKOS_LAMBDA(const int p_index) {
//    for(int p_index=num_chunks*num_lanes; p_index<np; p_index++) {
      float v0, v1, v2, v3, v4, v5;
//      auto k_field_scatter_access = k_f_sv.access();
      auto accum_sa = accum_sv.access();

      // Load position
      float dx = p_dx;
      float dy = p_dy;
      float dz = p_dz;
      int ii = pii;

      float hax = qdt_2mc*( (f_ex + dy*f_dexdy ) + dz*(f_dexdz + dy*f_d2exdydz) );
      float hay = qdt_2mc*( (f_ey + dz*f_deydz ) + dx*(f_deydx + dz*f_d2eydzdx) );
      float haz = qdt_2mc*( (f_ez + dx*f_dezdx ) + dy*(f_dezdy + dx*f_d2ezdxdy) );

      // Interpolate B
      float cbx = f_cbx + dx*f_dcbxdx;
      float cby = f_cby + dy*f_dcbydy;
      float cbz = f_cbz + dz*f_dcbzdz;

      // Load momentum
      float ux = p_ux;
      float uy = p_uy;
      float uz = p_uz;
      float q = p_w;

      // Half advance e
      ux += hax;
      uy += hay;
      uz += haz;

      v0 = qdt_2mc/std::sqrt(one + (ux*ux + (uy*uy + uz*uz)));

      // Boris - scalars
      v1 = cbx*cbx + (cby*cby + cbz*cbz);
      v2 = (v0*v0)*v1;
      v3 = v0*(one+v2*(one_third+v2*two_fifteenths));
      v4 = v3/(one+v1*(v3*v3));
      v4 += v4;
      // Boris - uprime
      v0 = ux + v3*(uy*cbz - uz*cby);
      v1 = uy + v3*(uz*cbx - ux*cbz);
      v2 = uz + v3*(ux*cby - uy*cbx);
      // Boris - rotation
      ux += v4*(v1*cbz - v2*cby);
      uy += v4*(v2*cbx - v0*cbz);
      uz += v4*(v0*cby - v1*cbx);
      // Half advance e
      ux += hax;
      uy += hay;
      uz += haz;
      // Store momentum
      p_ux = ux;
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

      bool inbnds = v3<=one &&  v4<=one &&  v5<=one &&
                    -v3<=one && -v4<=one && -v5<=one;
      if(inbnds) {
        // Common case (inbnds).  Note: accumulator values are 4 times
        // the total physical charge that passed through the appropriate
        // current quadrant in a time-step

        q *= qsp;
        p_dx = v3;                             // Store new position
        p_dy = v4;
        p_dz = v5;
        dx = v0;                                // Streak midpoint
        dy = v1;
        dz = v2;
        v5 = q*ux*uy*uz*one_third;              // Compute correction

#       define ACCUMULATE_J(X,Y,Z)                                 \
        v4  = q*u##X;   /* v2 = q ux                            */        \
        v1  = v4*d##Y;  /* v1 = q ux dy                         */        \
        v0  = v4-v1;    /* v0 = q ux (1-dy)                     */        \
        v1 += v4;       /* v1 = q ux (1+dy)                     */        \
        v4  = one+d##Z; /* v4 = 1+dz                            */        \
        v2  = v0*v4;    /* v2 = q ux (1-dy)(1+dz)               */        \
        v3  = v1*v4;    /* v3 = q ux (1+dy)(1+dz)               */        \
        v4  = one-d##Z; /* v4 = 1-dz                            */        \
        v0 *= v4;       /* v0 = q ux (1-dy)(1-dz)               */        \
        v1 *= v4;       /* v1 = q ux (1+dy)(1-dz)               */        \
        v0 += v5;       /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */        \
        v1 -= v5;       /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */        \
        v2 -= v5;       /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */        \
        v3 += v5;       /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */

        // TODO: That 2 needs to be 2*NGHOST eventually
        int iii = ii;
        int zi = iii/((nx+2)*(ny+2));
        iii -= zi*(nx+2)*(ny+2);
        int yi = iii/(nx+2);
        int xi = iii - yi*(nx+2);
//        ACCUMULATE_J( x,y,z );
//        k_field_scatter_access(ii, field_var::jfx) += cx*v0;
//        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx) += cx*v1;
//        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx) += cx*v2;
//        k_field_scatter_access(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx) += cx*v3;
//
//        ACCUMULATE_J( y,z,x );
//        k_field_scatter_access(ii, field_var::jfy) += cy*v0;
//        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v1;
//        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy) += cy*v2;
//        k_field_scatter_access(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v3;
//
//        ACCUMULATE_J( z,x,y );
//        k_field_scatter_access(ii, field_var::jfz) += cz*v0;
//        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz) += cz*v1;
//        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v2;
//        k_field_scatter_access(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v3;

        ACCUMULATE_J( x,y,z );
        accum_sa(ii, 0) += cx*v0;
        accum_sa(ii, 1) += cx*v1;
        accum_sa(ii, 2) += cx*v2;
        accum_sa(ii, 3) += cx*v3;

        ACCUMULATE_J( y,z,x );
        accum_sa(ii, 4) += cy*v0;
        accum_sa(ii, 5) += cy*v1;
        accum_sa(ii, 6) += cy*v2;
        accum_sa(ii, 7) += cy*v3;

        ACCUMULATE_J( z,x,y );
        accum_sa(ii, 8) += cz*v0;
        accum_sa(ii, 9) += cz*v1;
        accum_sa(ii, 10) += cz*v2;
        accum_sa(ii, 11) += cz*v3;
#       undef ACCUMULATE_J
      } else {
        DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
        local_pm->dispx = ux;
        local_pm->dispy = uy;
        local_pm->dispz = uz;
        local_pm->i     = p_index;

      if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
                     k_f_sv, accum_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
//        if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
//                           k_f_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
        {
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
          }
        }
      }
//    }
    });
  }

  Kokkos::Experimental::contribute(accumulator, accum_sv);
  Kokkos::MDRangePolicy<Kokkos::Rank<3>> unload_policy({1, 1, 1}, {nz+2, ny+2, nx+2});
  Kokkos::parallel_for("unload accumulator array", unload_policy, KOKKOS_LAMBDA(const int z, const int y, const int x) {
      int f0 = VOXEL(1, y, z, nx, ny, nz) + x-1;
      int a0 = VOXEL(1, y, z, nx, ny, nz) + x-1;
      int ax = VOXEL(0, y, z, nx, ny, nz) + x-1;
      int ay = VOXEL(1, y-1, z, nx, ny, nz) + x-1;
      int az = VOXEL(1, y, z-1, nx, ny, nz) + x-1;
      int ayz = VOXEL(1, y-1, z-1, nx, ny, nz) + x-1;
      int azx = VOXEL(0, y, z-1, nx, ny, nz) + x-1;
      int axy = VOXEL(0, y-1, z, nx, ny, nz) + x-1;
      k_field(f0, field_var::jfx) += ( accumulator(a0, 0) +
                                       accumulator(ay, 1) +
                                       accumulator(az, 2) +
                                       accumulator(ayz, 3) );
      k_field(f0, field_var::jfy) += ( accumulator(a0, 4) +
                                       accumulator(az, 5) +
                                       accumulator(ax, 6) +
                                       accumulator(azx, 7) );
      k_field(f0, field_var::jfz) += ( accumulator(a0, 8) +
                                       accumulator(ax, 9) +
                                       accumulator(ay, 10) +
                                       accumulator(axy, 11) );
  });
//  Kokkos::Experimental::contribute(k_field, k_f_sv);
#undef p_dx
#undef p_dy
#undef p_dz
#undef p_ux
#undef p_uy
#undef p_uz
#undef p_w 
#undef pii 

#undef f_cbx
#undef f_cby
#undef f_cbz
#undef f_ex 
#undef f_ey 
#undef f_ez 

#undef f_dexdy
#undef f_dexdz

#undef f_d2exdydz
#undef f_deydx   
#undef f_deydz   

#undef f_d2eydzdx
#undef f_dezdx   
#undef f_dezdy   

#undef f_d2ezdxdy
#undef f_dcbxdx  
#undef f_dcbydy  
#undef f_dcbzdz  
}

void
advance_p_kokkos_unified(
        k_particles_t& k_particles,
        k_particles_i_t& k_particles_i,
        k_particle_copy_t& k_particle_copy,
        k_particle_i_copy_t& k_particle_i_copy,
        k_particle_movers_t& k_particle_movers,
        k_particle_i_movers_t& k_particle_movers_i,
        k_field_sa_t k_f_sa,
        k_interpolator_t& k_interp,
        //k_particle_movers_t k_local_particle_movers,
        k_counter_t& k_nm,
        k_neighbor_t& k_neighbors,
        field_array_t* RESTRICT fa,
        const grid_t *g,
        const float qdt_2mc,
        const float cdt_dx,
        const float cdt_dy,
        const float cdt_dz,
        const float qsp,
        const int np,
        const int max_nm,
        const int nx,
        const int ny,
        const int nz)
{

  constexpr float one            = 1.;
  constexpr float one_third      = 1./3.;
  constexpr float two_fifteenths = 2./15.;

  k_field_t k_field = fa->k_f_d;
  float cx = 0.25 * g->rdy * g->rdz / g->dt;
  float cy = 0.25 * g->rdz * g->rdx / g->dt;
  float cz = 0.25 * g->rdx * g->rdy / g->dt;

  #define p_dx    k_particles(p_index, particle_var::dx)
  #define p_dy    k_particles(p_index, particle_var::dy)
  #define p_dz    k_particles(p_index, particle_var::dz)
  #define p_ux    k_particles(p_index, particle_var::ux)
  #define p_uy    k_particles(p_index, particle_var::uy)
  #define p_uz    k_particles(p_index, particle_var::uz)
  #define p_w     k_particles(p_index, particle_var::w)
  #define pii     k_particles_i(p_index)

  #define f_cbx k_interp(ii[LANE], interpolator_var::cbx)
  #define f_cby k_interp(ii[LANE], interpolator_var::cby)
  #define f_cbz k_interp(ii[LANE], interpolator_var::cbz)
  #define f_ex  k_interp(ii[LANE], interpolator_var::ex)
  #define f_ey  k_interp(ii[LANE], interpolator_var::ey)
  #define f_ez  k_interp(ii[LANE], interpolator_var::ez)

  #define f_dexdy    k_interp(ii[LANE], interpolator_var::dexdy)
  #define f_dexdz    k_interp(ii[LANE], interpolator_var::dexdz)

  #define f_d2exdydz k_interp(ii[LANE], interpolator_var::d2exdydz)
  #define f_deydx    k_interp(ii[LANE], interpolator_var::deydx)
  #define f_deydz    k_interp(ii[LANE], interpolator_var::deydz)

  #define f_d2eydzdx k_interp(ii[LANE], interpolator_var::d2eydzdx)
  #define f_dezdx    k_interp(ii[LANE], interpolator_var::dezdx)
  #define f_dezdy    k_interp(ii[LANE], interpolator_var::dezdy)

  #define f_d2ezdxdy k_interp(ii[LANE], interpolator_var::d2ezdxdy)
  #define f_dcbxdx   k_interp(ii[LANE], interpolator_var::dcbxdx)
  #define f_dcbydy   k_interp(ii[LANE], interpolator_var::dcbydy)
  #define f_dcbzdz   k_interp(ii[LANE], interpolator_var::dcbzdz)

//  #define p_dx    k_particles(p_index, particle_var::dx)
//  #define p_dy    k_particles(p_index, particle_var::dy)
//  #define p_dz    k_particles(p_index, particle_var::dz)
//  #define p_ux    k_particles(p_index, particle_var::ux)
//  #define p_uy    k_particles(p_index, particle_var::uy)
//  #define p_uz    k_particles(p_index, particle_var::uz)
//  #define p_w     k_particles(p_index, particle_var::w)
//  #define pii     k_particles_i(p_index)
//
//  #define f_cbx k_interp(ii, interpolator_var::cbx)
//  #define f_cby k_interp(ii, interpolator_var::cby)
//  #define f_cbz k_interp(ii, interpolator_var::cbz)
//  #define f_ex  k_interp(ii, interpolator_var::ex)
//  #define f_ey  k_interp(ii, interpolator_var::ey)
//  #define f_ez  k_interp(ii, interpolator_var::ez)
//
//  #define f_dexdy    k_interp(ii, interpolator_var::dexdy)
//  #define f_dexdz    k_interp(ii, interpolator_var::dexdz)
//
//  #define f_d2exdydz k_interp(ii, interpolator_var::d2exdydz)
//  #define f_deydx    k_interp(ii, interpolator_var::deydx)
//  #define f_deydz    k_interp(ii, interpolator_var::deydz)
//
//  #define f_d2eydzdx k_interp(ii, interpolator_var::d2eydzdx)
//  #define f_dezdx    k_interp(ii, interpolator_var::dezdx)
//  #define f_dezdy    k_interp(ii, interpolator_var::dezdy)
//
//  #define f_d2ezdxdy k_interp(ii, interpolator_var::d2ezdxdy)
//  #define f_dcbxdx   k_interp(ii, interpolator_var::dcbxdx)
//  #define f_dcbydy   k_interp(ii, interpolator_var::dcbydy)
//  #define f_dcbzdz   k_interp(ii, interpolator_var::dcbzdz)

  auto rangel = g->rangel;
  auto rangeh = g->rangeh;

  // TODO: is this the right place to do this?
  Kokkos::deep_copy(k_nm, 0);

#if defined( VPIC_ENABLE_ACCUMULATORS )
  Kokkos::View<float*[12]> accumulator("Accumulator", k_field.extent(0));
  Kokkos::deep_copy(accumulator, 0);
  auto current_sv = Kokkos::Experimental::create_scatter_view(accumulator);
#else
  k_field_sa_t current_sv = Kokkos::Experimental::create_scatter_view<>(k_field);;
#endif

#if defined( VPIC_ENABLE_VECTORIZATION ) && !defined( USE_GPU )
  constexpr int num_lanes = 32;
  int chunk_size = num_lanes;
  int num_chunks = np/num_lanes;
  if(num_chunks*num_lanes < np)
    num_chunks += 1;
  auto policy = Kokkos::TeamPolicy<>(num_chunks, 1, num_lanes);
#elif defined( VPIC_ENABLE_HIERARCHICAL )
  auto policy = Kokkos::TeamPolicy<>(LEAGUE_SIZE, TEAM_SIZE);
  int chunk_size = np/LEAGUE_SIZE;
  if(chunk_size*LEAGUE_SIZE < np)
    chunk_size += 1;
  constexpr int num_lanes = 1;
  int num_chunks = LEAGUE_SIZE;
#else
  constexpr int num_lanes = 1;
#endif

#if defined(VPIC_ENABLE_HIERARCHICAL) || defined(VPIC_ENABLE_VECTORIZATION)
  Kokkos::parallel_for("advance_p", policy, 
  KOKKOS_LAMBDA(const KOKKOS_TEAM_POLICY_DEVICE::member_type team_member) {
      auto current_sa = current_sv.access();
      int chunk = team_member.league_rank();
      int num_iters = chunk_size;
      if((chunk+1)*chunk_size > np)
        num_iters = np - chunk*chunk_size;
      size_t pi_offset = chunk*chunk_size;
#else
  auto policy = Kokkos::RangePolicy<>(0,np);
  Kokkos::parallel_for("advance_p", policy, KOKKOS_LAMBDA (const size_t pi_offset) {
      auto current_sa = current_sv.access();
#endif

#if defined ( VPIC_ENABLE_HIERARCHICAL ) && !defined( VPIC_ENABLE_VECTORIZATION )
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, num_iters), [&] (const size_t index) {
      size_t pi_offset = chunk*chunk_size + index;
#endif
      float v0[num_lanes];
      float v1[num_lanes];
      float v2[num_lanes];
      float v3[num_lanes];
      float v4[num_lanes];
      float v5[num_lanes];
      float dx[num_lanes];
      float dy[num_lanes];
      float dz[num_lanes];
      float ux[num_lanes];
      float uy[num_lanes];
      float uz[num_lanes];
      float hax[num_lanes];
      float hay[num_lanes];
      float haz[num_lanes];
      float cbx[num_lanes];
      float cby[num_lanes];
      float cbz[num_lanes];
      float q[num_lanes];
      int   ii[num_lanes];
      int   inbnds[num_lanes];

      float fcbx[num_lanes];
      float fcby[num_lanes];
      float fcbz[num_lanes];
      float fex[num_lanes];
      float fey[num_lanes];
      float fez[num_lanes];
      float fdexdy[num_lanes];
      float fdexdz[num_lanes];
      float fd2exdydz[num_lanes];
      float fdeydx[num_lanes];
      float fdeydz[num_lanes];
      float fd2eydzdx[num_lanes];
      float fdezdx[num_lanes];
      float fdezdy[num_lanes];
      float fd2ezdxdy[num_lanes];
      float fdcbxdx[num_lanes];
      float fdcbydy[num_lanes];
      float fdcbzdz[num_lanes];
      float *v6 = fex;
      float *v7 = fdexdy;
      float *v8 = fdexdz;
      float *v9 = fd2exdydz;
      float *v10 = fey;
      float *v11 = fdeydz;
      float *v12 = fdeydx;
      float *v13 = fd2eydzdx;

      size_t p_index = pi_offset;

      BEGIN_VECTOR_BLOCK {
        p_index = pi_offset + LANE;
        // Load position
        dx[LANE] = p_dx;
        dy[LANE] = p_dy;
        dz[LANE] = p_dz;
        // Load momentum
        ux[LANE] = p_ux;
        uy[LANE] = p_uy;
        uz[LANE] = p_uz;
        // Load weight
        q[LANE]  = p_w;
        // Load index
        ii[LANE] = pii;
      } END_VECTOR_BLOCK;

      load_interpolators<num_lanes>( fex, fdexdy, fdexdz, fd2exdydz,
                                     fey, fdeydz, fdeydx, fd2eydzdx,
                                     fez, fdezdx, fdezdy, fd2ezdxdy,
                                     fcbx, fdcbxdx,
                                     fcby, fdcbydy,
                                     fcbz, fdcbzdz,
                                     ii, k_interp);

      BEGIN_VECTOR_BLOCK {
        // Interpolate E
        hax[LANE] = qdt_2mc*( (fex[LANE] + dy[LANE]*fdexdy[LANE] ) + dz[LANE]*(fdexdz[LANE] + dy[LANE]*fd2exdydz[LANE]) );
        hay[LANE] = qdt_2mc*( (fey[LANE] + dz[LANE]*fdeydz[LANE] ) + dx[LANE]*(fdeydx[LANE] + dz[LANE]*fd2eydzdx[LANE]) );
        haz[LANE] = qdt_2mc*( (fez[LANE] + dx[LANE]*fdezdx[LANE] ) + dy[LANE]*(fdezdy[LANE] + dx[LANE]*fd2ezdxdy[LANE]) );
  
        // Interpolate B
        cbx[LANE] = fcbx[LANE] + dx[LANE]*fdcbxdx[LANE];
        cby[LANE] = fcby[LANE] + dy[LANE]*fdcbydy[LANE];
        cbz[LANE] = fcbz[LANE] + dz[LANE]*fdcbzdz[LANE];
  
        // Half advance e
        ux[LANE] += hax[LANE];
        uy[LANE] += hay[LANE];
        uz[LANE] += haz[LANE];
      } END_VECTOR_BLOCK;

      BEGIN_VECTOR_BLOCK {
        v0[LANE] = qdt_2mc/sqrtf(one + (ux[LANE]*ux[LANE] + (uy[LANE]*uy[LANE] + uz[LANE]*uz[LANE])));
      } END_VECTOR_BLOCK;

      BEGIN_VECTOR_BLOCK {
        p_index = pi_offset + LANE;

        // Boris - scalars
        v1[LANE] = cbx[LANE]*cbx[LANE] + (cby[LANE]*cby[LANE] + cbz[LANE]*cbz[LANE]);
        v2[LANE] = (v0[LANE]*v0[LANE])*v1[LANE];
        v3[LANE] = v0[LANE]*(one+v2[LANE]*(one_third+v2[LANE]*two_fifteenths));
        v4[LANE] = v3[LANE]/(one+v1[LANE]*(v3[LANE]*v3[LANE]));
        v4[LANE] += v4[LANE];
        // Boris - uprime
        v0[LANE] = ux[LANE] + v3[LANE]*(uy[LANE]*cbz[LANE] - uz[LANE]*cby[LANE]);
        v1[LANE] = uy[LANE] + v3[LANE]*(uz[LANE]*cbx[LANE] - ux[LANE]*cbz[LANE]);
        v2[LANE] = uz[LANE] + v3[LANE]*(ux[LANE]*cby[LANE] - uy[LANE]*cbx[LANE]);
        // Boris - rotation
        ux[LANE] += v4[LANE]*(v1[LANE]*cbz[LANE] - v2[LANE]*cby[LANE]);
        uy[LANE] += v4[LANE]*(v2[LANE]*cbx[LANE] - v0[LANE]*cbz[LANE]);
        uz[LANE] += v4[LANE]*(v0[LANE]*cby[LANE] - v1[LANE]*cbx[LANE]);
        // Half advance e
        ux[LANE] += hax[LANE];
        uy[LANE] += hay[LANE];
        uz[LANE] += haz[LANE];
        // Store momentum
        p_ux = ux[LANE];
        p_uy = uy[LANE];
        p_uz = uz[LANE];
      } END_VECTOR_BLOCK;

      BEGIN_VECTOR_BLOCK {
        v0[LANE]   = one/sqrtf(one + (ux[LANE]*ux[LANE]+ (uy[LANE]*uy[LANE] + uz[LANE]*uz[LANE])));
      } END_VECTOR_BLOCK;

      BEGIN_VECTOR_BLOCK {

        /**/                                      // Get norm displacement
        ux[LANE]  *= cdt_dx;
        uy[LANE]  *= cdt_dy;
        uz[LANE]  *= cdt_dz;
        ux[LANE]  *= v0[LANE];
        uy[LANE]  *= v0[LANE];
        uz[LANE]  *= v0[LANE];
        v0[LANE]   = dx[LANE] + ux[LANE];                           // Streak midpoint (inbnds)
        v1[LANE]   = dy[LANE] + uy[LANE];
        v2[LANE]   = dz[LANE] + uz[LANE];
        v3[LANE]   = v0[LANE] + ux[LANE];                           // New position
        v4[LANE]   = v1[LANE] + uy[LANE];
        v5[LANE]   = v2[LANE] + uz[LANE];
  
        inbnds[LANE] = v3[LANE]<=one &&  v4[LANE]<=one &&  v5[LANE]<=one &&
                      -v3[LANE]<=one && -v4[LANE]<=one && -v5[LANE]<=one;
      } END_VECTOR_BLOCK;
    
#ifdef VPIC_ENABLE_TEAM_REDUCTION
      int in_cell = particles_in_same_cell(team_member, ii, inbnds, num_iters);
#endif

      BEGIN_VECTOR_BLOCK {
        p_index = pi_offset + LANE;

        v3[LANE] = static_cast<float>(inbnds[LANE])*v3[LANE] + (1.0-static_cast<float>(inbnds[LANE]))*p_dx;
        v4[LANE] = static_cast<float>(inbnds[LANE])*v4[LANE] + (1.0-static_cast<float>(inbnds[LANE]))*p_dy;
        v5[LANE] = static_cast<float>(inbnds[LANE])*v5[LANE] + (1.0-static_cast<float>(inbnds[LANE]))*p_dz;
        q[LANE]  = static_cast<float>(inbnds[LANE])*q[LANE]*qsp;

        p_dx = v3[LANE];
        p_dy = v4[LANE];
        p_dz = v5[LANE];
        dx[LANE] = v0[LANE];
        dy[LANE] = v1[LANE];
        dz[LANE] = v2[LANE];
        v5[LANE] = q[LANE]*ux[LANE]*uy[LANE]*uz[LANE]*one_third;

#       define ACCUMULATE_J(X,Y,Z,v0,v1,v2,v3)                                              \
        v4[LANE]  = q[LANE]*u##X[LANE];   /* v2 = q ux                            */        \
        v1[LANE]  = v4[LANE]*d##Y[LANE];  /* v1 = q ux dy                         */        \
        v0[LANE]  = v4[LANE]-v1[LANE];    /* v0 = q ux (1-dy)                     */        \
        v1[LANE] += v4[LANE];             /* v1 = q ux (1+dy)                     */        \
        v4[LANE]  = one+d##Z[LANE];       /* v4 = 1+dz                            */        \
        v2[LANE]  = v0[LANE]*v4[LANE];    /* v2 = q ux (1-dy)(1+dz)               */        \
        v3[LANE]  = v1[LANE]*v4[LANE];    /* v3 = q ux (1+dy)(1+dz)               */        \
        v4[LANE]  = one-d##Z[LANE];       /* v4 = 1-dz                            */        \
        v0[LANE] *= v4[LANE];             /* v0 = q ux (1-dy)(1-dz)               */        \
        v1[LANE] *= v4[LANE];             /* v1 = q ux (1+dy)(1-dz)               */        \
        v0[LANE] += v5[LANE];             /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */        \
        v1[LANE] -= v5[LANE];             /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */        \
        v2[LANE] -= v5[LANE];             /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */        \
        v3[LANE] += v5[LANE];             /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */

        ACCUMULATE_J( x,y,z, v6,v7,v8,v9 );

        ACCUMULATE_J( y,z,x, v10,v11,v12,v13 );

        ACCUMULATE_J( z,x,y, v0,v1,v2,v3 );
      } END_VECTOR_BLOCK;

#ifdef VPIC_ENABLE_TEAM_REDUCTION
    if(in_cell) {
      int first = ii[0];
      reduce_and_accumulate_current(team_member, current_sa, num_iters, first, 
                                    nx, ny, nz, cx, cy, cz,
                                    v6, v7, v8, v9,
                                    v10, v11, v12, v13,
                                    v0, v1, v2, v3);
    } else {
#endif
      BEGIN_VECTOR_BLOCK {
        accumulate_current(current_sa, ii[LANE],
                     nx, ny, nz, cx, cy, cz, 
                     v6[LANE], v7[LANE], v8[LANE], v9[LANE],
                     v10[LANE], v11[LANE], v12[LANE], v13[LANE],
                     v0[LANE], v1[LANE], v2[LANE], v3[LANE]);
      } END_VECTOR_BLOCK;
#ifdef VPIC_ENABLE_TEAM_REDUCTION
    }
#endif
#       undef ACCUMULATE_J
      BEGIN_THREAD_BLOCK {
        if(!inbnds[LANE]) {
          p_index = pi_offset + LANE;

          DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
          local_pm->dispx = ux[LANE];
          local_pm->dispy = uy[LANE];
          local_pm->dispz = uz[LANE];
          local_pm->i     = p_index;

          if( move_p_kokkos_test( k_particles, k_particles_i, local_pm, // Unlikely
                             current_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
          {
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
            }
          }
        }
      } END_THREAD_BLOCK;
#if defined( VPIC_ENABLE_HIERARCHICAL ) && !defined( VPIC_ENABLE_VECTORIZATION )
      });
#endif
  });

#if defined( VPIC_ENABLE_ACCUMULATORS )
  Kokkos::Experimental::contribute(accumulator, current_sv);
  Kokkos::MDRangePolicy<Kokkos::Rank<3>> unload_policy({1, 1, 1}, {nz+2, ny+2, nx+2});
  Kokkos::parallel_for("unload accumulator array", unload_policy, 
  KOKKOS_LAMBDA(const int z, const int y, const int x) {
      int f0  = VOXEL(1, y, z, nx, ny, nz) + x-1;
      int a0  = VOXEL(1, y, z, nx, ny, nz) + x-1;
      int ax  = VOXEL(0, y, z, nx, ny, nz) + x-1;
      int ay  = VOXEL(1, y-1, z, nx, ny, nz) + x-1;
      int az  = VOXEL(1, y, z-1, nx, ny, nz) + x-1;
      int ayz = VOXEL(1, y-1, z-1, nx, ny, nz) + x-1;
      int azx = VOXEL(0, y, z-1, nx, ny, nz) + x-1;
      int axy = VOXEL(0, y-1, z, nx, ny, nz) + x-1;
      k_field(f0, field_var::jfx) += ( accumulator(a0, 0) +
                                       accumulator(ay, 1) +
                                       accumulator(az, 2) +
                                       accumulator(ayz, 3) );
      k_field(f0, field_var::jfy) += ( accumulator(a0, 4) +
                                       accumulator(az, 5) +
                                       accumulator(ax, 6) +
                                       accumulator(azx, 7) );
      k_field(f0, field_var::jfz) += ( accumulator(a0, 8) +
                                       accumulator(ax, 9) +
                                       accumulator(ay, 10) +
                                       accumulator(axy, 11) );
  });
#else
  Kokkos::Experimental::contribute(k_field, current_sv);
#endif

#undef p_dx
#undef p_dy
#undef p_dz
#undef p_ux
#undef p_uy
#undef p_uz
#undef p_w 
#undef pii 

#undef f_cbx
#undef f_cby
#undef f_cbz
#undef f_ex 
#undef f_ey 
#undef f_ez 

#undef f_dexdy
#undef f_dexdz

#undef f_d2exdydz
#undef f_deydx   
#undef f_deydz   

#undef f_d2eydzdx
#undef f_dezdx   
#undef f_dezdy   

#undef f_d2ezdxdy
#undef f_dcbxdx  
#undef f_dcbydy  
#undef f_dcbzdz  
}

void
advance_p_kokkos_gpu(
        k_particles_t& k_particles,
        k_particles_i_t& k_particles_i,
        k_particle_copy_t& k_particle_copy,
        k_particle_i_copy_t& k_particle_i_copy,
        k_particle_movers_t& k_particle_movers,
        k_particle_i_movers_t& k_particle_movers_i,
//        k_accumulators_sa_t k_accumulators_sa,
        k_field_sa_t k_f_sa,
        k_interpolator_t& k_interp,
        //k_particle_movers_t k_local_particle_movers,
        k_counter_t& k_nm,
        k_neighbor_t& k_neighbors,
        field_array_t* RESTRICT fa,
        const grid_t *g,
        const float qdt_2mc,
        const float cdt_dx,
        const float cdt_dy,
        const float cdt_dz,
        const float qsp,
//        const int na,
        const int np,
        const int max_nm,
        const int nx,
        const int ny,
        const int nz)
{

  constexpr float one            = 1.;
  constexpr float one_third      = 1./3.;
  constexpr float two_fifteenths = 2./15.;
  k_field_t k_field = fa->k_f_d;
  k_field_sa_t k_f_sv = Kokkos::Experimental::create_scatter_view<>(k_field);
  float cx = 0.25 * g->rdy * g->rdz / g->dt;
  float cy = 0.25 * g->rdz * g->rdx / g->dt;
  float cz = 0.25 * g->rdx * g->rdy / g->dt;

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

  // zero out nm, we could probably do this earlier if we're worried about it
  // slowing things down
  Kokkos::deep_copy(k_nm, 0);

#ifdef VPIC_ENABLE_HIERARCHICAL
  auto team_policy = Kokkos::TeamPolicy<>(LEAGUE_SIZE, TEAM_SIZE);
  int per_league = np/LEAGUE_SIZE;
  if(np%LEAGUE_SIZE > 0)
    per_league += 1;
  Kokkos::parallel_for("advance_p", team_policy, KOKKOS_LAMBDA(const KOKKOS_TEAM_POLICY_DEVICE::member_type team_member) {
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, per_league), [=] (size_t pindex) {
      int p_index = team_member.league_rank()*per_league + pindex;
      if(p_index < np) {
#else
  auto range_policy = Kokkos::RangePolicy<>(0,np);
  Kokkos::parallel_for("advance_p", range_policy, KOKKOS_LAMBDA (size_t p_index) {
#endif
      
    float v0, v1, v2, v3, v4, v5;
    auto  k_field_scatter_access = k_f_sv.access();

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

    float cbx  = f_cbx + dx*f_dcbxdx;             // Interpolate B
    float cby  = f_cby + dy*f_dcbydy;
    float cbz  = f_cbz + dz*f_dcbzdz;
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

#ifdef VPIC_ENABLE_TEAM_REDUCTION
    int reduce = 0;
    int inbnds = v3<=one && v4<=one && v5<=one && -v3<=one && -v4<=one && -v5<=one;
    int min_inbnds = inbnds;
    int max_inbnds = inbnds;
    team_member.team_reduce(Kokkos::Max<int>(min_inbnds));
    team_member.team_reduce(Kokkos::Min<int>(max_inbnds));
    int min_index = ii;
    int max_index = ii;
    team_member.team_reduce(Kokkos::Max<int>(max_index));
    team_member.team_reduce(Kokkos::Min<int>(min_index));
    reduce = min_inbnds == max_inbnds && min_index == max_index;
#endif

    // FIXME-KJB: COULD SHORT CIRCUIT ACCUMULATION IN THE CASE WHERE QSP==0!
    if(  v3<=one &&  v4<=one &&  v5<=one &&   // Check if inbnds
        -v3<=one && -v4<=one && -v5<=one ) {

      // Common case (inbnds).  Note: accumulator values are 4 times
      // the total physical charge that passed through the appropriate
      // current quadrant in a time-step

      q *= qsp;
      p_dx = v3;                             // Store new position
      p_dy = v4;
      p_dz = v5;
      dx = v0;                                // Streak midpoint
      dy = v1;
      dz = v2;
      v5 = q*ux*uy*uz*one_third;              // Compute correction

#     define ACCUMULATE_J(X,Y,Z)                                 \
      v4  = q*u##X;   /* v2 = q ux                            */        \
      v1  = v4*d##Y;  /* v1 = q ux dy                         */        \
      v0  = v4-v1;    /* v0 = q ux (1-dy)                     */        \
      v1 += v4;       /* v1 = q ux (1+dy)                     */        \
      v4  = one+d##Z; /* v4 = 1+dz                            */        \
      v2  = v0*v4;    /* v2 = q ux (1-dy)(1+dz)               */        \
      v3  = v1*v4;    /* v3 = q ux (1+dy)(1+dz)               */        \
      v4  = one-d##Z; /* v4 = 1-dz                            */        \
      v0 *= v4;       /* v0 = q ux (1-dy)(1-dz)               */        \
      v1 *= v4;       /* v1 = q ux (1+dy)(1-dz)               */        \
      v0 += v5;       /* v0 = q ux [ (1-dy)(1-dz) + uy*uz/3 ] */        \
      v1 -= v5;       /* v1 = q ux [ (1+dy)(1-dz) - uy*uz/3 ] */        \
      v2 -= v5;       /* v2 = q ux [ (1-dy)(1+dz) - uy*uz/3 ] */        \
      v3 += v5;       /* v3 = q ux [ (1+dy)(1+dz) + uy*uz/3 ] */

#ifdef VPIC_ENABLE_TEAM_REDUCTION
      if(reduce) {
        int iii = ii;
        int zi = iii/((nx+2)*(ny+2));
        iii -= zi*(nx+2)*(ny+2);
        int yi = iii/(nx+2);
        int xi = iii - yi*(nx+2);

        int i0 = ii;
        int i1 = VOXEL(xi,yi+1,zi,nx,ny,nz);
        int i2 = VOXEL(xi,yi,zi+1,nx,ny,nz);
        int i3 = VOXEL(xi,yi+1,zi+1,nx,ny,nz);
        ACCUMULATE_J( x,y,z );
        contribute_current(team_member, k_field_scatter_access, i0, i1, i2, i3, 
                            field_var::jfx, cx*v0, cx*v1, cx*v2, cx*v3);

        i1 = VOXEL(xi,yi,zi+1,nx,ny,nz);
        i2 = VOXEL(xi+1,yi,zi,nx,ny,nz);
        i3 = VOXEL(xi+1,yi,zi+1,nx,ny,nz);
        ACCUMULATE_J( y,z,x );
        contribute_current(team_member, k_field_scatter_access, i0, i1, i2, i3, 
                            field_var::jfy, cy*v0, cy*v1, cy*v2, cy*v3);

        i1 = VOXEL(xi+1,yi,zi,nx,ny,nz);
        i2 = VOXEL(xi,yi+1,zi,nx,ny,nz);
        i3 = VOXEL(xi+1,yi+1,zi,nx,ny,nz);
        ACCUMULATE_J( z,x,y );
        contribute_current(team_member, k_field_scatter_access, i0, i1, i2, i3, 
                            field_var::jfz, cz*v0, cz*v1, cz*v2, cz*v3);
      } else {
#endif
        // TODO: That 2 needs to be 2*NGHOST eventually
        int iii = ii;
        int zi = iii/((nx+2)*(ny+2));
        iii -= zi*(nx+2)*(ny+2);
        int yi = iii/(nx+2);
        int xi = iii - yi*(nx+2);
        ACCUMULATE_J( x,y,z );
        //Kokkos::atomic_add(&k_field(ii, field_var::jfx), cx*v0);
        //Kokkos::atomic_add(&k_field(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx), cx*v1);
        //Kokkos::atomic_add(&k_field(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx), cx*v2);
        //Kokkos::atomic_add(&k_field(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx), cx*v3);
        k_field_scatter_access(ii, field_var::jfx) += cx*v0;
        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfx) += cx*v1;
        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfx) += cx*v2;
        k_field_scatter_access(VOXEL(xi,yi+1,zi+1,nx,ny,nz), field_var::jfx) += cx*v3;

        ACCUMULATE_J( y,z,x );
        //Kokkos::atomic_add(&k_field(ii, field_var::jfy), cy*v0);
        //Kokkos::atomic_add(&k_field(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy), cy*v1);
        //Kokkos::atomic_add(&k_field(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy), cy*v2);
        //Kokkos::atomic_add(&k_field(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy), cy*v3);
        k_field_scatter_access(ii, field_var::jfy) += cy*v0;
        k_field_scatter_access(VOXEL(xi,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v1;
        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfy) += cy*v2;
        k_field_scatter_access(VOXEL(xi+1,yi,zi+1,nx,ny,nz), field_var::jfy) += cy*v3;

        ACCUMULATE_J( z,x,y );
        //Kokkos::atomic_add(&k_field(ii, field_var::jfz), cz*v0);
        //Kokkos::atomic_add(&k_field(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz), cz*v1);
        //Kokkos::atomic_add(&k_field(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz), cz*v2);
        //Kokkos::atomic_add(&k_field(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz), cz*v3);
        k_field_scatter_access(ii, field_var::jfz) += cz*v0;
        k_field_scatter_access(VOXEL(xi+1,yi,zi,nx,ny,nz), field_var::jfz) += cz*v1;
        k_field_scatter_access(VOXEL(xi,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v2;
        k_field_scatter_access(VOXEL(xi+1,yi+1,zi,nx,ny,nz), field_var::jfz) += cz*v3;
#ifdef VPIC_ENABLE_TEAM_REDUCTION
      }
#endif

#     undef ACCUMULATE_J
    } else {
      DECLARE_ALIGNED_ARRAY( particle_mover_t, 16, local_pm, 1 );
      local_pm->dispx = ux;
      local_pm->dispy = uy;
      local_pm->dispz = uz;
      local_pm->i     = p_index;

      //printf("Calling move_p index %d dx %e y %e z %e ux %e uy %e yz %e \n", p_index, ux, uy, uz, p_ux, p_uy, p_uz);
//      if( move_p_kokkos( k_particles, k_particles_i, local_pm, // Unlikely
//                     k_f_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
          if( move_p_kokkos_test( k_particles, k_particles_i, local_pm, // Unlikely
                             k_f_sv, g, k_neighbors, rangel, rangeh, qsp, cx, cy, cz, nx, ny, nz ) )
      {
        if( k_nm(0) < max_nm )
        {
            const int nm = Kokkos::atomic_fetch_add( &k_nm(0), 1 );
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
    }
#ifdef VPIC_ENABLE_HIERARCHICAL
  }
  });
#endif
  });
  Kokkos::Experimental::contribute(k_field, k_f_sv);


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
//           accumulator_array_t * RESTRICT aa,
           interpolator_array_t * RESTRICT ia,
           field_array_t* RESTRICT fa ) {
  //DECLARE_ALIGNED_ARRAY( advance_p_pipeline_args_t, 128, args, 1 );
  //DECLARE_ALIGNED_ARRAY( particle_mover_seg_t, 128, seg, MAX_PIPELINE+1 );
  //int rank;

  if( !sp )
  {
    ERROR(( "Bad args" ));
  }
  if( !ia  )
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
  advance_p_kokkos_unified(
//  advance_p_kokkos_cpu(
//  advance_p_kokkos_gpu(
          sp->k_p_d,
          sp->k_p_i_d,
          sp->k_pc_d,
          sp->k_pc_i_d,
          sp->k_pm_d,
          sp->k_pm_i_d,
//          aa->k_a_sa,
          fa->k_field_sa_d,
          ia->k_i_d,
          sp->k_nm_d,
          sp->g->k_neighbor_d,
          fa,
          sp->g,
          qdt_2mc,
          cdt_dx,
          cdt_dy,
          cdt_dz,
          sp->q,
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
}
