E = 18;
V = 11;
xq = 2.142;
xd = 3.1122;
delta = 30 / 180 / 3.14169

P = E*V*sin(delta)/xd + V^2*sin(2*delta)/2*(1/xq-1/xd)
Q = E*V*cos(delta)/xd - V^2*(cos(delta)^2/xd + sin(delta)^2/xq)

E1 = P*xd/V/sin(delta)-V*(1/xq-1/xd)*cos(delta)*xd
Q3 = P/sin(delta)*cos(delta) - V^2*(1/xq-1/xd)*cos(delta)^2 - V^2*(cos(delta)^2/xd+sin(delta)^2/xq)
Q4 = P/sin(delta)*cos(delta) - V^2/xq
tandelta = P*xq/(Q*xq+V^2)

E2 = P*xd/V/sqrt(1/(1+(1/tandelta)^2))-V*(1/xq-1/xd)*sqrt(1/(1+tandelta^2))*xd
E3 = P*xd/V/sqrt(1/(1+((Q*xq+V^2)/P/xq)^2)) -V*(1/xq-1/xd)*sqrt(1/(1+(P*xq/(Q*xq+V^2))^2))*xd
E4 = xd/V*sqrt(P^2*xq^2+(Q*xq+V^2)^2)/xq - V*(1/xq-1/xd)*(Q*xq+V^2)/sqrt((Q*xq+V^2)^2+P^2*xq^2)*xd
E5 = (xd*(P^2*xq^2+(Q*xq+V^2)^2)-V^2*(1/xq-1/xd)*xd*xq*(Q*xq+V^2))/(V*xq*sqrt((Q*xq+V^2)^2+P^2*xq^2))
E6 = (xd*(P^2*xq^2+(Q*xq+V^2)^2)-V^2*(xd-xq)*(Q*xq+V^2))/(V*xq*sqrt((Q*xq+V^2)^2+P^2*xq^2))
E7 = (P^2*xd*xq^2 + Q^2*xd*xq^2+Q*xq*V^2*xd +V^2*xq^2*Q+V^4*xq)/(V*xq*sqrt((Q*xq+V^2)^2+P^2*xq^2))
E8 = (P^2*xd*xq + Q^2*xd*xq +Q*V^2*xd +V^2*xq*Q+V^4)/(V*sqrt((Q*xq+V^2)^2+P^2*xq^2))


P = 120;
cosPhi = 0.9;
Sn = P/cosPhi;
Q = sqrt(Sn^2-P^2)
U = 11

En = (V^2*(V^2+Q*(xd+xq))+Sn^2*xd*xq)/V/sqrt(V^2*(V^2+2*Q*xq)+Sn^2*xq^2)

P = P / Sn
Q = Q / Sn
Zbase = V^2/Sn
V = 1
Sn = 1

xq = xq / Zbase;
xd = xd / Zbase;


Epu = (V^2*(V^2+Q*(xd+xq))+Sn^2*xd*xq)/V/sqrt(V^2*(V^2+2*Q*xq)+Sn^2*xq^2)










V*E/xd*sin(delta)+ V*V*(xd-xq)/xd/xq*sin(2*delta)/2

((P-V^2*sin(2*delta)*(xd-xq)/2/xd/xq)*xd/V/sin(delta))

(-2*P*xd*xq*cos(delta)+V^2*cos(delta)*sin(2*delta)*(xd-xq)+V^2*2*sin(delta)*(sin(delta)^2*xd+cos(delta)^2*xq))/2/xd/xq/sin(delta)

(-2*P*xd*xq*cos(delta) + V^2*cos(delta)*sin(2*delta)*xd + 2*V^2*sin(delta)^3*xd)/2/xd/xq/sin(delta)
(-2*P*xd*xq*cos(delta) + 2*V^2*xd*sin(delta))/2/xd/xq/sin(delta)

(-P*xq*cos(delta) + V^2*sin(delta))/xq/sin(delta)
V^2/xq - P*cos(delta)/sin(delta)
deltar = atan(P*xq/(V^2-Q*xq))

xd*P/V/sin(delta) - V*cos(delta)*(xd-xq)/xq
acos((V^2*sin(delta) - Q*sin(delta)*xq)/xq/P)
P*xd/V/sin(delta) - V*(xd-xq)/xq*(V^2*sin(delta)-Q*sin(delta)*xq)/xq/P

tga = P*xq/(V^2-xq*Q)

P*xd/V/sqrt(1/(1+1/tga^2)) - V*sqrt(1/(1+tga^2))*(xd-xq)/xq

P*xd/V/sqrt(1/(1+(V^2-xq*Q)^2/P^2/xq^2)) - V*sqrt(1/(1+P^2*xq^2/(V^2-xq*Q)^2))*(xd-xq)/xq
P*xd/V/sqrt(P^2*xq^2/(P^2*xq^2+(V^2-xq*Q)^2)) - V*sqrt((V^2-xq*Q)^2/(P^2*xq^2 + (V^2-xq*Q)^2))*(xd-xq)/xq

P*xd/V*sqrt((P^2*xq^2+(V^2-xq*Q)^2)/P^2/xq^2) - V*sqrt((V^2-xq*Q)^2/(P^2*xq^2 + (V^2-xq*Q)^2))*(xd-xq)/xq
xd/V*sqrt(P^2*xq^2+(V^2-xq*Q)^2)/xq - V*(V^2-xq*Q)/sqrt(P^2*xq^2 + (V^2-xq*Q)^2)*(xd-xq)/xq

(xd*(P^2*xq^2+(V^2-xq*Q)^2)-V^2*(xd-xq)*(V^2-xq*Q)) / sqrt((V^2-xq*Q)^2+P^2*xq^2)/V/xq








