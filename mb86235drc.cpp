// license:BSD-3-Clause
// copyright-holders:Ville Linde

/******************************************************************************

    MB86235 UML recompiler core

******************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "mb86235.h"
#include "mb86235fe.h"
#include "cpu/drcfe.h"
#include "cpu/drcuml.h"
#include "cpu/drcumlsh.h"

using namespace uml;


// map variables
#define MAPVAR_PC                       M0
#define MAPVAR_CYCLES                   M1

// exit codes
#define EXECUTE_OUT_OF_CYCLES           0
#define EXECUTE_MISSING_CODE            1
#define EXECUTE_UNMAPPED_CODE           2
#define EXECUTE_RESET_CACHE             3


#define AR(reg)					mem(&m_core->ar[(reg)])
#define AA(reg)					m_regmap[(reg)]
#define AB(reg)					m_regmap[(reg)+8]
#define MA(reg)					m_regmap[(reg)+16]
#define MB(reg)					m_regmap[(reg)+24]


inline void mb86235_device::alloc_handle(drcuml_state *drcuml, code_handle **handleptr, const char *name)
{
	if (*handleptr == nullptr)
		*handleptr = drcuml->handle_alloc(name);
}



static void cfunc_unimplemented(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_op();
}

static void cfunc_unimplemented_alu(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_alu();
}

static void cfunc_unimplemented_mul(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_mul();
}

static void cfunc_unimplemented_control(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_control();
}

static void cfunc_unimplemented_xfer1(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_xfer1();
}

static void cfunc_unimplemented_double_xfer1(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_double_xfer1();
}

static void cfunc_unimplemented_double_xfer2(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->unimplemented_double_xfer2();
}

static void cfunc_pcs_overflow(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->pcs_overflow();
}

static void cfunc_pcs_underflow(void *param)
{
	mb86235_device *cpu = (mb86235_device *)param;
	cpu->pcs_underflow();
}



void mb86235_device::unimplemented_op()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented op %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented op %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::unimplemented_alu()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented alu %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented alu %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::unimplemented_mul()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented mul %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented mul %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::unimplemented_control()
{
	UINT32 cop = m_core->arg0;
	printf("MB86235: PC=%08X: Unimplemented control %02X\n", m_core->pc, cop);
	fatalerror("MB86235: PC=%08X: Unimplemented control %02X\n", m_core->pc, cop);
}

void mb86235_device::unimplemented_xfer1()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented xfer1 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented xfer1 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::unimplemented_double_xfer1()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented double xfer1 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented double xfer1 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::unimplemented_double_xfer2()
{
	UINT64 op = m_core->arg64;
	printf("MB86235: PC=%08X: Unimplemented double xfer2 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
	fatalerror("MB86235: PC=%08X: Unimplemented double xfer2 %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
}

void mb86235_device::pcs_overflow()
{
	printf("MB86235: PC=%08X: PCS overflow\n", m_core->pc);
	fatalerror("MB86235: PC=%08X: PCS overflow\n", m_core->pc);
}

void mb86235_device::pcs_underflow()
{
	printf("MB86235: PC=%08X: PCS underflow\n", m_core->pc);
	fatalerror("MB86235: PC=%08X: PCS underflow\n", m_core->pc);
}




/*-------------------------------------------------
load_fast_iregs - load any fast integer
registers
-------------------------------------------------*/

inline void mb86235_device::load_fast_iregs(drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(m_regmap); regnum++)
	{
		if (m_regmap[regnum].is_int_register())
		{
		}
	}
}


/*-------------------------------------------------
save_fast_iregs - save any fast integer
registers
-------------------------------------------------*/

void mb86235_device::save_fast_iregs(drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(m_regmap); regnum++)
	{
		if (m_regmap[regnum].is_int_register())
		{
		}
	}
}




void mb86235_device::run_drc()
{
	drcuml_state *drcuml = m_drcuml.get();
	int execute_result;

	/* execute */
	do
	{
		execute_result = drcuml->execute(*m_entry);

		/* if we need to recompile, do it */
		if (execute_result == EXECUTE_MISSING_CODE)
		{
			compile_block(m_core->pc);
		}
		else if (execute_result == EXECUTE_UNMAPPED_CODE)
		{
			fatalerror("Attempted to execute unmapped code at PC=%08X\n", m_core->pc);
		}
		else if (execute_result == EXECUTE_RESET_CACHE)
		{
			flush_cache();
		}
	} while (execute_result != EXECUTE_OUT_OF_CYCLES);
}

