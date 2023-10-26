module puc_sim_adapter
  #(
    parameter DWIDTH = 16,
    parameter ILEN1_row = 32,
    parameter ILEN1_col = 32,
    parameter ILEN2_row = 32,
    parameter ILEN2_col = 32,
    parameter OLEN_row = 32, 
    parameter OLEN_col = 32, 
    parameter PY_SRC_NAME = "payload",
    parameter PY_FUNC_NAME = "callback"
    )
    (
        input clock,
        input reset,
        input ena, 
        input [31:0] in1_cols, input [31:0] in1_rows, input [31:0] in2_cols, input [31:0] in2_rows, input [31:0] out_cols, input [31:0] out_rows, 
        input  [(ILEN1_row * ILEN1_col * DWIDTH)-1:0] ivalue_1,
        input  [(ILEN2_row * ILEN2_col * DWIDTH)-1:0] ivalue_2,
        output [(OLEN_row * OLEN_col * DWIDTH)-1:0] ovalue 
      );

        import "DPI-C" function void init_python_env();
        import "DPI-C" function void deinit_python_env();
        import "DPI-C" function void array_handle2(
            string filename, string funcname, 
            output bit[DWIDTH-1:0] array_out[OLEN_row-1:0][OLEN_col-1:0][],
            input bit[DWIDTH-1:0] array_in1[ILEN1_row-1:0][ILEN1_col-1:0][],
            input bit[DWIDTH-1:0] array_in2[ILEN2_row-1:0][ILEN2_col-1:0][],
            input bit [31:0] in1_cols, input bit [31:0] in1_rows, 
            input bit [31:0] in2_cols, input bit [31:0] in2_rows, 
            input bit [31:0] out_cols, input bit [31:0] out_rows
          );
            bit [DWIDTH-1:0] dpi_iarg_1 [ILEN1_row-1:0][ILEN1_col-1:0];
            bit [DWIDTH-1:0] dpi_iarg_2 [ILEN2_row-1:0][ILEN2_col-1:0];
            bit [DWIDTH-1:0] dpi_oarg [OLEN_row-1:0][OLEN_col-1:0];

            initial begin
                init_python_env();
            end

            genvar i, j;
            generate 
            for (i = 0; i < ILEN1_row; i = i + 1) begin
              for (j = 0; j < ILEN1_col; j = j + 1) begin
                assign dpi_iarg_1[i][j] = ivalue_1[(i * ILEN1_col + j + 1 )*DWIDTH-1: (i * ILEN1_col + j)*DWIDTH];
              end
            end
            endgenerate

            generate 
            for (i = 0; i < ILEN2_row; i = i + 1) begin
              for (j = 0; j < ILEN2_col; j = j + 1) begin
                assign dpi_iarg_2[i][j] = ivalue_2[(i * ILEN2_col + j + 1 )*DWIDTH-1: (i * ILEN2_col + j)*DWIDTH];
              end
            end
            endgenerate

            generate 
            for (i = 0; i < OLEN_row; i = i + 1) begin
              for (j = 0; j < OLEN_col; j = j + 1) begin
                assign  ovalue[(i * ILEN2_col + j + 1 )*DWIDTH-1: (i * ILEN2_col + j)*DWIDTH] = dpi_oarg[i][j];
              end
            end
            endgenerate

            always @(posedge clock) begin
              if (ena) begin
                array_handle2(PY_SRC_NAME, PY_FUNC_NAME, dpi_oarg, dpi_iarg_1, dpi_iarg_2, 
                  in1_cols, in1_rows, in2_cols, in2_rows, out_cols, out_rows);
              end
            end

            endmodule
