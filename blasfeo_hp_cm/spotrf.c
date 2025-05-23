/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS for embedded optimization.                                                      *
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

#include <stdlib.h>
#include <stdio.h>

#include <blasfeo_target.h>
#include <blasfeo_block_size.h>
#include <blasfeo_common.h>
#include <blasfeo_s_aux.h>
#include <blasfeo_s_kernel.h>
#include <blasfeo_stdlib.h>



#if ( defined(BLAS_API) & defined(MF_PANELMAJ) )
#define blasfeo_dmat blasfeo_cm_dmat
#define blasfeo_hp_spotrf_l blasfeo_hp_cm_spotrf_l
#define blasfeo_hp_spotrf_u blasfeo_hp_cm_spotrf_u
#define blasfeo_hp_spotrf_l_mn blasfeo_hp_cm_spotrf_l_mn
#define blasfeo_spotrf_l blasfeo_cm_spotrf_l
#define blasfeo_spotrf_u blasfeo_cm_spotrf_u
#define blasfeo_spotrf_l_mn blasfeo_cm_spotrf_l_mn
#endif



// TODO move to a header file to reuse across routines
#define EL_SIZE 4 // single precision

#if defined(TARGET_X64_INTEL_HASWELL)
#define M_KERNEL 8 // max kernel: 8x4 // TODO keep updated !!!!!!!!!!!!!!
#define N_KERNEL 8 // max kernel: 8x8
#define L1_CACHE_EL (32*1024/EL_SIZE) // L1 data cache size: 32 kB
#define CACHE_LINE_EL (64/EL_SIZE) // data cache size: 64 bytes

#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53)
#define M_KERNEL 8 // max kernel: 8x4 // TODO keep updated !!!!!!!!!!!!!!
#define N_KERNEL 4 // max kernel: 8x4
#define L1_CACHE_EL (32*1024/EL_SIZE) // L1 data cache size: 32 kB
#define CACHE_LINE_EL (64/EL_SIZE) // data cache size: 64 bytes

#else // assume generic target
#define M_KERNEL 4 // max kernel: 4x4
#define N_KERNEL 4 // max kernel: 4x4
#define L1_CACHE_EL (32*1024/EL_SIZE) // L1 data cache size: 32 kB
#define CACHE_LINE_EL (64/EL_SIZE) // data cache size: 64 bytes // TODO 32-bytes for cortex A9
#endif



void blasfeo_hp_spotrf_l(int m, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{

#if defined(PRINT_NAME)
#ifdef EXT_DEP
	printf("\nblasfeo_hp_spotrf_l (cm) %d %p %d %d %p %d %d\n", m, sC, ci, cj, sD, di, dj);
#endif
#endif

	if(m<=0)
		return;

	// extract pointer to column-major matrices from structures
	int ldc = sC->m;
	int ldd = sD->m;
	float *C = sC->pA + ci + cj*ldc;
	float *D = sD->pA + di + dj*ldd;

//	printf("\n%p %d %p %d\n", C, ldc, D, ldd);

	int ii, jj;

	const int ps = 4; //D_PS;

#if defined(TARGET_X64_INTEL_HASWELL)
	int ps0 = 8;
	int ps8 = 8;
#else
	int ps0 = 4;
	int ps4 = 4;
#endif

#if defined(TARGET_GENERIC)
	float pU[M_KERNEL*K_MAX_STACK];
	float dU[K_MAX_STACK];
#else
	ALIGNED( float pU[M_KERNEL*K_MAX_STACK], 64 );
	ALIGNED( float dU[K_MAX_STACK], 64 );
#endif
	int sdu = (m+3)/4*4;
	sdu = sdu<K_MAX_STACK ? sdu : K_MAX_STACK;

	struct blasfeo_pm_smat tA;
	int sda;
	float *dA;
	int tA_size;
	void *mem;
	char *mem_align;
	int m1, n1, k1;
	int pack_B;

	const int m_kernel = M_KERNEL;
	const int l1_cache_el = L1_CACHE_EL;
	const int reals_per_cache_line = CACHE_LINE_EL;

	const int m_cache = (m+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
//	const int n_cache = (n+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
//	const int k_cache = (k+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	const int m_kernel_cache = (m_kernel+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	int m_min = m_cache<m_kernel_cache ? m_cache : m_kernel_cache;
//	int n_min = n_cache<m_kernel_cache ? n_cache : m_kernel_cache;


	float d_1 = 1.0;


//	goto l_1;
//	goto l_2;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	if(m>=200 | m>K_MAX_STACK)
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	if(m>=64 | m>K_MAX_STACK)
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	if(m>=16 | m>K_MAX_STACK)
#else
	if(m>=12 | m>K_MAX_STACK)
#endif
		{
		goto l_2;
		}
	else
		{
		goto l_1;
		}

	// never to get here
	return;


l_1:

	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_12_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu);
			}
		kernel_spotrf_nt_l_12x4_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
		kernel_spack_nn_8_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, sdu);
		kernel_spotrf_nt_l_8x8_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4);
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_1_left_4;
			}
		if(m-ii<=8)
			{
			goto l_1_left_8;
			}
		else
			{
			goto l_1_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; ii<m-7; ii+=8)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_8_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu);
			}
		kernel_spotrf_nt_l_8x4_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
		kernel_spack_nn_4_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps);
		kernel_spotrf_nt_l_4x4_lib44cc(jj+4, pU+4*sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4);
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_1_left_4;
			}
		else
			{
			goto l_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib4cccc(jj, pU, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_4_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps);
			}
		kernel_spotrf_nt_l_4x4_lib44cc(jj, pU, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
		}
	if(ii<m)
		{
		goto l_1_left_4;
		}
#endif
	goto l_1_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL)
l_1_left_12:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_12x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, ii-jj);
		kernel_spack_nn_12_vs_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
		}
	kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, m-jj);
	kernel_spack_nn_8_vs_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, sdu, m-ii-4);
	kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, m-ii-4);
	goto l_1_return;
#endif

// TODO use haswell 8x8
#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
l_1_left_8:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_8x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, ii-jj);
		kernel_spack_nn_8_vs_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
		}
	kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, m-jj);
	kernel_spack_nn_4_vs_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, m-ii-4);
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, pU+4*sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, m-jj-4);
	goto l_1_return;
#endif

l_1_left_4:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_4x4_vs_lib4cccc(jj, pU, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, ii-jj);
		kernel_spack_nn_4_vs_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, m-ii);
		}
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, pU, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, m-jj);
	goto l_1_return;

l_1_return:
	return;


l_2:

#ifdef EXT_DEP_MALLOC

	m1 = (m+128-1)/128*128;
	tA_size = blasfeo_pm_memsize_smat(ps0, m1, m1);
	blasfeo_malloc(&mem, tA_size+64);
	blasfeo_align_64_byte(mem, (void **) &mem_align);
	blasfeo_pm_create_smat(ps0, m, m, &tA, (void *) mem_align);

	sda = tA.cn;
	dA = tA.dA;

	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		jj = 0;
		for(; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_12_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda);
			}
		kernel_spotrf_nt_l_12x4_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
		kernel_spack_nn_8_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, sda);
#if defined(TARGET_X64_INTEL_HASWELL)
		kernel_spotrf_nt_l_8x8_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
		kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps); // TODO comment out ?? no !!!!!!!!!!!!!!!!!!!!
#else
		kernel_spotrf_nt_l_8x4_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
		kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps);
		kernel_spotrf_nt_l_4x4_lib44cc(jj+8, tA.pA+(ii+8)*sda, tA.pA+(jj+8)*sda, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dA+jj+8);
#endif
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_2_left_4;
			}
		if(m-ii<=8)
			{
			goto l_2_left_8;
			}
		else
			{
			goto l_2_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		jj = 0;
		for(; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_8_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda);
			}
		kernel_spotrf_nt_l_8x4_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
		kernel_spack_nn_4_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps);
		kernel_spotrf_nt_l_4x4_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_2_left_4;
			}
		else
			{
			goto l_2_left_8;
			}
		}
#endif
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-7; ii+=8)
		{
		jj = 0;
		for(; jj<ii; jj+=8)
			{
			kernel_strsm_nt_rl_inv_8x8_lib88ccc(jj+0, tA.pA+ii*sda, tA.pA+0+jj*sda, C+ii+(jj+0)*ldc, ldc, D+ii+(jj+0)*ldd, ldd, D+jj+0+(jj+0)*ldd, ldd, dA+jj+0);
//			kernel_strsm_nt_rl_inv_8x4_lib88ccc(jj+0, tA.pA+ii*sda, tA.pA+0+jj*sda, C+ii+(jj+0)*ldc, ldc, D+ii+(jj+0)*ldd, ldd, D+jj+0+(jj+0)*ldd, ldd, dA+jj+0);
//			kernel_strsm_nt_rl_inv_8x4_lib88ccc(jj+4, tA.pA+ii*sda, tA.pA+4+jj*sda, C+ii+(jj+4)*ldc, ldc, D+ii+(jj+4)*ldd, ldd, D+jj+4+(jj+4)*ldd, ldd, dA+jj+4);
			kernel_spack_nn_8_lib8(8, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps8);
			}
		kernel_spotrf_nt_l_8x8_lib88cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
		}
	if(ii<m)
		{
		goto l_2_left_8;
		}
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-7; ii+=8)
		{
		jj = 0;
		for(; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_8_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps4, sda);
			}
		kernel_spotrf_nt_l_8x4_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
		kernel_spack_nn_4_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps4);
		kernel_spotrf_nt_l_4x4_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_2_left_4;
			}
		else
			{
			goto l_2_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		jj = 0;
		for(; jj<ii; jj+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_4_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps4);
			}
		kernel_spotrf_nt_l_4x4_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
		}
	if(ii<m)
		{
		goto l_2_left_4;
		}
#endif
	goto l_2_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
l_2_left_12:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_12x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, ii-jj);
		kernel_spack_nn_12_vs_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
		}
	kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	kernel_spack_nn_8_vs_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, sda, m-ii-4);
#if defined(TARGET_X64_INTEL_HASWELL)
	kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, m-jj-4);
	// XXX not needed before return
//	kernel_spack_nn_4_vs_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-8);
#else
	kernel_spotrf_nt_l_8x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-(ii+4), m-(jj+4));
//	kernel_spack_nn_4_vs_lib4(8, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-8);
	kernel_spack_nn_4_vs_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-8);
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+8, tA.pA+(ii+8)*sda, tA.pA+(jj+8)*sda, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dA+jj+8, m-ii-8, m-jj-8);
#endif
	goto l_2_return;
#endif


#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
l_2_left_8:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_8x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, ii-jj);
		kernel_spack_nn_8_vs_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
		}
#if defined(TARGET_X64_INTEL_HASWELL)
	kernel_spotrf_nt_l_8x8_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
#else
	kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	kernel_spack_nn_4_vs_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, m-ii-4);
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, m-jj-4);
#endif
	goto l_2_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL)
l_2_left_8:
	for(jj=0; jj<ii; jj+=8)
		{
		kernel_strsm_nt_rl_inv_8x8_vs_lib88ccc(jj+0, tA.pA+ii*sda, tA.pA+0+jj*sda, C+ii+(jj+0)*ldc, ldc, D+ii+(jj+0)*ldd, ldd, D+jj+0+(jj+0)*ldd, ldd, dA+jj+0, m-ii, ii-jj-0);
//		kernel_strsm_nt_rl_inv_8x4_vs_lib88ccc(jj+0, tA.pA+ii*sda, tA.pA+0+jj*sda, C+ii+(jj+0)*ldc, ldc, D+ii+(jj+0)*ldd, ldd, D+jj+0+(jj+0)*ldd, ldd, dA+jj+0, m-ii, ii-jj-0);
//		kernel_strsm_nt_rl_inv_8x4_vs_lib88ccc(jj+4, tA.pA+ii*sda, tA.pA+4+jj*sda, C+ii+(jj+4)*ldc, ldc, D+ii+(jj+4)*ldd, ldd, D+jj+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii, ii-jj-4);
		kernel_spack_nn_8_vs_lib8(8, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps0, m-ii);
		}
	kernel_spotrf_nt_l_8x8_vs_lib88cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	goto l_2_return;
#endif
#if defined(TARGET_ARMV8A_ARM_CORTEX_A57) || defined(TARGET_ARMV8A_ARM_CORTEX_A53)
l_2_left_8:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_8x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, ii-jj);
//		kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, ii-jj);
//		kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+(ii+4)*sda, tA.pA+jj*sda, &d_1, C+(ii+4)+jj*ldc, ldc, D+(ii+4)+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-(ii+4), ii-jj);
		kernel_spack_nn_8_vs_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps4, sda, m-ii);
		}
//	kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+(ii+4)*sda, tA.pA+jj*sda, &d_1, C+(ii+4)+jj*ldc, ldc, D+(ii+4)+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-(ii+4), m-jj);
	kernel_spack_nn_4_vs_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps4, m-ii-4);
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, m-jj-4);
	goto l_2_return;
#endif

#if ! ( defined(TARGET_X64_INTEL_HASWELL) )
l_2_left_4:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, ii-jj);
		kernel_spack_nn_4_vs_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps4, m-ii);
		}
	kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, m-jj);
	goto l_2_return;
#endif