void mb86235_device::compile_block(offs_t pc)
{
	compiler_state compiler = { 0 };

	const opcode_desc *seqhead, *seqlast;
	const opcode_desc *desclist;
	bool override = false;

	drcuml_block *block;

	desclist = m_drcfe->describe_code(pc);

	bool succeeded = false;
	while (!succeeded)
	{
		try
		{
			block = m_drcuml->begin_block(4096);

			for (seqhead = desclist; seqhead != nullptr; seqhead = seqlast->next())
			{
				const opcode_desc *curdesc;
				UINT32 nextpc;

				/* determine the last instruction in this sequence */
				for (seqlast = seqhead; seqlast != nullptr; seqlast = seqlast->next())
					if (seqlast->flags & OPFLAG_END_SEQUENCE)
						break;
				assert(seqlast != nullptr);

				/* if we don't have a hash for this mode/pc, or if we are overriding all, add one */
				if (override || m_drcuml->hash_exists(0, seqhead->pc))
					UML_HASH(block, 0, seqhead->pc);                                        // hash    mode,pc

																							/* if we already have a hash, and this is the first sequence, assume that we */
																							/* are recompiling due to being out of sync and allow future overrides */
				else if (seqhead == desclist)
				{
					override = true;
					UML_HASH(block, 0, seqhead->pc);                                        // hash    mode,pc
				}

				/* otherwise, redispatch to that fixed PC and skip the rest of the processing */
				else
				{
					UML_LABEL(block, seqhead->pc | 0x80000000);                             // label   seqhead->pc
					UML_HASHJMP(block, 0, seqhead->pc, *m_nocode);                          // hashjmp <0>,seqhead->pc,nocode
					continue;
				}

				/* label this instruction, if it may be jumped to locally */
				if (seqhead->flags & OPFLAG_IS_BRANCH_TARGET)
					UML_LABEL(block, seqhead->pc | 0x80000000);                             // label   seqhead->pc

																							/* iterate over instructions in the sequence and compile them */
				for (curdesc = seqhead; curdesc != seqlast->next(); curdesc = curdesc->next())
					generate_sequence_instruction(block, &compiler, curdesc);

				/* if we need to return to the start, do it */
				if (seqlast->flags & OPFLAG_RETURN_TO_START)
					nextpc = pc;
				/* otherwise we just go to the next instruction */
				else
					nextpc = seqlast->pc + (seqlast->skipslots + 1);

				/* count off cycles and go there */
				generate_update_cycles(block, &compiler, nextpc, TRUE);                     // <subtract cycles>

				if (seqlast->next() == nullptr || seqlast->next()->pc != nextpc)
					UML_HASHJMP(block, 0, nextpc, *m_nocode);                               // hashjmp <mode>,nextpc,nocode
			}

			block->end();
			succeeded = true;
		}
		catch (drcuml_block::abort_compilation &)
		{
			flush_cache();
		}
	}
}



void mb86235_device::static_generate_entry_point()
{
	code_label skip = 1;
	drcuml_block *block;

	/* begin generating */
	block = m_drcuml->begin_block(20);

	/* forward references */
	alloc_handle(m_drcuml.get(), &m_nocode, "nocode");

	alloc_handle(m_drcuml.get(), &m_entry, "entry");
	UML_HANDLE(block, *m_entry);                                                            // handle  entry

	load_fast_iregs(block);                                                                 // <load fastregs>

	/* generate a hash jump via the current mode and PC */
	UML_HASHJMP(block, 0, mem(&m_core->pc), *m_nocode);   // hashjmp <mode>,<pc>,nocode

	block->end();
}


