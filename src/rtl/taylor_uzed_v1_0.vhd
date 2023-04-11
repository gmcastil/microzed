library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity taylor_uzed_v1_0 is
	generic (
		-- Users to add parameters here
		-- User parameters ends

		-- Do not modify the parameters beyond this line
		-- Parameters of Axi Slave Bus Interface S00_AXI
		C_S00_AXI_DATA_WIDTH	: integer	:= 32;
		C_S00_AXI_ADDR_WIDTH	: integer	:= 4
	);
	port (
		-- Users to add ports here
		interrupt     : out std_logic;
		-- User ports ends

		-- Do not modify the ports beyond this line
		-- Ports of Axi Slave Bus Interface S00_AXI
		s00_axi_aclk	  : in  std_logic;
		s00_axi_aresetn	: in  std_logic;
		s00_axi_awaddr	: in  std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
		s00_axi_awprot	: in  std_logic_vector(2 downto 0);
		s00_axi_awvalid	: in  std_logic;
		s00_axi_awready	: out std_logic;
		s00_axi_wdata	  : in  std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
		s00_axi_wstrb	  : in  std_logic_vector((C_S00_AXI_DATA_WIDTH/8)-1 downto 0);
		s00_axi_wvalid	: in  std_logic;
		s00_axi_wready	: out std_logic;
		s00_axi_bresp	  : out std_logic_vector(1 downto 0);
		s00_axi_bvalid	: out std_logic;
		s00_axi_bready	: in  std_logic;
		s00_axi_araddr	: in  std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
		s00_axi_arprot	: in  std_logic_vector(2 downto 0);
		s00_axi_arvalid	: in  std_logic;
		s00_axi_arready	: out std_logic;
		s00_axi_rdata	  : out std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
		s00_axi_rresp	  : out std_logic_vector(1 downto 0);
		s00_axi_rvalid	: out std_logic;
		s00_axi_rready	: in  std_logic
	);
end taylor_uzed_v1_0;

architecture arch_imp of taylor_uzed_v1_0 is

	-- component declaration
	component taylor_uzed_v1_0_S00_AXI is
		generic (
      C_S_AXI_DATA_WIDTH	: integer	:= 32;
      C_S_AXI_ADDR_WIDTH	: integer	:= 4
		);
		port (
      -- User interface
      reg0          : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
      reg1          : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
      reg2          : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
      reg3          : in  std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);

      -- AXI interface
      S_AXI_ACLK	  : in  std_logic;
      S_AXI_ARESETN	: in  std_logic;
      S_AXI_AWADDR	: in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
      S_AXI_AWPROT	: in  std_logic_vector(2 downto 0);
      S_AXI_AWVALID	: in  std_logic;
      S_AXI_AWREADY	: out std_logic;
      S_AXI_WDATA	  : in  std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
      S_AXI_WSTRB	  : in  std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
      S_AXI_WVALID	: in  std_logic;
      S_AXI_WREADY	: out std_logic;
      S_AXI_BRESP	  : out std_logic_vector(1 downto 0);
      S_AXI_BVALID	: out std_logic;
      S_AXI_BREADY	: in  std_logic;
      S_AXI_ARADDR	: in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
      S_AXI_ARPROT	: in  std_logic_vector(2 downto 0);
      S_AXI_ARVALID	: in  std_logic;
      S_AXI_ARREADY	: out std_logic;
      S_AXI_RDATA	  : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
      S_AXI_RRESP	  : out std_logic_vector(1 downto 0);
      S_AXI_RVALID	: out std_logic;
      S_AXI_RREADY	: in  std_logic
		);
	end component taylor_uzed_v1_0_S00_AXI;

	constant ADD        : std_logic_vector(1 downto 0) := "01";
	constant SUB        : std_logic_vector(1 downto 0) := "10";
	constant MULTIPLY   : std_logic_vector(1 downto 0) := "11";

  signal result       : signed(31 downto 0);

  signal reg0         : std_logic_vector(31 downto 0);
  signal reg1         : std_logic_vector(31 downto 0);
  signal reg2         : std_logic_vector(31 downto 0);
  signal reg3         : std_logic_vector(31 downto 0);

  constant  C         : signed(16 downto 0) := to_signed(-577, 17);
  constant  B         : signed(16 downto 0) := to_signed(57910, 17);
  constant  A         : signed(16 downto 0) := to_signed(33610, 17);

  signal int          : signed(16 downto 0);
  signal squared      : signed(33 downto 0);
  signal p2           : signed(50 downto 0);
  signal p1           : signed(33 downto 0);
  signal p0           : signed(16 downto 0);

