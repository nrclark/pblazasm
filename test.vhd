
library ieee; 
use ieee.std_logic_1164.all; 
use ieee.numeric_std.all; 

use work.pb2_pkg.all ; 

entity PicoCore is
	port ( 
		clk : in std_logic ; 
		rst : in std_logic ; 

		PB2I : out t_PB2I ; 
		PB2O : in t_PB2O ; 

		PB2_IRQ : in std_logic ; 
		PB2_IQA : out std_logic
	 ) ; 
end PicoCore ; 

architecture mix of PicoCore is
	-- types
	subtype PC_t is unsigned( 11 downto 0 ) ; 
	subtype SP_t is unsigned( 4 downto 0 ) ; 
	subtype IN_t is unsigned( 19 downto 0 ) ; 

	-- state
	signal pc : PC_t := ( others => '0' ) ; 
	signal c : std_logic := '0' ; 
	signal z : std_logic := '0' ; 
	signal i : std_logic := '0' ; 

	-- stack 
	signal spW : SP_t := ( others => '0' ) ; 
	signal spR : SP_t := ( others => '1' ) ; 
	signal nspW : SP_t ; 
	signal nspR : SP_t ; 
	signal stackW : std_logic := '0' ; 
	signal stackT : std_logic_vector( 13 downto 0 ) ; 
	alias addrR : std_logic_vector is stackT( 11 downto 0 ) ; 

	-- scratchpad
	signal scrO : std_logic_vector( 7 downto 0 ) ; 
	signal scrW : std_logic := '0' ; 

	-- data and address paths
	signal inst : IN_t := X"221C5" ; 
	alias addrN : unsigned is inst( 11 downto 0 ) ; 
	alias regA_addr : unsigned is inst( 11 downto 8 ) ; 
	alias regB_addr : unsigned is inst( 7 downto 4 ) ; 
	alias dataK : unsigned is inst( 7 downto 0 ) ; 
	signal regA : std_logic_vector( 7 downto 0 ) ; 
	alias dataA : std_logic_vector is regA( 7 downto 0 ) ; 
	signal regB : std_logic_vector( 7 downto 0 ) ; 
	signal dataB : std_logic_vector( 7 downto 0 ) ; 
	signal regI : std_logic_vector( 7 downto 0 ) ; 
	signal regW : std_logic := '0' ; 

	-- next state 
	signal pc_1, npc : PC_t ; 
	signal nc, nz, ni : std_logic ; 
	signal nrst : std_logic ; 

	-- io control 
	signal nioR : std_logic ; 
	signal nioW : std_logic ; 
	signal ioR : std_logic := '0' ; 
	signal ioW : std_logic := '0' ; 

	-- alu 
	signal aluOP : std_logic_vector( 2 downto 0 ) ; 
	signal alu : std_logic_vector( 7 downto 0 ) ; 
	signal ci, co : std_logic ; 

	function to_std_logic( l : boolean ) return std_ulogic is 
	begin 
		if l then 
			return '1' ; 
		else 
			return '0' ; 
		end if ; 
	end function to_std_logic ; 

	function xor_reduce( arg : std_logic_vector ) return std_logic is 
		variable result : std_logic ; 
	begin 
		result := '0' ; 
		for i in arg'range loop 
			result := result xor arg( i ) ; 
		end loop ; 
		return result ; 
	end ; 
