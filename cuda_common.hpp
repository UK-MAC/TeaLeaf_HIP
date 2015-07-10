/*Crown Copyright 2012 AWE.
 *
 * This file is part of CloverLeaf.
 *
 * CloverLeaf is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * CloverLeaf is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * CloverLeaf. If not, see http://www.gnu.org/licenses/.
 */

/*
 *  @brief CUDA common file
 *  @author Michael Boulton NVIDIA Corporation
 *  @details Contains common elements for cuda kernels
 */

#ifndef __CUDA_COMMON_INC
#define __CUDA_COMMON_INC

#include "thrust/device_allocator.h"
#include "thrust/extrema.h"
#include "kernel_files/cuda_kernel_header.hpp"
#include <map>
#include <vector>

// used in update_halo and for copying back to host for mpi transfers
#define FIELD_density       1
#define FIELD_energy0       2
#define FIELD_energy1       3
#define FIELD_u             4
#define FIELD_vector_p      5
#define FIELD_vector_sd     6
#define FIELD_vector_r      7
#define NUM_FIELDS          7

#define NUM_BUFFERED_FIELDS 7

#define TL_PREC_NONE        1
#define TL_PREC_JAC_DIAG    2
#define TL_PREC_JAC_BLOCK   3

#define TEA_ENUM_JACOBI     1
#define TEA_ENUM_CG         2
#define TEA_ENUM_CHEBYSHEV  3
#define TEA_ENUM_PPCG       4

// which side to pack - keep the same as in fortran file
#define CHUNK_LEFT 1
#define CHUNK_left 1
#define CHUNK_RIGHT 2
#define CHUNK_right 2
#define CHUNK_BOTTOM 3
#define CHUNK_bottom 3
#define CHUNK_TOP 4
#define CHUNK_top 4
#define EXTERNAL_FACE       (-1)

#define CELL_DATA   1
#define VERTEX_DATA 2
#define X_FACE_DATA 3
#define Y_FACE_DATA 4

#define INITIALISE_ARGS \
    /* values used to control operation */\
    int* in_x_min, \
    int* in_x_max, \
    int* in_y_min, \
    int* in_y_max

/*******************/

// disable checking for errors after kernel calls / memory allocation
#ifdef NO_ERR_CHK

// do nothing instead
#define CUDA_ERR_CHECK ;

#else

#include <iostream>

#define CUDA_ERR_CHECK errorHandler(__LINE__, __FILE__);

#endif //NO_ERR_CHK

/*******************/