void mb86235_device::static_generate_nocode_handler()
{
	drcuml_block *block;

	/* begin generating */
	block = m_drcuml->begin_block(10);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(m_drcuml.get(), &m_nocode, "nocode");
	UML_HANDLE(block, *m_nocode);                                                           // handle  nocode
	UML_GETEXP(block, I0);                                                                  // getexp  i0
	UML_MOV(block, mem(&m_core->pc), I0);                                                   // mov     [pc],i0
	save_fast_iregs(block);                                                                 // <save fastregs>
	UML_EXIT(block, EXECUTE_MISSING_CODE);                                                  // exit    EXECUTE_MISSING_CODE

	block->end();
}

void mb86235_device::static_generate_out_of_cycles()
{
	drcuml_block *block;

	/* begin generating */
	block = m_drcuml->begin_block(10);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(m_drcuml.get(), &m_out_of_cycles, "out_of_cycles");
	UML_HANDLE(block, *m_out_of_cycles);                                                    // handle  out_of_cycles
	UML_GETEXP(block, I0);                                                                  // getexp  i0
	UML_MOV(block, mem(&m_core->pc), I0);                                                   // mov     <pc>,i0
	save_fast_iregs(block);                                                                 // <save fastregs>
	UML_EXIT(block, EXECUTE_OUT_OF_CYCLES);                                                 // exit    EXECUTE_OUT_OF_CYCLES

	block->end();
}

void mb86235_device::static_generate_fifo()
{
	drcuml_block *block;

	// clear fifo in
	block = m_drcuml->begin_block(20);

	alloc_handle(m_drcuml.get(), &m_clear_fifo_in, "clear_fifo_in");
	UML_HANDLE(block, *m_clear_fifo_in);
	// TODO
	UML_RET(block);

	block->end();

	// clear fifo out
	block = m_drcuml->begin_block(20);

	alloc_handle(m_drcuml.get(), &m_clear_fifo_out, "clear_fifo_out");
	UML_HANDLE(block, *m_clear_fifo_out);
	// TODO
	UML_RET(block);

	block->end();
}

void mb86235_device::static_generate_memory_accessors()
{
	drcuml_block *block;
	code_label label = 1;

	// A-Bus read handler
	// I0 = address
	// I1 = return data
	// I2 = trashed
	block = m_drcuml->begin_block(128);

	alloc_handle(m_drcuml.get(), &m_read_abus, "read_abus");
	UML_HANDLE(block, *m_read_abus);
	UML_CMP(block, I0, 0x400);
	UML_JMPc(block, COND_GE, label);
	// internal A-RAM
	UML_SHL(block, I0, I0, 2);
	UML_READ(block, I1, I0, SIZE_DWORD, SPACE_DATA);
	UML_RET(block);
	// external
	UML_LABEL(block, label++);
	UML_AND(block, I0, I0, 0x3fff);
	UML_AND(block, I2, mem(&m_core->eb), ~0x3fff);
	UML_OR(block, I0, I0, I2);
	UML_SHL(block, I0, I0, 2);
	UML_READ(block, I1, I0, SIZE_DWORD, SPACE_DATA);
	UML_RET(block);

	block->end();

	// A-Bus write handler
	// I0 = address
	// I1 = data
	// I2 = trashed
	block = m_drcuml->begin_block(128);

	alloc_handle(m_drcuml.get(), &m_write_abus, "write_abus");
	UML_HANDLE(block, *m_write_abus);
	UML_CMP(block, I0, 0x400);
	UML_JMPc(block, COND_GE, label);
	// internal A-RAM
	UML_SHL(block, I0, I0, 2);
	UML_WRITE(block, I0, I1, SIZE_DWORD, SPACE_DATA);
	UML_RET(block);
	// external
	UML_LABEL(block, label++);
	UML_AND(block, I0, I0, 0x3fff);
	UML_AND(block, I2, mem(&m_core->eb), ~0x3fff);
	UML_OR(block, I0, I0, I2);
	UML_SHL(block, I0, I0, 2);
	UML_WRITE(block, I0, I1, SIZE_DWORD, SPACE_DATA);
	UML_RET(block);

	block->end();
}


void mb86235_device::flush_cache()
{
	/* empty the transient cache contents */
	m_drcuml->reset();

	try
	{
		// generate the entry point and out-of-cycles handlers
		static_generate_entry_point();
		static_generate_nocode_handler();
		static_generate_out_of_cycles();

		// generate utility functions
		static_generate_fifo();

		// generate exception handlers

		// generate memory accessors
		static_generate_memory_accessors();
	}
	catch (drcuml_block::abort_compilation &)
	{
		fatalerror("Error generating MB86235 static handlers\n");
	}
}



