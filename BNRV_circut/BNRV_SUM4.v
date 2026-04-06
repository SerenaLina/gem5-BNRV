module BNRV_SUM4 (
    input signed [7:0] x1, x2, x3, x4,
    input [1:0] w1, w2, w3, w4,
    output signed [31:0] o
    );


    wire signed [9:0] mo1, mo2, mo3, mo4;
    wire signed [11:0] res_temp;


    BNRV_4Mux Mux1 (.x(x1), .w(w1), .o(mo1));
    BNRV_4Mux Mux2 (.x(x2), .w(w2), .o(mo2));
    BNRV_4Mux Mux3 (.x(x3), .w(w3), .o(mo3));
    BNRV_4Mux Mux4 (.x(x4), .w(w4), .o(mo4));

    assign res_temp = ( { {2{mo1[9]}}, mo1 } + { {2{mo2[9]}}, mo2 } ) +
                      ( { {2{mo3[9]}}, mo3 } + { {2{mo4[9]}}, mo4 } );
    assign o = { {20{res_temp[11]}}, res_temp };
endmodule
