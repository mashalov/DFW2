Lad = .3;
L1d = 2;
Lfd = 3;
Lrc = 0.5;
Ll =  0.1;
id = -76.858735143957801;
i1d = 3;
ifd = 2.78;
iq = 42.480447772255637;
i1q = 2.5;
i2q = 3.1;
Laq = .74;
L1q = 2.1;
L2q = 2.3;
omega = 3.14;
Ra = .32;
Rfd = 0.080040073172612830;
R1d = .41;
R1q = .11;
R2q = .23;
% ???????? ????????? ???????????????
Psid = (Lad+Ll) * id + Lad * ifd + Lad * i1d;
Psifd = Lad * id  + (Lad + Lrc + Lfd) * ifd + (Lad + Lrc) * i1d;
Psi1d = Lad * id + (Lad + Lrc) * ifd + (Lad + Lrc + L1d) * i1d;
Psiq = (Laq + Ll)* iq + Laq * i1q + Laq * i2q;
Psi1q = Laq * iq + (Laq + L1q) * i1q + Laq * i2q;
Psi2q = Laq * iq + Laq * i1q + (Laq + L2q) * i2q;
% ????????? ???
ed = -Ra * id - omega * Psiq;
eq = -Ra * iq + omega * Psid;
% ??????????? ? ????????? ??? ??????????????? ?? d ? q
ed1 = -Ra * id - omega*(Laq + Ll) * iq - omega * Laq * i1q - omega * Laq * i2q;
eq1 = -Ra * iq + omega*(Lad + Ll) * id + omega * Lad * ifd + omega * Lad * i1d;  
% ?????????
deltaed1 = ed - ed1
deltaeq1 = eq - eq1

% ???????? ifd ? i1d ????? Psifd, Psi1d ? id
A = Lad + Lrc + Lfd;
B = Lad + Lrc + L1d;
C = Lad + Lrc;
detd = C^2 - A*B;
ifd2 = -B/detd * Psifd + C/detd * Psi1d + L1d*Lad/detd * id;
i1d2 =  C/detd * Psifd - A/detd * Psi1d + Lfd*Lad/detd * id;

% ?????????
deltaifd = ifd2 - ifd
deltai1d = i1d2 - i1d

% ???????? i1q ? i2q ????? Psi1q, Psi2q ? iq
D = Laq + L1q;
F = Laq + L2q;
detq = Laq^2 - D*F;

i1q = -F/detq * Psi1q + Laq/detq * Psi2q + Laq*L2q/detq * iq;
i2q = Laq/detq * Psi1q - D/detq * Psi2q + Laq*L1q/detq * iq;

% ?????????
deltai1q = D*i1q + Laq*i2q - Psi1q + Laq*iq
deltai2q = Laq*i1q + F*i2q - Psi2q + Laq*iq

% ?????????? ?? ??????? ? ???????? ?? ?????????? ?????????
ed2 = -Ra * id - omega * (Laq^2*(L2q + L1q) / detq + Laq + Ll) * iq + omega * Laq * L2q /detq * Psi1q + omega * Laq * L1q / detq * Psi2q;
eq2 = -Ra * iq + omega * (Lad^2*(Lfd + L1d) / detd + Lad + Ll) * id - omega * Lad * L1d / detd * Psifd - omega * Lad * Lfd / detd * Psi1d;

% ?????????
deltaed2 = ed2 - ed
deltaeq2 = eq2 - eq

Ed_Psi1q = Laq * L2q / detq;
Ed_Psi2q = Laq * L1q / detq;

Eq_Psifd =  -Lad * L1d / detd;
Eq_Psi1d =  -Lad * Lfd / detd;


LQ = Laq^2*(L2q + L1q) / detq + Laq + Ll;
LD = Lad^2*(Lfd + L1d) / detd + Lad + Ll;

ed3 = -Ra * id - omega * LQ * iq + omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q;
eq3 = -Ra * iq + omega * LD * id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d;


deltaed3 = ed3 - ed
deltaeq3 = eq3 - eq

Edb = omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q;
Eqb = omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d;