void mb86235_device::generate_sequence_instruction(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	/* add an entry for the log */
	//  if (m_drcuml->logging() && !(desc->flags & OPFLAG_VIRTUAL_NOOP))
	//      log_add_disasm_comment(block, desc->pc, desc->opptr.l[0]);

	/* set the PC map variable */
	UML_MAPVAR(block, MAPVAR_PC, desc->pc);                                                 // mapvar  PC,desc->pc

																							/* accumulate total cycles */
	compiler->cycles += desc->cycles;

	/* update the icount map variable */
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);                                     // mapvar  CYCLES,compiler->cycles

																							/* if we are debugging, call the debugger */
	if ((machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		UML_MOV(block, mem(&m_core->pc), desc->pc);                                         // mov     [pc],desc->pc
		save_fast_iregs(block);                                                             // <save fastregs>
		UML_DEBUG(block, desc->pc);                                                         // debug   desc->pc
	}

	/* if we hit an unmapped address, fatal error */
	if (desc->flags & OPFLAG_COMPILER_UNMAPPED)
	{
		UML_MOV(block, mem(&m_core->pc), desc->pc);                                         // mov     [pc],desc->pc
		save_fast_iregs(block);                                                             // <save fastregs>
		UML_EXIT(block, EXECUTE_UNMAPPED_CODE);                                             // exit    EXECUTE_UNMAPPED_CODE
	}

	/* if this is an invalid opcode, generate the exception now */
	//  if (desc->flags & OPFLAG_INVALID_OPCODE)
	//      UML_EXH(block, *m_exception[EXCEPTION_PROGRAM], 0x80000);                           // exh    exception_program,0x80000

	/* unless this is a virtual no-op, it's a regular instruction */
	if (!(desc->flags & OPFLAG_VIRTUAL_NOOP))
	{
		/* compile the instruction */
		if (!generate_opcode(block, compiler, desc))
		{
			UML_MOV(block, mem(&m_core->pc), desc->pc);                                     // mov     [pc],desc->pc
			UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);                         // dmov    [arg64],*desc->opptr.q
			UML_CALLC(block, cfunc_unimplemented, this);                                    // callc   cfunc_unimplemented,ppc
		}
	}
}

void mb86235_device::generate_update_cycles(drcuml_block *block, compiler_state *compiler, uml::parameter param, int allow_exception)
{
	/* account for cycles */
	if (compiler->cycles > 0)
	{
		UML_SUB(block, mem(&m_core->icount), mem(&m_core->icount), MAPVAR_CYCLES);          // sub     icount,icount,cycles
		UML_MAPVAR(block, MAPVAR_CYCLES, 0);                                                // mapvar  cycles,0
		if (allow_exception)
			UML_EXHc(block, COND_S, *m_out_of_cycles, param);                               // exh     out_of_cycles,nextpc
	}
	compiler->cycles = 0;
}


void mb86235_device::generate_ea(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int md, int arx, int ary, int disp)
{
	// Calculates EA into register I0

	switch (md)
	{
		case 0x1:	// @ARx++
			UML_MOV(block, I0, AR(arx));
			UML_ADD(block, AR(arx), AR(arx), 1);
			break;
		case 0xa:	// @ARx+disp12
			UML_ADD(block, I0, AR(arx), disp);
			break;

		default:
			fatalerror("generate_ea: md = %02X, PC = %08X", md, desc->pc);
			break;
	}
}



void mb86235_device::generate_reg_read(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter dst)
{
	switch (reg)
	{
		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			// AA0-7
			UML_MOV(block, dst, AA(reg & 7));
			break;

		default:
			fatalerror("generate_reg_read: unimplemented register %02X at %08X", reg, desc->pc);
			break;
	}
}


