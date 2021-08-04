syms s real; % скольжение
omega = 1 + s; % угловая частота
syms sv % угловая частота напряжения
omega2 = omega * omega;
syms r ld2 lq2 real; % сопротивление статора и xd'' xq''
zsq = 1 / (r * r + omega2 * ld2 * lq2); % сопротивление Machowski
syms Id Iq Vd Vq Psifd Psi1d Psi1q Psi2q Delta real; % переменные состояния

syms Ed_Psi1q Ed_Psi2q real; % коэффициенты ЭДС d
syms Eq_Psifd Eq_Psi1d real; % коэффициенты ЭДС q

syms Mj Pt Kdemp real; % момент инерции, мощность турбины и коэффициент демпфирования
syms Psid_id Psid_Psifd Psid_Psi1d real; % коэффициенты потокосцепления d
syms Psiq_iq Psiq_Psi1q Psiq_Psi2q real; % коэффициенты потокосцепления q

syms ExtEqe; % ЭДС возбуждения

syms Psifd_Psifd Psifd_Psi1d Psifd_id real;
syms Psi1d_Psifd Psi1d_Psi1d Psi1d_id real;
syms Psi1q_Psi1q Psi1q_Psi2q Psi1q_iq real;
syms Psi2q_Psi1q Psi2q_Psi2q Psi2q_iq real;

d_Id = -r * Id - omega * lq2 * Iq + omega * Ed_Psi1q * Psi1q - Vd;
d_Iq = -r * Iq + omega * ld2 * Id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d - Vq;
Te = (ld2 * Id + Psid_Psifd * Psifd + Psid_Psi1d * Psi1d) * Iq - (lq2 * Iq + Psiq_Psi1q * Psi1q) * Id;
d_s = (Pt / omega - Kdemp * s - Te) / Mj;
    



d_Ed = -r * Id - omega * lq2 * Iq + omega * Ed_Psi1q * Psi1q + omega * Ed_Psi2q * Psi2q - Vd;
d_Eq = -r * Iq + omega * ld2 * Id + omega * Eq_Psifd * Psifd + omega * Eq_Psi1d * Psi1d - Vq;

d_Psifd = ExtEqe + Psifd_Psifd * Psifd + Psifd_Psi1d * Psi1d + Psifd_id * Id;
d_Psi1d = Psi1d_Psifd * Psifd + Psi1d_Psi1d * Psi1d + Psi1d_id * Id;
d_Psi1q = Psi1q_Psi1q * Psi1q + Psi1q_iq * Iq;
d_Psi2q = Psi2q_Psi1q * Psi1q + Psi2q_Psi2q * Psi2q + Psi2q_iq * Iq;

dId_Id = simplify(diff(d_Id, Id))
dId_Iq = simplify(diff(d_Id, Iq))
dId_Vd = simplify(diff(d_Id, Vd))
dId_Vq = simplify(diff(d_Id, Vq))
dId_Psifd = simplify(diff(d_Id, Psifd))
dId_Psi1d = simplify(diff(d_Id, Psi1d))
dId_Psi1q = simplify(diff(d_Id, Psi1q))
dId_Psi2q = simplify(diff(d_Id, Psi2q))
dId_s = simplify(diff(d_Id, s))
dId_Delta = simplify(diff(d_Id, Delta))

dIq_Id = simplify(diff(d_Iq, Id))
dIq_Iq = simplify(diff(d_Iq, Iq))
dIq_Vd = simplify(diff(d_Iq, Vd))
dIq_Vq = simplify(diff(d_Iq, Vq))
dIq_Psifd = simplify(diff(d_Iq, Psifd))
dIq_Psi1d = simplify(diff(d_Iq, Psi1d))
dIq_Psi1q = simplify(diff(d_Iq, Psi1q))
dIq_Psi2q = simplify(diff(d_Iq, Psi2q))
dIq_s = simplify(diff(d_Iq, s))
dIq_Delta = simplify(diff(d_Iq, Delta))


ds_Id = simplify(diff(d_s, Id))
ds_Iq = simplify(diff(d_s, Iq))
ds_Vd = simplify(diff(d_s, Vd))
ds_Vq = simplify(diff(d_s, Vq))
ds_Psifd = simplify(diff(d_s, Psifd))
ds_Psi1d = simplify(diff(d_s, Psi1d))
ds_Psi1q = simplify(diff(d_s, Psi1q))
ds_Psi2q = simplify(diff(d_s, Psi2q))
ds_s = simplify(diff(d_s, s))
ds_sv = simplify(diff(d_s, sv))
ds_Delta = simplify(diff(d_s, Delta))