begin
	-- io connections
	PB2I.ck <= clk ; 
	PB2I.rs <= rst ; 
	PB2I.da <= dataA ; 
	PB2I.ad <= dataB ; 
	PB2I.wr <= ioW ; 
	PB2I.rd <= ioR ; 

	-- program counter
	pc_1 <= pc + 1 ; 

	-- stack
	stack_b : block is 
		type STACK_t is array( 0 to 31 ) of std_logic_vector( 13 downto 0 ) ; 
		signal stack_ram : STACK_t ; 
		signal nstackT : std_logic_vector( 13 downto 0 ) ; 
	begin
		nstackT( 13 ) <= z ; 
		nstackT( 12 ) <= c ; 
		nstackT( 11 downto 0 ) <= std_logic_vector( pc + 1 ) ; 
		process ( clk ) is 
		begin 
			if rising_edge( clk ) then 
				if stackW = '1' then 
					stack_ram( to_integer( spW ) ) <= nstackT ; 
				end if ; 
			end if ; 
		end process ; 

		stackT <= stack_ram( to_integer( spR ) ) ; 
	end block ; 

	-- registers
	rb : block is
		type REG_t is array( 0 to 15 ) of std_logic_vector( 7 downto 0 ) ; 
		signal register_ram : REG_t ; 
	begin
		process ( clk ) is
		begin
			if rising_edge( clk ) then
				if regW = '1' then
					register_ram( to_integer( regA_addr ) ) <= regI ; 
				end if ; 
		end if ; 
		end process ; 

		regA <= register_ram( to_integer( regA_addr ) ) ; 
		regB <= register_ram( to_integer( regB_addr ) ) ; 
	end block ; 

	-- scratchpad
	sb : block is 
		type SCRATCH_t is array( 0 to 255 ) of std_logic_vector( 7 downto 0 ) ; 
		signal scratch_ram : SCRATCH_t := ( 
			0   => X"2B", 1   => X"7E", 2   => X"15", 3   => X"16", 4   => X"28", 5   => X"AE", 6   => X"D2", 7   => X"A6", 
			8   => X"AB", 9   => X"F7", 10  => X"15", 11  => X"88", 12  => X"09", 13  => X"CF", 14  => X"4F", 15  => X"3C", 
			16  => X"32", 17  => X"43", 18  => X"F6", 19  => X"A8", 20  => X"88", 21  => X"5A", 22  => X"30", 23  => X"8D", 
			24  => X"31", 25  => X"31", 26  => X"98", 27  => X"A2", 28  => X"E0", 29  => X"37", 30  => X"07", 31  => X"34", 
			91  => X"0D", 92  => X"0A", 93  => X"20", 94  => X"20", 95  => X"64", 96  => X"29", 97  => X"75", 98  => X"6D", 
			99  => X"70", 100 => X"2C", 101 => X"20", 102 => X"73", 103 => X"29", 104 => X"63", 105 => X"72", 106 => X"61", 
			107 => X"74", 108 => X"63", 109 => X"68", 110 => X"70", 111 => X"61", 112 => X"64", 113 => X"2C", 114 => X"20", 
			115 => X"70", 116 => X"29", 117 => X"6F", 118 => X"72", 119 => X"74", 120 => X"73", 121 => X"2C", 122 => X"20", 
			123 => X"67", 124 => X"29", 125 => X"6F", 126 => X"2C", 127 => X"20", 128 => X"63", 129 => X"29", 130 => X"6F", 
			131 => X"75", 132 => X"6E", 133 => X"74", 134 => X"65", 135 => X"72", 136 => X"2C", 137 => X"20", 138 => X"72", 
			139 => X"29", 140 => X"69", 141 => X"6A", 142 => X"6E", 143 => X"64", 144 => X"61", 145 => X"65", 146 => X"6C", 
			148 => X"0D", 149 => X"0A", 150 => X"50", 151 => X"69", 152 => X"63", 153 => X"6F", 154 => X"43", 155 => X"6F", 
			156 => X"72", 157 => X"65", 158 => X"99", 159 => X"3A", 160 => X"20", 162 => X"20", 163 => X"63", 164 => X"79", 
			165 => X"63", 166 => X"6C", 167 => X"65", 168 => X"73", 170 => X"0D", 171 => X"0A", 172 => X"3F", 173 => X"3F", 
			
			others => X"00" 
		 ) ; 
	begin 
		process ( clk ) is 
		begin 
			if rising_edge( clk ) then 
				if scrW = '1' then 
					scratch_ram( to_integer( unsigned( dataB ) ) ) <= dataA ; 
				end if ; 
			end if ; 
		end process ; 

		scrO <= scratch_ram( to_integer( unsigned( dataB ) ) ) ; 
	end block ; 

	-- alu
	alu_b : block is
		signal mask : std_logic_vector( 7 downto 0 ) ; 
		signal sum : std_logic_vector( 7 downto 0 ) ; 
		signal chain : std_logic_vector( 8 downto 0 ) ; 
	begin
		chain( 0 ) <= ci ; 

		-- Manchester carry chain
		alu_g : for i in 0 to 7 generate
		begin
			sum( i ) <=
				dataA( i ) xor dataB( i )        when aluOP = "111" else -- ADD, ADDC 
				not( dataA( i ) xor dataB( i ) ) when aluOP = "110" else -- SUB, SUBC, COMP, CMPC 
				dataA( i ) xor dataB( i )        when aluOP = "101" else -- ADD, ADDC 
				not( dataA( i ) xor dataB( i ) ) when aluOP = "100" else -- SUB, SUBC, COMP, CMPC 

				dataA( i ) xor dataB( i )        when aluOP = "011" else -- XOR 
				dataA( i )  or dataB( i )        when aluOP = "010" else -- OR 
				dataA( i ) and dataB( i )        when aluOP = "001" else -- AND 
				               dataB( i )        when aluOP = "000" ;    -- MOVE 

			mask( i ) <= dataA( i ) and aluOP( 2 ) ; 

			chain( i + 1 ) <=
				chain( i ) when sum( i ) = '1' else
				mask( i ) ; 

			alu( i ) <= sum( i ) xor chain( i ) ; 
		end generate ; 

		co <= chain( 8 ) ; 
	end block ; 

	-- new state
	process ( rst, clk ) is 
	begin
		if rst = '1' then 
			c <= '0' ; 
			z <= '0' ; 
			i <= '0' ; 
			nrst <= '1' ; 
			ioR <= '0' ; 
			ioW <= '0' ; 

			pc <= ( others => '0' ) ; 
			spW <= ( others => '0' ) ; 
			spR <= ( others => '1' ) ; 
		elsif rising_edge( clk ) then
			if nioR = '1' and ioR = '0' then 
				ioR <= '1' ; 
			elsif nioW = '1' and ioW = '0' then 
				ioW <= '1' ; 
			elsif nrst = '0' then 
				c <= nc ; 
				z <= nz ; 
				i <= ni ; 

				pc <= npc ; 
				spW <= nspW ; 
				spR <= nspR ; 
				ioR <= '0' ; 
				ioW <= '0' ; 
			end if ; 
			nrst <= '0' ; 
		end if ; 
	end process ; 

	-- next state
	process ( PB2O, ioR, ioW, pc, pc_1, spW, spR, stackT, scrO, regI, dataA, regB, inst, dataB, alu, co, ci, c, z, i ) is
	begin
			npc <= pc_1 ; 
			nspW <= spW ; 
			nspR <= spR ; 
			regI <= dataB ; 
			aluOP <= ( others => '0' ) ; 
			ci <= '0' ; 

			nc <= c ; 
			nz <= z ; 
			ni <= i ; 

			nioR <= '0' ; 
			nioW <= '0' ; 
			scrW <= '0' ; 
			regW <= '0' ; 
			stackW <= '0' ; 

			-- program
			case pc is
			when X"000" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 000 : 221C5 
				npc <= addrN ; 
			when X"001" => 
				inst <= X"29000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RETI	DISABLE	; 001 : 29000 
				ni <= inst( 0 ) ; 
				npc <= unsigned( addrR ) ; 
				nc <= stackT( 12 ) ; 
				nz <= stackT( 13 ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"002" => 
				inst <= X"0A010" ; 
				dataB <= regB ; 
				-- LD  	s0, s1  	; 002 : 0A010 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"003" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 003 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"004" => 
				inst <= X"1D000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x00	; 004 : 1D000 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"005" => 
				inst <= X"31000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	 Z        	; 005 : 31000 
				if z = '1' then 
					npc <= unsigned( addrR ) ; 
					nspW <= spR ; 
					nspR <= spR - 1 ; 
				end if ; 
			when X"006" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 006 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"007" => 
				inst <= X"22002" ; 
				dataB <= regB ; 
				-- JUMP	0x002    	; 007 : 22002 
				npc <= addrN ; 
			when X"008" => 
				inst <= X"00E00" ; 
				dataB <= regB ; 
				-- MOVE	sE, s0  	; 008 : 00E00 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"009" => 
				inst <= X"1400E" ; 
				dataB <= regB ; 
				-- SR0 	s0      	; 009 : 1400E 
				regI <= '0' & dataA( 7 downto 1 ) ; 
				nc <= dataA( 0 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"00A" => 
				inst <= X"1400E" ; 
				dataB <= regB ; 
				-- SR0 	s0      	; 00A : 1400E 
				regI <= '0' & dataA( 7 downto 1 ) ; 
				nc <= dataA( 0 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"00B" => 
				inst <= X"1400E" ; 
				dataB <= regB ; 
				-- SR0 	s0      	; 00B : 1400E 
				regI <= '0' & dataA( 7 downto 1 ) ; 
				nc <= dataA( 0 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"00C" => 
				inst <= X"1400E" ; 
				dataB <= regB ; 
				-- SR0 	s0      	; 00C : 1400E 
				regI <= '0' & dataA( 7 downto 1 ) ; 
				nc <= dataA( 0 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"00D" => 
				inst <= X"2000F" ; 
				dataB <= regB ; 
				-- CALL	0x00F    	; 00D : 2000F 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"00E" => 
				inst <= X"000E0" ; 
				dataB <= regB ; 
				-- MOVE	s0, sE  	; 00E : 000E0 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"00F" => 
				inst <= X"0300F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- AND 	s0, 0x0F	; 00F : 0300F 
				aluOP <= B"001" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"010" => 
				inst <= X"1D00A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x0A	; 010 : 1D00A 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"011" => 
				inst <= X"3A013" ; 
				dataB <= regB ; 
				-- JUMP	C, 0x013 	; 011 : 3A013 
				if c = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"012" => 
				inst <= X"11007" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x07	; 012 : 11007 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"013" => 
				inst <= X"11030" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x30	; 013 : 11030 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"014" => 
				inst <= X"2201A" ; 
				dataB <= regB ; 
				-- JUMP	0x01A    	; 014 : 2201A 
				npc <= addrN ; 
			when X"015" => 
				inst <= X"01020" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x20	; 015 : 01020 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"016" => 
				inst <= X"2201A" ; 
				dataB <= regB ; 
				-- JUMP	0x01A    	; 016 : 2201A 
				npc <= addrN ; 
			when X"017" => 
				inst <= X"0100D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x0D	; 017 : 0100D 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"018" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 018 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"019" => 
				inst <= X"0100A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x0A	; 019 : 0100A 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"01A" => 
				inst <= X"09FEC" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	sF, 0xEC	; 01A : 09FEC 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"01B" => 
				inst <= X"0DF01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- TEST	sF, 0x01	; 01B : 0DF01 
				aluOP <= B"001" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= xor_reduce( regI ) ; 
			when X"01C" => 
				inst <= X"3601A" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x01A	; 01C : 3601A 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"01D" => 
				inst <= X"2D0ED" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s0, 0xED	; 01D : 2D0ED 
				nioW <= '1' ; 
			when X"01E" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 01E : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"01F" => 
				inst <= X"2003A" ; 
				dataB <= regB ; 
				-- CALL	0x03A    	; 01F : 2003A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"020" => 
				inst <= X"00E00" ; 
				dataB <= regB ; 
				-- MOVE	sE, s0  	; 020 : 00E00 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"021" => 
				inst <= X"19E30" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sE, 0x30	; 021 : 19E30 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"022" => 
				inst <= X"1DE0A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	sE, 0x0A	; 022 : 1DE0A 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"023" => 
				inst <= X"3A02B" ; 
				dataB <= regB ; 
				-- JUMP	C, 0x02B 	; 023 : 3A02B 
				if c = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"024" => 
				inst <= X"19E11" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sE, 0x11	; 024 : 19E11 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"025" => 
				inst <= X"1DE06" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	sE, 0x06	; 025 : 1DE06 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"026" => 
				inst <= X"3A02A" ; 
				dataB <= regB ; 
				-- JUMP	C, 0x02A 	; 026 : 3A02A 
				if c = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"027" => 
				inst <= X"19E20" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sE, 0x20	; 027 : 19E20 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"028" => 
				inst <= X"1DE06" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	sE, 0x06	; 028 : 1DE06 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"029" => 
				inst <= X"3E01F" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x01F	; 029 : 3E01F 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"02A" => 
				inst <= X"11E0A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	sE, 0x0A	; 02A : 11E0A 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"02B" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 02B : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"02C" => 
				inst <= X"000E0" ; 
				dataB <= regB ; 
				-- MOVE	s0, sE  	; 02C : 000E0 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"02D" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 02D : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"02E" => 
				inst <= X"2001F" ; 
				dataB <= regB ; 
				-- CALL	0x01F    	; 02E : 2001F 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"02F" => 
				inst <= X"00D00" ; 
				dataB <= regB ; 
				-- MOVE	sD, s0  	; 02F : 00D00 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"030" => 
				inst <= X"14D06" ; 
				dataB <= regB ; 
				-- SL0 	sD      	; 030 : 14D06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"031" => 
				inst <= X"14D06" ; 
				dataB <= regB ; 
				-- SL0 	sD      	; 031 : 14D06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"032" => 
				inst <= X"14D06" ; 
				dataB <= regB ; 
				-- SL0 	sD      	; 032 : 14D06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"033" => 
				inst <= X"14D06" ; 
				dataB <= regB ; 
				-- SL0 	sD      	; 033 : 14D06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"034" => 
				inst <= X"2001F" ; 
				dataB <= regB ; 
				-- CALL	0x01F    	; 034 : 2001F 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"035" => 
				inst <= X"040D0" ; 
				dataB <= regB ; 
				-- OR  	s0, sD  	; 035 : 040D0 
				aluOP <= B"010" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"036" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 036 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"037" => 
				inst <= X"09FEC" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	sF, 0xEC	; 037 : 09FEC 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"038" => 
				inst <= X"0DF20" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- TEST	sF, 0x20	; 038 : 0DF20 
				aluOP <= B"001" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= xor_reduce( regI ) ; 
			when X"039" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 039 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"03A" => 
				inst <= X"09FEC" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	sF, 0xEC	; 03A : 09FEC 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"03B" => 
				inst <= X"0DF20" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- TEST	sF, 0x20	; 03B : 0DF20 
				aluOP <= B"001" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= xor_reduce( regI ) ; 
			when X"03C" => 
				inst <= X"3203A" ; 
				dataB <= regB ; 
				-- JUMP	Z, 0x03A 	; 03C : 3203A 
				if z = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"03D" => 
				inst <= X"090ED" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s0, 0xED	; 03D : 090ED 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"03E" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 03E : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"03F" => 
				inst <= X"20060" ; 
				dataB <= regB ; 
				-- CALL	0x060    	; 03F : 20060 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"040" => 
				inst <= X"2005D" ; 
				dataB <= regB ; 
				-- CALL	0x05D    	; 040 : 2005D 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"041" => 
				inst <= X"20051" ; 
				dataB <= regB ; 
				-- CALL	0x051    	; 041 : 20051 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"042" => 
				inst <= X"01F01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	sF, 0x01	; 042 : 01F01 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"043" => 
				inst <= X"01309" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s3, 0x09	; 043 : 01309 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"044" => 
				inst <= X"200AA" ; 
				dataB <= regB ; 
				-- CALL	0x0AA    	; 044 : 200AA 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"045" => 
				inst <= X"200D7" ; 
				dataB <= regB ; 
				-- CALL	0x0D7    	; 045 : 200D7 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"046" => 
				inst <= X"200F0" ; 
				dataB <= regB ; 
				-- CALL	0x0F0    	; 046 : 200F0 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"047" => 
				inst <= X"20074" ; 
				dataB <= regB ; 
				-- CALL	0x074    	; 047 : 20074 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"048" => 
				inst <= X"20051" ; 
				dataB <= regB ; 
				-- CALL	0x051    	; 048 : 20051 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"049" => 
				inst <= X"19301" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s3, 0x01	; 049 : 19301 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"04A" => 
				inst <= X"36044" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x044	; 04A : 36044 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"04B" => 
				inst <= X"200AA" ; 
				dataB <= regB ; 
				-- CALL	0x0AA    	; 04B : 200AA 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"04C" => 
				inst <= X"200D7" ; 
				dataB <= regB ; 
				-- CALL	0x0D7    	; 04C : 200D7 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"04D" => 
				inst <= X"20074" ; 
				dataB <= regB ; 
				-- CALL	0x074    	; 04D : 20074 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"04E" => 
				inst <= X"20051" ; 
				dataB <= regB ; 
				-- CALL	0x051    	; 04E : 20051 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"04F" => 
				inst <= X"2006A" ; 
				dataB <= regB ; 
				-- CALL	0x06A    	; 04F : 2006A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"050" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 050 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"051" => 
				inst <= X"01120" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x20	; 051 : 01120 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"052" => 
				inst <= X"01240" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x40	; 052 : 01240 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"053" => 
				inst <= X"01010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x10	; 053 : 01010 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"054" => 
				inst <= X"0A410" ; 
				dataB <= regB ; 
				-- LD  	s4, s1  	; 054 : 0A410 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"055" => 
				inst <= X"0A520" ; 
				dataB <= regB ; 
				-- LD  	s5, s2  	; 055 : 0A520 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"056" => 
				inst <= X"06450" ; 
				dataB <= regB ; 
				-- XOR 	s4, s5  	; 056 : 06450 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"057" => 
				inst <= X"2E420" ; 
				dataB <= regB ; 
				-- ST  	s4, s2  	; 057 : 2E420 
				scrW <= '1' ; 
			when X"058" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 058 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"059" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 059 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"05A" => 
				inst <= X"19001" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x01	; 05A : 19001 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"05B" => 
				inst <= X"36054" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x054	; 05B : 36054 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"05C" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 05C : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"05D" => 
				inst <= X"01110" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x10	; 05D : 01110 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"05E" => 
				inst <= X"01240" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x40	; 05E : 01240 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"05F" => 
				inst <= X"22062" ; 
				dataB <= regB ; 
				-- JUMP	0x062    	; 05F : 22062 
				npc <= addrN ; 
			when X"060" => 
				inst <= X"01100" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x00	; 060 : 01100 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"061" => 
				inst <= X"01220" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x20	; 061 : 01220 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"062" => 
				inst <= X"01010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x10	; 062 : 01010 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"063" => 
				inst <= X"0A410" ; 
				dataB <= regB ; 
				-- LD  	s4, s1  	; 063 : 0A410 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"064" => 
				inst <= X"2E420" ; 
				dataB <= regB ; 
				-- ST  	s4, s2  	; 064 : 2E420 
				scrW <= '1' ; 
			when X"065" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 065 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"066" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 066 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"067" => 
				inst <= X"19001" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x01	; 067 : 19001 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"068" => 
				inst <= X"36063" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x063	; 068 : 36063 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"069" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 069 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"06A" => 
				inst <= X"01140" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x40	; 06A : 01140 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"06B" => 
				inst <= X"01230" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x30	; 06B : 01230 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"06C" => 
				inst <= X"01010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x10	; 06C : 01010 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"06D" => 
				inst <= X"0A410" ; 
				dataB <= regB ; 
				-- LD  	s4, s1  	; 06D : 0A410 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"06E" => 
				inst <= X"2E420" ; 
				dataB <= regB ; 
				-- ST  	s4, s2  	; 06E : 2E420 
				scrW <= '1' ; 
			when X"06F" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 06F : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"070" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 070 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"071" => 
				inst <= X"19001" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x01	; 071 : 19001 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"072" => 
				inst <= X"3606D" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x06D	; 072 : 3606D 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"073" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 073 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"074" => 
				inst <= X"0B42C" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x2C	; 074 : 0B42C 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"075" => 
				inst <= X"0B52D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x2D	; 075 : 0B52D 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"076" => 
				inst <= X"0B62E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x2E	; 076 : 0B62E 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"077" => 
				inst <= X"0B72F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x2F	; 077 : 0B72F 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"078" => 
				inst <= X"00840" ; 
				dataB <= regB ; 
				-- MOVE	s8, s4  	; 078 : 00840 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"079" => 
				inst <= X"00450" ; 
				dataB <= regB ; 
				-- MOVE	s4, s5  	; 079 : 00450 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"07A" => 
				inst <= X"00560" ; 
				dataB <= regB ; 
				-- MOVE	s5, s6  	; 07A : 00560 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"07B" => 
				inst <= X"00670" ; 
				dataB <= regB ; 
				-- MOVE	s6, s7  	; 07B : 00670 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"07C" => 
				inst <= X"00780" ; 
				dataB <= regB ; 
				-- MOVE	s7, s8  	; 07C : 00780 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"07D" => 
				inst <= X"00840" ; 
				dataB <= regB ; 
				-- MOVE	s8, s4  	; 07D : 00840 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"07E" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 07E : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"07F" => 
				inst <= X"00480" ; 
				dataB <= regB ; 
				-- MOVE	s4, s8  	; 07F : 00480 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"080" => 
				inst <= X"064F0" ; 
				dataB <= regB ; 
				-- XOR 	s4, sF  	; 080 : 064F0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"081" => 
				inst <= X"14F06" ; 
				dataB <= regB ; 
				-- SL0 	sF      	; 081 : 14F06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"082" => 
				inst <= X"3E084" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x084	; 082 : 3E084 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"083" => 
				inst <= X"07F1B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	sF, 0x1B	; 083 : 07F1B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"084" => 
				inst <= X"00850" ; 
				dataB <= regB ; 
				-- MOVE	s8, s5  	; 084 : 00850 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"085" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 085 : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"086" => 
				inst <= X"00580" ; 
				dataB <= regB ; 
				-- MOVE	s5, s8  	; 086 : 00580 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"087" => 
				inst <= X"00860" ; 
				dataB <= regB ; 
				-- MOVE	s8, s6  	; 087 : 00860 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"088" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 088 : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"089" => 
				inst <= X"00680" ; 
				dataB <= regB ; 
				-- MOVE	s6, s8  	; 089 : 00680 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"08A" => 
				inst <= X"00870" ; 
				dataB <= regB ; 
				-- MOVE	s8, s7  	; 08A : 00870 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"08B" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 08B : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"08C" => 
				inst <= X"00780" ; 
				dataB <= regB ; 
				-- MOVE	s7, s8  	; 08C : 00780 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"08D" => 
				inst <= X"01120" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x20	; 08D : 01120 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"08E" => 
				inst <= X"01010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x10	; 08E : 01010 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"08F" => 
				inst <= X"0A810" ; 
				dataB <= regB ; 
				-- LD  	s8, s1  	; 08F : 0A810 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"090" => 
				inst <= X"06480" ; 
				dataB <= regB ; 
				-- XOR 	s4, s8  	; 090 : 06480 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"091" => 
				inst <= X"2E410" ; 
				dataB <= regB ; 
				-- ST  	s4, s1  	; 091 : 2E410 
				scrW <= '1' ; 
			when X"092" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 092 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"093" => 
				inst <= X"0A810" ; 
				dataB <= regB ; 
				-- LD  	s8, s1  	; 093 : 0A810 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"094" => 
				inst <= X"06580" ; 
				dataB <= regB ; 
				-- XOR 	s5, s8  	; 094 : 06580 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"095" => 
				inst <= X"2E510" ; 
				dataB <= regB ; 
				-- ST  	s5, s1  	; 095 : 2E510 
				scrW <= '1' ; 
			when X"096" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 096 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"097" => 
				inst <= X"0A810" ; 
				dataB <= regB ; 
				-- LD  	s8, s1  	; 097 : 0A810 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"098" => 
				inst <= X"06680" ; 
				dataB <= regB ; 
				-- XOR 	s6, s8  	; 098 : 06680 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"099" => 
				inst <= X"2E610" ; 
				dataB <= regB ; 
				-- ST  	s6, s1  	; 099 : 2E610 
				scrW <= '1' ; 
			when X"09A" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 09A : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"09B" => 
				inst <= X"0A810" ; 
				dataB <= regB ; 
				-- LD  	s8, s1  	; 09B : 0A810 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"09C" => 
				inst <= X"06780" ; 
				dataB <= regB ; 
				-- XOR 	s7, s8  	; 09C : 06780 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"09D" => 
				inst <= X"2E710" ; 
				dataB <= regB ; 
				-- ST  	s7, s1  	; 09D : 2E710 
				scrW <= '1' ; 
			when X"09E" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 09E : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"09F" => 
				inst <= X"19004" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x04	; 09F : 19004 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0A0" => 
				inst <= X"3608F" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x08F	; 0A0 : 3608F 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"0A1" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0A1 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0A2" => 
				inst <= X"01004" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x04	; 0A2 : 01004 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0A3" => 
				inst <= X"0A810" ; 
				dataB <= regB ; 
				-- LD  	s8, s1  	; 0A3 : 0A810 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0A4" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 0A4 : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0A5" => 
				inst <= X"2E810" ; 
				dataB <= regB ; 
				-- ST  	s8, s1  	; 0A5 : 2E810 
				scrW <= '1' ; 
			when X"0A6" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 0A6 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0A7" => 
				inst <= X"19001" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x01	; 0A7 : 19001 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0A8" => 
				inst <= X"360A3" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x0A3	; 0A8 : 360A3 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"0A9" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0A9 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0AA" => 
				inst <= X"01240" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x40	; 0AA : 01240 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0AB" => 
				inst <= X"01010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x10	; 0AB : 01010 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0AC" => 
				inst <= X"0A820" ; 
				dataB <= regB ; 
				-- LD  	s8, s2  	; 0AC : 0A820 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0AD" => 
				inst <= X"200B3" ; 
				dataB <= regB ; 
				-- CALL	0x0B3    	; 0AD : 200B3 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0AE" => 
				inst <= X"2E820" ; 
				dataB <= regB ; 
				-- ST  	s8, s2  	; 0AE : 2E820 
				scrW <= '1' ; 
			when X"0AF" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 0AF : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0B0" => 
				inst <= X"19001" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x01	; 0B0 : 19001 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0B1" => 
				inst <= X"360AC" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x0AC	; 0B1 : 360AC 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"0B2" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0B2 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0B3" => 
				inst <= X"2D8F0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s8, 0xF0	; 0B3 : 2D8F0 
				nioW <= '1' ; 
			when X"0B4" => 
				inst <= X"098F0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s8, 0xF0	; 0B4 : 098F0 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"0B5" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0B5 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0B6" => 
				inst <= X"200C2" ; 
				dataB <= regB ; 
				-- CALL	0x0C2    	; 0B6 : 200C2 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0B7" => 
				inst <= X"00890" ; 
				dataB <= regB ; 
				-- MOVE	s8, s9  	; 0B7 : 00890 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0B8" => 
				inst <= X"14902" ; 
				dataB <= regB ; 
				-- RL  	s9      	; 0B8 : 14902 
				regI <= dataA( 6 downto 0 ) & dataA( 7 ) ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0B9" => 
				inst <= X"06890" ; 
				dataB <= regB ; 
				-- XOR 	s8, s9  	; 0B9 : 06890 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0BA" => 
				inst <= X"14902" ; 
				dataB <= regB ; 
				-- RL  	s9      	; 0BA : 14902 
				regI <= dataA( 6 downto 0 ) & dataA( 7 ) ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0BB" => 
				inst <= X"06890" ; 
				dataB <= regB ; 
				-- XOR 	s8, s9  	; 0BB : 06890 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0BC" => 
				inst <= X"14902" ; 
				dataB <= regB ; 
				-- RL  	s9      	; 0BC : 14902 
				regI <= dataA( 6 downto 0 ) & dataA( 7 ) ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0BD" => 
				inst <= X"06890" ; 
				dataB <= regB ; 
				-- XOR 	s8, s9  	; 0BD : 06890 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0BE" => 
				inst <= X"14902" ; 
				dataB <= regB ; 
				-- RL  	s9      	; 0BE : 14902 
				regI <= dataA( 6 downto 0 ) & dataA( 7 ) ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0BF" => 
				inst <= X"06890" ; 
				dataB <= regB ; 
				-- XOR 	s8, s9  	; 0BF : 06890 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0C0" => 
				inst <= X"07863" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	s8, 0x63	; 0C0 : 07863 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0C1" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0C1 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0C2" => 
				inst <= X"01900" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s9, 0x00	; 0C2 : 01900 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0C3" => 
				inst <= X"04880" ; 
				dataB <= regB ; 
				-- OR  	s8, s8  	; 0C3 : 04880 
				aluOP <= B"010" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0C4" => 
				inst <= X"31000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	 Z        	; 0C4 : 31000 
				if z = '1' then 
					npc <= unsigned( addrR ) ; 
					nspW <= spR ; 
					nspR <= spR - 1 ; 
				end if ; 
			when X"0C5" => 
				inst <= X"11901" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s9, 0x01	; 0C5 : 11901 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0C6" => 
				inst <= X"31000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	 Z        	; 0C6 : 31000 
				if z = '1' then 
					npc <= unsigned( addrR ) ; 
					nspW <= spR ; 
					nspR <= spR - 1 ; 
				end if ; 
			when X"0C7" => 
				inst <= X"00C80" ; 
				dataB <= regB ; 
				-- MOVE	sC, s8  	; 0C7 : 00C80 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0C8" => 
				inst <= X"00D90" ; 
				dataB <= regB ; 
				-- MOVE	sD, s9  	; 0C8 : 00D90 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0C9" => 
				inst <= X"200CD" ; 
				dataB <= regB ; 
				-- CALL	0x0CD    	; 0C9 : 200CD 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0CA" => 
				inst <= X"19E01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sE, 0x01	; 0CA : 19E01 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0CB" => 
				inst <= X"360C5" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x0C5	; 0CB : 360C5 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"0CC" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0CC : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0CD" => 
				inst <= X"01E00" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	sE, 0x00	; 0CD : 01E00 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"0CE" => 
				inst <= X"14D0E" ; 
				dataB <= regB ; 
				-- SR0 	sD      	; 0CE : 14D0E 
				regI <= '0' & dataA( 7 downto 1 ) ; 
				nc <= dataA( 0 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0CF" => 
				inst <= X"3A0D2" ; 
				dataB <= regB ; 
				-- JUMP	C, 0x0D2 	; 0CF : 3A0D2 
				if c = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"0D0" => 
				inst <= X"31000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	 Z        	; 0D0 : 31000 
				if z = '1' then 
					npc <= unsigned( addrR ) ; 
					nspW <= spR ; 
					nspR <= spR - 1 ; 
				end if ; 
			when X"0D1" => 
				inst <= X"220D3" ; 
				dataB <= regB ; 
				-- JUMP	0x0D3    	; 0D1 : 220D3 
				npc <= addrN ; 
			when X"0D2" => 
				inst <= X"06EC0" ; 
				dataB <= regB ; 
				-- XOR 	sE, sC  	; 0D2 : 06EC0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0D3" => 
				inst <= X"14C06" ; 
				dataB <= regB ; 
				-- SL0 	sC      	; 0D3 : 14C06 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"0D4" => 
				inst <= X"3E0CE" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x0CE	; 0D4 : 3E0CE 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"0D5" => 
				inst <= X"07C1B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	sC, 0x1B	; 0D5 : 07C1B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"0D6" => 
				inst <= X"220CE" ; 
				dataB <= regB ; 
				-- JUMP	0x0CE    	; 0D6 : 220CE 
				npc <= addrN ; 
			when X"0D7" => 
				inst <= X"0B741" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x41	; 0D7 : 0B741 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0D8" => 
				inst <= X"0B445" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x45	; 0D8 : 0B445 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0D9" => 
				inst <= X"0B549" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x49	; 0D9 : 0B549 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0DA" => 
				inst <= X"0B64D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x4D	; 0DA : 0B64D 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0DB" => 
				inst <= X"2F441" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x41	; 0DB : 2F441 
				scrW <= '1' ; 
			when X"0DC" => 
				inst <= X"2F545" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x45	; 0DC : 2F545 
				scrW <= '1' ; 
			when X"0DD" => 
				inst <= X"2F649" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x49	; 0DD : 2F649 
				scrW <= '1' ; 
			when X"0DE" => 
				inst <= X"2F74D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x4D	; 0DE : 2F74D 
				scrW <= '1' ; 
			when X"0DF" => 
				inst <= X"0B642" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x42	; 0DF : 0B642 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E0" => 
				inst <= X"0B746" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x46	; 0E0 : 0B746 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E1" => 
				inst <= X"0B44A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x4A	; 0E1 : 0B44A 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E2" => 
				inst <= X"0B54E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x4E	; 0E2 : 0B54E 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E3" => 
				inst <= X"2F442" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x42	; 0E3 : 2F442 
				scrW <= '1' ; 
			when X"0E4" => 
				inst <= X"2F546" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x46	; 0E4 : 2F546 
				scrW <= '1' ; 
			when X"0E5" => 
				inst <= X"2F64A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x4A	; 0E5 : 2F64A 
				scrW <= '1' ; 
			when X"0E6" => 
				inst <= X"2F74E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x4E	; 0E6 : 2F74E 
				scrW <= '1' ; 
			when X"0E7" => 
				inst <= X"0B543" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x43	; 0E7 : 0B543 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E8" => 
				inst <= X"0B647" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x47	; 0E8 : 0B647 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0E9" => 
				inst <= X"0B74B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x4B	; 0E9 : 0B74B 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0EA" => 
				inst <= X"0B44F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x4F	; 0EA : 0B44F 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0EB" => 
				inst <= X"2F443" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x43	; 0EB : 2F443 
				scrW <= '1' ; 
			when X"0EC" => 
				inst <= X"2F547" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x47	; 0EC : 2F547 
				scrW <= '1' ; 
			when X"0ED" => 
				inst <= X"2F64B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x4B	; 0ED : 2F64B 
				scrW <= '1' ; 
			when X"0EE" => 
				inst <= X"2F74F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x4F	; 0EE : 2F74F 
				scrW <= '1' ; 
			when X"0EF" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 0EF : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"0F0" => 
				inst <= X"0B440" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x40	; 0F0 : 0B440 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0F1" => 
				inst <= X"0B541" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x41	; 0F1 : 0B541 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0F2" => 
				inst <= X"0B642" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x42	; 0F2 : 0B642 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0F3" => 
				inst <= X"0B743" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x43	; 0F3 : 0B743 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0F4" => 
				inst <= X"20115" ; 
				dataB <= regB ; 
				-- CALL	0x115    	; 0F4 : 20115 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0F5" => 
				inst <= X"2F440" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x40	; 0F5 : 2F440 
				scrW <= '1' ; 
			when X"0F6" => 
				inst <= X"2F541" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x41	; 0F6 : 2F541 
				scrW <= '1' ; 
			when X"0F7" => 
				inst <= X"2F642" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x42	; 0F7 : 2F642 
				scrW <= '1' ; 
			when X"0F8" => 
				inst <= X"2F743" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x43	; 0F8 : 2F743 
				scrW <= '1' ; 
			when X"0F9" => 
				inst <= X"0B444" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x44	; 0F9 : 0B444 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0FA" => 
				inst <= X"0B545" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x45	; 0FA : 0B545 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0FB" => 
				inst <= X"0B646" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x46	; 0FB : 0B646 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0FC" => 
				inst <= X"0B747" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x47	; 0FC : 0B747 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"0FD" => 
				inst <= X"20115" ; 
				dataB <= regB ; 
				-- CALL	0x115    	; 0FD : 20115 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"0FE" => 
				inst <= X"2F444" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x44	; 0FE : 2F444 
				scrW <= '1' ; 
			when X"0FF" => 
				inst <= X"2F545" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x45	; 0FF : 2F545 
				scrW <= '1' ; 
			when X"100" => 
				inst <= X"2F646" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x46	; 100 : 2F646 
				scrW <= '1' ; 
			when X"101" => 
				inst <= X"2F747" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x47	; 101 : 2F747 
				scrW <= '1' ; 
			when X"102" => 
				inst <= X"0B448" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x48	; 102 : 0B448 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"103" => 
				inst <= X"0B549" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x49	; 103 : 0B549 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"104" => 
				inst <= X"0B64A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x4A	; 104 : 0B64A 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"105" => 
				inst <= X"0B74B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x4B	; 105 : 0B74B 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"106" => 
				inst <= X"20115" ; 
				dataB <= regB ; 
				-- CALL	0x115    	; 106 : 20115 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"107" => 
				inst <= X"2F448" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x48	; 107 : 2F448 
				scrW <= '1' ; 
			when X"108" => 
				inst <= X"2F549" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x49	; 108 : 2F549 
				scrW <= '1' ; 
			when X"109" => 
				inst <= X"2F64A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x4A	; 109 : 2F64A 
				scrW <= '1' ; 
			when X"10A" => 
				inst <= X"2F74B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x4B	; 10A : 2F74B 
				scrW <= '1' ; 
			when X"10B" => 
				inst <= X"0B44C" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s4, 0x4C	; 10B : 0B44C 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"10C" => 
				inst <= X"0B54D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s5, 0x4D	; 10C : 0B54D 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"10D" => 
				inst <= X"0B64E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s6, 0x4E	; 10D : 0B64E 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"10E" => 
				inst <= X"0B74F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- LD  	s7, 0x4F	; 10E : 0B74F 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"10F" => 
				inst <= X"20115" ; 
				dataB <= regB ; 
				-- CALL	0x115    	; 10F : 20115 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"110" => 
				inst <= X"2F44C" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s4, 0x4C	; 110 : 2F44C 
				scrW <= '1' ; 
			when X"111" => 
				inst <= X"2F54D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s5, 0x4D	; 111 : 2F54D 
				scrW <= '1' ; 
			when X"112" => 
				inst <= X"2F64E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s6, 0x4E	; 112 : 2F64E 
				scrW <= '1' ; 
			when X"113" => 
				inst <= X"2F74F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ST  	s7, 0x4F	; 113 : 2F74F 
				scrW <= '1' ; 
			when X"114" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 114 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"115" => 
				inst <= X"00940" ; 
				dataB <= regB ; 
				-- MOVE	s9, s4  	; 115 : 00940 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"116" => 
				inst <= X"06970" ; 
				dataB <= regB ; 
				-- XOR 	s9, s7  	; 116 : 06970 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"117" => 
				inst <= X"00A50" ; 
				dataB <= regB ; 
				-- MOVE	sA, s5  	; 117 : 00A50 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"118" => 
				inst <= X"06A60" ; 
				dataB <= regB ; 
				-- XOR 	sA, s6  	; 118 : 06A60 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"119" => 
				inst <= X"00B90" ; 
				dataB <= regB ; 
				-- MOVE	sB, s9  	; 119 : 00B90 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"11A" => 
				inst <= X"06BA0" ; 
				dataB <= regB ; 
				-- XOR 	sB, sA  	; 11A : 06BA0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"11B" => 
				inst <= X"00840" ; 
				dataB <= regB ; 
				-- MOVE	s8, s4  	; 11B : 00840 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"11C" => 
				inst <= X"06850" ; 
				dataB <= regB ; 
				-- XOR 	s8, s5  	; 11C : 06850 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"11D" => 
				inst <= X"14806" ; 
				dataB <= regB ; 
				-- SL0 	s8      	; 11D : 14806 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"11E" => 
				inst <= X"3E120" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x120	; 11E : 3E120 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"11F" => 
				inst <= X"0781B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	s8, 0x1B	; 11F : 0781B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"120" => 
				inst <= X"068B0" ; 
				dataB <= regB ; 
				-- XOR 	s8, sB  	; 120 : 068B0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"121" => 
				inst <= X"06480" ; 
				dataB <= regB ; 
				-- XOR 	s4, s8  	; 121 : 06480 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"122" => 
				inst <= X"008A0" ; 
				dataB <= regB ; 
				-- MOVE	s8, sA  	; 122 : 008A0 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"123" => 
				inst <= X"14806" ; 
				dataB <= regB ; 
				-- SL0 	s8      	; 123 : 14806 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"124" => 
				inst <= X"3E126" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x126	; 124 : 3E126 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"125" => 
				inst <= X"0781B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	s8, 0x1B	; 125 : 0781B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"126" => 
				inst <= X"068B0" ; 
				dataB <= regB ; 
				-- XOR 	s8, sB  	; 126 : 068B0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"127" => 
				inst <= X"06580" ; 
				dataB <= regB ; 
				-- XOR 	s5, s8  	; 127 : 06580 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"128" => 
				inst <= X"00860" ; 
				dataB <= regB ; 
				-- MOVE	s8, s6  	; 128 : 00860 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"129" => 
				inst <= X"06870" ; 
				dataB <= regB ; 
				-- XOR 	s8, s7  	; 129 : 06870 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"12A" => 
				inst <= X"14806" ; 
				dataB <= regB ; 
				-- SL0 	s8      	; 12A : 14806 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"12B" => 
				inst <= X"3E12D" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x12D	; 12B : 3E12D 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"12C" => 
				inst <= X"0781B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	s8, 0x1B	; 12C : 0781B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"12D" => 
				inst <= X"068B0" ; 
				dataB <= regB ; 
				-- XOR 	s8, sB  	; 12D : 068B0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"12E" => 
				inst <= X"06680" ; 
				dataB <= regB ; 
				-- XOR 	s6, s8  	; 12E : 06680 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"12F" => 
				inst <= X"00890" ; 
				dataB <= regB ; 
				-- MOVE	s8, s9  	; 12F : 00890 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"130" => 
				inst <= X"14806" ; 
				dataB <= regB ; 
				-- SL0 	s8      	; 130 : 14806 
				regI <= dataA( 6 downto 0 ) & '0' ; 
				nc <= dataA( 7 ) ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"131" => 
				inst <= X"3E133" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x133	; 131 : 3E133 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"132" => 
				inst <= X"0781B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- XOR 	s8, 0x1B	; 132 : 0781B 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"133" => 
				inst <= X"068B0" ; 
				dataB <= regB ; 
				-- XOR 	s8, sB  	; 133 : 068B0 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"134" => 
				inst <= X"06780" ; 
				dataB <= regB ; 
				-- XOR 	s7, s8  	; 134 : 06780 
				aluOP <= B"011" ; 
				regI <= alu ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				nc <= '0' ; 
				regW <= '1' ; 
			when X"135" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 135 : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"136" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 136 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"137" => 
				inst <= X"01550" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s5, 0x50	; 137 : 01550 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"138" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 138 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"139" => 
				inst <= X"19000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x00	; 139 : 19000 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"13A" => 
				inst <= X"1B1CA" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0xCA	; 13A : 1B1CA 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"13B" => 
				inst <= X"1B29A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x9A	; 13B : 1B29A 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"13C" => 
				inst <= X"1B33B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x3B	; 13C : 1B33B 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"13D" => 
				inst <= X"3E138" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x138	; 13D : 3E138 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"13E" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 13E : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"13F" => 
				inst <= X"11000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x00	; 13F : 11000 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"140" => 
				inst <= X"131CA" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0xCA	; 140 : 131CA 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"141" => 
				inst <= X"1329A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x9A	; 141 : 1329A 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"142" => 
				inst <= X"1333B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x3B	; 142 : 1333B 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"143" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 143 : 2E450 
				scrW <= '1' ; 
			when X"144" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 144 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"145" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 145 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"146" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 146 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"147" => 
				inst <= X"19000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x00	; 147 : 19000 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"148" => 
				inst <= X"1B1E1" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0xE1	; 148 : 1B1E1 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"149" => 
				inst <= X"1B2F5" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0xF5	; 149 : 1B2F5 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"14A" => 
				inst <= X"1B305" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x05	; 14A : 1B305 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"14B" => 
				inst <= X"3E146" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x146	; 14B : 3E146 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"14C" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 14C : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"14D" => 
				inst <= X"11000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x00	; 14D : 11000 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"14E" => 
				inst <= X"131E1" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0xE1	; 14E : 131E1 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"14F" => 
				inst <= X"132F5" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0xF5	; 14F : 132F5 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"150" => 
				inst <= X"13305" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x05	; 150 : 13305 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"151" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 151 : 2E450 
				scrW <= '1' ; 
			when X"152" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 152 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"153" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 153 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"154" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 154 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"155" => 
				inst <= X"19080" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x80	; 155 : 19080 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"156" => 
				inst <= X"1B196" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x96	; 156 : 1B196 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"157" => 
				inst <= X"1B298" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x98	; 157 : 1B298 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"158" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 158 : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"159" => 
				inst <= X"3E154" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x154	; 159 : 3E154 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"15A" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 15A : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"15B" => 
				inst <= X"11080" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x80	; 15B : 11080 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"15C" => 
				inst <= X"13196" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x96	; 15C : 13196 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"15D" => 
				inst <= X"13298" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x98	; 15D : 13298 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"15E" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 15E : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"15F" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 15F : 2E450 
				scrW <= '1' ; 
			when X"160" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 160 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"161" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 161 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"162" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 162 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"163" => 
				inst <= X"19040" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x40	; 163 : 19040 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"164" => 
				inst <= X"1B142" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x42	; 164 : 1B142 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"165" => 
				inst <= X"1B20F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x0F	; 165 : 1B20F 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"166" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 166 : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"167" => 
				inst <= X"3E162" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x162	; 167 : 3E162 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"168" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 168 : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"169" => 
				inst <= X"11040" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x40	; 169 : 11040 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"16A" => 
				inst <= X"13142" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x42	; 16A : 13142 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"16B" => 
				inst <= X"1320F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x0F	; 16B : 1320F 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"16C" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 16C : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"16D" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 16D : 2E450 
				scrW <= '1' ; 
			when X"16E" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 16E : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"16F" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 16F : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"170" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 170 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"171" => 
				inst <= X"190A0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0xA0	; 171 : 190A0 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"172" => 
				inst <= X"1B186" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x86	; 172 : 1B186 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"173" => 
				inst <= X"1B201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x01	; 173 : 1B201 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"174" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 174 : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"175" => 
				inst <= X"3E170" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x170	; 175 : 3E170 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"176" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 176 : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"177" => 
				inst <= X"110A0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0xA0	; 177 : 110A0 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"178" => 
				inst <= X"13186" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x86	; 178 : 13186 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"179" => 
				inst <= X"13201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x01	; 179 : 13201 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"17A" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 17A : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"17B" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 17B : 2E450 
				scrW <= '1' ; 
			when X"17C" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 17C : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"17D" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 17D : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"17E" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 17E : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"17F" => 
				inst <= X"19010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x10	; 17F : 19010 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"180" => 
				inst <= X"1B127" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x27	; 180 : 1B127 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"181" => 
				inst <= X"1B200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x00	; 181 : 1B200 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"182" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 182 : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"183" => 
				inst <= X"3E17E" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x17E	; 183 : 3E17E 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"184" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 184 : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"185" => 
				inst <= X"11010" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x10	; 185 : 11010 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"186" => 
				inst <= X"13127" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x27	; 186 : 13127 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"187" => 
				inst <= X"13200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x00	; 187 : 13200 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"188" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 188 : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"189" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 189 : 2E450 
				scrW <= '1' ; 
			when X"18A" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 18A : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"18B" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 18B : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"18C" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 18C : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"18D" => 
				inst <= X"190E8" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0xE8	; 18D : 190E8 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"18E" => 
				inst <= X"1B103" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x03	; 18E : 1B103 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"18F" => 
				inst <= X"1B200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x00	; 18F : 1B200 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"190" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 190 : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"191" => 
				inst <= X"3E18C" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x18C	; 191 : 3E18C 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"192" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 192 : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"193" => 
				inst <= X"110E8" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0xE8	; 193 : 110E8 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"194" => 
				inst <= X"13103" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x03	; 194 : 13103 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"195" => 
				inst <= X"13200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x00	; 195 : 13200 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"196" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 196 : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"197" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 197 : 2E450 
				scrW <= '1' ; 
			when X"198" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 198 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"199" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 199 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"19A" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 19A : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"19B" => 
				inst <= X"19064" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x64	; 19B : 19064 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"19C" => 
				inst <= X"1B100" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x00	; 19C : 1B100 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"19D" => 
				inst <= X"1B200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x00	; 19D : 1B200 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"19E" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 19E : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"19F" => 
				inst <= X"3E19A" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x19A	; 19F : 3E19A 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1A0" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 1A0 : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1A1" => 
				inst <= X"11064" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x64	; 1A1 : 11064 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1A2" => 
				inst <= X"13100" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x00	; 1A2 : 13100 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1A3" => 
				inst <= X"13200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x00	; 1A3 : 13200 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1A4" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 1A4 : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1A5" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 1A5 : 2E450 
				scrW <= '1' ; 
			when X"1A6" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 1A6 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1A7" => 
				inst <= X"01430" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s4, 0x30	; 1A7 : 01430 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1A8" => 
				inst <= X"11401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s4, 0x01	; 1A8 : 11401 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1A9" => 
				inst <= X"1900A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s0, 0x0A	; 1A9 : 1900A 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1AA" => 
				inst <= X"1B100" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s1, 0x00	; 1AA : 1B100 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"1AB" => 
				inst <= X"1B200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s2, 0x00	; 1AB : 1B200 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"1AC" => 
				inst <= X"1B300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUBC	s3, 0x00	; 1AC : 1B300 
				ci <= not c ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) and z ; 
				regW <= '1' ; 
			when X"1AD" => 
				inst <= X"3E1A8" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x1A8	; 1AD : 3E1A8 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1AE" => 
				inst <= X"19401" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s4, 0x01	; 1AE : 19401 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1AF" => 
				inst <= X"1100A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x0A	; 1AF : 1100A 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1B0" => 
				inst <= X"13100" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s1, 0x00	; 1B0 : 13100 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1B1" => 
				inst <= X"13200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s2, 0x00	; 1B1 : 13200 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1B2" => 
				inst <= X"13300" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADDC	s3, 0x00	; 1B2 : 13300 
				ci <= c ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) and z  ; 
				regW <= '1' ; 
			when X"1B3" => 
				inst <= X"2E450" ; 
				dataB <= regB ; 
				-- ST  	s4, s5  	; 1B3 : 2E450 
				scrW <= '1' ; 
			when X"1B4" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 1B4 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1B5" => 
				inst <= X"11030" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s0, 0x30	; 1B5 : 11030 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1B6" => 
				inst <= X"2E050" ; 
				dataB <= regB ; 
				-- ST  	s0, s5  	; 1B6 : 2E050 
				scrW <= '1' ; 
			when X"1B7" => 
				inst <= X"11501" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s5, 0x01	; 1B7 : 11501 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1B8" => 
				inst <= X"01000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x00	; 1B8 : 01000 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1B9" => 
				inst <= X"2E050" ; 
				dataB <= regB ; 
				-- ST  	s0, s5  	; 1B9 : 2E050 
				scrW <= '1' ; 
			when X"1BA" => 
				inst <= X"25000" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- RET 	         	; 1BA : 25000 
				npc <= unsigned( addrR ) ; 
				nspW <= spR ; 
				nspR <= spR - 1 ; 
			when X"1BB" => 
				inst <= X"01150" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x50	; 1BB : 01150 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1BC" => 
				inst <= X"22002" ; 
				dataB <= regB ; 
				-- JUMP	0x002    	; 1BC : 22002 
				npc <= addrN ; 
			when X"1BD" => 
				inst <= X"01150" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x50	; 1BD : 01150 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1BE" => 
				inst <= X"0A010" ; 
				dataB <= regB ; 
				-- LD  	s0, s1  	; 1BE : 0A010 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"1BF" => 
				inst <= X"1D030" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x30	; 1BF : 1D030 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1C0" => 
				inst <= X"36002" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x002	; 1C0 : 36002 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1C1" => 
				inst <= X"1D159" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s1, 0x59	; 1C1 : 1D159 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1C2" => 
				inst <= X"32002" ; 
				dataB <= regB ; 
				-- JUMP	Z, 0x002 	; 1C2 : 32002 
				if z = '1' then 
					npc <= addrN ; 
				end if ; 
			when X"1C3" => 
				inst <= X"11101" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s1, 0x01	; 1C3 : 11101 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1C4" => 
				inst <= X"221BE" ; 
				dataB <= regB ; 
				-- JUMP	0x1BE    	; 1C4 : 221BE 
				npc <= addrN ; 
			when X"1C5" => 
				inst <= X"01194" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x94	; 1C5 : 01194 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1C6" => 
				inst <= X"20002" ; 
				dataB <= regB ; 
				-- CALL	0x002    	; 1C6 : 20002 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1C7" => 
				inst <= X"2003A" ; 
				dataB <= regB ; 
				-- CALL	0x03A    	; 1C7 : 2003A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1C8" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 1C8 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1C9" => 
				inst <= X"1D03F" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x3F	; 1C9 : 1D03F 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1CA" => 
				inst <= X"361CE" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1CE	; 1CA : 361CE 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1CB" => 
				inst <= X"0115B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0x5B	; 1CB : 0115B 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1CC" => 
				inst <= X"20002" ; 
				dataB <= regB ; 
				-- CALL	0x002    	; 1CC : 20002 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1CD" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 1CD : 221C5 
				npc <= addrN ; 
			when X"1CE" => 
				inst <= X"1D064" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x64	; 1CE : 1D064 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1CF" => 
				inst <= X"361F0" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1F0	; 1CF : 361F0 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1D0" => 
				inst <= X"2D0C0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s0, 0xC0	; 1D0 : 2D0C0 
				nioW <= '1' ; 
			when X"1D1" => 
				inst <= X"01200" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s2, 0x00	; 1D1 : 01200 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1D2" => 
				inst <= X"01C10" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	sC, 0x10	; 1D2 : 01C10 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1D3" => 
				inst <= X"20017" ; 
				dataB <= regB ; 
				-- CALL	0x017    	; 1D3 : 20017 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1D4" => 
				inst <= X"00020" ; 
				dataB <= regB ; 
				-- MOVE	s0, s2  	; 1D4 : 00020 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1D5" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 1D5 : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1D6" => 
				inst <= X"0103A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x3A	; 1D6 : 0103A 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1D7" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 1D7 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1D8" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1D8 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1D9" => 
				inst <= X"01D10" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	sD, 0x10	; 1D9 : 01D10 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1DA" => 
				inst <= X"0A020" ; 
				dataB <= regB ; 
				-- LD  	s0, s2  	; 1DA : 0A020 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"1DB" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 1DB : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1DC" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 1DC : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1DD" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1DD : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1DE" => 
				inst <= X"19D01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sD, 0x01	; 1DE : 19D01 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1DF" => 
				inst <= X"361DA" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1DA	; 1DF : 361DA 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1E0" => 
				inst <= X"19210" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s2, 0x10	; 1E0 : 19210 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1E1" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1E1 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1E2" => 
				inst <= X"01D10" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	sD, 0x10	; 1E2 : 01D10 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1E3" => 
				inst <= X"0A020" ; 
				dataB <= regB ; 
				-- LD  	s0, s2  	; 1E3 : 0A020 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"1E4" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 1E4 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1E5" => 
				inst <= X"1D020" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x20	; 1E5 : 1D020 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1E6" => 
				inst <= X"3E1E8" ; 
				dataB <= regB ; 
				-- JUMP	NC, 0x1E8	; 1E6 : 3E1E8 
				if c = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1E7" => 
				inst <= X"0102E" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x2E	; 1E7 : 0102E 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1E8" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 1E8 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1E9" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1E9 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1EA" => 
				inst <= X"19D01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sD, 0x01	; 1EA : 19D01 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1EB" => 
				inst <= X"361E3" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1E3	; 1EB : 361E3 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1EC" => 
				inst <= X"19C01" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	sC, 0x01	; 1EC : 19C01 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"1ED" => 
				inst <= X"361D3" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1D3	; 1ED : 361D3 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1EE" => 
				inst <= X"2D0C0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s0, 0xC0	; 1EE : 2D0C0 
				nioW <= '1' ; 
			when X"1EF" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 1EF : 221C5 
				npc <= addrN ; 
			when X"1F0" => 
				inst <= X"1D073" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x73	; 1F0 : 1D073 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"1F1" => 
				inst <= X"36210" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x210	; 1F1 : 36210 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"1F2" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1F2 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1F3" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 1F3 : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1F4" => 
				inst <= X"00200" ; 
				dataB <= regB ; 
				-- MOVE	s2, s0  	; 1F4 : 00200 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1F5" => 
				inst <= X"20017" ; 
				dataB <= regB ; 
				-- CALL	0x017    	; 1F5 : 20017 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1F6" => 
				inst <= X"00020" ; 
				dataB <= regB ; 
				-- MOVE	s0, s2  	; 1F6 : 00020 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1F7" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 1F7 : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1F8" => 
				inst <= X"0103A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x3A	; 1F8 : 0103A 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"1F9" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 1F9 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1FA" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1FA : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1FB" => 
				inst <= X"0A020" ; 
				dataB <= regB ; 
				-- LD  	s0, s2  	; 1FB : 0A020 
				regI <= scrO ; 
				regW <= '1' ; 
			when X"1FC" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 1FC : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1FD" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 1FD : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1FE" => 
				inst <= X"2003A" ; 
				dataB <= regB ; 
				-- CALL	0x03A    	; 1FE : 2003A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"1FF" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 1FF : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"200" => 
				inst <= X"1D02B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x2B	; 200 : 1D02B 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"201" => 
				inst <= X"36204" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x204	; 201 : 36204 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"202" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 202 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"203" => 
				inst <= X"221F5" ; 
				dataB <= regB ; 
				-- JUMP	0x1F5    	; 203 : 221F5 
				npc <= addrN ; 
			when X"204" => 
				inst <= X"1D02D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x2D	; 204 : 1D02D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"205" => 
				inst <= X"36208" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x208	; 205 : 36208 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"206" => 
				inst <= X"19201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s2, 0x01	; 206 : 19201 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"207" => 
				inst <= X"221F5" ; 
				dataB <= regB ; 
				-- JUMP	0x1F5    	; 207 : 221F5 
				npc <= addrN ; 
			when X"208" => 
				inst <= X"1D03D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x3D	; 208 : 1D03D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"209" => 
				inst <= X"3620D" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x20D	; 209 : 3620D 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"20A" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 20A : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"20B" => 
				inst <= X"2E020" ; 
				dataB <= regB ; 
				-- ST  	s0, s2  	; 20B : 2E020 
				scrW <= '1' ; 
			when X"20C" => 
				inst <= X"221F5" ; 
				dataB <= regB ; 
				-- JUMP	0x1F5    	; 20C : 221F5 
				npc <= addrN ; 
			when X"20D" => 
				inst <= X"1D00D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x0D	; 20D : 1D00D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"20E" => 
				inst <= X"361F5" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x1F5	; 20E : 361F5 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"20F" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 20F : 221C5 
				npc <= addrN ; 
			when X"210" => 
				inst <= X"1D070" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x70	; 210 : 1D070 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"211" => 
				inst <= X"36230" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x230	; 211 : 36230 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"212" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 212 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"213" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 213 : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"214" => 
				inst <= X"00200" ; 
				dataB <= regB ; 
				-- MOVE	s2, s0  	; 214 : 00200 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"215" => 
				inst <= X"20017" ; 
				dataB <= regB ; 
				-- CALL	0x017    	; 215 : 20017 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"216" => 
				inst <= X"00020" ; 
				dataB <= regB ; 
				-- MOVE	s0, s2  	; 216 : 00020 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"217" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 217 : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"218" => 
				inst <= X"0103A" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s0, 0x3A	; 218 : 0103A 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"219" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 219 : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"21A" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 21A : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"21B" => 
				inst <= X"08020" ; 
				dataB <= regB ; 
				-- IN  	s0, s2  	; 21B : 08020 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"21C" => 
				inst <= X"20008" ; 
				dataB <= regB ; 
				-- CALL	0x008    	; 21C : 20008 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"21D" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 21D : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"21E" => 
				inst <= X"2003A" ; 
				dataB <= regB ; 
				-- CALL	0x03A    	; 21E : 2003A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"21F" => 
				inst <= X"2001A" ; 
				dataB <= regB ; 
				-- CALL	0x01A    	; 21F : 2001A 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"220" => 
				inst <= X"1D02B" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x2B	; 220 : 1D02B 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"221" => 
				inst <= X"36224" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x224	; 221 : 36224 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"222" => 
				inst <= X"11201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- ADD 	s2, 0x01	; 222 : 11201 
				ci <= '0' ; 
				aluOP <= B"101" ; 
				regI <= alu ; 
				nc <= co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"223" => 
				inst <= X"22215" ; 
				dataB <= regB ; 
				-- JUMP	0x215    	; 223 : 22215 
				npc <= addrN ; 
			when X"224" => 
				inst <= X"1D02D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x2D	; 224 : 1D02D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"225" => 
				inst <= X"36228" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x228	; 225 : 36228 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"226" => 
				inst <= X"19201" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- SUB 	s2, 0x01	; 226 : 19201 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
				regW <= '1' ; 
			when X"227" => 
				inst <= X"22215" ; 
				dataB <= regB ; 
				-- JUMP	0x215    	; 227 : 22215 
				npc <= addrN ; 
			when X"228" => 
				inst <= X"1D03D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x3D	; 228 : 1D03D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"229" => 
				inst <= X"3622D" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x22D	; 229 : 3622D 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"22A" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 22A : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"22B" => 
				inst <= X"2C020" ; 
				dataB <= regB ; 
				-- OUT 	s0, s2  	; 22B : 2C020 
				nioW <= '1' ; 
			when X"22C" => 
				inst <= X"22215" ; 
				dataB <= regB ; 
				-- JUMP	0x215    	; 22C : 22215 
				npc <= addrN ; 
			when X"22D" => 
				inst <= X"1D00D" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x0D	; 22D : 1D00D 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"22E" => 
				inst <= X"36215" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x215	; 22E : 36215 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"22F" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 22F : 221C5 
				npc <= addrN ; 
			when X"230" => 
				inst <= X"1D067" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x67	; 230 : 1D067 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"231" => 
				inst <= X"36237" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x237	; 231 : 36237 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"232" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 232 : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"233" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 233 : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"234" => 
				inst <= X"00400" ; 
				dataB <= regB ; 
				-- MOVE	s4, s0  	; 234 : 00400 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"235" => 
				inst <= X"2002E" ; 
				dataB <= regB ; 
				-- CALL	0x02E    	; 235 : 2002E 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"236" => 
				inst <= X"26400" ; 
				dataB <= regB ; 
				-- JUMP	s4, s0  	; 236 : 26400 
				npc( 11 downto 8 ) <= unsigned( dataA( 3 downto 0 ) ) ; 
				npc( 7 downto 0 ) <= unsigned( regB ) ; 
			when X"237" => 
				inst <= X"1D072" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x72	; 237 : 1D072 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"238" => 
				inst <= X"3623D" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x23D	; 238 : 3623D 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"239" => 
				inst <= X"2D0C0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s0, 0xC0	; 239 : 2D0C0 
				nioW <= '1' ; 
			when X"23A" => 
				inst <= X"2003F" ; 
				dataB <= regB ; 
				-- CALL	0x03F    	; 23A : 2003F 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"23B" => 
				inst <= X"2D0C0" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- OUT 	s0, 0xC0	; 23B : 2D0C0 
				nioW <= '1' ; 
			when X"23C" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 23C : 221C5 
				npc <= addrN ; 
			when X"23D" => 
				inst <= X"1D063" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- COMP	s0, 0x63	; 23D : 1D063 
				ci <= '1' ; 
				aluOP <= B"100" ; 
				regI <= alu ; 
				nc <= not co ; 
				nz <= to_std_logic( regI = X"00" ) ; 
			when X"23E" => 
				inst <= X"36249" ; 
				dataB <= regB ; 
				-- JUMP	NZ, 0x249	; 23E : 36249 
				if z = '0' then 
					npc <= addrN ; 
				end if ; 
			when X"23F" => 
				inst <= X"20015" ; 
				dataB <= regB ; 
				-- CALL	0x015    	; 23F : 20015 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"240" => 
				inst <= X"090C8" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s0, 0xC8	; 240 : 090C8 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"241" => 
				inst <= X"091C9" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s1, 0xC9	; 241 : 091C9 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"242" => 
				inst <= X"092CA" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s2, 0xCA	; 242 : 092CA 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"243" => 
				inst <= X"093CB" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- IN  	s3, 0xCB	; 243 : 093CB 
				nioR <= '1' ; 
				regI <= PB2O.da ; 
				regW <= ioR ; 
			when X"244" => 
				inst <= X"20136" ; 
				dataB <= regB ; 
				-- CALL	0x136    	; 244 : 20136 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"245" => 
				inst <= X"201BD" ; 
				dataB <= regB ; 
				-- CALL	0x1BD    	; 245 : 201BD 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"246" => 
				inst <= X"011A2" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0xA2	; 246 : 011A2 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"247" => 
				inst <= X"20002" ; 
				dataB <= regB ; 
				-- CALL	0x002    	; 247 : 20002 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"248" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 248 : 221C5 
				npc <= addrN ; 
			when X"249" => 
				inst <= X"011AA" ; 
				dataB <= std_logic_vector( dataK ) ; 
				-- MOVE	s1, 0xAA	; 249 : 011AA 
				aluOP <= B"000" ; 
				regI <= alu ; 
				regW <= '1' ; 
			when X"24A" => 
				inst <= X"20002" ; 
				dataB <= regB ; 
				-- CALL	0x002    	; 24A : 20002 
				npc <= addrN ; 
				nspW <= spW + 1 ; 
				nspR <= spW ; 
				stackW <= '1' ; 
			when X"24B" => 
				inst <= X"221C5" ; 
				dataB <= regB ; 
				-- JUMP	0x1C5    	; 24B : 221C5 
				npc <= addrN ; 
			when X"3FF" => 
				inst <= X"22001" ; 
				dataB <= regB ; 
				-- JUMP	0x001    	; 3FF : 22001 
				npc <= addrN ; 
			when others =>
				inst <= ( others => '0' ) ; 
				dataB <= regB ; 
			end case ; 

	end process ; 
end mix ; 