l_2_return:
	blasfeo_free(mem);
	return;

#else // EXT_DEP_MALLOC

	exit(1);

#endif // EXT_DEP_MALLOC

	// never to get here
	return;

	}



void blasfeo_hp_spotrf_u(int m, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{

#if defined(PRINT_NAME)
#ifdef EXT_DEP
	printf("\nblasfeo_hp_spotrf_u (cm) %d %p %d %d %p %d %d\n", m, sC, ci, cj, sD, di, dj);
#endif
#endif

	if(m<=0)
		return;

	// extract pointer to column-major matrices from structures
	int ldc = sC->m;
	int ldd = sD->m;
	float *C = sC->pA + ci + cj*ldc;
	float *D = sD->pA + di + dj*ldd;

//	printf("\n%p %d %p %d\n", C, ldc, D, ldd);

	int ii, jj;

	const int ps = 4; //D_PS;

#if defined(TARGET_GENERIC)
	float pU[4*K_MAX_STACK];
	float dU[K_MAX_STACK];
	float pD[M_KERNEL*N_KERNEL];
#else
	ALIGNED( float pU[M_KERNEL*K_MAX_STACK], 64 );
	ALIGNED( float dU[K_MAX_STACK], 64 );
	ALIGNED( float pD[M_KERNEL*N_KERNEL], 64 );
#endif
	int sdu = (m+3)/4*4;
	sdu = sdu<K_MAX_STACK ? sdu : K_MAX_STACK;

	struct blasfeo_pm_smat tA;
	int sda;
	float *dA;
	int tA_size;
	void *mem;
	char *mem_align;
	int m1, n1, k1;
	int pack_B;

	const int m_kernel = M_KERNEL;
	const int l1_cache_el = L1_CACHE_EL;
	const int reals_per_cache_line = CACHE_LINE_EL;

	const int m_cache = (m+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
//	const int n_cache = (n+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
//	const int k_cache = (k+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	const int m_kernel_cache = (m_kernel+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	int m_min = m_cache<m_kernel_cache ? m_cache : m_kernel_cache;
//	int n_min = n_cache<m_kernel_cache ? n_cache : m_kernel_cache;


	float d_1 = 1.0;


//	goto u_0;
//	goto u_1;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	if(m>=256 | m>K_MAX_STACK)
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	if(m>=64 | m>K_MAX_STACK)
#else
	if(m>=12 | m>K_MAX_STACK)
#endif
		{
		goto u_1;
		}
	else
		{
		goto u_0;
		}

	// never to get here
	return;



u_0:
	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, pU+jj*ps+0*sdu);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, pU+jj*ps+4*sdu);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+8)*ldc, ldc, pU+jj*ps+8*sdu);

			kernel_strsm_nn_ru_inv_12x4_lib4c44c(jj, pU, sdu, D+jj*ldd, ldd, &d_1, pU+jj*ps, sdu, pU+jj*ps, sdu, D+jj+jj*ldd, ldd, dU+jj);

			kernel_sunpack_nt_4_lib4(4, pU+jj*ps+0*sdu, D+jj+(ii+0)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, pU+jj*ps+4*sdu, D+jj+(ii+4)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, pU+jj*ps+8*sdu, D+jj+(ii+8)*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, pD+0*4);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, pD+4*4);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+8)*ldc, ldc, pD+8*4);
		kernel_spotrf_nt_l_12x4_lib4(ii, pU, sdu, pU, pD, ps, pD, ps, dU+ii);
		kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pD+4*4, D+ii+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pD+8*4, D+ii+(ii+8)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!

		kernel_spack_tn_4_lib4(4, D+ii+(ii+4)*ldd, ldd, pU+4*sdu+ii*ps);
		kernel_spack_tn_4_lib4(4, D+ii+(ii+8)*ldd, ldd, pU+8*sdu+ii*ps);

#if 1
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD+0*4);
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+8)*ldc, ldc, pD+8*4);
		kernel_spack_tn_4_lib4(4, C+ii+8+(ii+8)*ldc, ldc, pD+12*4);
		kernel_spotrf_nt_l_8x8_lib4(ii+4, pU+4*sdu, sdu, pU+4*sdu, sdu, pD, 8, pD, 8, dU+ii+4);
		kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+4+(ii+4)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!
		kernel_sunpack_nt_4_lib4(4, pD+8*4, D+ii+4+(ii+8)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pD+12*4, D+ii+8+(ii+8)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!
#else
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD+0*4);
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+8)*ldc, ldc, pD+4*4);
		kernel_spotrf_nt_l_8x4_lib4(ii+4, pU+4*sdu, sdu, pU+4*sdu, pD, ps, pD, ps, dU+ii+4);
		kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+4+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pD+4*4, D+ii+4+(ii+8)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!

		kernel_spack_tn_4_lib4(4, D+ii+4+(ii+8)*ldd, ldd, pU+8*sdu+(ii+4)*ps);

		kernel_spack_tn_4_lib4(4, C+ii+8+(ii+8)*ldc, ldc, pD);
		kernel_spotrf_nt_l_4x4_lib4(ii+8, pU+8*sdu, pU+8*sdu, pD, pD, dU+ii+8);
		kernel_sunpack_nt_4_lib4(4, pD, D+ii+8+(ii+8)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!
#endif
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto u_0_left_4;
			}
		if(m-ii<=8)
			{
			goto u_0_left_8;
			}
		else
			{
			goto u_0_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; ii<m-7; ii+=8)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, pU+jj*ps+0*sdu);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, pU+jj*ps+4*sdu);
			kernel_strsm_nn_ru_inv_8x4_lib4c44c(jj, pU, sdu, D+jj*ldd, ldd, &d_1, pU+jj*ps, sdu, pU+jj*ps, sdu, D+jj+jj*ldd, ldd, dU+jj);
			kernel_sunpack_nt_4_lib4(4, pU+jj*ps+0*sdu, D+jj+(ii+0)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, pU+jj*ps+4*sdu, D+jj+(ii+4)*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, pD+0*4);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, pD+4*4);
		kernel_spotrf_nt_l_8x4_lib4(ii, pU, sdu, pU, pD, ps, pD, ps, dU+ii);
		kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pD+4*4, D+ii+(ii+4)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!

		kernel_spack_tn_4_lib4(4, D+ii+(ii+4)*ldd, ldd, pU+4*sdu+ii*ps);

		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD);
		kernel_spotrf_nt_l_4x4_lib4(ii+4, pU+4*sdu, pU+4*sdu, pD, pD, dU+ii+4);
		kernel_sunpack_nt_4_lib4(4, pD, D+ii+4+(ii+4)*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto u_0_left_4;
			}
		else
			{
			goto u_0_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+ii*ldc, ldc, pU+jj*ps);
			kernel_strsm_nn_ru_inv_4x4_lib4c44c(jj, pU, D+jj*ldd, ldd, &d_1, pU+jj*ps, pU+jj*ps, D+jj+jj*ldd, ldd, dU+jj);
			kernel_sunpack_nt_4_lib4(4, pU+jj*ps, D+jj+ii*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+ii*ldc, ldc, pD);
		kernel_spotrf_nt_l_4x4_lib4(ii, pU, pU, pD, pD, dU+ii);
		kernel_sunpack_nt_4_lib4(4, pD, D+ii+ii*ldd, ldd); // TODO unpack l !!!!!!!!!!!!!!!!
		}
	if(ii<m)
		{
		goto u_0_left_4;
		}
