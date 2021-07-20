Lad = .3;
L1d = 2;
Lfd = 3;
Lrc = 0.5;
Ll =  0.1;
id = 5.33;
i1d = 3;
ifd = 2;
iq = 3.317;
i1q = 2.5;
i2q = 3.1;
Laq = .74;
L1q = 2.1;
L2q = 2.3;
omega = 3.14;
Ra = .32;
Rfd = .37;
R1d = .41;
R1q = .11;
R2q = .23;
% исходные уравнени€ потокосцеплений
Psid = (Lad+Ll) * id + Lad * ifd + Lad * i1d;
Psifd = Lad * id  + (Lad + Lrc + Lfd) * ifd + (Lad + Lrc) * i1d;
Psi1d = Lad * id + (Lad + Lrc) * ifd + (Lad + Lrc + L1d) * i1d;
Psiq = (Laq + Ll)* iq + Laq * i1q + Laq * i2q;
Psi1q = Laq * iq + (Laq + L1q) * i1q + Laq * i2q;
Psi2q = Laq * iq + Laq * i1q + (Laq + L2q) * i2q;
% уравнени€ Ёƒ—
ed = -Ra * id + omega * Psiq;
eq = -Ra * iq - omega * Psid;
% подставл€ем в уравнени€ Ёƒ— потокосцеплени€ по d и q
ed1 = -Ra * id + omega*(Laq + Ll) * iq + omega * Laq * i1q + omega * Laq * i2q;
eq1 = -Ra * iq - omega*(Lad + Ll) * id - omega * Lad * ifd - omega * Lad * i1d;  
% провер€ем
deltaed1 = ed - ed1
deltaeq1 = eq - eq1

% выражаем ifd и i1d через Psifd, Psi1d и id
A = Lad + Lrc + Lfd;
B = Lad + Lrc + L1d;
C = Lad + Lrc;
detd = C^2 - A*B;
ifd = -B/detd * Psifd + C/detd * Psi1d + L1d*Lad/detd * id;
i1d =  C/detd * Psifd - A/detd * Psi1d + Lfd*Lad/detd * id;
% провер€ем
deltaifd = A*ifd + C*i1d - Psifd + Lad * id
deltai1d = C*ifd + B*i1d - Psi1d + Lad * id

% выражаем i1q и i2q через Psi1q, Psi2q и iq
D = Laq + L1q;
F = Laq + L2q;
detq = Laq^2 - D*F;

i1q = -F/detq * Psi1q + Laq/detq * Psi2q + Laq*L2q/detq * iq;
i2q = Laq/detq * Psi1q - D/detq * Psi2q + Laq*L1q/detq * iq;

% провер€ем
deltai1q = D*i1q + Laq*i2q - Psi1q + Laq*iq
deltai2q = Laq*i1q + F*i2q - Psi2q + Laq*iq

ed2 = -Ra * id + omega * (Laq^2*(L2q + L1q) / detq + Laq + Ll) * iq - omega * Laq * L2q /detq * Psi1q - omega * Laq * L1q / detq * Psi2q;
eq2 = -Ra * iq - omega * (Lad^2*(Lfd + L1d) / detd + Lad + Ll) * id + omega * Lad * L1d / detd * Psifd + omega * Lad * Lfd / detd * Psi1d;

deltaed2 = ed2 - ed
deltaeq2 = eq2 - eq

Ed_Psi1q = -Laq * L2q / detq;
Ed_Psi2q = -Laq * L1q / detq;

Eq_Psifd =  Lad * L1d / detd;
Eq_Psi1d =  Lad * Lfd / detd;


LQ = (Laq^2*(L2q + L1q) / detq + Laq + Ll);
LD = (Lad^2*(Lfd + L1d) / detd + Lad + Ll);

ed3 = -Ra * id + omega * LQ * iq + omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q;
eq3 = -Ra * iq - omega * LD * id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d;

deltaed3 = ed3 - ed
deltaeq3 = eq3 - eq

Edb = omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q;
Eqb = omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d;

zsq = Ra^2 + omega^2*LD*LQ;

id2 = (Ra *(Edb - ed) + omega * LQ * (Eqb - eq)) / zsq;
iq2 = (Ra *(Eqb - eq) - omega * LD * (Edb - ed)) / zsq;

deltaid2 = id - id2
deltaiq2 = iq - iq2


dPsifd = Rfd * B / detd * Psifd - Rfd * C / detd * Psi1d - Rfd * L1d * Lad / detd * id;
dPsifd + Rfd * ifd;
dPsi1d = -R1d * C / detd * Psifd + R1d * A / detd * Psi1d - R1d * Lfd * Lad / detd * id;
dPsi1d + R1d * i1d;
dPsi1q = R1q * F / detq * Psi1q - R1q * Laq / detq * Psi2q - R1q * Laq * L2q / detq * iq;
dPsi1q + R1q * i1q;
dPsi2q = -R2q * Laq / detq * Psi1q + R2q * D / detq * Psi2q - R2q * Laq * L1q / detq * iq;
dPsi2q + R2q * i2q;

id3 = (-Ra * ed - omega * LQ * eq + omega^2 * LQ * Eq_Psifd * Psifd + omega^2 * LQ * Eq_Psi1d * Psi1d + Ra * omega * Ed_Psi1q * Psi1q + Ra * omega * Ed_Psi2q * Psi2q) / zsq
iq3 = (-Ra * eq + omega * LD * ed + Ra * omega * Eq_Psifd * Psifd + Ra * omega * Eq_Psi1d * Psi1d - omega^2 * LD * Ed_Psi1q * Psi1q - omega^2 * LD * Ed_Psi2q * Psi2q) / zsq

deltaid3 = id -id3
deltaiq3 = iq -iq3

Te = Psiq * id3 - Psid * iq3
Pair = Te * omega
Pel = ed * id3 + eq * iq3
Pel - Pair + Ra * (id3^2 + iq3^2)

