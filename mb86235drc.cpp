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



void mb86235_device::unimplemented_op()
{
	UINT64 op = m_core->arg64;
	fatalerror("MB86235: PC=%08X: Unimplemented op %04X%08X\n", m_core->pc, (UINT32)(op >> 32), (UINT32)(op));
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

		// generate exception handlers

		// generate memory accessors
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

int mb86235_device::generate_opcode(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT64 opcode = desc->opptr.q[0];

	return FALSE;
}