d_Psifd_Id = simplify(diff(d_Psifd, Id))
d_Psifd_Iq = simplify(diff(d_Psifd, Iq))
d_Psifd_Vd = simplify(diff(d_Psifd, Vd))
d_Psifd_Vq = simplify(diff(d_Psifd, Vq))
d_Psifd_Psifd = simplify(diff(d_Psifd, Psifd))
d_Psifd_Psi1d = simplify(diff(d_Psifd, Psi1d))
d_Psifd_Psi1q = simplify(diff(d_Psifd, Psi1q))
d_Psifd_Psi2q = simplify(diff(d_Psifd, Psi2q))
d_Psifd_s = simplify(diff(d_Psifd, s))
d_Psifd_sv = simplify(diff(d_Psifd, sv))
d_Psifd_Delta = simplify(diff(d_Psifd, Delta))


d_Psi1d_Id = simplify(diff(d_Psi1d, Id));
d_Psi1d_Iq = simplify(diff(d_Psi1d, Iq));
d_Psi1d_Vd = simplify(diff(d_Psi1d, Vd));
d_Psi1d_Vq = simplify(diff(d_Psi1d, Vq));
d_Psi1d_Psifd = simplify(diff(d_Psi1d, Psifd));
d_Psi1d_Psi1d = simplify(diff(d_Psi1d, Psi1d));
d_Psi1d_Psi1q = simplify(diff(d_Psi1d, Psi1q));
d_Psi1d_Psi2q = simplify(diff(d_Psi1d, Psi2q));
d_Psi1d_s = simplify(diff(d_Psi1d, s));
d_Psi1d_Delta = simplify(diff(d_Psi1d, Delta));

d_Psi1q_Id = simplify(diff(d_Psi1q, Id))
d_Psi1q_Iq = simplify(diff(d_Psi1q, Iq))
d_Psi1q_Vd = simplify(diff(d_Psi1q, Vd))
d_Psi1q_Vq = simplify(diff(d_Psi1q, Vq))
d_Psi1q_Psifd = simplify(diff(d_Psi1q, Psifd))
d_Psi1q_Psi1d = simplify(diff(d_Psi1q, Psi1d))
d_Psi1q_Psi1q = simplify(diff(d_Psi1q, Psi1q))
d_Psi1q_Psi2q = simplify(diff(d_Psi1q, Psi2q))
d_Psi1q_s = simplify(diff(d_Psi1q, s))
d_Psi1q_Delta = simplify(diff(d_Psi1q, Delta))

d_Psi2q_Id = simplify(diff(d_Psi2q, Id))
d_Psi2q_Iq = simplify(diff(d_Psi2q, Iq))
d_Psi2q_Vd = simplify(diff(d_Psi2q, Vd))
d_Psi2q_Vq = simplify(diff(d_Psi2q, Vq))
d_Psi2q_Psifd = simplify(diff(d_Psi2q, Psifd))
d_Psi2q_Psi1d = simplify(diff(d_Psi2q, Psi1d))
d_Psi2q_Psi1q = simplify(diff(d_Psi2q, Psi1q))
d_Psi2q_Psi2q = simplify(diff(d_Psi2q, Psi2q))
d_Psi2q_s = simplify(diff(d_Psi2q, s))
d_Psi2q_Delta = simplify(diff(d_Psi2q, Delta))


d_Ed_Id = simplify(diff(d_Ed, Id))
d_Ed_Iq = simplify(diff(d_Ed, Iq))
d_Ed_Vd = simplify(diff(d_Ed, Vd))
d_Ed_Vq = simplify(diff(d_Ed, Vq))
d_Ed_Psifd = simplify(diff(d_Ed, Psifd))
d_Ed_Psi1d = simplify(diff(d_Ed, Psi1d))
d_Ed_Psi1q = simplify(diff(d_Ed, Psi1q))
d_Ed_Psi2q = simplify(diff(d_Ed, Psi2q))
d_Ed_s = simplify(diff(d_Ed, s))
d_Ed_Delta = simplify(diff(d_Ed, Delta))

d_Eq_Id = simplify(diff(d_Eq, Id))
d_Eq_Iq = simplify(diff(d_Eq, Iq))
d_Eq_Vd = simplify(diff(d_Eq, Vd))
d_Eq_Vq = simplify(diff(d_Eq, Vq))
d_Eq_Psifd = simplify(diff(d_Eq, Psifd))
d_Eq_Psi1d = simplify(diff(d_Eq, Psi1d))
d_Eq_Psi1q = simplify(diff(d_Eq, Psi1q))
d_Eq_Psi2q = simplify(diff(d_Eq, Psi2q))
d_Eq_s = simplify(diff(d_Eq, s))
d_Eq_Delta = simplify(diff(d_Eq, Delta))

%J = jacobian([d_Id; d_Iq;  d_s; d_Psifd; d_Psi1d; d_Psi1q ; d_Psi2q], [Id Iq Vd Vq Psifd Psi1d Psi1q Psi2q s Delta])