zsq = Ra^2 + omega^2*LD*LQ;

id2 = (Ra *(Edb - ed) - omega * LQ * (Eqb - eq)) / zsq;
iq2 = (Ra *(Eqb - eq) + omega * LD * (Edb - ed)) / zsq;

deltaid2 = id - id2
deltaiq2 = iq - iq2

Psifd_Psifd	=  Rfd * B / detd;
Psifd_Psi1d	= -Rfd * C / detd;
Psifd_id	= -Rfd * Lad * L1d / detd;

Psi1d_Psifd	= -R1d * C / detd;
Psi1d_Psi1d	=  R1d * A / detd;
Psi1d_id	= -R1d * Lad * Lfd / detd;

Psi1q_Psi1q	=  R1q * F / detq;
Psi1q_Psi2q = -R1q * Laq / detq;
Psi1q_iq	= -R1q * Laq * L2q / detq;

Psi2q_Psi1q	= -R2q * Laq / detq;
Psi2q_Psi2q	=  R2q * D / detq;
Psi2q_iq	= -R2q * Laq * L1q / detq;



dPsifd = Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * id;
d_Psifd = dPsifd + Rfd * ifd

dPsi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * id;
d_Psi1d = dPsi1d + R1d * i1d

dPsi1q = Psi1q_Psi1q * Psi1q + Psi1q_Psi2q * Psi2q + Psi1q_iq * iq;
d_Psi1q = dPsi1q + R1q * i1q

dPsi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * iq;
d_Psi2q = dPsi2q + R2q * i2q

% ???? ? ???????? ?? ?????????? ?????????
id3 = (-Ra * ed + omega * LQ * eq - omega^2 * LQ * Eq_Psifd * Psifd - omega^2 * LQ * Eq_Psi1d * Psi1d + Ra * omega * Ed_Psi1q * Psi1q + Ra * omega * Ed_Psi2q * Psi2q) / zsq;
iq3 = (-Ra * eq - omega * LD * ed + Ra * omega * Eq_Psifd * Psifd + Ra * omega * Eq_Psi1d * Psi1d + omega^2 * LD * Ed_Psi1q * Psi1q + omega^2 * LD * Ed_Psi2q * Psi2q) / zsq;

% ?????????
deltaid3 = id -id3
deltaiq3 = iq -iq3

% ?????? ? ???????? ? ????
Te = Psiq * id - Psid * iq;
Pair = Te * omega;
Pel = ed * id3 + eq * iq3;
Pel - Pair + Ra * (id3^2 + iq3^2);

% ??????????????? ??? ??????? ???????
Psid2 = ((Lad + Ll) + Lad^2 / detd * (L1d + Lfd)) * id - Lad / detd * L1d * Psifd - Lad / detd * Lfd * Psi1d;
Psiq2 = ((Laq + Ll) + Laq^2 / detq * (L1q + L2q)) * iq - Laq / detq * L2q * Psi1q - Laq / detq * L1q * Psi2q;

deltaPsid = Psid2 - Psid
deltaPsiq = Psiq2 - Psiq

Psid_id = ((Lad + Ll) + Lad^2 / detd * (L1d + Lfd));
Psid_Psifd = - Lad / detd * L1d;
Psid_Psi1d = - Lad / detd * Lfd;

Psiq_iq = ((Laq + Ll) + Laq^2 / detq * (L1q + L2q)); 
Psiq_Psi1q = - Laq / detq * L2q; 
Psiq_Psi2q = - Laq / detq * L1q;


Psid3 = Psid_id * id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d;
Psiq3 = Psiq_iq * iq + Psiq_Psi1q * Psi1q + Psiq_Psi2q * Psi2q;

deltaPsid = Psid3 - Psid
deltaPsiq = Psiq3 - Psiq

% ?????? ???????
Te2 = Psiq_iq * iq * id + Psiq_Psi1q * Psi1q * id + Psiq_Psi2q * Psi2q * id - Psid_id * id * iq - Psid_Psifd * Psifd * iq - Psid_Psi1d * Psi1d * iq;
deltate = Te - Te2



