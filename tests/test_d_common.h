/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS For Embedded Optimization.                                                      *
* Copyright (C) 2019 by Gianluca Frison.                                                          *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* The 2-Clause BSD License                                                                        *
*                                                                                                 *
* Redistribution and use in source and binary forms, with or without                              *
* modification, are permitted provided that the following conditions are met:                     *
*                                                                                                 *
* 1. Redistributions of source code must retain the above copyright notice, this                  *
*    list of conditions and the following disclaimer.                                             *
* 2. Redistributions in binary form must reproduce the above copyright notice,                    *
*    this list of conditions and the following disclaimer in the documentation                    *
*    and/or other materials provided with the distribution.                                       *
*                                                                                                 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND                 *
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                   *
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                          *
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR                 *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES                  *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;                    *
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND                     *
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                      *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS                   *
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                    *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/

// BLASFEO routines
#include "../include/blasfeo_common.h"
#include "../include/blasfeo_d_blas.h"
#include "../include/blasfeo_d_aux.h"
#include "../include/blasfeo_d_kernel.h"

#include "../include/blasfeo_d_blas_api.h"

// BLASFEO External dependencies
#include "../include/blasfeo_i_aux_ext_dep.h"
#include "../include/blasfeo_v_aux_ext_dep.h"
#include "../include/blasfeo_d_aux_ext_dep.h"
#include "../include/blasfeo_timing.h"

// BLASFEO LA:REFERENCE routines
#include "../include/blasfeo_d_aux_ext_dep.h"
#include "../include/blasfeo_d_aux_ref.h"
#include "../include/blasfeo_d_blasfeo_ref_api.h"

#include "../include/blasfeo_d_aux_test.h"
#include "../include/d_blas.h"

// Macros specific for Double type
#define PRECISION Double
#define DOUBLE_PRECISION

#define REAL double

#define ZEROS d_zeros
#define FREE d_free

#define MAT blasfeo_dmat

#define ALLOCATE_MAT blasfeo_allocate_dmat
#define FREE_MAT blasfeo_free_dmat
#define GESE blasfeo_dgese
#define PACK_MAT blasfeo_pack_dmat
#define PRINT_MAT blasfeo_print_dmat

#define MAT_REF blasfeo_dmat

#define ALLOCATE_MAT_REF blasfeo_allocate_dmat
#define FREE_MAT_REF blasfeo_free_dmat

#define GESE_REF blasfeo_ref_dgese
#define GECP_REF blasfeo_ref_dgecp
#define PACK_MAT_REF blasfeo_ref_pack_dmat
#define PRINT_MAT_REF blasfeo_ref_print_dmat