#endif
	goto u_0_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
u_0_left_12:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, pU+jj*ps+0*sdu);
		kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, pU+jj*ps+4*sdu);
		kernel_spack_tn_4_vs_lib4(4, C+jj+(ii+8)*ldc, ldc, pU+jj*ps+8*sdu, m-ii-8);

		kernel_strsm_nn_ru_inv_12x4_lib4c44c(jj, pU, sdu, D+jj*ldd, ldd, &d_1, pU+jj*ps, sdu, pU+jj*ps, sdu, D+jj+jj*ldd, ldd, dU+jj); // TODO vs

		kernel_sunpack_nt_4_lib4(4, pU+jj*ps+0*sdu, D+jj+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, pU+jj*ps+4*sdu, D+jj+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_vs_lib4(4, pU+jj*ps+8*sdu, D+jj+(ii+8)*ldd, ldd, m-ii-8);
		}
	kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, pD+0*4);
	kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, pD+4*4);
	kernel_spack_tn_4_vs_lib4(4, C+ii+(ii+8)*ldc, ldc, pD+8*4, m-ii-8);
	kernel_spotrf_nt_l_12x4_vs_lib4(ii, pU, sdu, pU, pD, ps, pD, ps, dU+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+(ii+0)*ldd, ldd);
	kernel_sunpack_nt_4_lib4(4, pD+4*4, D+ii+(ii+4)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, pD+8*4, D+ii+(ii+8)*ldd, ldd, m-ii-8); // TODO pack vs with m and n, or triangle

	kernel_spack_tn_4_lib4(4, D+ii+(ii+4)*ldd, ldd, pU+4*sdu+ii*ps);
	kernel_spack_tn_4_lib4(4, D+ii+(ii+8)*ldd, ldd, pU+8*sdu+ii*ps);

#if 1
	kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD+0*4);
	kernel_spack_tn_4_vs_lib4(4, C+ii+4+(ii+8)*ldc, ldc, pD+8*4, m-ii-8);
	kernel_spack_tn_4_vs_lib4(4, C+ii+8+(ii+8)*ldc, ldc, pD+12*4, m-ii-8);
	kernel_spotrf_nt_l_8x8_vs_lib4(ii+4, pU+4*sdu, sdu, pU+4*sdu, sdu, pD, 8, pD, 8, dU+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+4+(ii+4)*ldd, ldd); // TODO pack vs with m and n, or triangle
	kernel_sunpack_nt_4_vs_lib4(4, pD+8*4, D+ii+4+(ii+8)*ldd, ldd, m-ii-8);
	kernel_sunpack_nt_4_vs_lib4(4, pD+12*4, D+ii+8+(ii+8)*ldd, ldd, m-ii-8); // TODO pack vs with m and n, or triangle
#else
	kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD+0*4);
	kernel_spack_tn_4_vs_lib4(4, C+ii+4+(ii+8)*ldc, ldc, pD+4*4, m-ii-8);
	kernel_spotrf_nt_l_8x4_vs_lib4(ii+4, pU+4*sdu, sdu, pU+4*sdu, pD, ps, pD, ps, dU+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+4+(ii+4)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, pD+4*4, D+ii+4+(ii+8)*ldd, ldd, m-ii-8); // TODO pack vs with m and n, or triangle

	kernel_spack_tn_4_lib4(4, D+ii+4+(ii+8)*ldd, ldd, pU+8*sdu+(ii+4)*ps);

	kernel_spack_tn_4_vs_lib4(4, C+ii+8+(ii+8)*ldc, ldc, pD, m-ii-8);
	kernel_spotrf_nt_l_4x4_vs_lib4(ii+8, pU+8*sdu, pU+8*sdu, pD, pD, dU+ii+8, m-ii-8, m-ii-8);
	kernel_sunpack_nt_4_vs_lib4(4, pD, D+ii+8+(ii+8)*ldd, ldd, m-ii-8); // TODO pack vs with m and n, or triangle
#endif
	goto u_0_return;