begin 

  -- 0th order term in the quadratic (always constant)
  p0        <= A;
  reg3      <= std_logic_vector(result);
  interrupt <= '0';

  process (s00_axi_aclk) begin
    if rising_edge(s00_axi_aclk) then
      squared <= signed( '0' & reg1(15 downto 0)) * signed( '0' & reg1(15 downto 0));
      -- 2nd order term in the quadratic
      p2      <= squared * C;
      -- 1st order term in the quadratic
      p1      <= signed('0' & reg1(15 downto 0) * B;
      -- Going to incur one cycle of latency beyond whatever the adder and
      -- multiplier bring
      int     <= p2(48 downto 32) + ("000" & p1(32 downto 19)) + p0;
      -- On the next cycle, we get the result
      result(15 downto 0) <= int(15 downto 0);
    else
      null;
    end if;
  end process;

--	process (s00_axi_aclk) begin
--    if rising_edge(s00_axi_aclk) then
--      -- Addition
--      if reg0(1 downto 0) = ADD then
--        result(31 downto 16) <= (others=>'0');
--        result(15 downto 0)  <= signed(reg1(15 downto 0)) + signed(reg2(15 downto 0));
--      -- Subtraction
--      elsif reg0(1 downto 0) = SUB then
--        result(31 downto 16) <= (others=>'0');
--        result(15 downto 0)  <= signed(reg1(15 downto 0)) - signed(reg2(15 downto 0));
--      -- Multiplication
--      elsif reg0(1 downto 0) = MULTIPLY then
--        result  <= signed(reg1(15 downto 0)) * signed(reg2(15 downto 0));
--      -- Nothing
--      else
--        null;
--      end if;
--    end if;
--  end process;

  -- Instantiation of Axi Bus Interface S00_AXI
  taylor_uzed_v1_0_S00_AXI_inst : taylor_uzed_v1_0_S00_AXI
    generic map (
      C_S_AXI_DATA_WIDTH	=> C_S00_AXI_DATA_WIDTH,
      C_S_AXI_ADDR_WIDTH	=> C_S00_AXI_ADDR_WIDTH
    )
    port map (
      reg0          => reg0,
      reg1          => reg1,
      reg2          => reg2,
      reg3          => reg3,
      S_AXI_ACLK	  => s00_axi_aclk,
      S_AXI_ARESETN	=> s00_axi_aresetn,
      S_AXI_AWADDR	=> s00_axi_awaddr,
      S_AXI_AWPROT	=> s00_axi_awprot,
      S_AXI_AWVALID	=> s00_axi_awvalid,
      S_AXI_AWREADY	=> s00_axi_awready,
      S_AXI_WDATA	  => s00_axi_wdata,
      S_AXI_WSTRB	  => s00_axi_wstrb,
      S_AXI_WVALID	=> s00_axi_wvalid,
      S_AXI_WREADY	=> s00_axi_wready,
      S_AXI_BRESP	  => s00_axi_bresp,
      S_AXI_BVALID	=> s00_axi_bvalid,
      S_AXI_BREADY	=> s00_axi_bready,
      S_AXI_ARADDR	=> s00_axi_araddr,
      S_AXI_ARPROT	=> s00_axi_arprot,
      S_AXI_ARVALID	=> s00_axi_arvalid,
      S_AXI_ARREADY	=> s00_axi_arready,
      S_AXI_RDATA	  => s00_axi_rdata,
      S_AXI_RRESP	  => s00_axi_rresp,
      S_AXI_RVALID	=> s00_axi_rvalid,
      S_AXI_RREADY	=> s00_axi_rready
    );

end arch_imp;
