// license:BSD-3-Clause
// copyright-holders:Ville Linde

/******************************************************************************

Front-end for MB86235 recompiler

******************************************************************************/

#pragma once

#include "mb86235.h"
#include "cpu/drcfe.h"

#ifndef __MB86235FE_H__
#define __MB86235FE_H__

class mb86235_frontend : public drc_frontend
{
public:
	mb86235_frontend(mb86235_device *core, UINT32 window_start, UINT32 window_end, UINT32 max_sequence);

protected:
	// required overrides
	virtual bool describe(opcode_desc &desc, const opcode_desc *prev) override;

private:

	mb86235_device *m_core;
};

#endif /* __SHARCFE_H__ */