#endif

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
u_0_left_8:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, pU+jj*ps+0*sdu);
		kernel_spack_tn_4_vs_lib4(4, C+jj+(ii+4)*ldc, ldc, pU+jj*ps+4*sdu, m-ii-4);

		kernel_strsm_nn_ru_inv_8x4_lib4c44c(jj, pU, sdu, D+jj*ldd, ldd, &d_1, pU+jj*ps, sdu, pU+jj*ps, sdu, D+jj+jj*ldd, ldd, dU+jj); // TODO vs

		kernel_sunpack_nt_4_lib4(4, pU+jj*ps+0*sdu, D+jj+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_vs_lib4(4, pU+jj*ps+4*sdu, D+jj+(ii+4)*ldd, ldd, m-ii-4);
		}
	kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, pD+0*4);
	kernel_spack_tn_4_vs_lib4(4, C+ii+(ii+4)*ldc, ldc, pD+4*4, m-ii-4);
	kernel_spotrf_nt_l_8x4_vs_lib4(ii, pU, sdu, pU, pD, ps, pD, ps, dU+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_lib4(4, pD+0*4, D+ii+(ii+0)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, pD+4*4, D+ii+(ii+4)*ldd, ldd, m-ii-4);

	kernel_spack_tn_4_lib4(4, D+ii+(ii+4)*ldd, ldd, pU+4*sdu+ii*ps);

	kernel_spack_tn_4_vs_lib4(4, C+ii+4+(ii+4)*ldc, ldc, pD, m-ii-4); // TODO pack vs with m and n, or triangle
	kernel_spotrf_nt_l_4x4_vs_lib4(ii+4, pU+4*sdu, pU+4*sdu, pD, pD, dU+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_vs_lib4(4, pD, D+ii+4+(ii+4)*ldd, ldd, m-ii-4); // TODO pack vs with m and n, or triangle
	goto u_0_return;
#endif

u_0_left_4:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_vs_lib4(4, C+jj+ii*ldc, ldc, pU+jj*ps, m-ii);
		kernel_strsm_nn_ru_inv_4x4_lib4c44c(jj, pU, D+jj*ldd, ldd, &d_1, pU+jj*ps, pU+jj*ps, D+jj+jj*ldd, ldd, dU+jj); // TODO vs
		kernel_sunpack_nt_4_vs_lib4(4, pU+jj*ps, D+jj+ii*ldd, ldd, m-ii); // TODO vs
		}
	kernel_spack_tn_4_vs_lib4(4, C+ii+ii*ldc, ldc, pD, m-ii); // TODO pack vs with m and n, or triangle
	kernel_spotrf_nt_l_4x4_vs_lib4(ii, pU, pU, pD, pD, dU+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_vs_lib4(4, pD, D+ii+ii*ldd, ldd, m-ii); // TODO pack vs with m and n, or triangle
	goto u_0_return;

u_0_return:
	return;



u_1:
	
#ifdef EXT_DEP_MALLOC

	m1 = (m+128-1)/128*128;
	tA_size = blasfeo_pm_memsize_smat(ps, m1, m1);
//	tA_size = blasfeo_memsize_smat(m, m);
	blasfeo_malloc(&mem, tA_size+64);
	blasfeo_align_64_byte(mem, (void **) &mem_align);
	blasfeo_pm_create_smat(ps, m, m, &tA, (void *) mem_align);

	sda = tA.cn;
	dA = tA.dA;

	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+jj*ps);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+jj*ps);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+jj*ps);
			kernel_strsm_nt_rl_inv_12x4_lib4(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, sda, tA.pA+ii*sda+jj*ps, sda, tA.pA+jj*sda+jj*ps, dA+jj);
			kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+jj*ps, D+jj+(ii+0)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+jj*ps, D+jj+(ii+4)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+jj*ps, D+jj+(ii+8)*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+ii*ps);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+ii*ps);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+ii*ps);
		kernel_spotrf_nt_l_12x4_lib4(ii, tA.pA+ii*sda, sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, sda, tA.pA+ii*sda+ii*ps, sda, dA+ii);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+ii*ps, D+ii+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+ii*ps, D+ii+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+ii*ps, D+ii+(ii+8)*ldd, ldd);
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+(ii+4)*ps);
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+4)*ps);
#if defined(TARGET_X64_INTEL_HASWELL)
		kernel_spack_tn_4_lib4(4, C+ii+8+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+8)*ps);
		kernel_spotrf_nt_l_8x8_lib4(ii+4, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, dA+ii+4);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+(ii+4)*ps, D+ii+4+(ii+8)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+(ii+8)*ps, D+ii+8+(ii+8)*ldd, ldd);
#else
		kernel_spotrf_nt_l_8x4_lib4(ii+4, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, dA+ii+4);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+(ii+4)*ps, D+ii+4+(ii+8)*ldd, ldd);
		kernel_spack_tn_4_lib4(4, C+ii+8+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+8)*ps);
		kernel_spotrf_nt_l_4x4_lib4(ii+8, tA.pA+(ii+8)*sda, tA.pA+(ii+8)*sda, tA.pA+(ii+8)*sda+(ii+8)*ps, tA.pA+(ii+8)*sda+(ii+8)*ps, dA+ii+8);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+8)*sda+(ii+8)*ps, D+ii+8+(ii+8)*ldd, ldd);
#endif
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto u_1_left_4;
			}
		if(m-ii<=8)
			{
			goto u_1_left_8;
			}
		else
			{
			goto u_1_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+jj*ps);
			kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+jj*ps);
			kernel_strsm_nt_rl_inv_8x4_lib4(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, sda, tA.pA+ii*sda+jj*ps, sda, tA.pA+jj*sda+jj*ps, dA+jj);
			kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+jj*ps, D+jj+(ii+0)*ldd, ldd);
			kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+jj*ps, D+jj+(ii+4)*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+ii*ps);
		kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+ii*ps);
		kernel_spotrf_nt_l_8x4_lib4(ii, tA.pA+ii*sda, sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, sda, tA.pA+ii*sda+ii*ps, sda, dA+ii);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+ii*ps, D+ii+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+ii*ps, D+ii+(ii+4)*ldd, ldd);
		kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+(ii+4)*ps);
		kernel_spotrf_nt_l_4x4_lib4(ii+4, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda+(ii+4)*ps, tA.pA+(ii+4)*sda+(ii+4)*ps, dA+ii+4);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd);
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto u_1_left_4;
			}
		else
			{
			goto u_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<ii; jj+=4)
			{
			kernel_spack_tn_4_lib4(4, C+jj+ii*ldc, ldc, tA.pA+ii*sda+jj*ps);
			kernel_strsm_nt_rl_inv_4x4_lib4(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, tA.pA+ii*sda+jj*ps, tA.pA+jj*sda+jj*ps, dA+jj);
			kernel_sunpack_nt_4_lib4(4, tA.pA+ii*sda+jj*ps, D+jj+ii*ldd, ldd);
			}
		kernel_spack_tn_4_lib4(4, C+ii+ii*ldc, ldc, tA.pA+ii*sda+ii*ps);
		kernel_spotrf_nt_l_4x4_lib4(ii, tA.pA+ii*sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, tA.pA+ii*sda+ii*ps, dA+ii);
		kernel_sunpack_nt_4_lib4(4, tA.pA+ii*sda+ii*ps, D+ii+ii*ldd, ldd);
		}
	if(ii<m)
		{
		goto u_1_left_4;
		}
#endif
	goto u_1_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
u_1_left_12:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+jj*ps);
		kernel_spack_tn_4_lib4(4, C+jj+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+jj*ps);
		kernel_spack_tn_4_vs_lib4(4, C+jj+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+jj*ps, m-ii-8);
		kernel_strsm_nt_rl_inv_12x4_vs_lib4(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, sda, tA.pA+ii*sda+jj*ps, sda, tA.pA+jj*sda+jj*ps, dA+jj, m-ii, m-jj);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+jj*ps, D+jj+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+jj*ps, D+jj+(ii+4)*ldd, ldd);
		kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+jj*ps, D+jj+(ii+8)*ldd, ldd, m-ii-8);
		}
	kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+ii*ps);
	kernel_spack_tn_4_lib4(4, C+ii+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+ii*ps);
	kernel_spack_tn_4_vs_lib4(4, C+ii+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+ii*ps, m-ii-8);
	kernel_spotrf_nt_l_12x4_vs_lib4(ii, tA.pA+ii*sda, sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, sda, tA.pA+ii*sda+ii*ps, sda, dA+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+ii*ps, D+ii+(ii+0)*ldd, ldd);
	kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+ii*ps, D+ii+(ii+4)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+ii*ps, D+ii+(ii+8)*ldd, ldd, m-ii-8);
	kernel_spack_tn_4_lib4(4, C+ii+4+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+(ii+4)*ps);
	kernel_spack_tn_4_vs_lib4(4, C+ii+4+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+4)*ps, m-ii-8);
