module BNRV_4Mux(
    input signed [7:0] x,
    input [1:0] w,
    output signed [9:0] o
);
    reg signed [9:0] t_o;

    wire signed [9:0] x_ext = { {2{x[7]}}, x };

    always @(*) begin
        case(w)
            2'b00: t_o = 10'sd0;
            2'b01: t_o = x_ext;
            2'b10: t_o = -x_ext;
            2'b11: t_o = -(x_ext << 1);
            default: t_o = 10'sd0;
        endcase
    end
    assign o = t_o;
endmodule
