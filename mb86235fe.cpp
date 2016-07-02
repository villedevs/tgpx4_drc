// license:BSD-3-Clause
// copyright-holders:Ville Linde

/******************************************************************************

    Front-end for MB86235 recompiler

******************************************************************************/

#include "emu.h"
#include "mb86235fe.h"


#define AA_USED(desc,x)				do { (desc).regin[0] |= 1 << (x); } while(0)
#define AA_MODIFIED(desc,x)			do { (desc).regout[0] |= 1 << (x); } while(0)
#define AB_USED(desc,x)				do { (desc).regin[0] |= 1 << (8+x); } while(0)
#define AB_MODIFIED(desc,x)			do { (desc).regout[0] |= 1 << (8+x); } while(0)
#define MA_USED(desc,x)				do { (desc).regin[0] |= 1 << (16+x); } while(0)
#define MA_MODIFIED(desc,x)			do { (desc).regout[0] |= 1 << (16+x); } while(0)
#define MB_USED(desc,x)				do { (desc).regin[0] |= 1 << (24+x); } while(0)
#define MB_MODIFIED(desc,x)			do { (desc).regout[0] |= 1 << (24+x); } while(0)
#define AR_USED(desc,x)				do { (desc).regin[1] |= 1 << (24+x); } while(0)
#define AR_MODIFIED(desc,x)			do { (desc).regout[1] |= 1 << (24+x); } while(0)

#define AZ_USED(desc)				do { (desc).regin[1] |= 1 << 0; } while (0)
#define AZ_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 0; } while (0)
#define AN_USED(desc)				do { (desc).regin[1] |= 1 << 1; } while (0)
#define AN_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 1; } while (0)
#define AV_USED(desc)				do { (desc).regin[1] |= 1 << 2; } while (0)
#define AV_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 2; } while (0)
#define AU_USED(desc)				do { (desc).regin[1] |= 1 << 3; } while (0)
#define AU_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 3; } while (0)
#define AD_USED(desc)				do { (desc).regin[1] |= 1 << 4; } while (0)
#define AD_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 4; } while (0)
#define ZC_USED(desc)				do { (desc).regin[1] |= 1 << 5; } while (0)
#define ZC_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 5; } while (0)
#define IL_USED(desc)				do { (desc).regin[1] |= 1 << 6; } while (0)
#define IL_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 6; } while (0)
#define NR_USED(desc)				do { (desc).regin[1] |= 1 << 7; } while (0)
#define NR_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 7; } while (0)
#define ZD_USED(desc)				do { (desc).regin[1] |= 1 << 8; } while (0)
#define ZD_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 8; } while (0)
#define MN_USED(desc)				do { (desc).regin[1] |= 1 << 9; } while (0)
#define MN_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 9; } while (0)
#define MZ_USED(desc)				do { (desc).regin[1] |= 1 << 10; } while (0)
#define MZ_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 10; } while (0)
#define MV_USED(desc)				do { (desc).regin[1] |= 1 << 11; } while (0)
#define MV_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 11; } while (0)
#define MU_USED(desc)				do { (desc).regin[1] |= 1 << 12; } while (0)
#define MU_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 12; } while (0)
#define MD_USED(desc)				do { (desc).regin[1] |= 1 << 13; } while (0)
#define MD_MODIFIED(desc)			do { (desc).regout[1] |= 1 << 13; } while (0)


mb86235_frontend::mb86235_frontend(mb86235_device *core, UINT32 window_start, UINT32 window_end, UINT32 max_sequence)
	: drc_frontend(*core, window_start, window_end, max_sequence),
	m_core(core)
{
}


bool mb86235_frontend::describe(opcode_desc &desc, const opcode_desc *prev)
{
	UINT64 opcode = desc.opptr.q[0] = m_core->m_direct->read_qword(desc.pc, 0);

	switch ((opcode >> 61) & 7)
	{
		case 0:     // ALU / MUL / double transfer (type 1)
			describe_alu(desc, (opcode >> 42) & 0x3fff);
			describe_mul(desc, (opcode >> 27) & 0x7fff);
			describe_double_xfer1(desc);
			break;
		case 1:     // ALU / MUL / transfer (type 1)
			describe_alu(desc, (opcode >> 42) & 0x3fff);
			describe_mul(desc, (opcode >> 27) & 0x7fff);
			describe_xfer1(desc);
			break;
		case 2:     // ALU / MUL / control
			describe_alu(desc, (opcode >> 42) & 0x3fff);
			describe_mul(desc, (opcode >> 27) & 0x7fff);
			describe_control(desc);
			break;
		case 4:     // ALU or MUL / double transfer (type 2)
			if (opcode & ((UINT64)(1) << 41))
				describe_alu(desc, (opcode >> 42) & 0x3fff);
			else
				describe_mul(desc, (opcode >> 42) & 0x3fff);
			describe_double_xfer2(desc);
			break;
		case 5:     // ALU or MUL / transfer (type 2)
			if (opcode & ((UINT64)(1) << 41))
				describe_alu(desc, (opcode >> 42) & 0x3fff);
			else
				describe_mul(desc, (opcode >> 42) & 0x3fff);
			describe_xfer2(desc);
			break;
		case 6:     // ALU or MUL / control
			if (opcode & ((UINT64)(1) << 41))
				describe_alu(desc, (opcode >> 42) & 0x3fff);
			else
				describe_mul(desc, (opcode >> 42) & 0x3fff);
			describe_control(desc);
			break;
		case 7:     // transfer (type 3)
			describe_xfer3(desc);
			break;

		default:
			break;
	}

	return false;
}

