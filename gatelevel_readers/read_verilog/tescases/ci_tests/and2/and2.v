/* Generated by Yosys 0.17+76 (git sha1 035496b50, gcc 9.1.0 -fPIC -Os) */

(* top =  1  *)
(* src = "./rtl/and2/and2.v:7.1-18.10" *)
module and2(a, b, c);
  (* src = "./rtl/and2/and2.v:12.12-12.13" *)
  input a;
  wire a;
  (* src = "./rtl/and2/and2.v:13.12-13.13" *)
  input b;
  wire b;
  (* src = "./rtl/and2/and2.v:14.13-14.14" *)
  output c;
  wire c;
  \$lut  \$abc$330$auto$blifparse.cc:515:parse_blif$331  (
    .A({ b, a, 1'h0 }),
    .Y(c)
  );
  defparam \$abc$330$auto$blifparse.cc:515:parse_blif$331 .LUT = 8'hca;
  defparam \$abc$330$auto$blifparse.cc:515:parse_blif$331 .WIDTH = 32'd3;
endmodule