#if defined(TARGET_X64_INTEL_HASWELL)
	kernel_spack_tn_4_vs_lib4(4, C+ii+8+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+8)*ps, m-ii-8); // TODO triangle
	kernel_spotrf_nt_l_8x8_vs_lib4(ii+4, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, dA+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+(ii+4)*ps, D+ii+4+(ii+8)*ldd, ldd, m-ii-8);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+(ii+8)*ps, D+ii+8+(ii+8)*ldd, ldd, m-ii-8); // TODO triangle
#else
	kernel_spotrf_nt_l_8x4_vs_lib4(ii+4, tA.pA+(ii+4)*sda, sda, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, tA.pA+(ii+4)*sda+(ii+4)*ps, sda, dA+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+(ii+4)*ps, D+ii+4+(ii+8)*ldd, ldd, m-ii-8);
	kernel_spack_tn_4_vs_lib4(4, C+ii+8+(ii+8)*ldc, ldc, tA.pA+(ii+8)*sda+(ii+8)*ps, m-ii-8); // TODO triangle
	kernel_spotrf_nt_l_4x4_vs_lib4(ii+8, tA.pA+(ii+8)*sda, tA.pA+(ii+8)*sda, tA.pA+(ii+8)*sda+(ii+8)*ps, tA.pA+(ii+8)*sda+(ii+8)*ps, dA+ii+8, m-ii-8, m-ii-8);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+8)*sda+(ii+8)*ps, D+ii+8+(ii+8)*ldd, ldd, m-ii-8); // TODO triangle
#endif
	goto u_1_return;
#endif

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
u_1_left_8:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_lib4(4, C+jj+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+jj*ps);
		kernel_spack_tn_4_vs_lib4(4, C+jj+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+jj*ps, m-ii-4);
		kernel_strsm_nt_rl_inv_8x4_vs_lib4(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, sda, tA.pA+ii*sda+jj*ps, sda, tA.pA+jj*sda+jj*ps, dA+jj, m-ii, m-jj);
		kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+jj*ps, D+jj+(ii+0)*ldd, ldd);
		kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+4)*sda+jj*ps, D+jj+(ii+4)*ldd, ldd, m-ii-4);
		}
	kernel_spack_tn_4_lib4(4, C+ii+(ii+0)*ldc, ldc, tA.pA+(ii+0)*sda+ii*ps);
	kernel_spack_tn_4_vs_lib4(4, C+ii+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+ii*ps, m-ii-4);
	kernel_spotrf_nt_l_8x4_vs_lib4(ii, tA.pA+ii*sda, sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, sda, tA.pA+ii*sda+ii*ps, sda, dA+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_lib4(4, tA.pA+(ii+0)*sda+ii*ps, D+ii+(ii+0)*ldd, ldd);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+4)*sda+ii*ps, D+ii+(ii+4)*ldd, ldd, m-ii-4);
	kernel_spack_tn_4_vs_lib4(4, C+ii+4+(ii+4)*ldc, ldc, tA.pA+(ii+4)*sda+(ii+4)*ps, m-ii-4); // TODO triangle
	kernel_spotrf_nt_l_4x4_vs_lib4(ii+4, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda, tA.pA+(ii+4)*sda+(ii+4)*ps, tA.pA+(ii+4)*sda+(ii+4)*ps, dA+ii+4, m-ii-4, m-ii-4);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+(ii+4)*sda+(ii+4)*ps, D+ii+4+(ii+4)*ldd, ldd, m-ii-4); // TODO triangle
	goto u_1_return;
#endif

u_1_left_4:
	for(jj=0; jj<ii; jj+=4)
		{
		kernel_spack_tn_4_vs_lib4(4, C+jj+ii*ldc, ldc, tA.pA+ii*sda+jj*ps, m-ii);
		kernel_strsm_nt_rl_inv_4x4_vs_lib4(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, tA.pA+ii*sda+jj*ps, tA.pA+ii*sda+jj*ps, tA.pA+jj*sda+jj*ps, dA+jj, m-ii, m-jj);
		kernel_sunpack_nt_4_vs_lib4(4, tA.pA+ii*sda+jj*ps, D+jj+ii*ldd, ldd, m-ii);
		}
	kernel_spack_tn_4_vs_lib4(4, C+ii+ii*ldc, ldc, tA.pA+ii*sda+ii*ps, m-ii); // TODO triangle
	kernel_spotrf_nt_l_4x4_vs_lib4(ii, tA.pA+ii*sda, tA.pA+ii*sda, tA.pA+ii*sda+ii*ps, tA.pA+ii*sda+ii*ps, dA+ii, m-ii, m-ii);
	kernel_sunpack_nt_4_vs_lib4(4, tA.pA+ii*sda+ii*ps, D+ii+ii*ldd, ldd, m-ii); // TODO triangle
	goto u_1_return;

u_1_return:
	blasfeo_free(mem);
	return;

#else // EXT_DEP_MALLOC

	exit(1);

#endif // EXT_DEP_MALLOC

	// never to get here
	return;

	}



void blasfeo_hp_spotrf_l_mn(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{

#if defined(PRINT_NAME)
#ifdef EXT_DEP
	printf("\nblasfeo_hp_spotrf_l_mn (cm) %d %d %p %d %d %p %d %d\n", m, n, sC, ci, cj, sD, di, dj);
#endif
#endif

	if(m<=0)
		return;

	// extract pointer to column-major matrices from structures
	int ldc = sC->m;
	int ldd = sD->m;
	float *C = sC->pA + ci + cj*ldc;
	float *D = sD->pA + di + dj*ldd;

//	printf("\n%p %d %p %d\n", C, ldc, D, ldd);

	int ii, jj;

	const int ps = 4; //D_PS;

#if defined(TARGET_GENERIC)
	float pU[4*K_MAX_STACK];
	float dU[K_MAX_STACK];
#else
	ALIGNED( float pU[M_KERNEL*K_MAX_STACK], 64 );
	ALIGNED( float dU[K_MAX_STACK], 64 );
#endif
	int sdu = (m+3)/4*4;
	sdu = sdu<K_MAX_STACK ? sdu : K_MAX_STACK;

	struct blasfeo_pm_smat tA;
	int sda;
	float *dA;
	int tA_size;
	void *mem;
	char *mem_align;
	int m1, n1, k1;
	int pack_B;

	const int m_kernel = M_KERNEL;
	const int l1_cache_el = L1_CACHE_EL;
	const int reals_per_cache_line = CACHE_LINE_EL;

	const int m_cache = (m+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	const int n_cache = (n+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
//	const int k_cache = (k+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	const int m_kernel_cache = (m_kernel+reals_per_cache_line-1)/reals_per_cache_line*reals_per_cache_line;
	int m_min = m_cache<m_kernel_cache ? m_cache : m_kernel_cache;
//	int n_min = n_cache<m_kernel_cache ? n_cache : m_kernel_cache;


	float d_1 = 1.0;
	int kend;


//	goto l_1;
//	goto l_2;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	if(m>=200 | m>K_MAX_STACK)
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	if(m>=64 | m>K_MAX_STACK)
#else
	if(m>=12 | m>K_MAX_STACK)
#endif
		{
		goto l_2;
		}
	else
		{
		goto l_1;
		}

	// never to get here
	return;


l_1:

	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_12_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu);
			}
		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_12x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_12_vs_lib4(kend, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
				}
			else // spotrf
				{
				if(jj<n-11)
					{
					kernel_spotrf_nt_l_12x4_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
					kernel_spack_nn_8_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, sdu);
#if defined(TARGET_X64_INTEL_HASWELL)
					kernel_spotrf_nt_l_8x8_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4);
					kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps);
#else
					kernel_spotrf_nt_l_8x4_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4);
					kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps);
					kernel_spotrf_nt_l_4x4_lib44cc(jj+8, pU+8*sdu, pU+8*sdu, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dU+jj+8);