void mb86235_frontend::describe_alu_input(opcode_desc &desc, int reg)
{
	switch (reg >> 3)
	{
		case 0:
			AA_USED(desc, reg & 7);
			break;

		case 1:
			AB_USED(desc, reg & 7);
			break;

		default:
			break;
	}
}

void mb86235_frontend::describe_mul_input(opcode_desc &desc, int reg)
{
	switch (reg >> 3)
	{
		case 0:
			MA_USED(desc, reg & 7);
			break;

		case 1:
			MB_USED(desc, reg & 7);
			break;

		default:
			break;
	}
}

void mb86235_frontend::describe_alumul_output(opcode_desc &desc, int reg)
{
	switch (reg >> 3)
	{
		case 0:
			MA_MODIFIED(desc, reg & 7);
			break;

		case 1:
			MB_MODIFIED(desc, reg & 7);
			break;

		case 2:
			AA_MODIFIED(desc, reg & 7);
			break;

		case 3:
			AB_MODIFIED(desc, reg & 7);
			break;

		default:
			break;
	}
}

void mb86235_frontend::describe_alu(opcode_desc &desc, UINT32 aluop)
{
	int i1 = (aluop >> 10) & 0xf;
	int i2 = (aluop >> 5) & 0x1f;
	int io = aluop & 0x1f;
	int op = (aluop >> 14) & 0x1f;

	switch (op)
	{
		case 0x00:		// FADD
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x01:		// FADDZ
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			ZC_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x02:		// FSUB
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x03:		// FSUBZ
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			ZC_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x04:		// FCMP
			describe_alu_input(desc, i1); describe_alu_input(desc, i2);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x05:		// FABS
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x06:		// FABC
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x07:		// NOP
			break;
		case 0x08:		// FEA
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x09:		// FES
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x0a:		// FRCP
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			ZD_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x0b:		// FRSQ
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			NR_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AU_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x0c:		// FLOG
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			IL_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x0d:		// CIF
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			break;
		case 0x0e:		// CFI
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AD_MODIFIED(desc);
			break;
		case 0x0f:		// CFIB
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AD_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x10:		// ADD
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x11:		// ADDZ
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			ZC_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x12:		// SUB
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x13:		// SUBZ
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			ZC_MODIFIED(desc);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x14:		// CMP
			describe_alu_input(desc, i1); describe_alu_input(desc, i2);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x15:		// ABS
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			break;
		case 0x16:		// ATR
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			break;
		case 0x17:		// ATRZ
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			ZC_MODIFIED(desc);
			break;
		case 0x18:		// AND
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x19:		// OR
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x1a:		// XOR
			describe_alu_input(desc, i1); describe_alu_input(desc, i2); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x1b:		// NOT
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x1c:		// LSR
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x1d:		// LSL
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			AV_MODIFIED(desc);
			AU_MODIFIED(desc);
			break;
		case 0x1e:		// ASR
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			break;
		case 0x1f:		// ASL
			describe_alu_input(desc, i1); describe_alumul_output(desc, io);
			AN_MODIFIED(desc);
			AZ_MODIFIED(desc);
			break;
	}
}

void mb86235_frontend::describe_mul(opcode_desc &desc, UINT32 mulop)
{
	int i1 = (mulop >> 10) & 0xf;
	int i2 = (mulop >> 5) & 0x1f;
	int io = mulop & 0x1f;
	int m = mulop & 0x4000;

	describe_mul_input(desc, i1);
	describe_mul_input(desc, i2);
	describe_alumul_output(desc, io);

	if (m)
	{
		// FMUL
		MN_MODIFIED(desc);
		MZ_MODIFIED(desc);
		MV_MODIFIED(desc);
		MU_MODIFIED(desc);
		MD_MODIFIED(desc);
	}
	else
	{
		// MUL
		MN_MODIFIED(desc);
		MZ_MODIFIED(desc);
		MV_MODIFIED(desc);
	}
}

void mb86235_frontend::describe_xfer1(opcode_desc &desc)
{

}

void mb86235_frontend::describe_double_xfer1(opcode_desc &desc)
{

}

void mb86235_frontend::describe_xfer2(opcode_desc &desc)
{

}

void mb86235_frontend::describe_double_xfer2(opcode_desc &desc)
{

}

void mb86235_frontend::describe_xfer3(opcode_desc &desc)
{

}

void mb86235_frontend::describe_control(opcode_desc &desc)
{

}