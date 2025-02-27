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

#ifndef BLASFEO_S_AUX_EXT_DEP_H_
#define BLASFEO_S_AUX_EXT_DEP_H_



#include <stdio.h>



#include "blasfeo_common.h"



#ifdef __cplusplus
extern "C" {
#endif



/************************************************
* s_aux_extern_depend_lib.c
************************************************/

/* column-major matrices */

#ifdef EXT_DEP_MALLOC
// dynamically allocate row*col floats of memory and set accordingly a pointer to float; set allocated memory to zero
void s_zeros(float **pA, int row, int col);
// dynamically allocate row*col floats of memory aligned to 64-byte boundaries and set accordingly a pointer to float; set allocated memory to zero
void s_zeros_align(float **pA, int row, int col);
// dynamically allocate size bytes of memory aligned to 64-byte boundaries and set accordingly a pointer to float; set allocated memory to zero
void s_zeros_align_bytes(float **pA, int size);
// free the memory allocated by d_zeros
void s_free(float *pA);
// free the memory allocated by d_zeros_align or d_zeros_align_bytes
void s_free_align(float *pA);
#endif
// print a column-major matrix
void s_print_mat(int m, int n, float *A, int lda);
// print in exponential notation a column-major matrix
void s_print_exp_mat(int m, int n, float *A, int lda);
// print the transposed of a column-major matrix
void s_print_tran_mat(int row, int col, float *A, int lda);
// print in exponential notation the transposed of a column-major matrix
void s_print_exp_tran_mat(int row, int col, float *A, int lda);
#ifdef EXT_DEP
// print to file a column-major matrix
void s_print_to_file_mat(FILE *file, int row, int col, float *A, int lda);
// print to file a column-major matrix in exponential format
void s_print_to_file_exp_mat(FILE *file, int row, int col, float *A, int lda);
// print to file the transposed of a column-major matrix
void s_print_tran_to_file_mat(FILE *file, int row, int col, float *A, int lda);
// print to file the transposed of a column-major matrix in exponential format
void s_print_tran_to_file_exp_mat(FILE *file, int row, int col, float *A, int lda);
// print to string a column-major matrix
void s_print_to_string_mat(char **buf_out, int row, int col, float *A, int lda);
#endif

/* strmat and strvec */

#ifdef EXT_DEP_MALLOC
// create a strmat for a matrix of size m*n by dynamically allocating memory
void blasfeo_allocate_smat(int m, int n, struct blasfeo_smat *sA);
// create a strvec for a vector of size m by dynamically allocating memory
void blasfeo_allocate_svec(int m, struct blasfeo_svec *sa);
// free the memory allocated by blasfeo_allocate_dmat
void blasfeo_free_smat(struct blasfeo_smat *sA);
// free the memory allocated by blasfeo_allocate_dvec
void blasfeo_free_svec(struct blasfeo_svec *sa);
#endif
// print a strmat
void blasfeo_print_smat(int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print in exponential notation a strmat
void blasfeo_print_exp_smat(int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print the transpose of a strmat
void blasfeo_print_tran_smat(int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print a strvec
void blasfeo_print_svec(int m, struct blasfeo_svec *sa, int ai);
// print in exponential notation a strvec
void blasfeo_print_exp_svec(int m, struct blasfeo_svec *sa, int ai);
// print the transposed of a strvec
void blasfeo_print_tran_svec(int m, struct blasfeo_svec *sa, int ai);
// print in exponential notation the transposed of a strvec
void blasfeo_print_exp_tran_svec(int m, struct blasfeo_svec *sa, int ai);
#ifdef EXT_DEP
// print to file a strmat
void blasfeo_print_to_file_smat(FILE *file, int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print to file a strmat in exponential format
void blasfeo_print_to_file_exp_smat(FILE *file, int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print to file a strvec
void blasfeo_print_to_file_svec(FILE *file, int m, struct blasfeo_svec *sa, int ai);
// print to file the transposed of a strvec
void blasfeo_print_to_file_tran_svec(FILE *file, int m, struct blasfeo_svec *sa, int ai);
// print to string a strmat
void blasfeo_print_to_string_smat(char **buf_out, int m, int n, struct blasfeo_smat *sA, int ai, int aj);
// print to string a strvec
void blasfeo_print_to_string_svec(char **buf_out, int m, struct blasfeo_svec *sa, int ai);
// print to string the transposed of a strvec
void blasfeo_print_to_string_tran_svec(char **buf_out, int m, struct blasfeo_svec *sa, int ai);
#endif



#ifdef __cplusplus
}
#endif



#endif  // BLASFEO_S_AUX_EXT_DEP_H_
