% Метод расчета фундаментальных параметров моделей СМ Парка
% I.M. Canay Modelling of alternating-current machines having multiple
% rotor circuits, IEEE Transaction on Energy Conversion, Vol. 9, No. 2,
% June 1993

% Метод работает в осях d и q, здесь будет подразумеваться ось d, переход к
% оси q осуществляется заменой индекса. Метод пригоден для любого
% количества обмоток, в данной реализации подразумевается две

format long

Xd = 1.77;   % синхронное сопротивление
Xl = 0.17;   % общее сопротивление утечки
Xad = Xd - Xl;  % сопротивление по оси
Td01 = 4.316475; % постояная времени ротора на ХХ
Td1 = 0.828745; % постоянная времени ротора на КЗ
X1s = 0.13; % сопротивление утечки первого контура
r1 = 0.0011; % активное сопротивление первого контура

% постоянные времени второго контура
Td02 = 0.374677;
Td2 = 0.054969;
% сопротивления второго контура
X2s = 0.035;
r2 = 0.012;
Xrc = 0.06; % сопротивление Canay

% характеристический полином для ХХ
% P0(p)= 1 + p*A0 + p^2*B0 + p^3*C0 + ... 
% A0 = Td01 + Td02 + Td03 + ... 
% B0 = Td1*Td2 + Td1*Td3 + Td2*Td3 + ... = k12*T01*T02 + k13*T01*T03 +
% k23*T02*T03 + k14*T01*T04 +...

A0 = Td01 + Td02;
B0 = K12(Xad, X1s + Xrc, X2s + Xrc, Xad + Xrc) * Td01 * Td02;

% находим корни характеристического полинома 
roots0 = Roots(B0, A0, 1);
% и новые постоянные времени на ХХ
cTd01 = -1 / roots0(1);
cTd02 = -1 / roots0(2);

% характеристический полином для КЗ
% P(p) = 1 + p*A + p^2*B + p^3*C + ...
% A =  Td1 + Td2 + Td3 + ...
% B = Td1*Td2 + Td1*Td3 + Td2*Td3 +... = k12*Td1*Td2 + k13*Td1*Td3 +
% k23*Td2*Td3 + k14*Td1*Td4 + ...
% kij рассчитывается с xdeltad = xad * xl / (xad + xl) вместо xad

A = Td1 + Td2;
Xdeltad = Xad * Xl / (Xad + Xl);
B = K12(Xdeltad, X1s + Xrc, X2s + Xrc, Xdeltad + Xrc) * Td1 * Td2;
roots = Roots(B, A, 1);
% новые постоянные времени на КЗ
cTd1 = -1 / roots(1);
cTd2 = -1 / roots(2);

xd1 = 0.329
xd2 = 0.253
Td1 = 0.859
Td2 = 0.0247
Td01 = 4.316475
Td02 = 0.374667

A0 = Xd / xd1 * Td1 + (Xd / xd2 - Xd / xd1 + 1) * Td2;
B0 = Xd / xd2 * Td1*Td2;

roots = Roots(B0,A0,1);
cTd01 = -1/roots(1);
cTd02 = -1/roots(2);

xe = - Xl;
A = Td1 + Td2;
Ae = 1 / (Xd + xe) * (Xd * A + xe * A0);
Be = (xd2 + xe) / (Xd + xe) * B0;

roots = Roots(Be, Ae, 1);
Tde01 = -1 / roots(1);
Tde02 = -1 / roots(2);

xde1 = (Xd + xe) / (1 - (Tde01 - cTd01) * (Tde01 - cTd02)/(Tde01*(Tde01 - Tde02)))
xde2 = (Xd + xe) * Tde01 * Tde02 / cTd01 / cTd02

deltay1 = 1 / xde1 - 1 / (Xd + xe);
deltay2 = 1 / xde2 - 1 / xde1;
l1 = 1 / deltay1
l2 = 1 / deltay2

w = 1; %2 * pi * 60

r1 = 1 / deltay1 / (Tde01 * w)
r2 = 1 / deltay2 / (Tde02 * w)



[r1,x1,r2,x2] = CanayParameters(1.63,0.217292727,0.151543636,0.150741818, 6.7,0.059)
%[r1,x1,r2,x2] = CanayParameters(1.77,0.329, 0.253, 0.17, 0.859, 0.0247)

xrcb = -1.0;
xrce = 1.0;
xrcstep = 0.005;
xrcsize = (xrce - xrcb) / xrcstep;

xrca = zeros(1, xrcsize);
irca = zeros(1, xrcsize);
drca = zeros(1, xrcsize);

xrc = xrcb;


for i = 1: xrcsize
    [r1n,l1n,r2n,l2n] = CouplingTransform(r1,l1,r2,l2,xrc);
    r1n = 0.0011;
    l1n = 0.13;
    r2n = 0.012;
    l2n = 0.035;
    
    cur = FieldCurrent(Xd-Xl, r1n,l1n,r2n,l2n,Xl,xrc);
    abs(cur) 
    angle(cur) * 180 / pi
    
    xrca(i) = xrc;
    xrc = xrc + xrcstep;
    irca(i) = abs(cur);
    drca(i) = angle(cur) * 180 / pi;
end

%subplot(2,2,1)
%plot(xrca,irca)
%subplot(2,2,2)
%plot(xrca,drca)





% Umans & Mallick
mTd1 = Td1;
mTd2 = Td2;

