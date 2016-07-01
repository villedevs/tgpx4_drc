// license:BSD-3-Clause
// copyright-holders:Angelo Salese, ElSemi
/*****************************************************************************
 *
 * MB86235 "TGPx4" (c) Fujitsu
 *
 * Written by Angelo Salese & ElSemi
 *
 * TODO:
 * - Everything!
 *
 *****************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "mb86235.h"
#include "mb86235fe.h"



#define CACHE_SIZE                      (1 * 1024 * 1024)
#define COMPILE_BACKWARDS_BYTES         128
#define COMPILE_FORWARDS_BYTES          512
#define COMPILE_MAX_INSTRUCTIONS        ((COMPILE_BACKWARDS_BYTES/4) + (COMPILE_FORWARDS_BYTES/4))
#define COMPILE_MAX_SEQUENCE            64



const device_type MB86235 = &device_creator<mb86235_device>;



/* Execute cycles */
void mb86235_device::execute_run()
{
	/*
	UINT32 opcode;

	do
	{
		debugger_instruction_hook(this, m_pc);

		opcode = mb86235_readop(m_pc);
		//m_pc++;

		switch( opcode )
		{
			default:
				mb86235_illegal();
				break;
		}

	} while( m_icount > 0 );
	*/
	run_drc();
}


void mb86235_device::device_start()
{
	m_program = &space(AS_PROGRAM);
	m_direct = &m_program->direct();
	m_data = &space(AS_DATA);

	m_core = (mb86235_internal_state *)m_cache.alloc_near(sizeof(mb86235_internal_state));
	memset(m_core, 0, sizeof(mb86235_internal_state));


	// init UML generator
	UINT32 umlflags = 0;
	m_drcuml = std::make_unique<drcuml_state>(*this, m_cache, umlflags, 1, 24, 0);

	// add UML symbols
	m_drcuml->symbol_add(&m_core->pc, sizeof(m_core->pc), "pc");
	m_drcuml->symbol_add(&m_core->icount, sizeof(m_core->icount), "icount");

	for (int i = 0; i < 8; i++)
	{
		char buf[10];
		sprintf(buf, "aa%d", i);
		m_drcuml->symbol_add(&m_core->aa[i], sizeof(m_core->aa[i]), buf);
		sprintf(buf, "ab%d", i);
		m_drcuml->symbol_add(&m_core->ab[i], sizeof(m_core->ab[i]), buf);
		sprintf(buf, "ma%d", i);
		m_drcuml->symbol_add(&m_core->ma[i], sizeof(m_core->ma[i]), buf);
		sprintf(buf, "mb%d", i);
		m_drcuml->symbol_add(&m_core->mb[i], sizeof(m_core->mb[i]), buf);
		sprintf(buf, "ar%d", i);
		m_drcuml->symbol_add(&m_core->ar[i], sizeof(m_core->ar[i]), buf);
	}

	m_drcuml->symbol_add(&m_core->flags.az, sizeof(m_core->flags.az), "flags_az");
	m_drcuml->symbol_add(&m_core->flags.an, sizeof(m_core->flags.an), "flags_an");
	m_drcuml->symbol_add(&m_core->flags.av, sizeof(m_core->flags.av), "flags_av");
	m_drcuml->symbol_add(&m_core->flags.au, sizeof(m_core->flags.au), "flags_au");
	m_drcuml->symbol_add(&m_core->flags.ad, sizeof(m_core->flags.ad), "flags_ad");
	m_drcuml->symbol_add(&m_core->flags.zc, sizeof(m_core->flags.zc), "flags_zc");
	m_drcuml->symbol_add(&m_core->flags.il, sizeof(m_core->flags.il), "flags_il");
	m_drcuml->symbol_add(&m_core->flags.nr, sizeof(m_core->flags.nr), "flags_nr");
	m_drcuml->symbol_add(&m_core->flags.zd, sizeof(m_core->flags.zd), "flags_zd");
	m_drcuml->symbol_add(&m_core->flags.mn, sizeof(m_core->flags.mn), "flags_mn");
	m_drcuml->symbol_add(&m_core->flags.mz, sizeof(m_core->flags.mz), "flags_mz");
	m_drcuml->symbol_add(&m_core->flags.mv, sizeof(m_core->flags.mv), "flags_mv");
	m_drcuml->symbol_add(&m_core->flags.mu, sizeof(m_core->flags.mu), "flags_mu");
	m_drcuml->symbol_add(&m_core->flags.md, sizeof(m_core->flags.md), "flags_md");


	m_drcuml->symbol_add(&m_core->arg0, sizeof(m_core->arg0), "arg0");
	m_drcuml->symbol_add(&m_core->arg1, sizeof(m_core->arg1), "arg1");
	m_drcuml->symbol_add(&m_core->arg2, sizeof(m_core->arg2), "arg2");
	m_drcuml->symbol_add(&m_core->arg3, sizeof(m_core->arg3), "arg3");


	m_drcfe = std::make_unique<mb86235_frontend>(this, COMPILE_BACKWARDS_BYTES, COMPILE_FORWARDS_BYTES, COMPILE_MAX_SEQUENCE);

	for (int i = 0; i < 8; i++)
	{
		m_regmap[i] = uml::mem(&m_core->aa[i]);
		m_regmap[i + 8] = uml::mem(&m_core->ab[i]);
		m_regmap[i + 16] = uml::mem(&m_core->ma[i]);
		m_regmap[i + 24] = uml::mem(&m_core->mb[i]);
	}


	// Register state for debugger
	state_add(MB86235_PC, "PC", m_core->pc).formatstr("%08X");
	state_add(MB86235_AR0, "AR0", m_core->ar[0]).formatstr("%08X");
	state_add(MB86235_AR1, "AR1", m_core->ar[1]).formatstr("%08X");
	state_add(MB86235_AR2, "AR2", m_core->ar[2]).formatstr("%08X");
	state_add(MB86235_AR3, "AR3", m_core->ar[3]).formatstr("%08X");
	state_add(MB86235_AR4, "AR4", m_core->ar[4]).formatstr("%08X");
	state_add(MB86235_AR5, "AR5", m_core->ar[5]).formatstr("%08X");
	state_add(MB86235_AR6, "AR6", m_core->ar[6]).formatstr("%08X");
	state_add(MB86235_AR7, "AR7", m_core->ar[7]).formatstr("%08X");
	state_add(MB86235_AA0, "AA0", m_core->aa[0]).formatstr("%08X");
	state_add(MB86235_AA1, "AA1", m_core->aa[1]).formatstr("%08X");
	state_add(MB86235_AA2, "AA2", m_core->aa[2]).formatstr("%08X");
	state_add(MB86235_AA3, "AA3", m_core->aa[3]).formatstr("%08X");
	state_add(MB86235_AA4, "AA4", m_core->aa[4]).formatstr("%08X");
	state_add(MB86235_AA5, "AA5", m_core->aa[5]).formatstr("%08X");
	state_add(MB86235_AA6, "AA6", m_core->aa[6]).formatstr("%08X");
	state_add(MB86235_AA7, "AA7", m_core->aa[7]).formatstr("%08X");
	state_add(MB86235_AB0, "AB0", m_core->ab[0]).formatstr("%08X");
	state_add(MB86235_AB1, "AB1", m_core->ab[1]).formatstr("%08X");
	state_add(MB86235_AB2, "AB2", m_core->ab[2]).formatstr("%08X");
	state_add(MB86235_AB3, "AB3", m_core->ab[3]).formatstr("%08X");
	state_add(MB86235_AB4, "AB4", m_core->ab[4]).formatstr("%08X");
	state_add(MB86235_AB5, "AB5", m_core->ab[5]).formatstr("%08X");
	state_add(MB86235_AB6, "AB6", m_core->ab[6]).formatstr("%08X");
	state_add(MB86235_AB7, "AB7", m_core->ab[7]).formatstr("%08X");
	state_add(MB86235_MA0, "MA0", m_core->ma[0]).formatstr("%08X");
	state_add(MB86235_MA1, "MA1", m_core->ma[1]).formatstr("%08X");
	state_add(MB86235_MA2, "MA2", m_core->ma[2]).formatstr("%08X");
	state_add(MB86235_MA3, "MA3", m_core->ma[3]).formatstr("%08X");
	state_add(MB86235_MA4, "MA4", m_core->ma[4]).formatstr("%08X");
	state_add(MB86235_MA5, "MA5", m_core->ma[5]).formatstr("%08X");
	state_add(MB86235_MA6, "MA6", m_core->ma[6]).formatstr("%08X");
	state_add(MB86235_MA7, "MA7", m_core->ma[7]).formatstr("%08X");
	state_add(MB86235_MB0, "MB0", m_core->ma[0]).formatstr("%08X");
	state_add(MB86235_MB1, "MB1", m_core->ma[1]).formatstr("%08X");
	state_add(MB86235_MB2, "MB2", m_core->ma[2]).formatstr("%08X");
	state_add(MB86235_MB3, "MB3", m_core->ma[3]).formatstr("%08X");
	state_add(MB86235_MB4, "MB4", m_core->ma[4]).formatstr("%08X");
	state_add(MB86235_MB5, "MB5", m_core->ma[5]).formatstr("%08X");
	state_add(MB86235_MB6, "MB6", m_core->ma[6]).formatstr("%08X");
	state_add(MB86235_MB7, "MB7", m_core->ma[7]).formatstr("%08X");
	state_add(STATE_GENPC, "GENPC", m_core->pc ).noshow();

	m_icountptr = &m_core->icount;
}