void mb86235_device::generate_reg_write(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter src)
{
	switch (reg)
	{
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			// MA0-7
			UML_MOV(block, MA(reg & 7), src);
			break;

		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			// AA0-7
			UML_MOV(block, AA(reg & 7), src);
			break;

		case 0x10:		// EB
			UML_MOV(block, mem(&m_core->eb), src);
			break;

		case 0x13:		// EO
			UML_MOV(block, mem(&m_core->eo), src);
			break;

		case 0x14:		// SP
			UML_MOV(block, mem(&m_core->sp), src);
			break;

		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			// AR0-7
			UML_MOV(block, AR(reg & 7), src);
			break;

		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			// MB0-7
			UML_MOV(block, MB(reg & 7), src);
			break;

		case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			// AB0-7
			UML_MOV(block, AB(reg & 7), src);
			break;

		case 0x34:		// PDR
			UML_MOV(block, mem(&m_core->pdr), src);
			break;

		case 0x35:		// DDR
			UML_MOV(block, mem(&m_core->ddr), src);
			break;

		default:
			fatalerror("generate_reg_write: unimplemented register %02X at %08X", reg, desc->pc);
			break;
	}
}



int mb86235_device::generate_opcode(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT64 opcode = desc->opptr.q[0];

	switch ((opcode >> 61) & 7)
	{
		case 0:     // ALU / MUL / double transfer (type 1)
			generate_double_xfer1(block, compiler, desc);
			generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			generate_mul(block, compiler, desc, (opcode >> 27) & 0x7fff);			
			break;
		case 1:     // ALU / MUL / transfer (type 1)
			generate_xfer1(block, compiler, desc);
			generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			generate_mul(block, compiler, desc, (opcode >> 27) & 0x7fff);			
			break;
		case 2:     // ALU / MUL / control			
			generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			generate_mul(block, compiler, desc, (opcode >> 27) & 0x7fff);			
			generate_control(block, compiler, desc);
			break;
		case 4:     // ALU or MUL / double transfer (type 2)
			generate_double_xfer2(block, compiler, desc);
			if (opcode & ((UINT64)(1) << 41))
				generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			else
				generate_mul(block, compiler, desc, (opcode >> 42) & 0x7fff);						
			break;
		case 5:     // ALU or MUL / transfer (type 2)
			generate_xfer2(block, compiler, desc);
			if (opcode & ((UINT64)(1) << 41))
				generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			else
				generate_mul(block, compiler, desc, (opcode >> 42) & 0x7fff);			
			break;
		case 6:     // ALU or MUL / control			
			if (opcode & ((UINT64)(1) << 41))
				generate_alu(block, compiler, desc, (opcode >> 42) & 0x7ffff);
			else
				generate_mul(block, compiler, desc, (opcode >> 42) & 0x7fff);			
			generate_control(block, compiler, desc);
			break;
		case 7:     // transfer (type 3)
			generate_xfer3(block, compiler, desc);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}


void mb86235_device::generate_alu(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int aluop)
{
	int i1 = (aluop >> 10) & 0xf;
	int i2 = (aluop >> 5) & 0x1f;
	int io = aluop & 0x1f;
	int op = (aluop >> 14) & 0x1f;

	switch (op)	
	{
		case 0x07:		// NOP
			break;

		default:
			UML_MOV(block, mem(&m_core->pc), desc->pc);
			UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);
			UML_CALLC(block, cfunc_unimplemented_alu, this);
			break;
	}
}

void mb86235_device::generate_mul(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int mulop)
{
	UML_MOV(block, mem(&m_core->pc), desc->pc);
	UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);
	UML_CALLC(block, cfunc_unimplemented_mul, this);
}


void mb86235_device::generate_branch(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	// I0 = target pc for dynamic branches

	compiler_state compiler_temp = *compiler;

	// save branch target
	if (desc->targetpc == BRANCH_TARGET_DYNAMIC)
	{
		UML_MOV(block, mem(&m_core->jmpdest), I0);                                     // mov     [jmpdest],i0
	}

	// compile delay slots
	generate_sequence_instruction(block, &compiler_temp, desc->delay.first());

	// update cycles and hash jump
	if (desc->targetpc != BRANCH_TARGET_DYNAMIC)
	{
		generate_update_cycles(block, &compiler_temp, desc->targetpc, TRUE);
		if (desc->flags & OPFLAG_INTRABLOCK_BRANCH)
			UML_JMP(block, desc->targetpc | 0x80000000);                                // jmp      targetpc | 0x80000000
		else
			UML_HASHJMP(block, 0, desc->targetpc, *m_nocode);                           // hashjmp  0,targetpc,nocode
	}
	else
	{
		generate_update_cycles(block, &compiler_temp, mem(&m_core->jmpdest), TRUE);
		UML_HASHJMP(block, 0, mem(&m_core->jmpdest), *m_nocode);                        // hashjmp  0,jmpdest,nocode
	}

	// update compiler label
	compiler->labelnum = compiler_temp.labelnum;

	/* reset the mapvar to the current cycles and account for skipped slots */
	compiler->cycles += desc->skipslots;
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);                                 // mapvar  CYCLES,compiler->cycles
}


