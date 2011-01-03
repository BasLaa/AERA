//	replicode_defs.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef	replicode_defs_h
#define	replicode_defs_h


#define	EXECUTIVE_DEVICE	0xA1000000

#define	VIEW_CODE_MAX_SIZE	13	//	size of the code of the largest view (grp view) + 1 (oid used by rMems); view set opcode's index is 0.

#define	VIEW_OPCODE		0
#define	VIEW_SYNC		1
#define	VIEW_IJT		2	//	iptr to timestamp (+3 atoms)
#define	VIEW_SLN		3
#define	VIEW_RES		4
#define	VIEW_HOST		5
#define	VIEW_ORG		6
#define	VIEW_ACT		7
#define	GRP_VIEW_COV	7
#define	GRP_VIEW_VIS	8
#define	VIEW_OID		12

#define	VIEW_ARITY		6
#define	PGM_VIEW_ARITY	7

#define	OBJECT_CLASS	0

#define	GRP_UPR				1
#define	GRP_SLN_THR			2
#define	GRP_ACT_THR			3
#define	GRP_VIS_THR			4
#define	GRP_C_SLN			5
#define	GRP_C_SLN_THR		6
#define	GRP_C_ACT			7
#define	GRP_C_ACT_THR		8
#define	GRP_DCY_PER			9
#define	GRP_DCY_TGT			10
#define	GRP_DCY_PRD			11
#define	GRP_DCY_AUTO		12
#define	GRP_SLN_CHG_THR		13
#define	GRP_SLN_CHG_PRD		14
#define	GRP_ACT_CHG_THR		15
#define	GRP_ACT_CHG_PRD		16
#define	GRP_AVG_SLN			17
#define	GRP_HIGH_SLN		18
#define	GRP_LOW_SLN			19
#define	GRP_AVG_ACT			20
#define	GRP_HIGH_ACT		21
#define	GRP_LOW_ACT			22
#define	GRP_HIGH_SLN_THR	23
#define	GRP_LOW_SLN_THR		24
#define	GRP_SLN_NTF_PRD		25
#define	GRP_HIGH_ACT_THR	26
#define	GRP_LOW_ACT_THR		27
#define	GRP_ACT_NTF_PRD		28
#define	GRP_NTF_NEW			29
#define	GRP_LOW_RES_THR		30
#define	GRP_NTF_GRPS		31
#define	GRP_ARITY			32

#define	PGM_TPL_ARGS	1
#define	PGM_INPUTS		2
#define	PGM_PRODS		3
#define	PGM_ARITY		4

#define	IPGM_PGM	1
#define	IPGM_ARGS	2
#define	IPGM_RUN	3
#define	IPGM_TSC	4
#define	IPGM_NFR	5
#define	IPGM_ARITY	6

#define	ICPP_PGM_NAME	1
#define	ICPP_PGM_ARGS	2
#define	ICPP_PGM_RUN	3
#define	ICPP_PGM_TSC	4
#define	ICPP_PGM_NFR	5
#define	ICPP_PGM_ARITY	6

#define	MD_HEAD		1
#define	MD_OUT_GRPS	2
#define	MD_NTF_GRPS	3

#define	FMD_TSC		4
#define	IMD_FMD		4

#define	FMD_ARITY	5
#define	IMD_ARITY	5

#define	MK_RDX_ARITY		4
#define	MK_ANTI_RDX_ARITY	3

#define	CMD_FUNCTION	1
#define	CMD_DEVICE		2
#define	CMD_ARGS		3

#define	VAR_TOL			1
#define	VAR_ARITY		2

#define	VAL_HLD_ARITY	2

#define	FACT_OBJ		1
#define	FACT_TIME		2
#define	FACT_CFD		3
#define	FACT_ARITY		4

#define	MK_PRED_OBJ		1
#define	MK_PRED_FMD		2
#define	MK_PRED_CFD		3
#define	MK_PRED_ARITY	4

#define	MK_ASMP_OBJ		1
#define	MK_ASMP_SRC		2
#define	MK_ASMP_CFD		3
#define	MK_ASMP_ARITY	4

#define	MK_GOAL_OBJ		1
#define	MK_GOAL_IMD		2
#define	MK_GOAL_ARITY	3

#define	MK_HYP_OBJ		1
#define	MK_HYP_ARITY	2

#define	MK_SIM_OBJ		1
#define	MK_SIM_SRC		2
#define	MK_SIM_ARITY	3

#define	MK_SUCCESS_OBJ		1
#define	MK_SUCCESS_P_RATE	2
#define	MK_SUCCESS_N_RATE	3
#define	MK_SUCCESS_ARITY	4


#endif