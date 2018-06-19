/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BinaryLoader.h
 * Author: s0457958
 *
 * Created on 20 November 2013, 15:25
 */

#ifndef BINARYLOADER_H
#define BINARYLOADER_H

#include <cstdint>
#include <string>
#include <elf.h>

#include "abi/Address.h"

namespace archsim
{
	namespace abi
	{
		class EmulationModel;
		class UserEmulationModel;
		class SystemEmulationModel;

		namespace loader
		{

			class BinaryLoader
			{
			public:
				BinaryLoader(EmulationModel& emulation_model, bool load_symbols);
				virtual ~BinaryLoader();

				bool LoadBinary(std::string filename);

				Address GetEntryPoint();

			protected:
				std::string _binary_filename;
				const char* _binary_data;
				unsigned int _binary_size;
				Address _entry_point;
				EmulationModel& _emulation_model;

				virtual bool ProcessBinary(bool load_symbols) = 0;

			private:
				bool _load_symbols;
			};

			class FlatBinaryLoader : public BinaryLoader
			{
			public:
				FlatBinaryLoader(EmulationModel& emulation_model, Address base_addr);
				virtual ~FlatBinaryLoader();

			protected:
				bool ProcessBinary(bool load_symbols);

			private:
				Address _base_addr;
			};

			class ZImageBinaryLoader : public BinaryLoader
			{
			public:
				ZImageBinaryLoader(EmulationModel& emulation_model, Address base_addr, std::string symbol_map);
				ZImageBinaryLoader(EmulationModel& emulation_model, Address base_addr);
				virtual ~ZImageBinaryLoader();

			protected:
				bool ProcessBinary(bool load_symbols);

			private:
				bool LoadSymbolMap(std::string map_file);

				Address _base_addr;
				std::string _symbol_map;
			};

			class ElfBinaryLoader : public BinaryLoader
			{
			public:
				ElfBinaryLoader(EmulationModel& emulation_model, bool load_symbols);
				virtual ~ElfBinaryLoader() = 0;

			protected:
				Address _load_bias;
				Elf32_Ehdr *_elf_header;

				bool ProcessBinary(bool load_symbols);
				virtual bool PrepareLoad() = 0;
				virtual bool LoadSegment(Elf32_Phdr *segment) = 0;

			private:
				bool LoadSymbols();
				bool ProcessSymbolTable(Elf32_Shdr *string_table_section, Elf32_Shdr *symbol_table_section);
			};

			class UserElfBinaryLoader : public ElfBinaryLoader
			{
			public:
				UserElfBinaryLoader(UserEmulationModel& emulation_model, bool load_symbols);
				~UserElfBinaryLoader();

				Address GetInitialProgramBreak();

				Address GetProgramHeaderLocation();
				Address GetProgramHeaderEntrySize();
				unsigned int GetProgramHeaderEntryCount();

			protected:
				bool PrepareLoad();
				bool LoadSegment(Elf32_Phdr *segment);

			private:
				Address _initial_brk;
				Address _ph_loc;
			};

			class SystemElfBinaryLoader : public ElfBinaryLoader
			{
			public:
				SystemElfBinaryLoader(SystemEmulationModel& emulation_model, bool load_symbols);
				~SystemElfBinaryLoader();

			protected:
				bool PrepareLoad();
				bool LoadSegment(Elf32_Phdr *segment);
			};
		}
	}
}

#endif /* BINARYLOADER_H */
