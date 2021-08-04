% ����� ������� ��������������� ���������� ������� �� �����
% I.M. Canay Modelling of alternating-current machines having multiple
% rotor circuits, IEEE Transaction on Energy Conversion, Vol. 9, No. 2,
% June 1993

% ����� �������� � ���� d � q, ����� ����� ��������������� ��� d, ������� �
% ��� q �������������� ������� �������. ����� �������� ��� ������
% ���������� �������, � ������ ���������� ��������������� ���

format long

Xd = 1.77;   % ���������� �������������
Xl = 0.17;   % ����� ������������� ������
Xad = Xd - Xl;  % ������������� �� ���
Td01 = 4.316475; % ��������� ������� ������ �� ��
Td1 = 0.828745; % ���������� ������� ������ �� ��
X1s = 0.13; % ������������� ������ ������� �������
r1 = 0.0011; % �������� ������������� ������� �������

% ���������� ������� ������� �������
Td02 = 0.374677;
Td2 = 0.054969;
% ������������� ������� �������
X2s = 0.035;
r2 = 0.012;
Xrc = 0.06; % ������������� Canay

% ������������������ ������� ��� ��
% P0(p)= 1 + p*A0 + p^2*B0 + p^3*C0 + ... 
% A0 = Td01 + Td02 + Td03 + ... 
% B0 = Td1*Td2 + Td1*Td3 + Td2*Td3 + ... = k12*T01*T02 + k13*T01*T03 +
% k23*T02*T03 + k14*T01*T04 +...

A0 = Td01 + Td02;
B0 = K12(Xad, X1s + Xrc, X2s + Xrc, Xad + Xrc) * Td01 * Td02;

% ������� ����� ������������������� �������� 
roots0 = Roots(B0, A0, 1);
% � ����� ���������� ������� �� ��
cTd01 = -1 / roots0(1);
cTd02 = -1 / roots0(2);

% ������������������ ������� ��� ��
% P(p) = 1 + p*A + p^2*B + p^3*C + ...
% A =  Td1 + Td2 + Td3 + ...
% B = Td1*Td2 + Td1*Td3 + Td2*Td3 +... = k12*Td1*Td2 + k13*Td1*Td3 +
% k23*Td2*Td3 + k14*Td1*Td4 + ...
% kij �������������� � xdeltad = xad * xl / (xad + xl) ������ xad

A = Td1 + Td2;
Xdeltad = Xad * Xl / (Xad + Xl);
B = K12(Xdeltad, X1s + Xrc, X2s + Xrc, Xdeltad + Xrc) * Td1 * Td2;
roots = Roots(B, A, 1);
% ����� ���������� ������� �� ��
cTd1 = -1 / roots(1);
cTd2 = -1 / roots(2);

xd1 = 0.329;
xd2 = 0.253;
Td1 = 0.859;
Td2 = 0.0247;

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

w = 1 %2 * pi * 60

r1 = 1 / deltay1 / (Tde01 * w)
r2 = 1 / deltay2 / (Tde02 * w)

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



% ����� ��������� ���������� ������� ��. �����

%Tsum = Td01 + Td02;
%D = 0.25 * Tsum^2 - Td01*Td02 / (1 - Xad * Xad / ((Xad + X1s)*(Xad + X2s)));
%nTd01 = 0.5 * Tsum + sqrt(D)
%nTd02 = Tsum - nTd01


function k12 = K12(xad, x1s, x2s, x12)
    k12 = 1 /((xad + x1s)*(xad + x2s)) * det([xad + x1s, x12; x12, xad + x2s]);
end

function [roots] = Roots(a,b,c)
    D = sqrt(b^2 - 4 * a * c);
    roots = [(-b - D) / 2 / a, (-b + D) / 2 / a];
    [~,idx] = sort(abs(roots));
    roots = roots(idx);
end