// enormous ugly macro that profiles kernels + checks if there were any errors
#define CUDALAUNCH(funcname, ...)                               \
    if (profiler_on)                                            \
    {                                                           \
        cudaEventCreate(&_t0);                                  \
        cudaEventCreate(&_t1);                                  \
        cudaEventRecord(_t0);                                   \
    }                                                           \
    funcname<<<grid_dim, block_shape>>>(kernel_info, __VA_ARGS__); \
    CUDA_ERR_CHECK;                                             \
    if (profiler_on)                                            \
    {                                                           \
        cudaEventRecord(_t1);                                   \
        cudaEventSynchronize(_t1);                              \
        cudaEventElapsedTime(&taken, _t0, _t1);                 \
        std::string func_name(#funcname);                       \
        if (kernel_times.end() != kernel_times.find(func_name)) \
        {                                                       \
            kernel_times.at(func_name) += taken;                \
        }                                                       \
        else                                                    \
        {                                                       \
            kernel_times[func_name] = taken;                    \
        }                                                       \
    }

/*******************/

typedef struct cell_info {
    const int x_extra;
    const int y_extra;
    const int x_invert;
    const int y_invert;
    const int x_face;
    const int y_face;
    const int grid_type;

    cell_info
    (int in_x_extra, int in_y_extra,
    int in_x_invert, int in_y_invert,
    int in_x_face, int in_y_face,
    int in_grid_type)
    :x_extra(in_x_extra), y_extra(in_y_extra),
    x_invert(in_x_invert), y_invert(in_y_invert),
    x_face(in_x_face), y_face(in_y_face),
    grid_type(in_grid_type)
    {
        ;
    }

} cell_info_t;

struct kernel_info_t {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
    int halo_depth;
    int preconditioner_type;
    int x_offset;
    int y_offset;

    kernel_info_t
    (int in_x_min,
     int in_x_max,
     int in_y_min,
     int in_y_max,
     int in_halo_depth,
     int in_preconditioner_type,
     int in_x_offset,
     int in_y_offset)
    :x_min(in_x_min),
     x_max(in_x_max),
     y_min(in_y_min),
     y_max(in_y_max),
     halo_depth(in_halo_depth),
     preconditioner_type(in_preconditioner_type),
     x_offset(in_x_offset),
     y_offset(in_y_offset)
    {
        ;
    }

    kernel_info_t(){}
};

// types of array data
const static cell_info_t CELL(    0, 0,  1,  1, 0, 0, CELL_DATA);

class CloverleafCudaChunk
{
private:
    // work arrays
    double* volume;
    double* soundspeed;

    double* density;
    double* energy0;
    double* energy1;
    double* xarea;
    double* yarea;

    double* cellx;
    double* celly;
    double* celldx;
    double* celldy;
    double* vertexx;
    double* vertexy;
    double* vertexdx;
    double* vertexdy;

    double* u;
    double* u0;

    // holding temporary stuff like post_vol etc.
    double* vector_p;
    double* vector_r;
    double* vector_w;
    double* vector_z;
    double* vector_Mi;
    double* vector_Kx;
    double* vector_Ky;
    double* vector_sd;

    double * ch_alphas_device, * ch_betas_device;

    // buffers used in mpi transfers
    double * left_buffer;
    double * right_buffer;
    double * bottom_buffer;
    double * top_buffer;

    // holding temporary stuff like post_vol etc.
    double* reduce_buf_1;
    double* reduce_buf_2;
    double* reduce_buf_3;
    double* reduce_buf_4;

    // used for reductions in calc dt, pdv, field summary
    thrust::device_ptr< double > reduce_ptr_1;
    thrust::device_ptr< double > reduce_ptr_2;
    thrust::device_ptr< double > reduce_ptr_3;
    thrust::device_ptr< double > reduce_ptr_4;

    // number of blocks for work space
    int num_blocks;
    dim3 grid_dim;

    // struct to pass in common values
    kernel_info_t kernel_info;

    int preconditioner_type;
    int halo_exchange_depth;

    // values used to control operation
    int x_min;
    int x_max;
    int y_min;
    int y_max;

    // if being profiled
    bool profiler_on;
    // for recording times if profiling is on
    std::map<std::string, double> kernel_times;
    // events used for timing
    float taken;
    cudaEvent_t _t0, _t1;

    // calculate rx/ry to pass back to fortran
    void calcrxry
    (double dt, double * rx, double * ry);

    void errorHandler
    (int line_num, const char* file);

    void update_array_boundary
    (cell_info_t const& grid_type,
     const int* chunk_neighbours,
     double* cur_array_d,
     int depth);

    // upload ch_alphas and ch_betas to device
    void upload_ch_coefs
    (const double * ch_alphas, const double * ch_betas,
     const int n_coefs);
public:
    // kernels
    void field_summary_kernel(double* vol, double* mass,
        double* ie, double* temp);

    void generate_chunk_kernel(const int number_of_states,
        const double* state_density, const double* state_energy,
        const double* state_xmin, const double* state_xmax,
        const double* state_ymin, const double* state_ymax,
        const double* state_radius, const int* state_geometry,
        const int g_rect, const int g_circ, const int g_point);
    void generate_chunk_init_u(void);

    void initialise_chunk_kernel(double d_xmin, double d_ymin,
        double d_dx, double d_dy);

    void update_halo_kernel(const int* fields, int depth,
        const int* chunk_neighbours);

    void set_field_kernel();

    // Tea leaf
    void tea_leaf_init_jacobi(int, double, double*, double*);
    void tea_leaf_kernel_jacobi(double*);

    void tea_leaf_init_cg(double*);
    void tea_leaf_kernel_cg_calc_w(double* pw);
    void tea_leaf_kernel_cg_calc_ur(double alpha, double* rrn);
    void tea_leaf_kernel_cg_calc_p(double beta);

    void tea_leaf_cheby_copy_u
    (void);
    void tea_leaf_calc_2norm_kernel
    (int norm_array, double* norm);
    void tea_leaf_kernel_cheby_init
    (const double * ch_alphas, const double * ch_betas, int n_coefs,
     const double theta);
    void tea_leaf_kernel_cheby_iterate
    (const int cheby_calc_steps);

    void ppcg_init
    (const double * ch_alphas, const double * ch_betas,
     const int n_inner_steps);
    void ppcg_init_p
    (double * rro);
    void ppcg_init_sd
    (double theta);
    void ppcg_inner
    (int ppcg_cur_step, int bounds_extra,
     int * chunk_neighbours);

    void tea_leaf_finalise();
    void tea_leaf_common_init
    (int coefficient, double dt, double * rx, double * ry,
     int * chunk_neighbours, int * zero_boundary, int reflective_boundary);
    void tea_leaf_calc_residual
    (void);

    int tea_solver;

    std::map<std::string, double*> arr_names;
    std::vector<double> dumpArray
    (const std::string& arr_name, int x_extra, int y_extra);

    void packUnpackAllBuffers
    (int fields[NUM_FIELDS], int offsets[NUM_FIELDS], int depth,
     int face, int pack, double * buffer);

    CloverleafCudaChunk
    (INITIALISE_ARGS);

    CloverleafCudaChunk
    (void);
    ~CloverleafCudaChunk
    (void);
};

extern CloverleafCudaChunk cuda_chunk;

// this function gets called when something goes wrong
#define DIE(...) cloverDie(__LINE__, __FILE__, __VA_ARGS__)
void cloverDie
(int line, const char* filename, const char* format, ...);

#endif

