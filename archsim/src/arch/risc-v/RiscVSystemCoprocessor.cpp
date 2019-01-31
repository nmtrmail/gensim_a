/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"

DeclareLogContext(LogRiscVSystem, "RV-System");
using namespace archsim::arch::riscv;

RiscVSystemCoprocessor::RiscVSystemCoprocessor(archsim::core::thread::ThreadInstance* hart, RiscVMMU* mmu) : hart_(hart), mmu_(mmu)
{

}

bool RiscVSystemCoprocessor::Initialise()
{
	return true;
}

bool RiscVSystemCoprocessor::Read64(uint32_t address, uint64_t& data)
{
	LC_DEBUG1(LogRiscVSystem) << "CSR Read 0x" << std::hex << address << "...";
	switch(address) {
		case 0x106:
			data = SCOUNTEREN;
			break;

		case 0x180: //SATP
			data = mmu_->GetSATP();
			break;

		case 0x300:
			data = ReadMSTATUS();
			break;

		case 0x301: { // MISA
			data = 0;
			// RV64
			data |= 2ULL << 62;

			// A
			data |= 1;

			// D
			data |= 1 << 3;

			// F
			data |= 1 << 5;

			// I
			data |= 1 << 8;

			// M
			data |= 1 << 12;

			// S
			data |= 1 << 18;

			// U
			data |= 1 << 20;

			break;
		}

		case 0x302: // MEDELEG
			UNEXPECTED; // handled in model

		case 0x303: // MIDELEG
			UNEXPECTED; // handled in model

		case 0x304:
			data = MIE;
			break;

		case 0x305:
			data = MTVEC;
			break;

		case 0x306:
			data = MCOUNTEREN;
			return true;

		case 0x340:
			data = MSCRATCH;
			break;

		case 0x3a0: // PMPCFG
		case 0x3a1:
		case 0x3a2:
		case 0x3a3:
			data = 0; // RAZ (...?)
			return true;

		case 0x3b0: // PMPADDR
		case 0x3b1:
		case 0x3b2:
		case 0x3b3:
		case 0x3b4:
		case 0x3b5:
		case 0x3b6:
		case 0x3b7:
		case 0x3b8:
		case 0x3b9:
		case 0x3ba:
		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
			data = 0; // RAZ (...?)
			return true;

		case 0xf14: // Hardware Thread ID
			data = 0;
			break;

		default:
			UNIMPLEMENTED;
	}
	LC_DEBUG1(LogRiscVSystem) << "CSR Read 0x" << std::hex << address << " -> 0x" << std::hex << data;
	return true;
}

bool RiscVSystemCoprocessor::Write64(uint32_t address, uint64_t data)
{
	LC_DEBUG1(LogRiscVSystem) << "CSR Write 0x" << std::hex << address << " <- 0x" << std::hex << data;
	switch(address) {
		case 0x106:
			SCOUNTEREN = data;
			return true;

		case 0x180: //SATP
			mmu_->SetSATP(data);
			return true;

		case 0x300: // MSTATUS
			WriteMSTATUS(data);
			return true;

		case 0x301: // MISA
			// ignore writes
			return true;

		case 0x302: // MEDELEG
			UNEXPECTED; // handled in model

		case 0x303: // MIDELEG
			UNEXPECTED; // handled in model

		case 0x304:
			MIE = data;
			return true;

		case 0x305:
			MTVEC = data;
			return true;

		case 0x306:
			MCOUNTEREN = data;
			return true;

		case 0x340:
			MSCRATCH = data;
			return true;

		case 0x3a0: // PMPCFG
		case 0x3a1:
		case 0x3a2:
		case 0x3a3:
			// ignore (...?)
			return true;

		case 0x3b0: // PMPADDR
		case 0x3b1:
		case 0x3b2:
		case 0x3b3:
		case 0x3b4:
		case 0x3b5:
		case 0x3b6:
		case 0x3b7:
		case 0x3b8:
		case 0x3b9:
		case 0x3ba:
		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
			// ignore (...?)
			return true;

		case 0xf14: // Hardware Thread ID
			// Writes ignored
			return true;

		default:
			UNIMPLEMENTED;
	}
}

void RiscVSystemCoprocessor::MachinePendInterrupt(uint64_t mask)
{
	MIP |= mask;

	CheckForInterrupts();
}

