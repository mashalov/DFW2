syms Pt Kdemp s Vre Vim Ire Iim Mj sv real
sp1 = 1.0 + s;
sp2 = 1.0 + sv;
es = (Pt / sp1 - Kdemp  * s - (Vre * Ire + Vim * Iim) / sp2) / Mj;

d_Es_Vre = diff(es, Vre)
d_Es_Vim = diff(es, Vim)
d_Es_Ire = diff(es, Ire)
d_Es_Iim= diff(es, Iim)
d_Es_s = diff(es, s)
d_Es_sv = diff(es, sv)

syms Delta
DR = [ cos(Delta), - sin(Delta); sin(Delta), cos(Delta)]
RD = [cos(Delta), sin(Delta) ; -sin(Delta), cos(Delta)]

simplify(DR*RD)