void mb86235_device::generate_control(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT64 op = desc->opptr.q[0];
	int ef1 = (op >> 16) & 0x3f;
	int ef2 = op & 0xffff;
	int cop = (op >> 22) & 0x1f;
	int rel12 = (op & 0x800) ? (0xfffff000 | (op & 0xfff)) : (op & 0xfff);

	switch (cop)
	{
		case 0x00:		// NOP
			break;

		case 0x03:		// 
			if (ef1 == 1)	// CLRFI
				UML_CALLH(block, *m_clear_fifo_in);
			else if (ef1 == 2)	// CLRFO
				UML_CALLH(block, *m_clear_fifo_out);
			else if (ef1 == 3)	// CLRF
			{
				UML_CALLH(block, *m_clear_fifo_in);
				UML_CALLH(block, *m_clear_fifo_out);
			}
			break;

		case 0x08:		// SETM #imm16
			UML_MOV(block, mem(&m_core->mod), ef2);
			break;

		case 0x1a:		// DCALL
		{
			// push PC
			code_label no_overflow = compiler->labelnum++;
			UML_CMP(block, mem(&m_core->pcs_ptr), 4);
			UML_JMPc(block, COND_L, no_overflow);
			UML_MOV(block, mem(&m_core->pc), desc->pc);
			UML_CALLC(block, cfunc_pcs_overflow, this);

			UML_LABEL(block, no_overflow);
			UML_MOV(block, I0, desc->pc + 2);
			UML_STORE(block, m_core->pcs, mem(&m_core->pcs_ptr), I0, SIZE_DWORD, SCALE_x4);
			UML_ADD(block, mem(&m_core->pcs_ptr), mem(&m_core->pcs_ptr), 1);
			
			generate_branch(block, compiler, desc);
			break;
		}

		case 0x1b:		// DRET
		{
			// pop PC
			code_label no_underflow = compiler->labelnum++;
			UML_CMP(block, mem(&m_core->pcs_ptr), 0);
			UML_JMPc(block, COND_G, no_underflow);
			UML_MOV(block, mem(&m_core->pc), desc->pc);
			UML_CALLC(block, cfunc_pcs_underflow, this);

			UML_LABEL(block, no_underflow);
			UML_SUB(block, mem(&m_core->pcs_ptr), mem(&m_core->pcs_ptr), 1);
			UML_LOAD(block, I0, m_core->pcs, mem(&m_core->pcs_ptr), SIZE_DWORD, SCALE_x4);
			
			generate_branch(block, compiler, desc);
			break;
		}

		default:
			UML_MOV(block, mem(&m_core->pc), desc->pc);
			UML_MOV(block, mem(&m_core->arg0), cop);
			UML_CALLC(block, cfunc_unimplemented_control, this);
			break;
	}
}

void mb86235_device::generate_xfer1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UML_MOV(block, mem(&m_core->pc), desc->pc);
	UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);
	UML_CALLC(block, cfunc_unimplemented_xfer1, this);
}

void mb86235_device::generate_double_xfer1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UML_MOV(block, mem(&m_core->pc), desc->pc);
	UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);
	UML_CALLC(block, cfunc_unimplemented_double_xfer1, this);
}