mTd01 = mTd1 * Xd / xd1;
mTd02 = mTd2 * xd1 / xd2;

Md = Xd - Xl;

A = Md ^2 / (Xd*(mTd01 + mTd02 - mTd1 - mTd2));
a = (Xd * (mTd1+mTd2)-Xl*(mTd01 + mTd02))/Md;
b = (Xd*mTd1*mTd2 - Xl*mTd01*mTd02)/Md;
c = (mTd01*mTd02 - mTd1*mTd2)/(mTd01+mTd02 - mTd1 - mTd2);
d = sqrt(a^2-4*b);
ra = 2*A*d/(a-2*c+d)
la = ra*(a+d)/2
rb = 2*a*d/(2*c-a+d)
lb = rb*(a-d)/2


A = (1 - Xd/xd1 + Xd / xd2);
B = -Td01 - Td02;
C = Td01 * Td02 * xd2 / xd1;

roots = Roots(A,B,C)

Td2 = roots(1)
Td1 = Td01 * Td02 * xd2 / Td2 / Xd


Xd / xd1 * Td1 + (1 - Xd / xd1 + Xd / xd2) * Td2 - Td01 - Td02
Td1 * Td2 * Xd / xd2 - Td01 * Td02

% метод пересчета постоянных времени им. НИИПТ

%Tsum = Td01 + Td02;
%D = 0.25 * Tsum^2 - Td01*Td02 / (1 - Xad * Xad / ((Xad + X1s)*(Xad + X2s)));
%nTd01 = 0.5 * Tsum + sqrt(D)
%nTd02 = Tsum - nTd01

% одна обмотка canay

xd2 = 0.340
Td2 = 0.829
A0 = Xd / xd2 * Td2
xe = -Xl
Td2e = 1 / (Xd + xe) * (Xd*Td2 + xe * A0)
xde1 = (Xd + xe) * Td2e / A0
deltay1 = 1 / xde1 - 1 / (Xd + xe);
l1 = 1 / deltay1
r1 = l1 / Td2e 

function k12 = K12(xad, x1s, x2s, x12)
    k12 = 1 /((xad + x1s)*(xad + x2s)) * det([xad + x1s, x12; x12, xad + x2s]);
end

function [roots] = Roots(a,b,c)
    D = sqrt(b^2 - 4 * a * c);
    roots = [(-b - D) / 2 / a, (-b + D) / 2 / a];
    [~,idx] = sort(abs(roots));
    roots = roots(idx);
end

function [r1, x1, r2, x2] = CanayParameters(xd, xd1, xd2, xl, Td01, Td02)
    [Td1, Td2] = SCTimeConstants(xd, xd1, xd2, Td01, Td02);
    A0 = xd / xd1 * Td1 + (xd / xd2 - xd / xd1 + 1) * Td2;
    B0 = xd / xd2 * Td1*Td2;
    roots = Roots(B0,A0,1);
    cTd01 = -1/roots(1);
    cTd02 = -1/roots(2);
    xe = - xl;
    A = Td1 + Td2;
    Ae = 1 / (xd + xe) * (xd * A + xe * A0);
    Be = (xd2 + xe) / (xd + xe) * B0;
    roots = Roots(Be, Ae, 1);
    Tde01 = -1 / roots(1);
    Tde02 = -1 / roots(2);
    xde1 = (xd + xe) / (1 - (Tde01 - cTd01) * (Tde01 - cTd02)/(Tde01*(Tde01 - Tde02)))
    xde2 = (xd + xe) * Tde01 * Tde02 / cTd01 / cTd02
    deltay1 = 1 / xde1 - 1 / (xd + xe);
    deltay2 = 1 / xde2 - 1 / xde1;
    x1 = 1 / deltay1;
    x2 = 1 / deltay2;
    w = 1; %2 * pi * 60
    r1 = x1 / (Tde01 * w);
    r2 = x2 / (Tde02 * w);
end

function [Td1, Td2] = SCTimeConstants(xd, xd1, xd2, Td01, Td02)
	A = (1 - xd / xd1 + xd / xd2);
	B = -Td01 - Td02;
	C = Td01 * Td02 * xd2 / xd1;
    roots = Roots(A,B,C);
    Td2 = roots(1);
    Td1 = Td01 * Td02 * xd2 / Td2 / xd;
end

function [r1n,x1n,r2n,x2n] = CouplingTransform(r1, x1, r2, x2, xr)
    H = sqrt(((x1-xr)/r1 - (x2-xr)/r2)^2 + 4 * xr^2/r1/r2);
    G =  (x1-xr)/r1 + (x2-xr)/r2;
    T1 = 0.5 * (G+H);
    T2 = 0.5 * (G-H);
    r1n = r1*r2*H/((r1+r2)*T1-x1-x2);
    r2n = -r1*r2*H/((r1+r2)*T2-x1-x2);
    x1n = r1n*T1;
    x2n = r2n*T2;
end

function current = FieldCurrent(xd, r1, x1, r2, x2, xa, xr)
    z1 = r1 + x1*1i;
    z2 = r2 + x2*1i;
    z12r = z1 * z2 / (z1 + z2) + xr*1i;
    zf = z12r * xd*1i / (z12r + xd*1i) + xa*1i;
    ii = 1 / zf;
    u2 = 1 - ii * xa*1i;
    i12r = u2 / z12r;
    u1 = u2 - i12r * xr*1i;
    current = u1 / z1;
end