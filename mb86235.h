// license:BSD-3-Clause
// copyright-holders:Angelo Salese, ElSemi
/*****************************************************************************
 *
 * template for CPU cores
 *
 *****************************************************************************/

#pragma once

#ifndef __MB86235_H__
#define __MB86235_H__

#include "cpu/drcfe.h"
#include "cpu/drcuml.h"

class mb86235_frontend;




#define OP_USERFLAG_FIFOIN			0x1
#define OP_USERFLAG_FIFOOUT0		0x2
#define OP_USERFLAG_FIFOOUT1		0x4


class mb86235_device :  public cpu_device
{
	friend class mb86235_frontend;

public:
	// construction/destruction
	mb86235_device(const machine_config &mconfig, const char *_tag, device_t *_owner, UINT32 _clock);

	void unimplemented_op();
	void unimplemented_alu();
	void unimplemented_control();
	void unimplemented_xfer1();
	void unimplemented_double_xfer1();
	void unimplemented_double_xfer2();
	void pcs_overflow();
	void pcs_underflow();

	void fifoin_w(UINT64 data);

	enum
	{
		MB86235_PC = 1,
		MB86235_AA0, MB86235_AA1, MB86235_AA2, MB86235_AA3, MB86235_AA4, MB86235_AA5, MB86235_AA6, MB86235_AA7,
		MB86235_AB0, MB86235_AB1, MB86235_AB2, MB86235_AB3, MB86235_AB4, MB86235_AB5, MB86235_AB6, MB86235_AB7,
		MB86235_MA0, MB86235_MA1, MB86235_MA2, MB86235_MA3, MB86235_MA4, MB86235_MA5, MB86235_MA6, MB86235_MA7,
		MB86235_MB0, MB86235_MB1, MB86235_MB2, MB86235_MB3, MB86235_MB4, MB86235_MB5, MB86235_MB6, MB86235_MB7,
		MB86235_AR0, MB86235_AR1, MB86235_AR2, MB86235_AR3, MB86235_AR4, MB86235_AR5, MB86235_AR6, MB86235_AR7,
	};

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_execute_interface overrides
	virtual UINT32 execute_min_cycles() const override { return 1; }
	virtual UINT32 execute_max_cycles() const override { return 7; }
	virtual UINT32 execute_input_lines() const override { return 0; }
	virtual void execute_run() override;
	//virtual void execute_set_input(int inputnum, int state);

	// device_memory_interface overrides
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const override { return (spacenum == AS_PROGRAM) ? &m_program_config : ((spacenum == AS_DATA) ? &m_dataa_config : (spacenum == AS_IO) ? &m_datab_config : nullptr); }

	// device_state_interface overrides
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const override;

	// device_disasm_interface overrides
	virtual UINT32 disasm_min_opcode_bytes() const override { return 8; }
	virtual UINT32 disasm_max_opcode_bytes() const override { return 8; }
	virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options) override;

	direct_read_data *m_direct;

private:

	struct mb86235_flags
	{
		UINT32 az;
		UINT32 an;
		UINT32 av;
		UINT32 au;
		UINT32 ad;
		UINT32 zc;
		UINT32 il;
		UINT32 nr;
		UINT32 zd;
		UINT32 mn;
		UINT32 mz;
		UINT32 mv;
		UINT32 mu;
		UINT32 md;
	};

	struct fifo
	{
		int rpos;
		int wpos;
		int num;
		UINT64 data[8];
	};

	struct mb86235_internal_state
	{
		UINT32 pc;
		UINT32 aa[8];
		UINT32 ab[8];
		UINT32 ma[8];
		UINT32 mb[8];
		UINT32 ar[8];

		UINT32 sp;
		UINT32 eb;
		UINT32 eo;
		UINT32 rpc;
		UINT32 lpc;

		UINT32 prp;
		UINT32 pwp;
		UINT32 pr[24];

		UINT32 mod;
		mb86235_flags flags;

		int icount;

		UINT32 arg0;
		UINT32 arg1;
		UINT32 arg2;
		UINT32 arg3;
		UINT64 arg64;

		UINT32 pcs[4];
		int pcs_ptr;

		UINT32 jmpdest;
		UINT32 alutemp;
		UINT32 multemp;

		UINT32 pdr;
		UINT32 ddr;

		float fp0;

		fifo fifoin;
		fifo fifoout0;
		fifo fifoout1;
	};

	mb86235_internal_state  *m_core;

	drc_cache m_cache;
	std::unique_ptr<drcuml_state> m_drcuml;
	std::unique_ptr<mb86235_frontend> m_drcfe;
	uml::parameter   m_regmap[32];

	uml::code_handle *m_entry;                      /* entry point */
	uml::code_handle *m_nocode;                     /* nocode exception handler */
	uml::code_handle *m_out_of_cycles;              /* out of cycles exception handler */
	uml::code_handle *m_clear_fifo_in;
	uml::code_handle *m_clear_fifo_out0;
	uml::code_handle *m_clear_fifo_out1;
	uml::code_handle *m_read_fifo_in;
	uml::code_handle *m_write_fifo_out0;
	uml::code_handle *m_write_fifo_out1;
	uml::code_handle *m_read_abus;
	uml::code_handle *m_write_abus;

	address_space_config m_program_config;
	address_space_config m_dataa_config;
	address_space_config m_datab_config;

	address_space *m_program;
	address_space *m_dataa;
	address_space *m_datab;

	/* internal compiler state */
	struct compiler_state
	{
		UINT32 cycles;                             /* accumulated cycles */
		UINT8  checkints;                          /* need to check interrupts before next instruction */
		uml::code_label  labelnum;                 /* index for local labels */
	};

	void run_drc();
	void flush_cache();
	void alloc_handle(drcuml_state *drcuml, uml::code_handle **handleptr, const char *name);
	void compile_block(offs_t pc);
	void load_fast_iregs(drcuml_block *block);
	void save_fast_iregs(drcuml_block *block);
	void static_generate_entry_point();
	void static_generate_nocode_handler();
	void static_generate_out_of_cycles();
	void static_generate_fifo();
	void static_generate_memory_accessors();
	void generate_sequence_instruction(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_update_cycles(drcuml_block *block, compiler_state *compiler, uml::parameter param, int allow_exception);
	int generate_opcode(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_alu(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int aluop, bool alu_temp);
	void generate_mul(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int mulop, bool mul_temp);
	void generate_control(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_xfer1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_double_xfer1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_xfer2(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_double_xfer2(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_xfer3(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_branch(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
	void generate_ea(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int md, int arx, int ary, int disp);
	void generate_reg_read(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter dst);
	void generate_reg_write(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter src);
	void generate_alu_input(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter dst, bool fp);
	uml::parameter get_alu1_input(int reg);
	uml::parameter get_alu_output(int reg);
	void generate_mul_input(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int reg, uml::parameter dst, bool fp);
	uml::parameter get_mul1_input(int reg);
	void generate_condition(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int cc, bool not, uml::code_label skip_label);
	void generate_branch_target(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, int type, int ef2);
	bool has_register_clash(const opcode_desc *desc, int outreg);
	bool aluop_has_result(int aluop);
};


extern const device_type MB86235;


CPU_DISASSEMBLE( mb86235 );

#endif /* __MB86235_H__ */