#endif
					}
				else
					{
					kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
					kend = n-jj<4 ? n-jj : 4;
					kernel_spack_nn_8_vs_lib4(kend, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, sdu, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
					if(jj<n-8)
						{
						kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, n-jj-4);
						kend = n-jj-4<4 ? n-jj-4 : 4;
						kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps, m-ii-4);
						}
					else
						{
#endif
					if(jj<n-4)
						{
						kernel_spotrf_nt_l_8x4_vs_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, n-jj-4);
						kend = n-jj-4<4 ? n-jj-4 : 4;
						kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps, m-ii-4);
						if(jj<n-8)
							{
							kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+8, pU+8*sdu, pU+8*sdu, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dU+jj+8, m-ii-8, n-jj-8);
							}
						}
#if defined(TARGET_X64_INTEL_HASWELL)
						}
#endif
					}
				}
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_1_left_4;
			}
		if(m-ii<=8)
			{
			goto l_1_left_8;
			}
		else
			{
			goto l_1_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; ii<m-7; ii+=8)
		{
		for(jj=0; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_8_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu);
			}
		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_8x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_8_vs_lib4(kend, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
				}
			else // spotrf
				{
				if(jj<n-7)
					{
					kernel_spotrf_nt_l_8x4_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
					kernel_spack_nn_4_lib4(4, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps);
					kernel_spotrf_nt_l_4x4_lib44cc(jj+4, pU+4*sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4);
					}
				else
					{
					kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
					kend = n-jj<4 ? n-jj : 4;
					kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, m-ii-4);
					if(jj<n-4)
						{
						kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, pU+4*sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, n-jj-4);
						}
					}
				}
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_1_left_4;
			}
		else
			{
			goto l_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib4cccc(jj, pU, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj);
			kernel_spack_nn_4_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps);
			}
		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_4x4_vs_lib4cccc(jj, pU, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_4_vs_lib4(kend, D+ii+jj*ldd, ldd, pU+jj*ps, m-ii);
				}
			else // spotrf
				{
				if(jj<n-3)
					{
					kernel_spotrf_nt_l_4x4_lib44cc(jj, pU, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj);
					}
				else
					{
					kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, pU, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
					}
				}
			}
		}
	if(ii<m)
		{
		goto l_1_left_4;
		}
#endif
	goto l_1_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL)
l_1_left_12:
	for(jj=0; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_12x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		kernel_spack_nn_12_vs_lib4(4, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
		}
	if(jj<n)
		{
		kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_8_vs_lib4(kend, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, sdu, m-ii-4);
#if defined(TARGET_X64_INTEL_HASWELL)
		if(jj<n-8)
			{
			kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-(ii+4), n-(jj+4));
			// XXX not needed before return
//			kend = n-jj-4<4 ? n-jj-4 : 4;
//			kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps, m-ii-8);
			goto l_1_return;
			}
#endif
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_8x4_vs_lib44cc(jj+4, pU+4*sdu, sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-(ii+4), n-(jj+4));
			kend = n-jj-4<4 ? n-jj-4 : 4;
			kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, pU+8*sdu+(jj+4)*ps, m-ii-8);
			if(jj<n-8)
				{
				kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+8, pU+8*sdu, pU+8*sdu, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dU+jj+8, m-ii-8, n-jj-8);
				}
			}
		}
	goto l_1_return;
#endif

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
l_1_left_8:
	for(jj=0; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_8x4_vs_lib4cccc(jj, pU, sdu, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_8_vs_lib4(kend, D+ii+jj*ldd, ldd, pU+jj*ps, sdu, m-ii);
		}
	if(jj<n)
		{
#if defined(TARGET_X64_INTEL_HASWELL)
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_8x8_vs_lib44cc(jj, pU, sdu, pU, sdu, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
			// XXX not needed before return
//			kend = n-jj<4 ? n-jj : 4;
//			kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, m-ii-4);
			goto l_1_return;
			}
#endif
		kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, pU, sdu, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, pU+4*sdu+jj*ps, m-ii-4);
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, pU+4*sdu, pU+4*sdu, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dU+jj+4, m-ii-4, n-jj-4);
			}
		}
	goto l_1_return;
#endif

l_1_left_4:
	for(jj=0; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_4x4_vs_lib4cccc(jj, pU, D+jj, ldd, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_4_vs_lib4(kend, D+ii+jj*ldd, ldd, pU+jj*ps, m-ii);
		}
	if(jj<n)
		{
		kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, pU, pU, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dU+jj, m-ii, n-jj);
		}
	goto l_1_return;

l_1_return:
	return;


l_2:
	
#ifdef EXT_DEP_MALLOC

	m1 = (m+128-1)/128*128;
	n1 = (n+128-1)/128*128;
	tA_size = blasfeo_pm_memsize_smat(ps, m1, n1);
	blasfeo_malloc(&mem, tA_size+64);
	blasfeo_align_64_byte(mem, (void **) &mem_align);
	blasfeo_pm_create_smat(ps, m, n, &tA, (void *) mem_align);

	sda = tA.cn;
	dA = tA.dA;

	ii = 0;
#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		jj = 0;
		for(; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_12x4_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_12_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda);
			}
		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_12x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_12_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
				}
			else // spotrf
				{
				if(jj<n-11)
					{
					kernel_spotrf_nt_l_12x4_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
					kernel_spack_nn_8_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, sda);
#if defined(TARGET_X64_INTEL_HASWELL)
					kernel_spotrf_nt_l_8x8_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
					kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps);