void mb86235_device::generate_xfer2(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT64 opcode = desc->opptr.q[0];

	int op = (opcode >> 39) & 3;
	int trm = (opcode >> 38) & 1;
	int dir = (opcode >> 37) & 1;
	int sr = (opcode >> 31) & 0x7f;
	int dr = (opcode >> 24) & 0x7f;
	int ary = (opcode >> 4) & 7;
	int md = opcode & 0xf;

	int disp14 = (opcode >> 7) & 0x3fff;
	if (disp14 & 0x2000) disp14 |= 0xffffc000;

	if (op == 0)	// MOV2
	{
		if (trm == 0)
		{
			if (sr == 0x58)
			{
				// MOV2 #imm24, DR
				generate_reg_write(block, compiler, desc, dr & 0x3f, uml::parameter(opcode & 0xffffff));
			}
			else
			{
				if ((sr & 0x40) == 0)
				{
					generate_reg_read(block, compiler, desc, sr & 0x3f, I1);
				}
				else
				{
					generate_ea(block, compiler, desc, md, sr & 7, ary, disp14);
					if (sr & 0x20)	// RAM-B
					{
						UML_SHL(block, I0, I0, 2);
						UML_READ(block, I1, I0, SIZE_DWORD, SPACE_IO);
					}
					else // RAM-A
					{
						UML_CALLH(block, *m_read_abus);
					}					
				}

				if ((dr & 0x40) == 0)
				{
					generate_reg_write(block, compiler, desc, dr & 0x3f, I1);
				}
				else
				{
					generate_ea(block, compiler, desc, md, dr & 7, ary, disp14);
					if (sr & 0x20)	// RAM-B
					{
						UML_SHL(block, I0, I0, 2);
						UML_WRITE(block, I0, I1, SIZE_DWORD, SPACE_IO);
					}
					else // RAM-A
					{
						UML_CALLH(block, *m_write_abus);
					}
				}
			}
		}
		else
		{
			// external transfer
			if (dir == 0)
			{
				generate_reg_read(block, compiler, desc, dr & 0x3f, I0);
				UML_ADD(block, I1, mem(&m_core->eb), mem(&m_core->eo));
				UML_ADD(block, I1, I1, disp14);
				UML_SHL(block, I1, I1, 2);
				UML_WRITE(block, I1, I0, SIZE_DWORD, SPACE_DATA);
			}
			else
			{
				UML_ADD(block, I1, mem(&m_core->eb), mem(&m_core->eo));
				UML_ADD(block, I1, I1, disp14);
				UML_SHL(block, I1, I1, 2);
				UML_READ(block, I0, I1, SIZE_DWORD, SPACE_DATA);
				generate_reg_write(block, compiler, desc, dr & 0x3f, I0);
			}

			// update EO
			UML_ADD(block, mem(&m_core->eo), mem(&m_core->eo), disp14);
		}
	}
	else if (op == 2)	// MOV4
	{
		fatalerror("generate_xfer2 MOV4 at %08X (%08X%08X)", desc->pc, (UINT32)(opcode >> 32), (UINT32)(opcode));
	}
}

void mb86235_device::generate_double_xfer2(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UML_MOV(block, mem(&m_core->pc), desc->pc);
	UML_DMOV(block, mem(&m_core->arg64), desc->opptr.q[0]);
	UML_CALLC(block, cfunc_unimplemented_double_xfer2, this);
}

void mb86235_device::generate_xfer3(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT64 opcode = desc->opptr.q[0];

	UINT32 imm = (UINT32)(opcode >> 27);
	int dr = (opcode >> 19) & 0x7f;	
	int ary = (opcode >> 4) & 7;
	int md = opcode & 0xf;

	int disp = (opcode >> 7) & 0xfff;
	if (disp & 0x800) disp |= 0xfffff800;

	switch (dr >> 5)
	{
		case 0:
		case 1:		// reg
			generate_reg_write(block, compiler, desc, dr & 0x3f, uml::parameter(imm));
			break;

		case 2:		// RAM-A
			generate_ea(block, compiler, desc, md, dr & 7, ary, disp);
			UML_MOV(block, I1, imm);
			UML_CALLH(block, *m_write_abus);
			break;

		case 3:		// RAM-B
			generate_ea(block, compiler, desc, md, dr & 7, ary, disp);
			UML_SHL(block, I0, I0, 2);
			UML_WRITE(block, I0, imm, SIZE_DWORD, SPACE_IO);
			break;
	}
}