void RiscVSystemCoprocessor::CheckForInterrupts()
{
	uint8_t priv_mode = hart_->GetExecutionRing();
	// check for M mode interrupts
	// M mode interrupts enabled if privilege < M, or if priv == M and MIE
	if(priv_mode < 3 || (priv_mode == 3 && STATUS.MIE)) {
		// M mode interrupts are enabled, check for a machine mode interrupt

		// a machine mode interrupt is pending if MIE & MIP & ~MIDELEG
		// i.e., an interrupt is enabled, and pending, and the interrupt is not delegated
		if(MIE & MIP & ~MIDELEG) {
			// figure out which interrupt should be taken and service it in M mode
			UNIMPLEMENTED;

			// done.
			return;
		}
	}

	// check for S mode interrupts
	// S mode interrupts enabled if privilege < S, or if priv == S and SIE
	if(priv_mode < 2 || (priv_mode == 2 && STATUS.SIE)) {
		// a supervisor mode interrupt is pending if MIE & MIP & MIDELEG & ~SIDELEG
		// i.e., if an interrupt is enabled, and pending, and delegated to S mode, and not delegated to U mode
		if(MIE & MIP & MIDELEG & ~SIDELEG) {
			// figure out which interrupt should be taken and service it in S mode
			UNIMPLEMENTED;

			// done
			return;
		}
	}

	// check for U mode interrupts
	// U mode interrupts enabled if priv == S and SIE
	if(priv_mode == 0 && STATUS.UIE) {
		// a supervisor mode interrupt is pending if MIE & MIP & MIDELEG & ~SIDELEG
		// i.e., if an interrupt is enabled, and pending, and delegated to S mode, and not delegated to U mode
		if(MIE & MIP & MIDELEG & SIDELEG) {
			// figure out which interrupt should be taken and service it in S mode
			UNIMPLEMENTED;

			// done
			return;
		}
	}

	// no interrupts are pending
}

uint64_t RiscVSystemCoprocessor::STATUS_t::ReadMSTATUS()
{
	uint64_t data = 0;
	data |= (uint64_t)SD << 63;
	data |= (uint64_t)SXL << 34;
	data |= (uint64_t)UXL << 32;
	data |= (uint64_t)TSR << 22;
	data |= (uint64_t)TW << 21;
	data |= (uint64_t)TVM << 20;
	data |= (uint64_t)MXR << 19;
	data |= (uint64_t)SUM << 18;
	data |= (uint64_t)MPRV << 17;
	data |= (uint64_t)XS << 15;
	data |= (uint64_t)FS << 13;
	data |= (uint64_t)MPP << 11;
	data |= (uint64_t)SPP << 8;
	data |= (uint64_t)MPIE << 7;
	data |= (uint64_t)SPIE << 5;
	data |= (uint64_t)UPIE << 4;
	data |= (uint64_t)MIE << 3;
	data |= (uint64_t)SIE << 1;
	data |= (uint64_t)UIE << 0;

	return data;
}

void RiscVSystemCoprocessor::STATUS_t::WriteMSTATUS(uint64_t data)
{
	SD = UNSIGNED_BITS_64(data, 63,62);
	SXL = UNSIGNED_BITS_64(data, 34, 33);
	UXL = UNSIGNED_BITS_64(data, 32, 31);
	TSR = BITSEL(data, 22);
	TW = BITSEL(data, 21);
	TVM = BITSEL(data, 20);
	MXR = BITSEL(data, 19);
	SUM = BITSEL(data, 18);
	MPRV = BITSEL(data, 17);
	XS = UNSIGNED_BITS_64(data, 16,15);
	FS = UNSIGNED_BITS_64(data, 14,13);
	MPP = UNSIGNED_BITS_64(data, 12,11);
	SPP = BITSEL(data, 8);
	MPIE = BITSEL(data, 7);
	SPIE = BITSEL(data, 5);
	UPIE = BITSEL(data, 4);
	MIE = BITSEL(data, 3);
	SIE = BITSEL(data, 1);
	UIE = BITSEL(data, 0);
}

uint64_t RiscVSystemCoprocessor::ReadMSTATUS()
{
	return STATUS.ReadMSTATUS();
}

void RiscVSystemCoprocessor::WriteMSTATUS(uint64_t data)
{
	STATUS.WriteMSTATUS(data);
}


// Don't support any accesses other than 64 bit
bool RiscVSystemCoprocessor::Read16(uint32_t address, uint16_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Read32(uint32_t address, uint32_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Read8(uint32_t address, uint8_t& data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write16(uint32_t address, uint16_t data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write32(uint32_t address, uint32_t data)
{
	UNEXPECTED;
}

bool RiscVSystemCoprocessor::Write8(uint32_t address, uint8_t data)
{
	UNEXPECTED;
}