#else
					kernel_spotrf_nt_l_8x4_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
					kernel_spack_nn_4_lib4(4, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps);
					kernel_spotrf_nt_l_4x4_lib44cc(jj+8, tA.pA+(ii+8)*sda, tA.pA+(jj+8)*sda, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dA+jj+8);
#endif
					}
				else
					{
					kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
					kend = n-jj<4 ? n-jj : 4;
					kernel_spack_nn_8_vs_lib4(kend, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, sda, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
					if(jj<n-8)
						{
						kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, n-jj-4);
						kend = n-jj-4<4 ? n-jj-4 : 4;
						kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-4);
						}
					else
						{
#endif
					if(jj<n-4)
						{
						kernel_spotrf_nt_l_8x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, n-jj-4);
						kend = n-jj-4<4 ? n-jj-4 : 4;
						kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-4);
						if(jj<n-8)
							{
							kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+8, tA.pA+(ii+8)*sda, tA.pA+(jj+8)*sda, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dA+jj+8, m-ii-8, n-jj-8);
							}
						}
#if defined(TARGET_X64_INTEL_HASWELL)
						}
#endif
					}
				}
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_2_left_4;
			}
		if(m-ii<=8)
			{
			goto l_2_left_8;
			}
		else
			{
			goto l_2_left_12;
			}
		}
#elif 0//defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		jj = 0;
		for(; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_8x4_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_8_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda);
			}
		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_8x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_8_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
				}
			else // spotrf
				{
				if(jj<n-7)
					{
					kernel_spotrf_nt_l_8x4_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
					kernel_spack_nn_4_lib4(4, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps);
					kernel_spotrf_nt_l_4x4_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4);
					}
				else
					{
					kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
					kend = n-jj<4 ? n-jj : 4;
					kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, m-ii-4);
					if(jj<n-4)
						{
						kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, n-jj-4);
						}
					}
				}
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto l_2_left_4;
			}
		else
			{
			goto l_2_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		jj = 0;
		for(; jj<ii & jj<n-3; jj+=4)
			{
			kernel_strsm_nt_rl_inv_4x4_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj);
			kernel_spack_nn_4_lib4(4, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps);
			}

		if(jj<n)
			{
			if(jj<ii) // strsm
				{
				kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
				kend = n-jj<4 ? n-jj : 4;
				kernel_spack_nn_4_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, m-ii);
				}
			else // spotrf
				{
				if(jj<n-3)
					{
					kernel_spotrf_nt_l_4x4_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj);
					}
				else
					{
					kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
					}
				}
			}
		}
	if(ii<m)
		{
		goto l_2_left_4;
		}
#endif
	goto l_2_return;

#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
l_2_left_12:
	jj = 0;
	for(; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_12x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_12_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
		}
	if(jj<n)
		{
		kernel_spotrf_nt_l_12x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_8_vs_lib4(kend, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, sda, m-ii-4);
#if defined(TARGET_X64_INTEL_HASWELL)
		if(jj<n-8)
			{
			kernel_spotrf_nt_l_8x8_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-(ii+4), n-(jj+4));
			// XXX not needed before return
//			kend = n-jj-4<4 ? n-jj-4 : 4;
//			kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-8);
			goto l_2_return;
			}
#endif
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_8x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-(ii+4), n-(jj+4));
			kend = n-jj-4<4 ? n-jj-4 : 4;
			kernel_spack_nn_4_vs_lib4(kend, D+ii+8+(jj+4)*ldd, ldd, tA.pA+(ii+8)*sda+(jj+4)*ps, m-ii-8);
			if(jj<n-8)
				{
				kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+8, tA.pA+(ii+8)*sda, tA.pA+(jj+8)*sda, C+ii+8+(jj+8)*ldc, ldc, D+ii+8+(jj+8)*ldd, ldd, dA+jj+8, m-ii-8, n-jj-8);
				}
			}
		}
	goto l_2_return;
#endif


#if 0//defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
l_2_left_8:
	for(jj=0; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_8x4_vs_lib44ccc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_8_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, sda, m-ii);
		}
	if(jj<n)
		{
#if defined(TARGET_X64_INTEL_HASWELL)
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_8x8_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
			// XXX not needed before return
//			kend = n-jj<4 ? n-jj : 4;
//			kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, m-ii-4);
			goto l_2_return;
			}
#endif
		kernel_spotrf_nt_l_8x4_vs_lib44cc(jj, tA.pA+ii*sda, sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_4_vs_lib4(kend, D+ii+4+jj*ldd, ldd, tA.pA+(ii+4)*sda+jj*ps, m-ii-4);
		if(jj<n-4)
			{
			kernel_spotrf_nt_l_4x4_vs_lib44cc(jj+4, tA.pA+(ii+4)*sda, tA.pA+(jj+4)*sda, C+ii+4+(jj+4)*ldc, ldc, D+ii+4+(jj+4)*ldd, ldd, dA+jj+4, m-ii-4, n-jj-4);
			}
		}
	goto l_2_return;
#endif

l_2_left_4:
	for(jj=0; jj<ii & jj<n; jj+=4)
		{
		kernel_strsm_nt_rl_inv_4x4_vs_lib44ccc(jj, tA.pA+ii*sda, tA.pA+jj*sda, &d_1, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, D+jj+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		kend = n-jj<4 ? n-jj : 4;
		kernel_spack_nn_4_vs_lib4(kend, D+ii+jj*ldd, ldd, tA.pA+ii*sda+jj*ps, m-ii);
		}
	if(jj<n)
		{
		kernel_spotrf_nt_l_4x4_vs_lib44cc(jj, tA.pA+ii*sda, tA.pA+jj*sda, C+ii+jj*ldc, ldc, D+ii+jj*ldd, ldd, dA+jj, m-ii, n-jj);
		}
	goto l_2_return;

l_2_return:
	free(mem);
	return;

#else // EXT_DEP_MALLOC

	exit(1);

#endif // EXT_DEP_MALLOC

	// never to get here
	return;

	}



#if defined(LA_HIGH_PERFORMANCE)



void blasfeo_spotrf_l(int m, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	blasfeo_hp_spotrf_l(m, sC, ci, cj, sD, di, dj);
	}



void blasfeo_spotrf_u(int m, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	blasfeo_hp_spotrf_u(m, sC, ci, cj, sD, di, dj);
	}



void blasfeo_spotrf_l_mn(int m, int n, struct blasfeo_smat *sC, int ci, int cj, struct blasfeo_smat *sD, int di, int dj)
	{
	blasfeo_hp_spotrf_l_mn(m, n, sC, ci, cj, sD, di, dj);
	}



#endif


