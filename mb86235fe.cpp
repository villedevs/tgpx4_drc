// license:BSD-3-Clause
// copyright-holders:Ville Linde

/******************************************************************************

    Front-end for MB86235 recompiler

******************************************************************************/

#include "emu.h"
#include "mb86235fe.h"


mb86235_frontend::mb86235_frontend(mb86235_device *core, UINT32 window_start, UINT32 window_end, UINT32 max_sequence)
	: drc_frontend(*core, window_start, window_end, max_sequence),
	m_core(core)
{
}


bool mb86235_frontend::describe(opcode_desc &desc, const opcode_desc *prev)
{
	UINT64 opcode = desc.opptr.q[0] = m_core->m_direct->read_qword(desc.pc, 0);

	return false;
}