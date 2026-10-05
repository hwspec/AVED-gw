
module wrapper (
  input  wire        s_axi_aclk,
  input  wire        s_axi_aresetn,

  input  wire [31:0] S_AXI_awaddr,
  input  wire        S_AXI_awvalid,
  output wire        S_AXI_awready,
  input  wire [31:0] S_AXI_wdata,
  input  wire [ 3:0] S_AXI_wstrb,
  input  wire        S_AXI_wvalid,
  output wire        S_AXI_wready,
  output wire [ 1:0] S_AXI_bresp,
  output wire        S_AXI_bvalid,
  input  wire        S_AXI_bready,
  input  wire [31:0] S_AXI_araddr,
  input  wire        S_AXI_arvalid,
  output wire        S_AXI_arready,
  output wire [31:0] S_AXI_rdata,
  output wire [ 1:0] S_AXI_rresp,
  output wire        S_AXI_rvalid,
  input  wire        S_AXI_rready
);

  wire s_axi_reset = ~s_axi_aresetn;

  Axi4Lite32Cmd u_core (
    .clock           (s_axi_aclk),
    .reset           (s_axi_reset),
    .S_AXI_awaddr    (S_AXI_awaddr),
    .S_AXI_awvalid   (S_AXI_awvalid),
    .S_AXI_awready   (S_AXI_awready),
    .S_AXI_wdata     (S_AXI_wdata),
    .S_AXI_wstrb     (S_AXI_wstrb),
    .S_AXI_wvalid    (S_AXI_wvalid),
    .S_AXI_wready    (S_AXI_wready),
    .S_AXI_bresp     (S_AXI_bresp),
    .S_AXI_bvalid    (S_AXI_bvalid),
    .S_AXI_bready    (S_AXI_bready),
    .S_AXI_araddr    (S_AXI_araddr),
    .S_AXI_arvalid   (S_AXI_arvalid),
    .S_AXI_arready   (S_AXI_arready),
    .S_AXI_rdata     (S_AXI_rdata),
    .S_AXI_rresp     (S_AXI_rresp),
    .S_AXI_rvalid    (S_AXI_rvalid),
    .S_AXI_rready    (S_AXI_rready)
  );
endmodule