void mb86235_device::device_reset()
{
	flush_cache();

	m_core->pc = 0;
}

#if 0
void mb86235_cpu_device::execute_set_input(int irqline, int state)
{
	switch(irqline)
	{
		case MB86235_INT_INTRM:
			m_intrm_pending = (state == ASSERT_LINE);
			m_intrm_state = state;
			break;
		case MB86235_RESET:
			if (state == ASSERT_LINE)
				m_reset_pending = 1;
			m_reset_state = state;
			break;
		case MB86235_INT_INTR:
			if (state == ASSERT_LINE)
				m_intr_pending = 1;
			m_intr_state = state;
			break;
	}
}
#endif

mb86235_device::mb86235_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: cpu_device(mconfig, MB86235, "MB86235", tag, owner, clock, "mb86235", __FILE__)
	, m_program_config("program", ENDIANNESS_LITTLE, 64, 32, -3)
	, m_data_config("data", ENDIANNESS_LITTLE, 32, 32, -2)
	, m_cache(CACHE_SIZE + sizeof(mb86235_internal_state))
	, m_drcuml(nullptr)
	, m_drcfe(nullptr)
{
}


void mb86235_device::state_string_export(const device_state_entry &entry, std::string &str) const
{
	switch (entry.index())
	{
		case STATE_GENFLAGS:
			str = string_format("?");
			break;
	}
}

offs_t mb86235_device::disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options)
{
	extern CPU_DISASSEMBLE( mb86235 );
	return CPU_DISASSEMBLE_NAME(mb86235)(this, buffer, pc, oprom, opram, options);
}
