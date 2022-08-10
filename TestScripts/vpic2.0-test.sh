#!/bin/bash

src_dir=$HOME/VPIC/vpic-kokkos
vpic_scripts=$src_dir/TestScripts
scratch=/lustre/scratch5/matsekh/VPIC
builds=$scratch/vpic-kokkos-build
results=$scratch/Multi_WeakScalingResults-vpic-kokkos
slurm_file=slurm.out

module load gnuplot

function vpic_kokkos_benchmark(){

    prg_env=$1 
    dir_path=$2

    # set up directory tree 
    test_dir=$results/$dir_path
    mkdir -p $test_dir
    cd $test_dir    
    ### mkdir -p weak
    build_dir=$builds/$dir_path    
    slurm_output=$results/$dir_path/$slurm_file
    
    # set up weak scaling test
    cd $test_dir    
    deck1=$build_dir/portability_test_x2-y2-z2.Linux
    deck2=$build_dir/portability_test_x4-y2-z2.Linux
    deck3=$build_dir/portability_test_x4-y4-z2.Linux
    deck4=$build_dir/portability_test_x4-y4-z4.Linux
    deck5=$build_dir/portability_test_x8-y4-z4.Linux
    deck6=$build_dir/portability_test_x8-y8-z4.Linux
    deck7=$build_dir/portability_test_x8-y8-z8.Linux

    # launch weak scaling test
    sbatch -o $slurm_output -p gpu -N 1 -t 6:00:00 $vpic_scripts/vpic2.0_weak_scaling.sh $prg_env $thread $deck4 $deck3 $deck2 $deck1 $vpic_scripts $slurm_output
}

prog_env="gnu"
thread="openmp"
bench_path=openmp/no-team_no-hierarch 
vpic_kokkos_benchmark $prog_env $bench_path $thread 
 
prog_env="gnu"
thread="openmp"
bench_path=openmp/no-team_hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread

prog_env="gnu"
thread="openmp"
bench_path=openmp/team_no-hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread

prog_env="gnu"
thread="openmp"
bench_path=openmp/team_hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread

prog_env="gnu"
thread="pthreads"
bench_path=pthreads/no-team_no-hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread
 
prog_env="gnu"
thread="pthreads"
bench_path=pthreads/no-team_hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread

prog_env="gnu"
thread="pthreads"
bench_path=pthreads/team_no-hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread

prog_env="gnu"
thread="pthreads"
bench_path=pthreads/team_hierarch
vpic_kokkos_benchmark $prog_env $bench_path $thread
 
