module top;

    import "DPI-C" function void init_python_env();
    import "DPI-C" function void deinit_python_env();
    
    // Due to the definition from SV-DPI docs, the appended square bracket pair is required,
    // in despite of the zero dimension data, e.g. `bit[N:0] val` should be `bit[N:0] val[]`,
    // or the compiler will parse it as `svBitVecVal` instead of `svOpenArrayHandle`.

    import "DPI-C" function void array_handle1(
        string filename, string funcname, 
        output bit[15:0] array_out[5:0][1:0][],
        input bit[23:0] array_in[5:0][1:0][]);

    import "DPI-C" function void array_handle2(
        string filename, string funcname, 
        output bit[15:0] array_out[5:0][1:0][],
        input bit[23:0] array_in1[5:0][1:0][],
        input bit[11:0] array_in2[7:0][]);

    initial begin
        bit [15:0] ad_value [5:0][1:0];
        bit [23:0] da_value1 [5:0][1:0];
        bit [11:0] da_value2 [7:0];

        foreach(da_value1[i, j])
        begin
            da_value1[i][j] = 24'(i) + 24'(j << 8);
        end

        foreach(da_value2[i])
        begin
            da_value2[i] = 12'(i);
        end

        init_python_env();

        array_handle1("test", "ops_single", ad_value, da_value1);

        foreach(ad_value[i, j])
        begin
            $display("[SV] ad_value[%d][%d] = %d", i, j, ad_value[i][j]);
        end

        deinit_python_env();
        $finish;
    end

endmodule
